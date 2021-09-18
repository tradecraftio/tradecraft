// Copyright (c) 2011-2022 The Bitcoin Core developers
// Copyright (c) 2011-2023 The Freicoin Developers
//
// This program is free software: you can redistribute it and/or modify it under
// the terms of version 3 of the GNU Affero General Public License as published
// by the Free Software Foundation.
//
// This program is distributed in the hope that it will be useful, but WITHOUT
// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
// FOR A PARTICULAR PURPOSE.  See the GNU Affero General Public License for more
// details.
//
// You should have received a copy of the GNU Affero General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.

#include <qt/pstoperationsdialog.h>

#include <core_io.h>
#include <interfaces/node.h>
#include <key_io.h>
#include <node/pst.h>
#include <policy/policy.h>
#include <qt/freicoinunits.h>
#include <qt/forms/ui_pstoperationsdialog.h>
#include <qt/guiutil.h>
#include <qt/optionsmodel.h>
#include <util/fs.h>
#include <util/strencodings.h>

#include <fstream>
#include <iostream>
#include <string>

using node::AnalyzePST;
using node::DEFAULT_MAX_RAW_TX_FEE_RATE;
using node::PSTAnalysis;

PSTOperationsDialog::PSTOperationsDialog(
    QWidget* parent, WalletModel* wallet_model, ClientModel* client_model) : QDialog(parent, GUIUtil::dialog_flags),
                                                                             m_ui(new Ui::PSTOperationsDialog),
                                                                             m_wallet_model(wallet_model),
                                                                             m_client_model(client_model)
{
    m_ui->setupUi(this);

    connect(m_ui->signTransactionButton, &QPushButton::clicked, this, &PSTOperationsDialog::signTransaction);
    connect(m_ui->broadcastTransactionButton, &QPushButton::clicked, this, &PSTOperationsDialog::broadcastTransaction);
    connect(m_ui->copyToClipboardButton, &QPushButton::clicked, this, &PSTOperationsDialog::copyToClipboard);
    connect(m_ui->saveButton, &QPushButton::clicked, this, &PSTOperationsDialog::saveTransaction);

    connect(m_ui->closeButton, &QPushButton::clicked, this, &PSTOperationsDialog::close);

    m_ui->signTransactionButton->setEnabled(false);
    m_ui->broadcastTransactionButton->setEnabled(false);
}

PSTOperationsDialog::~PSTOperationsDialog()
{
    delete m_ui;
}

void PSTOperationsDialog::openWithPST(PartiallySignedTransaction pstx)
{
    m_transaction_data = pstx;

    bool complete = FinalizePST(pstx); // Make sure all existing signatures are fully combined before checking for completeness.
    if (m_wallet_model) {
        size_t n_could_sign;
        TransactionError err = m_wallet_model->wallet().fillPST(SIGHASH_ALL, /*sign=*/false, /*bip32derivs=*/true, &n_could_sign, m_transaction_data, complete);
        if (err != TransactionError::OK) {
            showStatus(tr("Failed to load transaction: %1")
                           .arg(QString::fromStdString(TransactionErrorString(err).translated)),
                       StatusLevel::ERR);
            return;
        }
        m_ui->signTransactionButton->setEnabled(!complete && !m_wallet_model->wallet().privateKeysDisabled() && n_could_sign > 0);
    } else {
        m_ui->signTransactionButton->setEnabled(false);
    }

    m_ui->broadcastTransactionButton->setEnabled(complete);

    updateTransactionDisplay();
}

void PSTOperationsDialog::signTransaction()
{
    bool complete;
    size_t n_signed;

    WalletModel::UnlockContext ctx(m_wallet_model->requestUnlock());

    TransactionError err = m_wallet_model->wallet().fillPST(SIGHASH_ALL, /*sign=*/true, /*bip32derivs=*/true, &n_signed, m_transaction_data, complete);

    if (err != TransactionError::OK) {
        showStatus(tr("Failed to sign transaction: %1")
            .arg(QString::fromStdString(TransactionErrorString(err).translated)), StatusLevel::ERR);
        return;
    }

    updateTransactionDisplay();

    if (!complete && !ctx.isValid()) {
        showStatus(tr("Cannot sign inputs while wallet is locked."), StatusLevel::WARN);
    } else if (!complete && n_signed < 1) {
        showStatus(tr("Could not sign any more inputs."), StatusLevel::WARN);
    } else if (!complete) {
        showStatus(tr("Signed %1 inputs, but more signatures are still required.").arg(n_signed),
            StatusLevel::INFO);
    } else {
        showStatus(tr("Signed transaction successfully. Transaction is ready to broadcast."),
            StatusLevel::INFO);
        m_ui->broadcastTransactionButton->setEnabled(true);
    }
}

void PSTOperationsDialog::broadcastTransaction()
{
    CMutableTransaction mtx;
    if (!FinalizeAndExtractPST(m_transaction_data, mtx)) {
        // This is never expected to fail unless we were given a malformed PST
        // (e.g. with an invalid signature.)
        showStatus(tr("Unknown error processing transaction."), StatusLevel::ERR);
        return;
    }

    CTransactionRef tx = MakeTransactionRef(mtx);
    std::string err_string;
    TransactionError error =
        m_client_model->node().broadcastTransaction(tx, DEFAULT_MAX_RAW_TX_FEE_RATE.GetFeePerK(), err_string);

    if (error == TransactionError::OK) {
        showStatus(tr("Transaction broadcast successfully! Transaction ID: %1")
            .arg(QString::fromStdString(tx->GetHash().GetHex())), StatusLevel::INFO);
    } else {
        showStatus(tr("Transaction broadcast failed: %1")
            .arg(QString::fromStdString(TransactionErrorString(error).translated)), StatusLevel::ERR);
    }
}

void PSTOperationsDialog::copyToClipboard() {
    CDataStream ssTx(SER_NETWORK, PROTOCOL_VERSION);
    ssTx << m_transaction_data;
    GUIUtil::setClipboard(HexStr(ssTx.str()).c_str());
    showStatus(tr("PST copied to clipboard."), StatusLevel::INFO);
}

void PSTOperationsDialog::saveTransaction() {
    CDataStream ssTx(SER_NETWORK, PROTOCOL_VERSION);
    ssTx << m_transaction_data;

    QString selected_filter;
    QString filename_suggestion = "";
    bool first = true;
    for (const CTxOut& out : m_transaction_data.tx->vout) {
        if (!first) {
            filename_suggestion.append("-");
        }
        CTxDestination address;
        ExtractDestination(out.scriptPubKey, address);
        QString amount = FreicoinUnits::format(m_client_model->getOptionsModel()->getDisplayUnit(), out.nValue);
        QString address_str = QString::fromStdString(EncodeDestination(address));
        filename_suggestion.append(address_str + "-" + amount);
        first = false;
    }
    filename_suggestion.append(".pst");
    QString filename = GUIUtil::getSaveFileName(this,
        tr("Save Transaction Data"), filename_suggestion,
        //: Expanded name of the binary PST file format. See: BIP 174.
        tr("Partially Signed Transaction (Binary)") + QLatin1String(" (*.pst)"), &selected_filter);
    if (filename.isEmpty()) {
        return;
    }
    std::ofstream out{filename.toLocal8Bit().data(), std::ofstream::out | std::ofstream::binary};
    out << ssTx.str();
    out.close();
    showStatus(tr("PST saved to disk."), StatusLevel::INFO);
}

void PSTOperationsDialog::updateTransactionDisplay() {
    m_ui->transactionDescription->setText(QString::fromStdString(renderTransaction(m_transaction_data)));
    showTransactionStatus(m_transaction_data);
}

std::string PSTOperationsDialog::renderTransaction(const PartiallySignedTransaction &pstx)
{
    QString tx_description = "";
    CAmount totalAmount = 0;
    for (const CTxOut& out : pstx.tx->vout) {
        CTxDestination address;
        ExtractDestination(out.scriptPubKey, address);
        totalAmount += out.nValue;
        tx_description.append(tr(" * Sends %1 to %2")
            .arg(FreicoinUnits::formatWithUnit(FreicoinUnit::FRC, out.nValue))
            .arg(QString::fromStdString(EncodeDestination(address))));
        // Check if the address is one of ours
        if (m_wallet_model != nullptr && m_wallet_model->wallet().txoutIsMine(out)) tx_description.append(" (" + tr("own address") + ")");
        tx_description.append("<br>");
    }

    PSTAnalysis analysis = AnalyzePST(pstx);
    tx_description.append(" * ");
    if (!*analysis.fee) {
        // This happens if the transaction is missing input UTXO information.
        tx_description.append(tr("Unable to calculate transaction fee or total transaction amount."));
    } else {
        tx_description.append(tr("Pays transaction fee: "));
        tx_description.append(FreicoinUnits::formatWithUnit(FreicoinUnit::FRC, *analysis.fee));

        // add total amount in all subdivision units
        tx_description.append("<hr />");
        QStringList alternativeUnits;
        for (const FreicoinUnits::Unit u : FreicoinUnits::availableUnits())
        {
            if(u != m_client_model->getOptionsModel()->getDisplayUnit()) {
                alternativeUnits.append(FreicoinUnits::formatHtmlWithUnit(u, totalAmount));
            }
        }
        tx_description.append(QString("<b>%1</b>: <b>%2</b>").arg(tr("Total Amount"))
            .arg(FreicoinUnits::formatHtmlWithUnit(m_client_model->getOptionsModel()->getDisplayUnit(), totalAmount)));
        tx_description.append(QString("<br /><span style='font-size:10pt; font-weight:normal;'>(=%1)</span>")
            .arg(alternativeUnits.join(" " + tr("or") + " ")));
    }

    size_t num_unsigned = CountPSTUnsignedInputs(pstx);
    if (num_unsigned > 0) {
        tx_description.append("<br><br>");
        tx_description.append(tr("Transaction has %1 unsigned inputs.").arg(QString::number(num_unsigned)));
    }

    return tx_description.toStdString();
}

void PSTOperationsDialog::showStatus(const QString &msg, StatusLevel level) {
    m_ui->statusBar->setText(msg);
    switch (level) {
        case StatusLevel::INFO: {
            m_ui->statusBar->setStyleSheet("QLabel { background-color : lightgreen }");
            break;
        }
        case StatusLevel::WARN: {
            m_ui->statusBar->setStyleSheet("QLabel { background-color : orange }");
            break;
        }
        case StatusLevel::ERR: {
            m_ui->statusBar->setStyleSheet("QLabel { background-color : red }");
            break;
        }
    }
    m_ui->statusBar->show();
}

size_t PSTOperationsDialog::couldSignInputs(const PartiallySignedTransaction &pstx) {
    if (!m_wallet_model) {
        return 0;
    }

    size_t n_signed;
    bool complete;
    TransactionError err = m_wallet_model->wallet().fillPST(SIGHASH_ALL, /*sign=*/false, /*bip32derivs=*/false, &n_signed, m_transaction_data, complete);

    if (err != TransactionError::OK) {
        return 0;
    }
    return n_signed;
}

void PSTOperationsDialog::showTransactionStatus(const PartiallySignedTransaction &pstx) {
    PSTAnalysis analysis = AnalyzePST(pstx);
    size_t n_could_sign = couldSignInputs(pstx);

    switch (analysis.next) {
        case PSTRole::UPDATER: {
            showStatus(tr("Transaction is missing some information about inputs."), StatusLevel::WARN);
            break;
        }
        case PSTRole::SIGNER: {
            QString need_sig_text = tr("Transaction still needs signature(s).");
            StatusLevel level = StatusLevel::INFO;
            if (!m_wallet_model) {
                need_sig_text += " " + tr("(But no wallet is loaded.)");
                level = StatusLevel::WARN;
            } else if (m_wallet_model->wallet().privateKeysDisabled()) {
                need_sig_text += " " + tr("(But this wallet cannot sign transactions.)");
                level = StatusLevel::WARN;
            } else if (n_could_sign < 1) {
                need_sig_text += " " + tr("(But this wallet does not have the right keys.)"); // XXX wording
                level = StatusLevel::WARN;
            }
            showStatus(need_sig_text, level);
            break;
        }
        case PSTRole::FINALIZER:
        case PSTRole::EXTRACTOR: {
            showStatus(tr("Transaction is fully signed and ready for broadcast."), StatusLevel::INFO);
            break;
        }
        default: {
            showStatus(tr("Transaction status is unknown."), StatusLevel::ERR);
            break;
        }
    }
}

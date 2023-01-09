// Copyright (c) 2011-2020 The Bitcoin Core developers
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

#ifndef FREICOIN_QT_PSTOPERATIONSDIALOG_H
#define FREICOIN_QT_PSTOPERATIONSDIALOG_H

#include <QDialog>

#include <pst.h>
#include <qt/clientmodel.h>
#include <qt/walletmodel.h>

namespace Ui {
class PSTOperationsDialog;
}

/** Dialog showing transaction details. */
class PSTOperationsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit PSTOperationsDialog(QWidget* parent, WalletModel* walletModel, ClientModel* clientModel);
    ~PSTOperationsDialog();

    void openWithPST(PartiallySignedTransaction pstx);

public Q_SLOTS:
    void signTransaction();
    void broadcastTransaction();
    void copyToClipboard();
    void saveTransaction();

private:
    Ui::PSTOperationsDialog* m_ui;
    PartiallySignedTransaction m_transaction_data;
    WalletModel* m_wallet_model;
    ClientModel* m_client_model;

    enum class StatusLevel {
        INFO,
        WARN,
        ERR
    };

    size_t couldSignInputs(const PartiallySignedTransaction &pstx);
    void updateTransactionDisplay();
    std::string renderTransaction(const PartiallySignedTransaction &pstx);
    void showStatus(const QString &msg, StatusLevel level);
    void showTransactionStatus(const PartiallySignedTransaction &pstx);
};

#endif // FREICOIN_QT_PSTOPERATIONSDIALOG_H

// Copyright (c) 2011-2021 The Bitcoin Core developers
// Copyright (c) 2011-2024 The Freicoin Developers
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

#include <qt/receiverequestdialog.h>
#include <qt/forms/ui_receiverequestdialog.h>

#include <qt/freicoinunits.h>
#include <qt/guiutil.h>
#include <qt/optionsmodel.h>
#include <qt/qrimagewidget.h>
#include <qt/walletmodel.h>

#include <QDialog>
#include <QString>

#if defined(HAVE_CONFIG_H)
#include <config/freicoin-config.h> /* for USE_QRCODE */
#endif

ReceiveRequestDialog::ReceiveRequestDialog(QWidget* parent)
    : QDialog(parent, GUIUtil::dialog_flags),
      ui(new Ui::ReceiveRequestDialog)
{
    ui->setupUi(this);
    GUIUtil::handleCloseWindowShortcut(this);
}

ReceiveRequestDialog::~ReceiveRequestDialog()
{
    delete ui;
}

void ReceiveRequestDialog::setModel(WalletModel *_model)
{
    this->model = _model;

    if (_model)
        connect(_model->getOptionsModel(), &OptionsModel::displayUnitChanged, this, &ReceiveRequestDialog::updateDisplayUnit);

    // update the display unit if necessary
    update();
}

void ReceiveRequestDialog::setInfo(const SendCoinsRecipient &_info)
{
    this->info = _info;
    setWindowTitle(tr("Request payment to %1").arg(info.label.isEmpty() ? info.address : info.label));
    QString uri = GUIUtil::formatFreicoinURI(info);

#ifdef USE_QRCODE
    if (ui->qr_code->setQR(uri, info.address)) {
        connect(ui->btnSaveAs, &QPushButton::clicked, ui->qr_code, &QRImageWidget::saveImage);
    } else {
        ui->btnSaveAs->setEnabled(false);
    }
#else
    ui->btnSaveAs->hide();
    ui->qr_code->hide();
#endif

    ui->uri_content->setText("<a href=\"" + uri + "\">" + GUIUtil::HtmlEscape(uri) + "</a>");
    ui->address_content->setText(info.address);

    if (!info.amount) {
        ui->amount_tag->hide();
        ui->amount_content->hide();
    } // Amount is set in updateDisplayUnit() slot.
    updateDisplayUnit();

    if (!info.label.isEmpty()) {
        ui->label_content->setText(info.label);
    } else {
        ui->label_tag->hide();
        ui->label_content->hide();
    }

    if (!info.message.isEmpty()) {
        ui->message_content->setText(info.message);
    } else {
        ui->message_tag->hide();
        ui->message_content->hide();
    }

    if (!model->getWalletName().isEmpty()) {
        ui->wallet_content->setText(model->getWalletName());
    } else {
        ui->wallet_tag->hide();
        ui->wallet_content->hide();
    }

    ui->btnVerify->setVisible(model->wallet().hasExternalSigner());

    connect(ui->btnVerify, &QPushButton::clicked, [this] {
        model->displayAddress(info.address.toStdString());
    });
}

void ReceiveRequestDialog::updateDisplayUnit()
{
    if (!model) return;
    ui->amount_content->setText(FreicoinUnits::formatWithUnit(model->getOptionsModel()->getDisplayUnit(), info.amount));
}

void ReceiveRequestDialog::on_btnCopyURI_clicked()
{
    GUIUtil::setClipboard(GUIUtil::formatFreicoinURI(info));
}

void ReceiveRequestDialog::on_btnCopyAddress_clicked()
{
    GUIUtil::setClipboard(info.address);
}

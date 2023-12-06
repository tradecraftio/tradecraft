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

#ifndef FREICOIN_QT_ASKPASSPHRASEDIALOG_H
#define FREICOIN_QT_ASKPASSPHRASEDIALOG_H

#include <QDialog>

#include <support/allocators/secure.h>

class WalletModel;

namespace Ui {
    class AskPassphraseDialog;
}

/** Multifunctional dialog to ask for passphrases. Used for encryption, unlocking, and changing the passphrase.
 */
class AskPassphraseDialog : public QDialog
{
    Q_OBJECT

public:
    enum Mode {
        Encrypt,    /**< Ask passphrase twice and encrypt */
        Unlock,     /**< Ask passphrase and unlock */
        ChangePass, /**< Ask old passphrase + new passphrase twice */
    };

    explicit AskPassphraseDialog(Mode mode, QWidget *parent, SecureString* passphrase_out = nullptr);
    ~AskPassphraseDialog();

    void accept() override;

    void setModel(WalletModel *model);

private:
    Ui::AskPassphraseDialog *ui;
    Mode mode;
    WalletModel* model{nullptr};
    bool fCapsLock{false};
    SecureString* m_passphrase_out;

private Q_SLOTS:
    void textChanged();
    void secureClearPassFields();
    void toggleShowPassword(bool);

protected:
    bool event(QEvent *event) override;
    bool eventFilter(QObject *object, QEvent *event) override;
};

#endif // FREICOIN_QT_ASKPASSPHRASEDIALOG_H

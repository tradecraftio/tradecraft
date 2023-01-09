// Copyright (c) 2011-2021 The Bitcoin Core developers
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

#ifndef FREICOIN_QT_SENDCOINSENTRY_H
#define FREICOIN_QT_SENDCOINSENTRY_H

#include <qt/sendcoinsrecipient.h>

#include <QStackedWidget>

class WalletModel;
class PlatformStyle;

namespace interfaces {
class Node;
} // namespace interfaces

namespace Ui {
    class SendCoinsEntry;
}

/**
 * A single entry in the dialog for sending freicoins.
 * Stacked widget, with different UIs for payment requests
 * with a strong payee identity.
 */
class SendCoinsEntry : public QStackedWidget
{
    Q_OBJECT

public:
    explicit SendCoinsEntry(const PlatformStyle *platformStyle, QWidget *parent = nullptr);
    ~SendCoinsEntry();

    void setModel(WalletModel *model);
    bool validate(interfaces::Node& node);
    SendCoinsRecipient getValue();

    /** Return whether the entry is still empty and unedited */
    bool isClear();

    void setValue(const SendCoinsRecipient &value);
    void setAddress(const QString &address);
    void setAmount(const CAmount &amount);

    /** Set up the tab chain manually, as Qt messes up the tab chain by default in some cases
     *  (issue https://bugreports.qt-project.org/browse/QTBUG-10907).
     */
    QWidget *setupTabChain(QWidget *prev);

    void setFocus();

public Q_SLOTS:
    void clear();
    void checkSubtractFeeFromAmount();

Q_SIGNALS:
    void removeEntry(SendCoinsEntry *entry);
    void useAvailableBalance(SendCoinsEntry* entry);
    void payAmountChanged();
    void subtractFeeFromAmountChanged();

private Q_SLOTS:
    void deleteClicked();
    void useAvailableBalanceClicked();
    void on_payTo_textChanged(const QString &address);
    void on_addressBookButton_clicked();
    void on_pasteButton_clicked();
    void updateDisplayUnit();

protected:
    void changeEvent(QEvent* e) override;

private:
    SendCoinsRecipient recipient;
    Ui::SendCoinsEntry *ui;
    WalletModel *model;
    const PlatformStyle *platformStyle;

    bool updateLabel(const QString &address);
};

#endif // FREICOIN_QT_SENDCOINSENTRY_H

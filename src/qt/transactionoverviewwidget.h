// Copyright (c) 2021 The Bitcoin Core developers
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

#ifndef BITCOIN_QT_TRANSACTIONOVERVIEWWIDGET_H
#define BITCOIN_QT_TRANSACTIONOVERVIEWWIDGET_H

#include <qt/transactiontablemodel.h>

#include <QListView>
#include <QSize>
#include <QSizePolicy>

QT_BEGIN_NAMESPACE
class QShowEvent;
class QWidget;
QT_END_NAMESPACE

class TransactionOverviewWidget : public QListView
{
    Q_OBJECT

public:
    explicit TransactionOverviewWidget(QWidget* parent = nullptr) : QListView(parent) {}

    QSize sizeHint() const override
    {
        return {sizeHintForColumn(TransactionTableModel::ToAddress), QListView::sizeHint().height()};
    }

protected:
    void showEvent(QShowEvent* event) override
    {
        Q_UNUSED(event);
        QSizePolicy sp = sizePolicy();
        sp.setHorizontalPolicy(QSizePolicy::Minimum);
        setSizePolicy(sp);
    }
};

#endif // BITCOIN_QT_TRANSACTIONOVERVIEWWIDGET_H

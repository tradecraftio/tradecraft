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

#ifndef BITCOIN_QT_QVALUECOMBOBOX_H
#define BITCOIN_QT_QVALUECOMBOBOX_H

#include <QComboBox>
#include <QVariant>

/* QComboBox that can be used with QDataWidgetMapper to select ordinal values from a model. */
class QValueComboBox : public QComboBox
{
    Q_OBJECT

    Q_PROPERTY(QVariant value READ value WRITE setValue NOTIFY valueChanged USER true)

public:
    explicit QValueComboBox(QWidget *parent = nullptr);

    QVariant value() const;
    void setValue(const QVariant &value);

    /** Specify model role to use as ordinal value (defaults to Qt::UserRole) */
    void setRole(int role);

Q_SIGNALS:
    void valueChanged();

private:
    int role;

private Q_SLOTS:
    void handleSelectionChanged(int idx);
};

#endif // BITCOIN_QT_QVALUECOMBOBOX_H

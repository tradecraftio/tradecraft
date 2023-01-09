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

#ifndef FREICOIN_QT_FREICOINADDRESSVALIDATOR_H
#define FREICOIN_QT_FREICOINADDRESSVALIDATOR_H

#include <QValidator>

/** Base58 entry widget validator, checks for valid characters and
 * removes some whitespace.
 */
class FreicoinAddressEntryValidator : public QValidator
{
    Q_OBJECT

public:
    explicit FreicoinAddressEntryValidator(QObject *parent);

    State validate(QString &input, int &pos) const override;
};

/** Freicoin address widget validator, checks for a valid freicoin address.
 */
class FreicoinAddressCheckValidator : public QValidator
{
    Q_OBJECT

public:
    explicit FreicoinAddressCheckValidator(QObject *parent);

    State validate(QString &input, int &pos) const override;
};

#endif // FREICOIN_QT_FREICOINADDRESSVALIDATOR_H

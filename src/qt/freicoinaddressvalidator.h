// Copyright (c) 2011-2014 The Bitcoin Core developers
// Copyright (c) 2011-2021 The Freicoin Developers
//
// This program is free software: you can redistribute it and/or
// modify it under the conjunctive terms of BOTH version 3 of the GNU
// Affero General Public License as published by the Free Software
// Foundation AND the MIT/X11 software license.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Affero General Public License and the MIT/X11 software license for
// more details.
//
// You should have received a copy of both licenses along with this
// program.  If not, see <https://www.gnu.org/licenses/> and
// <http://www.opensource.org/licenses/mit-license.php>

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

    State validate(QString &input, int &pos) const;
};

/** Freicoin address widget validator, checks for a valid freicoin address.
 */
class FreicoinAddressCheckValidator : public QValidator
{
    Q_OBJECT

public:
    explicit FreicoinAddressCheckValidator(QObject *parent);

    State validate(QString &input, int &pos) const;
};

#endif // FREICOIN_QT_FREICOINADDRESSVALIDATOR_H

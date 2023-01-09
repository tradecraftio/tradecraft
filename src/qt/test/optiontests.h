// Copyright (c) 2019 The Bitcoin Core developers
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

#ifndef BITCOIN_QT_TEST_OPTIONTESTS_H
#define BITCOIN_QT_TEST_OPTIONTESTS_H

#include <qt/optionsmodel.h>

#include <QObject>

class OptionTests : public QObject
{
    Q_OBJECT
public:
    explicit OptionTests(interfaces::Node& node) : m_node(node) {}

private Q_SLOTS:
    void optionTests();

private:
    interfaces::Node& m_node;
};

#endif // BITCOIN_QT_TEST_OPTIONTESTS_H

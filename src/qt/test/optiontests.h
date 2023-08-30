// Copyright (c) 2019-2022 The Bitcoin Core developers
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

#ifndef FREICOIN_QT_TEST_OPTIONTESTS_H
#define FREICOIN_QT_TEST_OPTIONTESTS_H

#include <qt/optionsmodel.h>
#include <univalue.h>
#include <util/settings.h>

#include <QObject>

class OptionTests : public QObject
{
    Q_OBJECT
public:
    explicit OptionTests(interfaces::Node& node);

private Q_SLOTS:
    void init(); // called before each test function execution.
    void migrateSettings();
    void integerGetArgBug();
    void parametersInteraction();
    void extractFilter();

private:
    interfaces::Node& m_node;
    util::Settings m_previous_settings;
};

#endif // FREICOIN_QT_TEST_OPTIONTESTS_H

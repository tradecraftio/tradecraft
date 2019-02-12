// Copyright (c) 2009-2014 The Bitcoin developers
// Copyright (c) 2011-2019 The Freicoin developers
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

#if defined(HAVE_CONFIG_H)
#include "config/bitcoin-config.h"
#endif

#include "util.h"
#include "uritests.h"

#ifdef ENABLE_WALLET
#include "paymentservertests.h"
#endif

#include <QCoreApplication>
#include <QObject>
#include <QTest>

#if defined(QT_STATICPLUGIN) && QT_VERSION < 0x050000
#include <QtPlugin>
Q_IMPORT_PLUGIN(qcncodecs)
Q_IMPORT_PLUGIN(qjpcodecs)
Q_IMPORT_PLUGIN(qtwcodecs)
Q_IMPORT_PLUGIN(qkrcodecs)
#endif

// This is all you need to run all the tests
int main(int argc, char *argv[])
{
    SetupEnvironment();
    bool fInvalid = false;

    // Don't remove this, it's needed to access
    // QCoreApplication:: in the tests
    QCoreApplication app(argc, argv);
    app.setApplicationName("Bitcoin-Qt-test");

    URITests test1;
    if (QTest::qExec(&test1) != 0)
        fInvalid = true;
#ifdef ENABLE_WALLET
    PaymentServerTests test2;
    if (QTest::qExec(&test2) != 0)
        fInvalid = true;
#endif

    return fInvalid;
}

// Copyright (c) 2009-2016 The Bitcoin Core developers
// Copyright (c) 2011-2021 The Freicoin Developers
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

#if defined(HAVE_CONFIG_H)
#include "config/freicoin-config.h"
#endif

#include "chainparams.h"
#include "key.h"
#include "rpcnestedtests.h"
#include "util.h"
#include "uritests.h"
#include "compattests.h"

#ifdef ENABLE_WALLET
#include "paymentservertests.h"
#endif

#include <QCoreApplication>
#include <QObject>
#include <QTest>

#include <openssl/ssl.h>

#if defined(QT_STATICPLUGIN) && QT_VERSION < 0x050000
#include <QtPlugin>
Q_IMPORT_PLUGIN(qcncodecs)
Q_IMPORT_PLUGIN(qjpcodecs)
Q_IMPORT_PLUGIN(qtwcodecs)
Q_IMPORT_PLUGIN(qkrcodecs)
#endif

extern void noui_connect();

// This is all you need to run all the tests
int main(int argc, char *argv[])
{
    ECC_Start();
    SetupEnvironment();
    SetupNetworking();
    SelectParams(CBaseChainParams::MAIN);
    noui_connect();

    bool fInvalid = false;

    // Don't remove this, it's needed to access
    // QCoreApplication:: in the tests
    QCoreApplication app(argc, argv);
    app.setApplicationName("Freicoin-Qt-test");

    SSL_library_init();

    URITests test1;
    if (QTest::qExec(&test1) != 0)
        fInvalid = true;
#ifdef ENABLE_WALLET
    PaymentServerTests test2;
    if (QTest::qExec(&test2) != 0)
        fInvalid = true;
#endif
    RPCNestedTests test3;
    if (QTest::qExec(&test3) != 0)
        fInvalid = true;
    CompatTests test4;
    if (QTest::qExec(&test4) != 0)
        fInvalid = true;

    ECC_Stop();
    return fInvalid;
}

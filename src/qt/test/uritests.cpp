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

#include "uritests.h"

#include "guiutil.h"
#include "walletmodel.h"

#include <QUrl>

void URITests::uriTests()
{
    SendCoinsRecipient rv;
    QUrl uri;
    uri.setUrl(QString("bitcoin:175tWpb8K1S7NmH4Zx6rewF9WQrcZv245W?req-dontexist="));
    QVERIFY(!GUIUtil::parseBitcoinURI(uri, &rv));

    uri.setUrl(QString("bitcoin:175tWpb8K1S7NmH4Zx6rewF9WQrcZv245W?dontexist="));
    QVERIFY(GUIUtil::parseBitcoinURI(uri, &rv));
    QVERIFY(rv.address == QString("175tWpb8K1S7NmH4Zx6rewF9WQrcZv245W"));
    QVERIFY(rv.label == QString());
    QVERIFY(rv.amount == 0);

    uri.setUrl(QString("bitcoin:175tWpb8K1S7NmH4Zx6rewF9WQrcZv245W?label=Wikipedia Example Address"));
    QVERIFY(GUIUtil::parseBitcoinURI(uri, &rv));
    QVERIFY(rv.address == QString("175tWpb8K1S7NmH4Zx6rewF9WQrcZv245W"));
    QVERIFY(rv.label == QString("Wikipedia Example Address"));
    QVERIFY(rv.amount == 0);

    uri.setUrl(QString("bitcoin:175tWpb8K1S7NmH4Zx6rewF9WQrcZv245W?amount=0.001"));
    QVERIFY(GUIUtil::parseBitcoinURI(uri, &rv));
    QVERIFY(rv.address == QString("175tWpb8K1S7NmH4Zx6rewF9WQrcZv245W"));
    QVERIFY(rv.label == QString());
    QVERIFY(rv.amount == 100000);

    uri.setUrl(QString("bitcoin:175tWpb8K1S7NmH4Zx6rewF9WQrcZv245W?amount=1.001"));
    QVERIFY(GUIUtil::parseBitcoinURI(uri, &rv));
    QVERIFY(rv.address == QString("175tWpb8K1S7NmH4Zx6rewF9WQrcZv245W"));
    QVERIFY(rv.label == QString());
    QVERIFY(rv.amount == 100100000);

    uri.setUrl(QString("bitcoin:175tWpb8K1S7NmH4Zx6rewF9WQrcZv245W?amount=100&label=Wikipedia Example"));
    QVERIFY(GUIUtil::parseBitcoinURI(uri, &rv));
    QVERIFY(rv.address == QString("175tWpb8K1S7NmH4Zx6rewF9WQrcZv245W"));
    QVERIFY(rv.amount == 10000000000LL);
    QVERIFY(rv.label == QString("Wikipedia Example"));

    uri.setUrl(QString("bitcoin:175tWpb8K1S7NmH4Zx6rewF9WQrcZv245W?message=Wikipedia Example Address"));
    QVERIFY(GUIUtil::parseBitcoinURI(uri, &rv));
    QVERIFY(rv.address == QString("175tWpb8K1S7NmH4Zx6rewF9WQrcZv245W"));
    QVERIFY(rv.label == QString());

    QVERIFY(GUIUtil::parseBitcoinURI("bitcoin://175tWpb8K1S7NmH4Zx6rewF9WQrcZv245W?message=Wikipedia Example Address", &rv));
    QVERIFY(rv.address == QString("175tWpb8K1S7NmH4Zx6rewF9WQrcZv245W"));
    QVERIFY(rv.label == QString());

    uri.setUrl(QString("bitcoin:175tWpb8K1S7NmH4Zx6rewF9WQrcZv245W?req-message=Wikipedia Example Address"));
    QVERIFY(GUIUtil::parseBitcoinURI(uri, &rv));

    uri.setUrl(QString("bitcoin:175tWpb8K1S7NmH4Zx6rewF9WQrcZv245W?amount=1,000&label=Wikipedia Example"));
    QVERIFY(!GUIUtil::parseBitcoinURI(uri, &rv));

    uri.setUrl(QString("bitcoin:175tWpb8K1S7NmH4Zx6rewF9WQrcZv245W?amount=1,000.0&label=Wikipedia Example"));
    QVERIFY(!GUIUtil::parseBitcoinURI(uri, &rv));
}

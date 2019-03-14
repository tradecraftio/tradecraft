// Copyright (c) 2012-2014 The Bitcoin Core developers
// Copyright (c) 2011-2019 The Freicoin Developers
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

#include "wallet/wallet.h"
#include "wallet/walletdb.h"

#include "test/test_freicoin.h"

#include <stdint.h>

#include <boost/foreach.hpp>
#include <boost/test/unit_test.hpp>

extern CWallet* pwalletMain;

BOOST_FIXTURE_TEST_SUITE(accounting_tests, TestingSetup)

static void
GetResults(CWalletDB& walletdb, std::map<CAmount, CAccountingEntry>& results)
{
    std::list<CAccountingEntry> aes;

    results.clear();
    BOOST_CHECK(walletdb.ReorderTransactions(pwalletMain) == DB_LOAD_OK);
    walletdb.ListAccountCreditDebit("", aes);
    BOOST_FOREACH(CAccountingEntry& ae, aes)
    {
        results[ae.nOrderPos] = ae;
    }
}

BOOST_AUTO_TEST_CASE(acc_orderupgrade)
{
    CWalletDB walletdb(pwalletMain->strWalletFile);
    std::vector<CWalletTx*> vpwtx;
    CWalletTx wtx;
    CAccountingEntry ae;
    std::map<CAmount, CAccountingEntry> results;

    LOCK(pwalletMain->cs_wallet);

    ae.strAccount = "";
    ae.nCreditDebit = 1;
    ae.nTime = 1333333333;
    ae.strOtherAccount = "b";
    ae.strComment = "";
    walletdb.WriteAccountingEntry(ae);

    wtx.mapValue["comment"] = "z";
    pwalletMain->AddToWallet(wtx, false, &walletdb);
    vpwtx.push_back(&pwalletMain->mapWallet[wtx.GetHash()]);
    vpwtx[0]->nTimeReceived = (unsigned int)1333333335;
    vpwtx[0]->nOrderPos = -1;

    ae.nTime = 1333333336;
    ae.strOtherAccount = "c";
    walletdb.WriteAccountingEntry(ae);

    GetResults(walletdb, results);

    BOOST_CHECK(pwalletMain->nOrderPosNext == 3);
    BOOST_CHECK(2 == results.size());
    BOOST_CHECK(results[0].nTime == 1333333333);
    BOOST_CHECK(results[0].strComment.empty());
    BOOST_CHECK(1 == vpwtx[0]->nOrderPos);
    BOOST_CHECK(results[2].nTime == 1333333336);
    BOOST_CHECK(results[2].strOtherAccount == "c");


    ae.nTime = 1333333330;
    ae.strOtherAccount = "d";
    ae.nOrderPos = pwalletMain->IncOrderPosNext();
    walletdb.WriteAccountingEntry(ae);

    GetResults(walletdb, results);

    BOOST_CHECK(results.size() == 3);
    BOOST_CHECK(pwalletMain->nOrderPosNext == 4);
    BOOST_CHECK(results[0].nTime == 1333333333);
    BOOST_CHECK(1 == vpwtx[0]->nOrderPos);
    BOOST_CHECK(results[2].nTime == 1333333336);
    BOOST_CHECK(results[3].nTime == 1333333330);
    BOOST_CHECK(results[3].strComment.empty());


    wtx.mapValue["comment"] = "y";
    {
        CMutableTransaction tx(wtx);
        --tx.nLockTime;  // Just to change the hash :)
        *static_cast<CTransaction*>(&wtx) = CTransaction(tx);
    }
    pwalletMain->AddToWallet(wtx, false, &walletdb);
    vpwtx.push_back(&pwalletMain->mapWallet[wtx.GetHash()]);
    vpwtx[1]->nTimeReceived = (unsigned int)1333333336;

    wtx.mapValue["comment"] = "x";
    {
        CMutableTransaction tx(wtx);
        --tx.nLockTime;  // Just to change the hash :)
        *static_cast<CTransaction*>(&wtx) = CTransaction(tx);
    }
    pwalletMain->AddToWallet(wtx, false, &walletdb);
    vpwtx.push_back(&pwalletMain->mapWallet[wtx.GetHash()]);
    vpwtx[2]->nTimeReceived = (unsigned int)1333333329;
    vpwtx[2]->nOrderPos = -1;

    GetResults(walletdb, results);

    BOOST_CHECK(results.size() == 3);
    BOOST_CHECK(pwalletMain->nOrderPosNext == 6);
    BOOST_CHECK(0 == vpwtx[2]->nOrderPos);
    BOOST_CHECK(results[1].nTime == 1333333333);
    BOOST_CHECK(2 == vpwtx[0]->nOrderPos);
    BOOST_CHECK(results[3].nTime == 1333333336);
    BOOST_CHECK(results[4].nTime == 1333333330);
    BOOST_CHECK(results[4].strComment.empty());
    BOOST_CHECK(5 == vpwtx[1]->nOrderPos);


    ae.nTime = 1333333334;
    ae.strOtherAccount = "e";
    ae.nOrderPos = -1;
    walletdb.WriteAccountingEntry(ae);

    GetResults(walletdb, results);

    BOOST_CHECK(results.size() == 4);
    BOOST_CHECK(pwalletMain->nOrderPosNext == 7);
    BOOST_CHECK(0 == vpwtx[2]->nOrderPos);
    BOOST_CHECK(results[1].nTime == 1333333333);
    BOOST_CHECK(2 == vpwtx[0]->nOrderPos);
    BOOST_CHECK(results[3].nTime == 1333333336);
    BOOST_CHECK(results[3].strComment.empty());
    BOOST_CHECK(results[4].nTime == 1333333330);
    BOOST_CHECK(results[4].strComment.empty());
    BOOST_CHECK(results[5].nTime == 1333333334);
    BOOST_CHECK(6 == vpwtx[1]->nOrderPos);
}

BOOST_AUTO_TEST_SUITE_END()

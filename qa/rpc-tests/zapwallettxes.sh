#!/usr/bin/env bash
# Copyright (c) 2014 The Bitcoin Core developers
# Copyright (c) 2010-2019 The Freicoin developers
#
# This program is free software: you can redistribute it and/or modify
# it under the conjunctive terms of BOTH version 3 of the GNU Affero
# General Public License as published by the Free Software Foundation
# AND the MIT/X11 software license.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Affero General Public License and the MIT/X11 software license for
# more details.
#
# You should have received a copy of both licenses along with this
# program.  If not, see <https://www.gnu.org/licenses/> and
# <http://www.opensource.org/licenses/mit-license.php>

# Test -zapwallettxes=<mode>

if [ $# -lt 1 ]; then
        echo "Usage: $0 path_to_binaries"
        echo "e.g. $0 ../../src"
        echo "Env vars FREICOIND and FREICOINCLI may be used to specify the exact binaries used"
        exit 1
fi

set -f

FREICOIND=${FREICOIND:-${1}/freicoind}
CLI=${FREICOINCLI:-${1}/freicoin-cli}

DIR="${BASH_SOURCE%/*}"
SENDANDWAIT="${DIR}/send.sh"
if [[ ! -d "$DIR" ]]; then DIR="$PWD"; fi
. "$DIR/util.sh"

D=$(mktemp -d test.XXXXX)

D1=${D}/node1
CreateDataDir "$D1" port=11000 rpcport=11001
B1ARGS="-datadir=$D1"
$FREICOIND $B1ARGS &
B1PID=$!

D2=${D}/node2
CreateDataDir "$D2" port=11010 rpcport=11011
B2ARGS="-datadir=$D2"
$FREICOIND $B2ARGS &
B2PID=$!

function CleanUp {
$CLI $B2ARGS stop > /dev/null 2>&1
wait $B2PID
$CLI $B1ARGS stop > /dev/null 2>&1
wait $B1PID

rm -rf $D
}

# 110 blocks, 10 mature == 500 XBT
$CLI $B1ARGS setgenerate true 110
$CLI $B2ARGS setgenerate true 110

CheckBalance "$B1ARGS" 500
CheckBalance "$B2ARGS" 500

# Send 10 XBT
TXID1_DEFAULT=$($CLI $B1ARGS sendtoaddress "mrhz5ZgSF3C1BSdyCKt3gEdhKoRL5BNfJV" 10)
TXID2_DEFAULT=$($CLI $B2ARGS sendtoaddress "mrhz5ZgSF3C1BSdyCKt3gEdhKoRL5BNfJV" 10)

CheckBalance $B1ARGS 490
CheckBalance $B2ARGS 490

# Move 10 XBT to testaccount
TMP=$($CLI $B1ARGS move "" "testaccount" 10)
TMP=$($CLI $B2ARGS move "" "testaccount" 10)

CheckBalance $B1ARGS 10 "testaccount"
CheckBalance $B2ARGS 10 "testaccount"

# Send 1 XBT from testaccount
TXID1_TESTACCOUNT=$($CLI $B1ARGS sendfrom "testaccount" "mrhz5ZgSF3C1BSdyCKt3gEdhKoRL5BNfJV" 1)
TXID2_TESTACCOUNT=$($CLI $B2ARGS sendfrom "testaccount" "mrhz5ZgSF3C1BSdyCKt3gEdhKoRL5BNfJV" 1)

CheckBalance $B1ARGS 9 "testaccount"
CheckBalance $B2ARGS 9 "testaccount"

CheckBalance $B1ARGS 489
CheckBalance $B2ARGS 489

# Confirm transactions
$CLI $B1ARGS setgenerate true 1
$CLI $B2ARGS setgenerate true 1

# Create unconfirmed transaction
TXID1_UNCONFIRMED=$($CLI $B1ARGS sendtoaddress "mrhz5ZgSF3C1BSdyCKt3gEdhKoRL5BNfJV" 1)
TXID2_UNCONFIRMED=$($CLI $B2ARGS sendtoaddress "mrhz5ZgSF3C1BSdyCKt3gEdhKoRL5BNfJV" 1)

# check balance (we created another 50 and spent 1 in the meantime)
CheckBalance $B1ARGS 538
CheckBalance $B2ARGS 538

# Safety check, if unconfirmed transactions are there
$CLI $B1ARGS gettransaction $TXID1_UNCONFIRMED > /dev/null 2>&1
if [[ $? -ne 0 ]] ; then
    echoerr "gettransaction1_1: $TXID1_UNCONFIRMED failed"
    CleanUp
    exit 1
fi
$CLI $B2ARGS gettransaction $TXID2_UNCONFIRMED > /dev/null 2>&1
if [[ $? -ne 0 ]] ; then
    echoerr "gettransaction2_1: $TXID2_UNCONFIRMED failed"
    CleanUp
    exit 1
fi

# stop nodes
$CLI $B2ARGS stop > /dev/null 2>&1
wait $B2PID
$CLI $B1ARGS stop > /dev/null 2>&1
wait $B1PID

# restart nodes with -zapwallettxes
$FREICOIND -zapwallettxes=1 $B1ARGS &
B1PID=$!
$FREICOIND -zapwallettxes=2 $B2ARGS &
B2PID=$!

# check if confirmed transactions are there
$CLI $B1ARGS gettransaction $TXID1_DEFAULT > /dev/null 2>&1
if [[ $? -ne 0 ]] ; then
    echoerr "check confirmed transaction 1: $TXID1_DEFAULT failed"
    CleanUp
    exit 1
fi
$CLI $B2ARGS gettransaction $TXID2_DEFAULT > /dev/null 2>&1
if [[ $? -ne 0 ]] ; then
    echoerr "check confirmed transaction 2: $TXID2_DEFAULT failed"
    CleanUp
    exit 1
fi
$CLI $B1ARGS gettransaction $TXID1_TESTACCOUNT > /dev/null 2>&1
if [[ $? -ne 0 ]] ; then
    echoerr "check confirmed transaction 3: $TXID1_TESTACCOUNT failed"
    CleanUp
    exit 1
fi
$CLI $B2ARGS gettransaction $TXID2_TESTACCOUNT > /dev/null 2>&1
if [[ $? -ne 0 ]] ; then
    echoerr "check confirmed transaction 4: $TXID2_TESTACCOUNT failed"
    CleanUp
    exit 1
fi

# check if unconfirmed transaction is gone
$CLI $B1ARGS gettransaction $TXID1_UNCONFIRMED > /dev/null 2>&1
if [[ $? -eq 0 ]] ; then
    echoerr "check unconfirmed transaction 1: $TXID1_UNCONFIRMED failed"
    CleanUp
    exit 1
fi
$CLI $B2ARGS gettransaction $TXID2_UNCONFIRMED > /dev/null 2>&1
if [[ $? -eq 0 ]] ; then
    echoerr "check unconfirmed transaction 2: $TXID2_UNCONFIRMED failed"
    CleanUp
    exit 1
fi

# check zapwallet mode 1, testaccount balance must be 9 (keeping transaction metadata)
CheckBalance $B1ARGS 9 "testaccount"

# check zapwallet mode 2, testaccount balance must be 10 (dropping transaction metadata)
CheckBalance $B2ARGS 10 "testaccount"

echo "Tests successful, cleaning up"
CleanUp
exit 0

#!/usr/bin/env bash
#
# Copyright (c) 2019 The Bitcoin Core developers
# Copyright (c) 2010-2023 The Freicoin Developers
#
# This program is free software: you can redistribute it and/or modify it under
# the terms of version 3 of the GNU Affero General Public License as published
# by the Free Software Foundation.
#
# This program is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
# FOR A PARTICULAR PURPOSE.  See the GNU Affero General Public License for more
# details.
#
# You should have received a copy of the GNU Affero General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>.
#
export LC_ALL=C

set -ueo pipefail

if (( $# < 3 )); then
  echo 'Usage: utxo_snapshot.sh <generate-at-height> <snapshot-out-path> <bitcoin-cli-call ...>'
  echo
  echo "  if <snapshot-out-path> is '-', don't produce a snapshot file but instead print the "
  echo "  expected assumeutxo hash"
  echo
  echo 'Examples:'
  echo
  echo "  ./contrib/devtools/utxo_snapshot.sh 570000 utxo.dat ./src/bitcoin-cli -datadir=\$(pwd)/testdata"
  echo '  ./contrib/devtools/utxo_snapshot.sh 570000 - ./src/bitcoin-cli'
  exit 1
fi

GENERATE_AT_HEIGHT="${1}"; shift;
OUTPUT_PATH="${1}"; shift;
# Most of the calls we make take a while to run, so pad with a lengthy timeout.
BITCOIN_CLI_CALL="${*} -rpcclienttimeout=9999999"

# Block we'll invalidate/reconsider to rewind/fast-forward the chain.
PIVOT_BLOCKHASH=$($BITCOIN_CLI_CALL getblockhash $(( GENERATE_AT_HEIGHT + 1 )) )

(>&2 echo "Rewinding chain back to height ${GENERATE_AT_HEIGHT} (by invalidating ${PIVOT_BLOCKHASH}); this may take a while")
${BITCOIN_CLI_CALL} invalidateblock "${PIVOT_BLOCKHASH}"

if [[ "${OUTPUT_PATH}" = "-" ]]; then
  (>&2 echo "Generating txoutset info...")
  ${BITCOIN_CLI_CALL} gettxoutsetinfo | grep hash_serialized_2 | sed 's/^.*: "\(.\+\)\+",/\1/g'
else
  (>&2 echo "Generating UTXO snapshot...")
  ${BITCOIN_CLI_CALL} dumptxoutset "${OUTPUT_PATH}"
fi

(>&2 echo "Restoring chain to original height; this may take a while")
${BITCOIN_CLI_CALL} reconsiderblock "${PIVOT_BLOCKHASH}"

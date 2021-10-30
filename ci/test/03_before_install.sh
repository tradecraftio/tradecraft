#!/usr/bin/env bash
#
# Copyright (c) 2018 The Bitcoin Core developers
# Copyright (c) 2010-2021 The Freicoin Developers
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

export LC_ALL=C.UTF-8

# Add llvm-symbolizer directory to PATH. Needed to get symbolized stack traces from the sanitizers.
PATH=$PATH:/usr/lib/llvm-6.0/bin/
export PATH

BEGIN_FOLD () {
  echo ""
  CURRENT_FOLD_NAME=$1
  echo "travis_fold:start:${CURRENT_FOLD_NAME}"
}

END_FOLD () {
  RET=$?
  echo "travis_fold:end:${CURRENT_FOLD_NAME}"
  if [ $RET != 0 ]; then
    echo "${CURRENT_FOLD_NAME} failed with status code ${RET}"
  fi
}


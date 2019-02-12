#!/bin/bash

# Copyright (c) 2011-2023 The Freicoin Developers.
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

set -e

cd $(dirname "${BASH_SOURCE[0]}")/../..

for fn in `git grep "Distributed under the MIT software license" | cut -d':' -f1`; do
  if [ x"$fn" != "xcontrib/devtools/relicensefile.py" ]; then
    python3 contrib/devtools/relicensefile.py "$fn"
  fi
done

for fn in `git grep "Distributed under the MIT/X11 software license" | cut -d':' -f1`; do
  if [ x"$fn" != "xcontrib/devtools/relicensefile.py" ]; then
    python3 contrib/devtools/relicensefile.py "$fn"
  fi
done

# Files having to do with the stratum mining server are licensed under the
# MIT/X11 license, so that patches can be shared with Bitcoin Core.
for file in \
    src/node/miner.cpp \
    src/node/miner.h \
    src/stratum.cpp \
    src/stratum.h \
    src/wallet/miner.cpp \
    src/wallet/miner.h \
; do
    git restore --staged "$file"
    git checkout -- "$file"
done

git commit -a -m '[License] Update copyright declaration in code files.'

# End of File

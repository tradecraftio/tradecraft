#!/bin/sh
# Copyright (c) 2014-2019 The Bitcoin Core developers
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

export LC_ALL=C
set -e

UNSIGNED="$1"
SIGNATURE="$2"
ROOTDIR=dist
OUTDIR=signed-app
SIGNAPPLE=signapple

if [ -z "$UNSIGNED" ]; then
  echo "usage: $0 <unsigned app> <signature>"
  exit 1
fi

if [ -z "$SIGNATURE" ]; then
  echo "usage: $0 <unsigned app> <signature>"
  exit 1
fi

${SIGNAPPLE} apply ${UNSIGNED} ${SIGNATURE}
mv ${ROOTDIR} ${OUTDIR}
echo "Signed: ${OUTDIR}"

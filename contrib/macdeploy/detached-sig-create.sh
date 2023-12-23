#!/bin/sh
# Copyright (c) 2014-2022 The Bitcoin Core developers
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

ROOTDIR=dist
BUNDLE="${ROOTDIR}/Freicoin-Qt.app"
BINARY="${BUNDLE}/Contents/MacOS/Freicoin-Qt"
SIGNAPPLE=signapple
TEMPDIR=sign.temp
ARCH=$(${SIGNAPPLE} info ${BINARY} | head -n 1 | cut -d " " -f 1)
OUT="signature-osx-${ARCH}.tar.gz"
OUTROOT=osx/dist

if [ -z "$1" ]; then
  echo "usage: $0 <signapple args>"
  echo "example: $0 <path to key>"
  exit 1
fi

rm -rf ${TEMPDIR}
mkdir -p ${TEMPDIR}

${SIGNAPPLE} sign -f --detach "${TEMPDIR}/${OUTROOT}"  "$@" "${BUNDLE}" --hardened-runtime

tar -C "${TEMPDIR}" -czf "${OUT}" .
rm -rf "${TEMPDIR}"
echo "Created ${OUT}"

#!/bin/sh
# Copyright (c) 2014-2021 The Bitcoin Core developers
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
if [ -z "$OSSLSIGNCODE" ]; then
  OSSLSIGNCODE=osslsigncode
fi

if [ -z "$1" ]; then
  echo "usage: $0 <osslcodesign args>"
  echo "example: $0 -key codesign.key"
  exit 1
fi

OUT=signature-win.tar.gz
SRCDIR=unsigned
WORKDIR=./.tmp
OUTDIR="${WORKDIR}/out"
OUTSUBDIR="${OUTDIR}/win"
TIMESERVER=http://timestamp.comodoca.com
CERTFILE="win-codesign.cert"

mkdir -p "${OUTSUBDIR}"
# shellcheck disable=SC2046
basename -a $(ls -1 "${SRCDIR}"/*-unsigned.exe) | while read UNSIGNED; do
  echo Signing "${UNSIGNED}"
  "${OSSLSIGNCODE}" sign -certs "${CERTFILE}" -t "${TIMESERVER}" -h sha256 -in "${SRCDIR}/${UNSIGNED}" -out "${WORKDIR}/${UNSIGNED}" "$@"
  "${OSSLSIGNCODE}" extract-signature -pem -in "${WORKDIR}/${UNSIGNED}" -out "${OUTSUBDIR}/${UNSIGNED}.pem" && rm "${WORKDIR}/${UNSIGNED}"
done

rm -f "${OUT}"
tar -C "${OUTDIR}" -czf "${OUT}" .
rm -rf "${WORKDIR}"
echo "Created ${OUT}"

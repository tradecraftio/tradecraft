#!/bin/sh
# Copyright (c) 2014-2015 The Bitcoin Core developers
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

set -e

ROOTDIR=dist
BUNDLE="${ROOTDIR}/Freicoin-Qt.app"
CODESIGN=codesign
TEMPDIR=sign.temp
TEMPLIST=${TEMPDIR}/signatures.txt
OUT=signature.tar.gz
OUTROOT=osx

if [ ! -n "$1" ]; then
  echo "usage: $0 <codesign args>"
  echo "example: $0 -s MyIdentity"
  exit 1
fi

rm -rf ${TEMPDIR} ${TEMPLIST}
mkdir -p ${TEMPDIR}

${CODESIGN} -f --file-list ${TEMPLIST} "$@" "${BUNDLE}"

grep -v CodeResources < "${TEMPLIST}" | while read i; do
  TARGETFILE="${BUNDLE}/`echo "${i}" | sed "s|.*${BUNDLE}/||"`"
  SIZE=`pagestuff "$i" -p | tail -2 | grep size | sed 's/[^0-9]*//g'`
  OFFSET=`pagestuff "$i" -p | tail -2 | grep offset | sed 's/[^0-9]*//g'`
  SIGNFILE="${TEMPDIR}/${OUTROOT}/${TARGETFILE}.sign"
  DIRNAME="`dirname "${SIGNFILE}"`"
  mkdir -p "${DIRNAME}"
  echo "Adding detached signature for: ${TARGETFILE}. Size: ${SIZE}. Offset: ${OFFSET}"
  dd if="$i" of="${SIGNFILE}" bs=1 skip=${OFFSET} count=${SIZE} 2>/dev/null
done

grep CodeResources < "${TEMPLIST}" | while read i; do
  TARGETFILE="${BUNDLE}/`echo "${i}" | sed "s|.*${BUNDLE}/||"`"
  RESOURCE="${TEMPDIR}/${OUTROOT}/${TARGETFILE}"
  DIRNAME="`dirname "${RESOURCE}"`"
  mkdir -p "${DIRNAME}"
  echo "Adding resource for: "${TARGETFILE}""
  cp "${i}" "${RESOURCE}"
done

rm ${TEMPLIST}

tar -C "${TEMPDIR}" -czf "${OUT}" .
rm -rf "${TEMPDIR}"
echo "Created ${OUT}"

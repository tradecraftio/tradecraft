#!/usr/bin/env bash
# Copyright (c) 2016-2019 The Bitcoin Core developers
# Copyright (c) 2010-2022 The Freicoin Developers
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
TOPDIR=${TOPDIR:-$(git rev-parse --show-toplevel)}
BUILDDIR=${BUILDDIR:-$TOPDIR}

BINDIR=${BINDIR:-$BUILDDIR/src}
MANDIR=${MANDIR:-$TOPDIR/doc/man}

FREICOIND=${FREICOIND:-$BINDIR/freicoind}
FREICOINCLI=${FREICOINCLI:-$BINDIR/freicoin-cli}
FREICOINTX=${FREICOINTX:-$BINDIR/freicoin-tx}
WALLET_TOOL=${WALLET_TOOL:-$BINDIR/freicoin-wallet}
FREICOINQT=${FREICOINQT:-$BINDIR/qt/freicoin-qt}

[ ! -x $FREICOIND ] && echo "$FREICOIND not found or not executable." && exit 1

# The autodetected version git tag can screw up manpage output a little bit
read -r -a FRCVER <<< "$($FREICOINCLI --version | head -n1 | awk -F'[ -]' '{ print $6, $7 }')"

# Create a footer file with copyright content.
# This gets autodetected fine for freicoind if --version-string is not set,
# but has different outcomes for freicoin-qt and freicoin-cli.
echo "[COPYRIGHT]" > footer.h2m
$FREICOIND --version | sed -n '1!p' >> footer.h2m

for cmd in $FREICOIND $FREICOINCLI $FREICOINTX $WALLET_TOOL $FREICOINQT; do
  cmdname="${cmd##*/}"
  help2man -N --version-string=${FRCVER[0]} --include=footer.h2m -o ${MANDIR}/${cmdname}.1 ${cmd}
  sed -i "s/\\\-${FRCVER[1]}//g" ${MANDIR}/${cmdname}.1
done

rm -f footer.h2m

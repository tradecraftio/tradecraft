#!/usr/bin/env bash

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
FRCVER=($($FREICOINCLI --version | head -n1 | awk -F'[ -]' '{ print $6, $7 }'))

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

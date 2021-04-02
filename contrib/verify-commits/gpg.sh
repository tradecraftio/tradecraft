#!/bin/sh
# Copyright (c) 2014-2016 The Bitcoin Core developers
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

INPUT=$(cat /dev/stdin)
VALID=false
REVSIG=false
IFS='
'
for LINE in $(echo "$INPUT" | gpg --trust-model always "$@" 2>/dev/null); do
	case "$LINE" in
	"[GNUPG:] VALIDSIG "*)
		while read KEY; do
			[ "${LINE#?GNUPG:? VALIDSIG * * * * * * * * * }" = "$KEY" ] && VALID=true
		done < ./contrib/verify-commits/trusted-keys
		;;
	"[GNUPG:] REVKEYSIG "*)
		[ "$FREICOIN_VERIFY_COMMITS_ALLOW_REVSIG" != 1 ] && exit 1
		REVSIG=true
		GOODREVSIG="[GNUPG:] GOODSIG ${LINE#* * *}"
		;;
	esac
done
if ! $VALID; then
	exit 1
fi
if $VALID && $REVSIG; then
	echo "$INPUT" | gpg --trust-model always "$@" | grep "\[GNUPG:\] \(NEWSIG\|SIG_ID\|VALIDSIG\)" 2>/dev/null
	echo "$GOODREVSIG"
else
	echo "$INPUT" | gpg --trust-model always "$@" 2>/dev/null
fi

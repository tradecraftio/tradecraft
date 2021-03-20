#!/bin/sh
# Copyright (c) 2012-2016 The Bitcoin Core developers
# Copyright (c) 2010-2021 The Freicoin Developers
#
# This program is free software: you can redistribute it and/or modify
# it under the conjunctive terms of BOTH version 3 of the GNU Affero
# General Public License as published by the Free Software Foundation
# AND the MIT/X11 software license.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Affero General Public License and the MIT/X11 software license for
# more details.
#
# You should have received a copy of both licenses along with this
# program.  If not, see <https://www.gnu.org/licenses/> and
# <http://www.opensource.org/licenses/mit-license.php>

if [ $# -gt 1 ]; then
    cd "$2"
fi
if [ $# -gt 0 ]; then
    FILE="$1"
    shift
    if [ -f "$FILE" ]; then
        INFO="$(head -n 1 "$FILE")"
    fi
else
    echo "Usage: $0 <filename> <srcroot>"
    exit 1
fi

DESC=""
SUFFIX=""
if [ -e "$(which git 2>/dev/null)" -a "$(git rev-parse --is-inside-work-tree 2>/dev/null)" = "true" ]; then
    # clean 'dirty' status of touched files that haven't been modified
    git diff >/dev/null 2>/dev/null 

    # if latest commit is tagged and not dirty, then override using the tag name
    RAWDESC=$(git describe --abbrev=0 2>/dev/null)
    if [ "$(git rev-parse HEAD)" = "$(git rev-list -1 $RAWDESC 2>/dev/null)" ]; then
        git diff-index --quiet HEAD -- && DESC=$RAWDESC
    fi

    # otherwise generate suffix from git, i.e. string like "59887e8-dirty"
    SUFFIX=$(git rev-parse --short HEAD)
    git diff-index --quiet HEAD -- || SUFFIX="$SUFFIX-dirty"
fi

if [ -n "$DESC" ]; then
    NEWINFO="#define BUILD_DESC \"$DESC\""
elif [ -n "$SUFFIX" ]; then
    NEWINFO="#define BUILD_SUFFIX $SUFFIX"
else
    NEWINFO="// No build information available"
fi

# only update build.h if necessary
if [ "$INFO" != "$NEWINFO" ]; then
    echo "$NEWINFO" >"$FILE"
fi

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

FREICOIN_CONFIG_ALL="--disable-dependency-tracking --prefix=$BASE_BUILD_DIR/depends/$HOST --bindir=$BASE_OUTDIR/bin --libdir=$BASE_OUTDIR/lib"
if [ -z "$NO_DEPENDS" ]; then
  DOCKER_EXEC ccache --max-size=$CCACHE_SIZE
fi

BEGIN_FOLD autogen
if [ -n "$CONFIG_SHELL" ]; then
  DOCKER_EXEC "$CONFIG_SHELL" -c "./autogen.sh"
else
  DOCKER_EXEC ./autogen.sh
fi
END_FOLD

mkdir -p build
cd build || (echo "could not enter build directory"; exit 1)

BEGIN_FOLD configure
DOCKER_EXEC ../configure --cache-file=config.cache $FREICOIN_CONFIG_ALL $FREICOIN_CONFIG || ( cat config.log && false)
END_FOLD

BEGIN_FOLD distdir
DOCKER_EXEC make distdir VERSION=$HOST
END_FOLD

cd "freicoin-$HOST" || (echo "could not enter distdir freicoin-$HOST"; exit 1)

BEGIN_FOLD configure
DOCKER_EXEC ./configure --cache-file=../config.cache $FREICOIN_CONFIG_ALL $FREICOIN_CONFIG || ( cat config.log && false)
END_FOLD

set -o errtrace
trap 'DOCKER_EXEC "cat ${BASE_BUILD_DIR}/sanitizer-output/* 2> /dev/null"' ERR

BEGIN_FOLD build
DOCKER_EXEC make $MAKEJOBS $GOAL || ( echo "Build failure. Verbose build follows." && DOCKER_EXEC make $GOAL V=1 ; false )
END_FOLD

cd ${BASE_BUILD_DIR} || (echo "could not enter travis build dir $BASE_BUILD_DIR"; exit 1)

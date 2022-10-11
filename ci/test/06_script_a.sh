#!/usr/bin/env bash
#
# Copyright (c) 2018-2020 The Bitcoin Core developers
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

export LC_ALL=C.UTF-8

FREICOIN_CONFIG_ALL="--enable-suppress-external-warnings --disable-dependency-tracking --prefix=$DEPENDS_DIR/$HOST --bindir=$BASE_OUTDIR/bin --libdir=$BASE_OUTDIR/lib"
if [ -z "$NO_WERROR" ]; then
  FREICOIN_CONFIG_ALL="${FREICOIN_CONFIG_ALL} --enable-werror"
fi
DOCKER_EXEC "ccache --zero-stats --max-size=$CCACHE_SIZE"

BEGIN_FOLD autogen
if [ -n "$CONFIG_SHELL" ]; then
  DOCKER_EXEC "$CONFIG_SHELL" -c "./autogen.sh"
else
  DOCKER_EXEC ./autogen.sh
fi
END_FOLD

DOCKER_EXEC mkdir -p "${BASE_BUILD_DIR}"
export P_CI_DIR="${BASE_BUILD_DIR}"

BEGIN_FOLD configure
DOCKER_EXEC "${BASE_ROOT_DIR}/configure" --cache-file=config.cache $FREICOIN_CONFIG_ALL $FREICOIN_CONFIG || ( (DOCKER_EXEC cat config.log) && false)
END_FOLD

BEGIN_FOLD distdir
DOCKER_EXEC make distdir VERSION=$HOST
END_FOLD

export P_CI_DIR="${BASE_BUILD_DIR}/freicoin-$HOST"

BEGIN_FOLD configure
DOCKER_EXEC ./configure --cache-file=../config.cache $FREICOIN_CONFIG_ALL $FREICOIN_CONFIG || ( (DOCKER_EXEC cat config.log) && false)
END_FOLD

set -o errtrace
trap 'DOCKER_EXEC "cat ${BASE_SCRATCH_DIR}/sanitizer-output/* 2> /dev/null"' ERR

if [[ ${USE_MEMORY_SANITIZER} == "true" ]]; then
  # MemorySanitizer (MSAN) does not support tracking memory initialization done by
  # using the Linux getrandom syscall. Avoid using getrandom by undefining
  # HAVE_SYS_GETRANDOM. See https://github.com/google/sanitizers/issues/852 for
  # details.
  DOCKER_EXEC 'grep -v HAVE_SYS_GETRANDOM src/config/freicoin-config.h > src/config/freicoin-config.h.tmp && mv src/config/freicoin-config.h.tmp src/config/freicoin-config.h'
fi

BEGIN_FOLD build
DOCKER_EXEC make $MAKEJOBS $GOAL || ( echo "Build failure. Verbose build follows." && DOCKER_EXEC make $GOAL V=1 ; false )
END_FOLD

BEGIN_FOLD cache_stats
DOCKER_EXEC "ccache --version | head -n 1 && ccache --show-stats"
DOCKER_EXEC du -sh "${DEPENDS_DIR}"/*/
DOCKER_EXEC du -sh "${PREVIOUS_RELEASES_DIR}"
END_FOLD

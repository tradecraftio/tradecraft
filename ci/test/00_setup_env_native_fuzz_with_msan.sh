#!/usr/bin/env bash
#
# Copyright (c) 2020-present The Bitcoin Core developers
# Copyright (c) 2010-2024 The Freicoin Developers
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

export CI_IMAGE_NAME_TAG="docker.io/ubuntu:24.04"
LIBCXX_DIR="/msan/cxx_build/"
export MSAN_FLAGS="-fsanitize=memory -fsanitize-memory-track-origins=2 -fno-omit-frame-pointer -g -O1 -fno-optimize-sibling-calls"
LIBCXX_FLAGS="-nostdinc++ -nostdlib++ -isystem ${LIBCXX_DIR}include/c++/v1 -L${LIBCXX_DIR}lib -Wl,-rpath,${LIBCXX_DIR}lib -lc++ -lc++abi -lpthread -Wno-unused-command-line-argument"
export MSAN_AND_LIBCXX_FLAGS="${MSAN_FLAGS} ${LIBCXX_FLAGS}"

export CONTAINER_NAME="ci_native_fuzz_msan"
export PACKAGES="ninja-build"
# BDB generates false-positives and will be removed in future
export DEP_OPTS="DEBUG=1 NO_BDB=1 NO_QT=1 CC=clang CXX=clang++ CFLAGS='${MSAN_FLAGS}' CXXFLAGS='${MSAN_AND_LIBCXX_FLAGS}'"
export GOAL="install"
# _FORTIFY_SOURCE is not compatible with MSAN.
export BITCOIN_CONFIG="--enable-fuzz --with-sanitizers=fuzzer,memory CPPFLAGS='-DBOOST_MULTI_INDEX_ENABLE_SAFE_MODE -U_FORTIFY_SOURCE'"
export USE_MEMORY_SANITIZER="true"
export RUN_UNIT_TESTS="false"
export RUN_FUNCTIONAL_TESTS="false"
export RUN_FUZZ_TESTS=true
export CCACHE_MAXSIZE=250M

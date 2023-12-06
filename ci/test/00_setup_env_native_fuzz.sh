#!/usr/bin/env bash
#
# Copyright (c) 2019-present The Bitcoin Core developers
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

export LC_ALL=C.UTF-8

export CI_IMAGE_NAME_TAG="docker.io/ubuntu:23.10"  # This version will reach EOL in Jul 2024, and can be replaced by "ubuntu:24.04" (or anything else that ships the wanted clang version).
export CONTAINER_NAME=ci_native_fuzz
export PACKAGES="clang-17 llvm-17 libclang-rt-17-dev libevent-dev libboost-dev libsqlite3-dev"
export NO_DEPENDS=1
export RUN_UNIT_TESTS=false
export RUN_FUNCTIONAL_TESTS=false
export RUN_FUZZ_TESTS=true
export GOAL="install"
export CI_CONTAINER_CAP="--cap-add SYS_PTRACE"  # If run with (ASan + LSan), the container needs access to ptrace (https://github.com/google/sanitizers/issues/764)
export FREICOIN_CONFIG="--enable-fuzz --with-sanitizers=fuzzer,address,undefined,float-divide-by-zero,integer \
CC='clang-17 -ftrivial-auto-var-init=pattern' CXX='clang++-17 -ftrivial-auto-var-init=pattern'"
export CCACHE_MAXSIZE=200M

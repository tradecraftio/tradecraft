#!/usr/bin/env bash
#
# Copyright (c) 2019-2022 The Bitcoin Core developers
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

export CI_IMAGE_NAME_TAG="debian:bookworm"
export CONTAINER_NAME=ci_native_fuzz_valgrind
export PACKAGES="clang llvm libclang-rt-dev python3 libevent-dev bsdmainutils libboost-dev libsqlite3-dev valgrind"
export NO_DEPENDS=1
export RUN_UNIT_TESTS=false
export RUN_FUNCTIONAL_TESTS=false
export RUN_FUZZ_TESTS=true
export FUZZ_TESTS_CONFIG="--valgrind"
export GOAL="install"
# Temporarily pin dwarf 4, until using Valgrind 3.20 or later
export FREICOIN_CONFIG="--enable-fuzz --with-sanitizers=fuzzer CC=clang CXX=clang++ CFLAGS='-gdwarf-4' CXXFLAGS='-gdwarf-4'"
export CCACHE_SIZE=200M

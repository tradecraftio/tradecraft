#!/usr/bin/env bash
#
# Copyright (c) 2019 The Bitcoin Core developers
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

export HOST=x86_64-unknown-linux-gnu
export PACKAGES="clang llvm python3 libssl1.0-dev libevent-dev bsdmainutils libboost-system-dev libboost-filesystem-dev libboost-chrono-dev libboost-test-dev libboost-thread-dev"
export NO_DEPENDS=1
export RUN_UNIT_TESTS=false
export RUN_FUNCTIONAL_TESTS=false
export RUN_FUZZ_TESTS=true
export GOAL="install"
export FREICOIN_CONFIG="--enable-fuzz --with-sanitizers=fuzzer,address,undefined CC=clang CXX=clang++"

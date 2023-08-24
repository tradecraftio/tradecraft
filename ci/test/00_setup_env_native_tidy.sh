#!/usr/bin/env bash
#
# Copyright (c) 2022 The Bitcoin Core developers
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

export DOCKER_NAME_TAG="ubuntu:22.04"
export CONTAINER_NAME=ci_native_tidy
export PACKAGES="clang libclang-dev llvm-dev clang-tidy bear cmake libevent-dev libboost-dev libminiupnpc-dev libnatpmp-dev libzmq3-dev systemtap-sdt-dev libqt5gui5 libqt5core5a libqt5dbus5 qttools5-dev qttools5-dev-tools libqrencode-dev libsqlite3-dev libdb++-dev"
export NO_DEPENDS=1
export RUN_UNIT_TESTS=false
export RUN_FUNCTIONAL_TESTS=false
export RUN_FUZZ_TESTS=false
export RUN_TIDY=true
export GOAL="install"
export FREICOIN_CONFIG="CC=clang CXX=clang++ --enable-c++20 --with-incompatible-bdb --disable-hardening CFLAGS='-O0 -g0' CXXFLAGS='-O0 -g0'"
export CCACHE_SIZE=200M

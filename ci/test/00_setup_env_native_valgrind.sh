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

export CONTAINER_NAME=ci_native_valgrind
export PACKAGES="valgrind clang llvm python3-zmq libevent-dev bsdmainutils libboost-system-dev libboost-filesystem-dev libboost-test-dev libboost-thread-dev libdb5.3++-dev libminiupnpc-dev libzmq3-dev"
export USE_VALGRIND=1
export NO_DEPENDS=1
export TEST_RUNNER_EXTRA="--exclude rpc_bind"  # Excluded for now, see https://github.com/bitcoin/bitcoin/issues/17765#issuecomment-602068547
export GOAL="install"
export BITCOIN_CONFIG="--enable-zmq --with-incompatible-bdb --with-gui=no CC=clang CXX=clang++"  # TODO enable GUI

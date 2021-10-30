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
export DOCKER_NAME_TAG=ubuntu:16.04
export PACKAGES="clang llvm python3-zmq qtbase5-dev qttools5-dev-tools libssl-dev libevent-dev bsdmainutils libboost-system-dev libboost-filesystem-dev libboost-chrono-dev libboost-test-dev libboost-thread-dev libdb5.3++-dev libminiupnpc-dev libzmq3-dev libqrencode-dev"
export NO_DEPENDS=1
export GOAL="install"
export BITCOIN_CONFIG="--enable-zmq --disable-wallet --with-gui=qt5 CPPFLAGS=-DDEBUG_LOCKORDER --with-sanitizers=thread --disable-hardening --disable-asm CC=clang CXX=clang++"

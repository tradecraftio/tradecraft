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

export HOST=i686-pc-linux-gnu
export DEP_OPTS="PROTOBUF=1"
export PACKAGES="g++-multilib python3-zmq"
export GOAL="install"
export FREICOIN_CONFIG="--enable-zmq --with-gui=qt5 --enable-bip70 --enable-glibc-back-compat --enable-reduce-exports LDFLAGS=-static-libstdc++"
export CONFIG_SHELL="/bin/dash"

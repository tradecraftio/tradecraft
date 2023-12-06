#!/usr/bin/env bash
#
# Copyright (c) 2020-2022 The Bitcoin Core developers
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

export HOST=i686-pc-linux-gnu
export CONTAINER_NAME=ci_i686_centos
export CI_IMAGE_NAME_TAG="quay.io/centos/amd64:stream9"
export CI_BASE_PACKAGES="gcc-c++ glibc-devel.x86_64 libstdc++-devel.x86_64 glibc-devel.i686 libstdc++-devel.i686 ccache libtool make git python3 python3-pip which patch lbzip2 xz procps-ng dash rsync coreutils bison util-linux e2fsprogs"
export PIP_PACKAGES="pyzmq"
export GOAL="install"
export NO_WERROR=1  # Suppress error: #warning _FORTIFY_SOURCE > 2 is treated like 2 on this platform [-Werror=cpp]
export FREICOIN_CONFIG="--enable-zmq --with-gui=qt5 --enable-reduce-exports"
export CONFIG_SHELL="/bin/dash"

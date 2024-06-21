#!/usr/bin/env bash
#
# Copyright (c) 2019-present The Bitcoin Core developers
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

export HOST=aarch64-linux-android
export PACKAGES="unzip openjdk-8-jdk gradle"
export CONTAINER_NAME=ci_android
export CI_IMAGE_NAME_TAG="docker.io/amd64/ubuntu:22.04"

export RUN_UNIT_TESTS=false
export RUN_FUNCTIONAL_TESTS=false

export ANDROID_API_LEVEL=28
export ANDROID_BUILD_TOOLS_VERSION=28.0.3
export ANDROID_NDK_VERSION=23.2.8568313
export ANDROID_TOOLS_URL=https://dl.google.com/android/repository/commandlinetools-linux-8512546_latest.zip
export ANDROID_HOME="${DEPENDS_DIR}/SDKs/android"
export ANDROID_NDK_HOME="${ANDROID_HOME}/ndk/${ANDROID_NDK_VERSION}"
export DEP_OPTS="ANDROID_SDK=${ANDROID_HOME} ANDROID_NDK=${ANDROID_NDK_HOME} ANDROID_API_LEVEL=${ANDROID_API_LEVEL} ANDROID_TOOLCHAIN_BIN=${ANDROID_NDK_HOME}/toolchains/llvm/prebuilt/linux-x86_64/bin/"

export FREICOIN_CONFIG="--disable-tests --enable-gui-tests --disable-bench --disable-fuzz-binary --without-utils --without-libs --without-daemon"

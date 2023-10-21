#!/usr/bin/env bash
#
# Copyright (c) 2018-2022 The Bitcoin Core developers
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

export LC_ALL=C

${CI_RETRY_EXE} apt-get update
# Lint dependencies:
# - curl/xz-utils (to install shellcheck)
# - git (used in many lint scripts)
# - gpg (used by verify-commits)
${CI_RETRY_EXE} apt-get install -y curl xz-utils git gpg

if [ -z "${SKIP_PYTHON_INSTALL}" ]; then
    PYTHON_PATH=/tmp/python
    if [ ! -d "${PYTHON_PATH}/bin" ]; then
      (
        git clone https://github.com/pyenv/pyenv.git
        cd pyenv/plugins/python-build || exit 1
        ./install.sh
      )
      # For dependencies see https://github.com/pyenv/pyenv/wiki#suggested-build-environment
      ${CI_RETRY_EXE} apt-get install -y build-essential libssl-dev zlib1g-dev \
        libbz2-dev libreadline-dev libsqlite3-dev curl llvm \
        libncursesw5-dev xz-utils tk-dev libxml2-dev libxmlsec1-dev libffi-dev liblzma-dev \
        clang
      env CC=clang python-build "$(cat "${BASE_ROOT_DIR}/.python-version")" "${PYTHON_PATH}"
    fi
    export PATH="${PYTHON_PATH}/bin:${PATH}"
    command -v python3
    python3 --version
fi

${CI_RETRY_EXE} pip3 install codespell==2.2.1
${CI_RETRY_EXE} pip3 install flake8==5.0.4
${CI_RETRY_EXE} pip3 install mypy==0.971
${CI_RETRY_EXE} pip3 install pyzmq==24.0.1
${CI_RETRY_EXE} pip3 install vulture==2.6

SHELLCHECK_VERSION=v0.8.0
curl -sL "https://github.com/koalaman/shellcheck/releases/download/${SHELLCHECK_VERSION}/shellcheck-${SHELLCHECK_VERSION}.linux.x86_64.tar.xz" | \
    tar --xz -xf - --directory /tmp/
mv "/tmp/shellcheck-${SHELLCHECK_VERSION}/shellcheck" /usr/bin/

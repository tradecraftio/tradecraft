#!/usr/bin/env bash
#
# Copyright (c) 2018-2021 The Bitcoin Core developers
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
${CI_RETRY_EXE} apt-get install -y clang-format-9 python3-pip curl git gawk jq
update-alternatives --install /usr/bin/clang-format      clang-format      "$(which clang-format-9     )" 100
update-alternatives --install /usr/bin/clang-format-diff clang-format-diff "$(which clang-format-diff-9)" 100

${CI_RETRY_EXE} pip3 install codespell==2.1.0
${CI_RETRY_EXE} pip3 install flake8==4.0.1
${CI_RETRY_EXE} pip3 install mypy==0.942
${CI_RETRY_EXE} pip3 install pyzmq==22.3.0
${CI_RETRY_EXE} pip3 install vulture==2.3

SHELLCHECK_VERSION=v0.8.0
curl -sL "https://github.com/koalaman/shellcheck/releases/download/${SHELLCHECK_VERSION}/shellcheck-${SHELLCHECK_VERSION}.linux.x86_64.tar.xz" | tar --xz -xf - --directory /tmp/
export PATH="/tmp/shellcheck-${SHELLCHECK_VERSION}:${PATH}"

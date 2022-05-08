#!/usr/bin/env bash
#
# Copyright (c) 2018-2019 The Bitcoin Core developers
# Copyright (c) 2010-2022 The Freicoin Developers
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

travis_retry sudo apt update && sudo apt install -y clang-format-9
sudo update-alternatives --install /usr/bin/clang-format      clang-format      $(which clang-format-9     ) 100
sudo update-alternatives --install /usr/bin/clang-format-diff clang-format-diff $(which clang-format-diff-9) 100

travis_retry pip3 install codespell==1.17.1
travis_retry pip3 install flake8==3.8.3
travis_retry pip3 install yq
travis_retry pip3 install mypy==0.781

SHELLCHECK_VERSION=v0.7.1
curl -sL "https://github.com/koalaman/shellcheck/releases/download/${SHELLCHECK_VERSION}/shellcheck-${SHELLCHECK_VERSION}.linux.x86_64.tar.xz" | tar --xz -xf - --directory /tmp/
export PATH="/tmp/shellcheck-${SHELLCHECK_VERSION}:${PATH}"

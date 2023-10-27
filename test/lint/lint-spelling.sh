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
#
# Warn in case of spelling errors.
# Note: Will exit successfully regardless of spelling errors.

export LC_ALL=C

if ! command -v codespell > /dev/null; then
    echo "Skipping spell check linting since codespell is not installed."
    exit 0
fi

IGNORE_WORDS_FILE=test/lint/lint-spelling.ignore-words.txt
mapfile -t FILES < <(git ls-files -- ":(exclude)build-aux/m4/" ":(exclude)contrib/seeds/*.txt" ":(exclude)depends/" ":(exclude)doc/release-notes/" ":(exclude)src/leveldb/" ":(exclude)src/crc32c/" ":(exclude)src/qt/locale/" ":(exclude)src/qt/*.qrc" ":(exclude)src/secp256k1/" ":(exclude)src/minisketch/" ":(exclude)src/univalue/" ":(exclude)contrib/builder-keys/keys.txt" ":(exclude)contrib/guix/patches")
if ! codespell --check-filenames --disable-colors --quiet-level=7 --ignore-words=${IGNORE_WORDS_FILE} "${FILES[@]}"; then
    echo "^ Warning: codespell identified likely spelling errors. Any false positives? Add them to the list of ignored words in ${IGNORE_WORDS_FILE}"
fi

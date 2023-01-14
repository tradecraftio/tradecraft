#!/usr/bin/env python3
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

"""
Warn in case of spelling errors.
Note: Will exit successfully regardless of spelling errors.
"""

from subprocess import check_output, STDOUT, CalledProcessError

IGNORE_WORDS_FILE = 'test/lint/spelling.ignore-words.txt'
FILES_ARGS = ['git', 'ls-files', '--', ":(exclude)build-aux/m4/", ":(exclude)contrib/seeds/*.txt", ":(exclude)depends/", ":(exclude)doc/release-notes/", ":(exclude)src/leveldb/", ":(exclude)src/crc32c/", ":(exclude)src/qt/locale/", ":(exclude)src/qt/*.qrc", ":(exclude)src/secp256k1/", ":(exclude)src/minisketch/", ":(exclude)contrib/builder-keys/keys.txt", ":(exclude)contrib/guix/patches"]


def check_codespell_install():
    try:
        check_output(["codespell", "--version"])
    except FileNotFoundError:
        print("Skipping spell check linting since codespell is not installed.")
        exit(0)


def main():
    check_codespell_install()

    files = check_output(FILES_ARGS).decode("utf-8").splitlines()
    codespell_args = ['codespell', '--check-filenames', '--disable-colors', '--quiet-level=7', '--ignore-words={}'.format(IGNORE_WORDS_FILE)] + files

    try:
        check_output(codespell_args, stderr=STDOUT)
    except CalledProcessError as e:
        print(e.output.decode("utf-8"), end="")
        print('^ Warning: codespell identified likely spelling errors. Any false positives? Add them to the list of ignored words in {}'.format(IGNORE_WORDS_FILE))


if __name__ == "__main__":
    main()

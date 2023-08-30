#!/usr/bin/env python3
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
#
# Check that all logs are terminated with '\n'
#
# Some logs are continued over multiple lines. They should be explicitly
# commented with /* Continued */

import re
import sys

from subprocess import check_output


def main():
    logs_list = check_output(["git", "grep", "--extended-regexp", r"(LogPrintLevel|LogPrintfCategory|LogPrintf?)\(", "--", "*.cpp"], text=True, encoding="utf8").splitlines()

    unterminated_logs = [line for line in logs_list if not re.search(r'(\\n"|/\* Continued \*/)', line)]

    if unterminated_logs != []:
        print("All calls to LogPrintf(), LogPrintfCategory(), LogPrint(), LogPrintLevel(), and WalletLogPrintf() should be terminated with \"\\n\".")
        print("")

        for line in unterminated_logs:
            print(line)

        sys.exit(1)


if __name__ == "__main__":
    main()

#!/usr/bin/env python3
# Copyright (c) 2015-2018 The Bitcoin Core developers
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

'''
This checks if all command line args are documented.
Return value is 0 to indicate no error.

Author: @MarcoFalke
'''

from subprocess import check_output
import re
import sys

FOLDER_GREP = 'src'
FOLDER_TEST = 'src/test/'
REGEX_ARG = '(?:ForceSet|SoftSet|Get|Is)(?:Bool)?Args?(?:Set)?\("(-[^"]+)"'
REGEX_DOC = 'AddArg\("(-[^"=]+?)(?:=|")'
CMD_ROOT_DIR = '`git rev-parse --show-toplevel`/{}'.format(FOLDER_GREP)
CMD_GREP_ARGS = r"git grep --perl-regexp '{}' -- {} ':(exclude){}'".format(REGEX_ARG, CMD_ROOT_DIR, FOLDER_TEST)
CMD_GREP_DOCS = r"git grep --perl-regexp '{}' {}".format(REGEX_DOC, CMD_ROOT_DIR)
# list unsupported, deprecated and duplicate args as they need no documentation
SET_DOC_OPTIONAL = set(['-rpcssl', '-benchmark', '-h', '-help', '-socks', '-tor', '-debugnet', '-whitelistalwaysrelay', '-promiscuousmempoolflags', '-blockminsize', '-dbcrashratio', '-forcecompactdb', '-usehd'])


def main():
    used = check_output(CMD_GREP_ARGS, shell=True, universal_newlines=True)
    docd = check_output(CMD_GREP_DOCS, shell=True, universal_newlines=True)

    args_used = set(re.findall(re.compile(REGEX_ARG), used))
    args_docd = set(re.findall(re.compile(REGEX_DOC), docd)).union(SET_DOC_OPTIONAL)
    args_need_doc = args_used.difference(args_docd)
    args_unknown = args_docd.difference(args_used)

    print("Args used        : {}".format(len(args_used)))
    print("Args documented  : {}".format(len(args_docd)))
    print("Args undocumented: {}".format(len(args_need_doc)))
    print(args_need_doc)
    print("Args unknown     : {}".format(len(args_unknown)))
    print(args_unknown)

    sys.exit(len(args_need_doc))


if __name__ == "__main__":
    main()

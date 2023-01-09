#!/usr/bin/env python3
# Copyright (c) 2020 The Bitcoin Core developers
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

import argparse
import subprocess
import requests
import sys

parser = argparse.ArgumentParser(description='Script to get coins from a faucet.', epilog='You may need to start with double-dash (--) when providing freicoin-cli arguments.')
parser.add_argument('-c', '--cmd', dest='cmd', default='freicoin-cli', help='freicoin-cli command to use')
parser.add_argument('-f', '--faucet', dest='faucet', default='https://signetfaucet.com/claim', help='URL of the faucet')
parser.add_argument('-a', '--addr', dest='addr', default='', help='Freicoin address to which the faucet should send')
parser.add_argument('-p', '--password', dest='password', default='', help='Faucet password, if any')
parser.add_argument('freicoin_cli_args', nargs='*', help='Arguments to pass on to freicoin-cli (default: -signet)')

args = parser.parse_args()

if args.addr == '':
    if args.freicoin_cli_args == []:
        args.freicoin_cli_args = ['-signet']
    # get address for receiving coins
    try:
        args.addr = subprocess.check_output([args.cmd] + args.freicoin_cli_args + ['getnewaddress', 'faucet', 'bech32']).strip()
    except FileNotFoundError:
        print('The binary', args.cmd, 'could not be found.')
        exit()

data = {'address': args.addr, 'password': args.password}
try:
    res = requests.post(args.faucet, data=data)
except:
    print('Unexpected error when contacting faucet:', sys.exc_info()[0])
    exit()
print(res.text)

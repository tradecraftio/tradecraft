#!/usr/bin/env python3
# Copyright (c) 2018-2022 The Bitcoin Core developers
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

import os
import sys
import argparse
import json

def perform_pre_checks():
    mock_result_path = os.path.join(os.getcwd(), "mock_result")
    if os.path.isfile(mock_result_path):
        with open(mock_result_path, "r", encoding="utf8") as f:
            mock_result = f.read()
        if mock_result[0]:
            sys.stdout.write(mock_result[2:])
            sys.exit(int(mock_result[0]))

def enumerate(args):
    sys.stdout.write(json.dumps([{"fingerprint": "00000001", "type": "trezor", "model": "trezor_t"}]))

def getdescriptors(args):
    xpub = "tpubD6NzVbkrYhZ4WaWSyoBvQwbpLkojyoTZPRsgXELWz3Popb3qkjcJyJUGLnL4qHHoQvao8ESaAstxYSnhyswJ76uZPStJRJCTKvosUCJZL5B"

    sys.stdout.write(json.dumps({
        "receive": [
            "pkh([00000001/44'/1'/" + args.account + "']" + xpub + "/0/*)#vt6w3l3j",
            "wpk([00000001/84'/1'/" + args.account + "']" + xpub + "/0/*)#5zw5r7xs",
        ],
        "internal": [
            "pkh([00000001/44'/1'/" + args.account + "']" + xpub + "/1/*)#all0v2p2",
            "wpk([00000001/84'/1'/" + args.account + "']" + xpub + "/1/*)#9kt47tkg",
        ]
    }))


def displayaddress(args):
    # Several descriptor formats are acceptable, so allowing for potential
    # changes to InferDescriptor:
    if args.fingerprint != "00000001":
        return sys.stdout.write(json.dumps({"error": "Unexpected fingerprint", "fingerprint": args.fingerprint}))

    expected_desc = [
        "wpk([00000001/84'/1'/0'/0/0]02c97dc3f4420402e01a113984311bf4a1b8de376cac0bdcfaf1b3ac81f13433c7)#0yneg42r",
    ]
    if args.desc not in expected_desc:
        return sys.stdout.write(json.dumps({"error": "Unexpected descriptor", "desc": args.desc}))

    return sys.stdout.write(json.dumps({"address": "fcrt1qm90ugl4d48jv8n6e5t9ln6t9zlpm5th6pwvzqu"}))

def signtx(args):
    if args.fingerprint != "00000001":
        return sys.stdout.write(json.dumps({"error": "Unexpected fingerprint", "fingerprint": args.fingerprint}))

    with open(os.path.join(os.getcwd(), "mock_pst"), "r", encoding="utf8") as f:
        mock_pst = f.read()

    if args.fingerprint == "00000001" :
        sys.stdout.write(json.dumps({
            "pst": mock_pst,
            "complete": True
        }))
    else:
        sys.stdout.write(json.dumps({"pst": args.pst}))

parser = argparse.ArgumentParser(prog='./signer.py', description='External signer mock')
parser.add_argument('--fingerprint')
parser.add_argument('--chain', default='main')
parser.add_argument('--stdin', action='store_true')

subparsers = parser.add_subparsers(description='Commands', dest='command')
subparsers.required = True

parser_enumerate = subparsers.add_parser('enumerate', help='list available signers')
parser_enumerate.set_defaults(func=enumerate)

parser_getdescriptors = subparsers.add_parser('getdescriptors')
parser_getdescriptors.set_defaults(func=getdescriptors)
parser_getdescriptors.add_argument('--account', metavar='account')

parser_displayaddress = subparsers.add_parser('displayaddress', help='display address on signer')
parser_displayaddress.add_argument('--desc', metavar='desc')
parser_displayaddress.set_defaults(func=displayaddress)

parser_signtx = subparsers.add_parser('signtx')
parser_signtx.add_argument('pst', metavar='pst')

parser_signtx.set_defaults(func=signtx)

if not sys.stdin.isatty():
    buffer = sys.stdin.read()
    if buffer and buffer.rstrip() != "":
        sys.argv.extend(buffer.rstrip().split(" "))

args = parser.parse_args()

perform_pre_checks()

args.func(args)

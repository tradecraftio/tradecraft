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

import sys
import os
from random import SystemRandom
import base64
import hmac

def generate_salt():
    # This uses os.urandom() underneath
    cryptogen = SystemRandom()

    # Create 16 byte hex salt
    salt_sequence = [cryptogen.randrange(256) for _ in range(16)]
    return ''.join([format(r, 'x') for r in salt_sequence])

def generate_password():
    """Create 32 byte b64 password"""
    return base64.urlsafe_b64encode(os.urandom(32)).decode('utf-8')

def password_to_hmac(salt, password):
    m = hmac.new(bytearray(salt, 'utf-8'), bytearray(password, 'utf-8'), 'SHA256')
    return m.hexdigest()

def main():
    if len(sys.argv) < 2:
        sys.stderr.write('Please include username (and an optional password, will generate one if not provided) as an argument.\n')
        sys.exit(0)

    username = sys.argv[1]

    salt = generate_salt()
    if len(sys.argv) > 2:
        password = sys.argv[2]
    else:
        password = generate_password()
    password_hmac = password_to_hmac(salt, password)

    print('String to be appended to freicoin.conf:')
    print('rpcauth={0}:{1}${2}'.format(username, salt, password_hmac))
    print('Your password:\n{0}'.format(password))

if __name__ == '__main__':
    main()

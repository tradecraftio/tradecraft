#!/usr/bin/env python3
# Copyright (c) 2015-2018 The Bitcoin Core developers
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
"""Test share/rpcauth/rpcauth.py
"""
import base64
import configparser
import hmac
import importlib
import os
import sys
import unittest

class TestRPCAuth(unittest.TestCase):
    def setUp(self):
        config = configparser.ConfigParser()
        config_path = os.path.abspath(
            os.path.join(os.sep, os.path.abspath(os.path.dirname(__file__)),
            "../config.ini"))
        with open(config_path, encoding="utf8") as config_file:
            config.read_file(config_file)
        sys.path.insert(0, os.path.dirname(config['environment']['RPCAUTH']))
        self.rpcauth = importlib.import_module('rpcauth')

    def test_generate_salt(self):
        for i in range(16, 32 + 1):
            self.assertEqual(len(self.rpcauth.generate_salt(i)), i * 2)

    def test_generate_password(self):
        password = self.rpcauth.generate_password()
        expected_password = base64.urlsafe_b64encode(
            base64.urlsafe_b64decode(password)).decode('utf-8')
        self.assertEqual(expected_password, password)

    def test_check_password_hmac(self):
        salt = self.rpcauth.generate_salt(16)
        password = self.rpcauth.generate_password()
        password_hmac = self.rpcauth.password_to_hmac(salt, password)

        m = hmac.new(bytearray(salt, 'utf-8'),
            bytearray(password, 'utf-8'), 'SHA256')
        expected_password_hmac = m.hexdigest()

        self.assertEqual(expected_password_hmac, password_hmac)

if __name__ == '__main__':
    unittest.main()

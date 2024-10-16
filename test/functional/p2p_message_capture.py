#!/usr/bin/env python3
# Copyright (c) 2020-2022 The Bitcoin Core developers
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
"""Test per-peer message capture capability.

Additionally, the output of contrib/message-capture/message-capture-parser.py should be verified manually.
"""

import glob
from io import BytesIO
import os

from test_framework.p2p import P2PDataStore, MESSAGEMAP
from test_framework.test_framework import FreicoinTestFramework
from test_framework.util import assert_equal

TIME_SIZE = 8
LENGTH_SIZE = 4
MSGTYPE_SIZE = 12

def mini_parser(dat_file: str) -> None:
    """Parse a data file created by CaptureMessageToFile.

    From the data file we'll only check the structure.

    We won't care about things like:
    - Deserializing the payload of the message
        - This is managed by the deserialize methods in test_framework.messages
    - The order of the messages
        - There's no reason why we can't, say, change the order of the messages in the handshake
    - Message Type
        - We can add new message types

    We're ignoring these because they're simply too brittle to test here.
    """
    with open(dat_file, 'rb') as f_in:
        # This should have at least one message in it
        assert os.fstat(f_in.fileno()).st_size >= TIME_SIZE + LENGTH_SIZE + MSGTYPE_SIZE
        while True:
            tmp_header_raw = f_in.read(TIME_SIZE + LENGTH_SIZE + MSGTYPE_SIZE)
            if not tmp_header_raw:
                break
            tmp_header = BytesIO(tmp_header_raw)
            tmp_header.read(TIME_SIZE) # skip the timestamp field
            msgtype = tmp_header.read(MSGTYPE_SIZE).rstrip(b'\x00')
            assert msgtype in MESSAGEMAP
            length: int = int.from_bytes(tmp_header.read(LENGTH_SIZE), "little")
            data = f_in.read(length)
            assert_equal(len(data), length)



class MessageCaptureTest(FreicoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1
        self.extra_args = [["-capturemessages"]]
        self.setup_clean_chain = True

    def run_test(self):
        capturedir = self.nodes[0].chain_path / "message_capture"
        # Connect a node so that the handshake occurs
        self.nodes[0].add_p2p_connection(P2PDataStore())
        self.nodes[0].disconnect_p2ps()
        recv_file = glob.glob(os.path.join(capturedir, "*/msgs_recv.dat"))[0]
        mini_parser(recv_file)
        sent_file = glob.glob(os.path.join(capturedir, "*/msgs_sent.dat"))[0]
        mini_parser(sent_file)


if __name__ == '__main__':
    MessageCaptureTest().main()

#!/usr/bin/env python3
# Copyright (c) 2014-2016 The Bitcoin Core developers
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

"""
    ZMQ example using python3's asyncio

    Freicoin should be started with the command line arguments:
        freicoind -testnet -daemon \
                -zmqpubhashblock=tcp://127.0.0.1:28102 \
                -zmqpubrawtx=tcp://127.0.0.1:28102 \
                -zmqpubhashtx=tcp://127.0.0.1:28102 \
                -zmqpubhashblock=tcp://127.0.0.1:28102

    We use the asyncio library here.  `self.handle()` installs itself as a
    future at the end of the function.  Since it never returns with the event
    loop having an empty stack of futures, this creates an infinite loop.  An
    alternative is to wrap the contents of `handle` inside `while True`.

    The `@asyncio.coroutine` decorator and the `yield from` syntax found here
    was introduced in python 3.4 and has been deprecated in favor of the `async`
    and `await` keywords respectively.

    A blocking example using python 2.7 can be obtained from the git history:
    https://github.com/tradecraftio/tradecraft/blob/37a7fe9e440b83e2364d5498931253937abe9294/contrib/zmq/zmq_sub.py
"""

import binascii
import asyncio
import zmq
import zmq.asyncio
import signal
import struct
import sys

if not (sys.version_info.major >= 3 and sys.version_info.minor >= 4):
    print("This example only works with Python 3.4 and greater")
    exit(1)

port = 28102

class ZMQHandler():
    def __init__(self):
        self.loop = zmq.asyncio.install()
        self.zmqContext = zmq.asyncio.Context()

        self.zmqSubSocket = self.zmqContext.socket(zmq.SUB)
        self.zmqSubSocket.setsockopt_string(zmq.SUBSCRIBE, "hashblock")
        self.zmqSubSocket.setsockopt_string(zmq.SUBSCRIBE, "hashtx")
        self.zmqSubSocket.setsockopt_string(zmq.SUBSCRIBE, "rawblock")
        self.zmqSubSocket.setsockopt_string(zmq.SUBSCRIBE, "rawtx")
        self.zmqSubSocket.connect("tcp://127.0.0.1:%i" % port)

    @asyncio.coroutine
    def handle(self) :
        msg = yield from self.zmqSubSocket.recv_multipart()
        topic = msg[0]
        body = msg[1]
        sequence = "Unknown"
        if len(msg[-1]) == 4:
          msgSequence = struct.unpack('<I', msg[-1])[-1]
          sequence = str(msgSequence)
        if topic == b"hashblock":
            print('- HASH BLOCK ('+sequence+') -')
            print(binascii.hexlify(body))
        elif topic == b"hashtx":
            print('- HASH TX  ('+sequence+') -')
            print(binascii.hexlify(body))
        elif topic == b"rawblock":
            print('- RAW BLOCK HEADER ('+sequence+') -')
            print(binascii.hexlify(body[:80]))
        elif topic == b"rawtx":
            print('- RAW TX ('+sequence+') -')
            print(binascii.hexlify(body))
        # schedule ourselves to receive the next message
        asyncio.ensure_future(self.handle())

    def start(self):
        self.loop.add_signal_handler(signal.SIGINT, self.stop)
        self.loop.create_task(self.handle())
        self.loop.run_forever()

    def stop(self):
        self.loop.stop()
        self.zmqContext.destroy()

daemon = ZMQHandler()
daemon.start()

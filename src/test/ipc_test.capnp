# Copyright (c) 2023 The Bitcoin Core developers
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

@0xd71b0fc8727fdf83;

using Cxx = import "/capnp/c++.capnp";
$Cxx.namespace("gen");

using Proxy = import "/mp/proxy.capnp";
$Proxy.include("test/ipc_test.h");
$Proxy.includeTypes("ipc/capnp/common-types.h");

interface FooInterface $Proxy.wrap("FooImplementation") {
    add @0 (a :Int32, b :Int32) -> (result :Int32);
    passOutPoint @1 (arg :Data) -> (result :Data);
    passUniValue @2 (arg :Text) -> (result :Text);
}

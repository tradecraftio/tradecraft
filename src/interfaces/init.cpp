// Copyright (c) 2021 The Bitcoin Core developers
// Copyright (c) 2011-2023 The Freicoin Developers
//
// This program is free software: you can redistribute it and/or modify it under
// the terms of version 3 of the GNU Affero General Public License as published
// by the Free Software Foundation.
//
// This program is distributed in the hope that it will be useful, but WITHOUT
// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
// FOR A PARTICULAR PURPOSE.  See the GNU Affero General Public License for more
// details.
//
// You should have received a copy of the GNU Affero General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.

#include <interfaces/chain.h>
#include <interfaces/echo.h>
#include <interfaces/init.h>
#include <interfaces/node.h>
#include <interfaces/wallet.h>

namespace interfaces {
std::unique_ptr<Node> Init::makeNode() { return {}; }
std::unique_ptr<Chain> Init::makeChain() { return {}; }
std::unique_ptr<WalletLoader> Init::makeWalletLoader(Chain& chain) { return {}; }
std::unique_ptr<Echo> Init::makeEcho() { return {}; }
Ipc* Init::ipc() { return nullptr; }
} // namespace interfaces

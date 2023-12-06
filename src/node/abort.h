// Copyright (c) 2023 The Bitcoin Core developers
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

#ifndef FREICOIN_NODE_ABORT_H
#define FREICOIN_NODE_ABORT_H

#include <util/translation.h>

#include <atomic>
#include <string>

namespace node {
void AbortNode(std::atomic<int>& exit_status, const std::string& debug_message, const bilingual_str& user_message = {}, bool shutdown = true);
} // namespace node

#endif // FREICOIN_NODE_ABORT_H

// Copyright (c) 2019-2022 The Bitcoin Core developers
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

#include <crypto/siphash.h>
#include <random.h>
#include <span.h>
#include <util/hasher.h>

SaltedTxidHasher::SaltedTxidHasher() : k0(GetRand<uint64_t>()), k1(GetRand<uint64_t>()) {}

SaltedOutpointHasher::SaltedOutpointHasher(bool deterministic) :
    k0(deterministic ? 0x8e819f2607a18de6 : GetRand<uint64_t>()),
    k1(deterministic ? 0xf4020d2e3983b0eb : GetRand<uint64_t>())
{}

SaltedSipHasher::SaltedSipHasher() : m_k0(GetRand<uint64_t>()), m_k1(GetRand<uint64_t>()) {}

size_t SaltedSipHasher::operator()(const Span<const unsigned char>& script) const
{
    return CSipHasher(m_k0, m_k1).Write(script).Finalize();
}

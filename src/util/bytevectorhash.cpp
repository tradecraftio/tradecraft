// Copyright (c) 2018 The Bitcoin Core developers
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
#include <util/bytevectorhash.h>

ByteVectorHash::ByteVectorHash()
{
    GetRandBytes(reinterpret_cast<unsigned char*>(&m_k0), sizeof(m_k0));
    GetRandBytes(reinterpret_cast<unsigned char*>(&m_k1), sizeof(m_k1));
}

size_t ByteVectorHash::operator()(const std::vector<unsigned char>& input) const
{
    return CSipHasher(m_k0, m_k1).Write(input.data(), input.size()).Finalize();
}

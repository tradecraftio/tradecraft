// Copyright (c) 2020-2021 The Bitcoin Core developers
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

#ifndef FREICOIN_UTIL_HASH_TYPE_H
#define FREICOIN_UTIL_HASH_TYPE_H

template <typename HashType>
class BaseHash
{
protected:
    HashType m_hash;

public:
    BaseHash() : m_hash() {}
    explicit BaseHash(const HashType& in) : m_hash(in) {}

    SERIALIZE_METHODS(BaseHash, obj) {
        READWRITE(obj.m_hash);
    }

    bool IsNull() const
    {
        return m_hash.IsNull();
    }

    unsigned char* begin()
    {
        return m_hash.begin();
    }

    const unsigned char* begin() const
    {
        return m_hash.begin();
    }

    unsigned char* end()
    {
        return m_hash.end();
    }

    const unsigned char* end() const
    {
        return m_hash.end();
    }

    operator std::vector<unsigned char>() const
    {
        return std::vector<unsigned char>{m_hash.begin(), m_hash.end()};
    }

    std::string ToString() const
    {
        return m_hash.ToString();
    }

    bool operator==(const BaseHash<HashType>& other) const noexcept
    {
        return m_hash == other.m_hash;
    }

    bool operator!=(const BaseHash<HashType>& other) const noexcept
    {
        return !(m_hash == other.m_hash);
    }

    bool operator<(const BaseHash<HashType>& other) const noexcept
    {
        return m_hash < other.m_hash;
    }

    size_t size() const
    {
        return m_hash.size();
    }

    unsigned char* data() { return m_hash.data(); }
    const unsigned char* data() const { return m_hash.data(); }
};

#endif // FREICOIN_UTIL_HASH_TYPE_H

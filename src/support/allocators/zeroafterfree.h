// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2022 The Bitcoin Core developers
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

#ifndef FREICOIN_SUPPORT_ALLOCATORS_ZEROAFTERFREE_H
#define FREICOIN_SUPPORT_ALLOCATORS_ZEROAFTERFREE_H

#include <support/cleanse.h>

#include <memory>
#include <vector>

template <typename T>
struct zero_after_free_allocator {
    using value_type = T;

    zero_after_free_allocator() noexcept = default;
    template <typename U>
    zero_after_free_allocator(const zero_after_free_allocator<U>&) noexcept
    {
    }

    T* allocate(std::size_t n)
    {
        return std::allocator<T>{}.allocate(n);
    }

    void deallocate(T* p, std::size_t n)
    {
        if (p != nullptr)
            memory_cleanse(p, sizeof(T) * n);
        std::allocator<T>{}.deallocate(p, n);
    }

    template <typename U>
    friend bool operator==(const zero_after_free_allocator&, const zero_after_free_allocator<U>&) noexcept
    {
        return true;
    }
    template <typename U>
    friend bool operator!=(const zero_after_free_allocator&, const zero_after_free_allocator<U>&) noexcept
    {
        return false;
    }
};

/** Byte-vector that clears its contents before deletion. */
using SerializeData = std::vector<std::byte, zero_after_free_allocator<std::byte>>;

#endif // FREICOIN_SUPPORT_ALLOCATORS_ZEROAFTERFREE_H

// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2021 The Bitcoin Core developers
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

#ifndef FREICOIN_SUPPORT_ALLOCATORS_SECURE_H
#define FREICOIN_SUPPORT_ALLOCATORS_SECURE_H

#include <support/lockedpool.h>
#include <support/cleanse.h>

#include <memory>
#include <string>

//
// Allocator that locks its contents from being paged
// out of memory and clears its contents before deletion.
//
template <typename T>
struct secure_allocator {
    using value_type = T;

    secure_allocator() = default;
    template <typename U>
    secure_allocator(const secure_allocator<U>&) noexcept {}

    T* allocate(std::size_t n)
    {
        T* allocation = static_cast<T*>(LockedPoolManager::Instance().alloc(sizeof(T) * n));
        if (!allocation) {
            throw std::bad_alloc();
        }
        return allocation;
    }

    void deallocate(T* p, std::size_t n)
    {
        if (p != nullptr) {
            memory_cleanse(p, sizeof(T) * n);
        }
        LockedPoolManager::Instance().free(p);
    }

    template <typename U>
    friend bool operator==(const secure_allocator&, const secure_allocator<U>&) noexcept
    {
        return true;
    }
    template <typename U>
    friend bool operator!=(const secure_allocator&, const secure_allocator<U>&) noexcept
    {
        return false;
    }
};

// This is exactly like std::string, but with a custom allocator.
// TODO: Consider finding a way to make incoming RPC request.params[i] mlock()ed as well
typedef std::basic_string<char, std::char_traits<char>, secure_allocator<char> > SecureString;

template<typename T>
struct SecureUniqueDeleter {
    void operator()(T* t) noexcept {
        secure_allocator<T>().deallocate(t, 1);
    }
};

template<typename T>
using secure_unique_ptr = std::unique_ptr<T, SecureUniqueDeleter<T>>;

template<typename T, typename... Args>
secure_unique_ptr<T> make_secure_unique(Args&&... as)
{
    T* p = secure_allocator<T>().allocate(1);

    // initialize in place, and return as secure_unique_ptr
    try {
        return secure_unique_ptr<T>(new (p) T(std::forward(as)...));
    } catch (...) {
        secure_allocator<T>().deallocate(p, 1);
        throw;
    }
}

#endif // FREICOIN_SUPPORT_ALLOCATORS_SECURE_H

// Copyright (c) 2015-2021 The Bitcoin Core developers
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

#include <sync.h>
#include <test/util/setup_common.h>

#include <boost/test/unit_test.hpp>

#include <stdexcept>

BOOST_AUTO_TEST_SUITE(reverselock_tests)

BOOST_AUTO_TEST_CASE(reverselock_basics)
{
    Mutex mutex;
    WAIT_LOCK(mutex, lock);

    BOOST_CHECK(lock.owns_lock());
    {
        REVERSE_LOCK(lock);
        BOOST_CHECK(!lock.owns_lock());
    }
    BOOST_CHECK(lock.owns_lock());
}

BOOST_AUTO_TEST_CASE(reverselock_multiple)
{
    Mutex mutex2;
    Mutex mutex;
    WAIT_LOCK(mutex2, lock2);
    WAIT_LOCK(mutex, lock);

    // Make sure undoing two locks succeeds
    {
        REVERSE_LOCK(lock);
        BOOST_CHECK(!lock.owns_lock());
        REVERSE_LOCK(lock2);
        BOOST_CHECK(!lock2.owns_lock());
    }
    BOOST_CHECK(lock.owns_lock());
    BOOST_CHECK(lock2.owns_lock());
}

BOOST_AUTO_TEST_CASE(reverselock_errors)
{
    Mutex mutex2;
    Mutex mutex;
    WAIT_LOCK(mutex2, lock2);
    WAIT_LOCK(mutex, lock);

#ifdef DEBUG_LOCKORDER
    bool prev = g_debug_lockorder_abort;
    g_debug_lockorder_abort = false;

    // Make sure trying to reverse lock a previous lock fails
    BOOST_CHECK_EXCEPTION(REVERSE_LOCK(lock2), std::logic_error, HasReason("lock2 was not most recent critical section locked"));
    BOOST_CHECK(lock2.owns_lock());

    g_debug_lockorder_abort = prev;
#endif

    // Make sure trying to reverse lock an unlocked lock fails
    lock.unlock();

    BOOST_CHECK(!lock.owns_lock());

    bool failed = false;
    try {
        REVERSE_LOCK(lock);
    } catch(...) {
        failed = true;
    }

    BOOST_CHECK(failed);
    BOOST_CHECK(!lock.owns_lock());

    // Locking the original lock after it has been taken by a reverse lock
    // makes no sense. Ensure that the original lock no longer owns the lock
    // after giving it to a reverse one.

    lock.lock();
    BOOST_CHECK(lock.owns_lock());
    {
        REVERSE_LOCK(lock);
        BOOST_CHECK(!lock.owns_lock());
    }

    BOOST_CHECK(failed);
    BOOST_CHECK(lock.owns_lock());
}

BOOST_AUTO_TEST_SUITE_END()

// Copyright (c) 2016 The Bitcoin Core developers
// Copyright (c) 2011-2021 The Freicoin Developers
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

#include <event2/event.h>

#ifdef EVENT_SET_MEM_FUNCTIONS_IMPLEMENTED
// It would probably be ideal to define dummy test(s) that report skipped, but boost::test doesn't seem to make that practical (at least not in versions available with common distros)

#include <map>
#include <stdlib.h>

#include "support/events.h"

#include "test/test_freicoin.h"

#include <vector>

#include <boost/test/unit_test.hpp>

static std::map<void*, short> tags;
static std::map<void*, uint16_t> orders;
static uint16_t tagSequence = 0;

static void* tag_malloc(size_t sz) {
    void* mem = malloc(sz);
    if (!mem) return mem;
    tags[mem]++;
    orders[mem] = tagSequence++;
    return mem;
}

static void tag_free(void* mem) {
    tags[mem]--;
    orders[mem] = tagSequence++;
    free(mem);
}

BOOST_FIXTURE_TEST_SUITE(raii_event_tests, BasicTestingSetup)

BOOST_AUTO_TEST_CASE(raii_event_creation)
{
    event_set_mem_functions(tag_malloc, realloc, tag_free);
    
    void* base_ptr = NULL;
    {
        auto base = obtain_event_base();
        base_ptr = (void*)base.get();
        BOOST_CHECK(tags[base_ptr] == 1);
    }
    BOOST_CHECK(tags[base_ptr] == 0);
    
    void* event_ptr = NULL;
    {
        auto base = obtain_event_base();
        auto event = obtain_event(base.get(), -1, 0, NULL, NULL);

        base_ptr = (void*)base.get();
        event_ptr = (void*)event.get();

        BOOST_CHECK(tags[base_ptr] == 1);
        BOOST_CHECK(tags[event_ptr] == 1);
    }
    BOOST_CHECK(tags[base_ptr] == 0);
    BOOST_CHECK(tags[event_ptr] == 0);
    
    event_set_mem_functions(malloc, realloc, free);
}

BOOST_AUTO_TEST_CASE(raii_event_order)
{
    event_set_mem_functions(tag_malloc, realloc, tag_free);
    
    void* base_ptr = NULL;
    void* event_ptr = NULL;
    {
        auto base = obtain_event_base();
        auto event = obtain_event(base.get(), -1, 0, NULL, NULL);

        base_ptr = (void*)base.get();
        event_ptr = (void*)event.get();

        // base should have allocated before event
        BOOST_CHECK(orders[base_ptr] < orders[event_ptr]);
    }
    // base should be freed after event
    BOOST_CHECK(orders[base_ptr] > orders[event_ptr]);

    event_set_mem_functions(malloc, realloc, free);
}

BOOST_AUTO_TEST_SUITE_END()

#endif  // EVENT_SET_MEM_FUNCTIONS_IMPLEMENTED

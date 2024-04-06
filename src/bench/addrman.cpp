// Copyright (c) 2020-2022 The Bitcoin Core developers
// Copyright (c) 2011-2024 The Freicoin Developers
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

#include <addrman.h>
#include <bench/bench.h>
#include <netgroup.h>
#include <random.h>
#include <util/check.h>
#include <util/time.h>

#include <optional>
#include <vector>

/* A "source" is a source address from which we have received a bunch of other addresses. */

static constexpr size_t NUM_SOURCES = 64;
static constexpr size_t NUM_ADDRESSES_PER_SOURCE = 256;

static NetGroupManager EMPTY_NETGROUPMAN{std::vector<bool>()};
static constexpr uint32_t ADDRMAN_CONSISTENCY_CHECK_RATIO{0};

static std::vector<CAddress> g_sources;
static std::vector<std::vector<CAddress>> g_addresses;

static void CreateAddresses()
{
    if (g_sources.size() > 0) { // already created
        return;
    }

    FastRandomContext rng(uint256(std::vector<unsigned char>(32, 123)));

    auto randAddr = [&rng]() {
        in6_addr addr;
        memcpy(&addr, rng.randbytes(sizeof(addr)).data(), sizeof(addr));

        uint16_t port;
        memcpy(&port, rng.randbytes(sizeof(port)).data(), sizeof(port));
        if (port == 0) {
            port = 1;
        }

        CAddress ret(CService(addr, port), NODE_NETWORK);

        ret.nTime = Now<NodeSeconds>();

        return ret;
    };

    for (size_t source_i = 0; source_i < NUM_SOURCES; ++source_i) {
        g_sources.emplace_back(randAddr());
        g_addresses.emplace_back();
        for (size_t addr_i = 0; addr_i < NUM_ADDRESSES_PER_SOURCE; ++addr_i) {
            g_addresses[source_i].emplace_back(randAddr());
        }
    }
}

static void AddAddressesToAddrMan(AddrMan& addrman)
{
    for (size_t source_i = 0; source_i < NUM_SOURCES; ++source_i) {
        addrman.Add(g_addresses[source_i], g_sources[source_i]);
    }
}

static void FillAddrMan(AddrMan& addrman)
{
    CreateAddresses();

    AddAddressesToAddrMan(addrman);
}

/* Benchmarks */

static void AddrManAdd(benchmark::Bench& bench)
{
    CreateAddresses();

    bench.run([&] {
        AddrMan addrman{EMPTY_NETGROUPMAN, /*deterministic=*/false, ADDRMAN_CONSISTENCY_CHECK_RATIO};
        AddAddressesToAddrMan(addrman);
    });
}

static void AddrManSelect(benchmark::Bench& bench)
{
    AddrMan addrman{EMPTY_NETGROUPMAN, /*deterministic=*/false, ADDRMAN_CONSISTENCY_CHECK_RATIO};

    FillAddrMan(addrman);

    bench.run([&] {
        const auto& address = addrman.Select();
        assert(address.first.GetPort() > 0);
    });
}

static void AddrManGetAddr(benchmark::Bench& bench)
{
    AddrMan addrman{EMPTY_NETGROUPMAN, /*deterministic=*/false, ADDRMAN_CONSISTENCY_CHECK_RATIO};

    FillAddrMan(addrman);

    bench.run([&] {
        const auto& addresses = addrman.GetAddr(/*max_addresses=*/2500, /*max_pct=*/23, /*network=*/std::nullopt);
        assert(addresses.size() > 0);
    });
}

static void AddrManAddThenGood(benchmark::Bench& bench)
{
    auto markSomeAsGood = [](AddrMan& addrman) {
        for (size_t source_i = 0; source_i < NUM_SOURCES; ++source_i) {
            for (size_t addr_i = 0; addr_i < NUM_ADDRESSES_PER_SOURCE; ++addr_i) {
                addrman.Good(g_addresses[source_i][addr_i]);
            }
        }
    };

    CreateAddresses();

    bench.run([&] {
        // To make the benchmark independent of the number of evaluations, we always prepare a new addrman.
        // This is necessary because AddrMan::Good() method modifies the object, affecting the timing of subsequent calls
        // to the same method and we want to do the same amount of work in every loop iteration.
        //
        // This has some overhead (exactly the result of AddrManAdd benchmark), but that overhead is constant so improvements in
        // AddrMan::Good() will still be noticeable.
        AddrMan addrman{EMPTY_NETGROUPMAN, /*deterministic=*/false, ADDRMAN_CONSISTENCY_CHECK_RATIO};
        AddAddressesToAddrMan(addrman);

        markSomeAsGood(addrman);
    });
}

BENCHMARK(AddrManAdd, benchmark::PriorityLevel::HIGH);
BENCHMARK(AddrManSelect, benchmark::PriorityLevel::HIGH);
BENCHMARK(AddrManGetAddr, benchmark::PriorityLevel::HIGH);
BENCHMARK(AddrManAddThenGood, benchmark::PriorityLevel::HIGH);

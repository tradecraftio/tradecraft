// Copyright (c) 2011-2016 The Bitcoin Core developers
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

#define BOOST_TEST_MODULE Bitcoin Test Suite

#include "net.h"

#include <boost/test/unit_test.hpp>

std::unique_ptr<CConnman> g_connman;

void Shutdown(void* parg)
{
  exit(EXIT_SUCCESS);
}

void StartShutdown()
{
  exit(EXIT_SUCCESS);
}

bool ShutdownRequested()
{
  return false;
}

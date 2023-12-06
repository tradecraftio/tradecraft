// Copyright (c) 2020-2022 The Bitcoin Core developers
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

#include <test/util/index.h>

#include <index/base.h>
#include <shutdown.h>
#include <util/check.h>
#include <util/time.h>

void IndexWaitSynced(const BaseIndex& index)
{
    while (!index.BlockUntilSyncedToCurrentChain()) {
        // Assert shutdown was not requested to abort the test, instead of looping forever, in case
        // there was an unexpected error in the index that caused it to stop syncing and request a shutdown.
        Assert(!ShutdownRequested());

        UninterruptibleSleep(100ms);
    }
    assert(index.GetSummary().synced);
}

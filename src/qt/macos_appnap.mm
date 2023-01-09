// Copyright (c) 2011-2018 The Bitcoin Core developers
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

#include "macos_appnap.h"

#include <AvailabilityMacros.h>
#include <Foundation/NSProcessInfo.h>
#include <Foundation/Foundation.h>

class CAppNapInhibitor::CAppNapImpl
{
public:
    ~CAppNapImpl()
    {
        if(activityId)
            enableAppNap();
    }

    void disableAppNap()
    {
        if (!activityId)
        {
            @autoreleasepool {
                const NSActivityOptions activityOptions =
                NSActivityUserInitiatedAllowingIdleSystemSleep &
                ~(NSActivitySuddenTerminationDisabled |
                NSActivityAutomaticTerminationDisabled);

                id processInfo = [NSProcessInfo processInfo];
                if ([processInfo respondsToSelector:@selector(beginActivityWithOptions:reason:)])
                {
                    activityId = [processInfo beginActivityWithOptions: activityOptions reason:@"Temporarily disable App Nap for bitcoin-qt."];
                    [activityId retain];
                }
            }
        }
    }

    void enableAppNap()
    {
        if(activityId)
        {
            @autoreleasepool {
                id processInfo = [NSProcessInfo processInfo];
                if ([processInfo respondsToSelector:@selector(endActivity:)])
                    [processInfo endActivity:activityId];

                [activityId release];
                activityId = nil;
            }
        }
    }

private:
    NSObject* activityId;
};

CAppNapInhibitor::CAppNapInhibitor() : impl(new CAppNapImpl()) {}

CAppNapInhibitor::~CAppNapInhibitor() = default;

void CAppNapInhibitor::disableAppNap()
{
    impl->disableAppNap();
}

void CAppNapInhibitor::enableAppNap()
{
    impl->enableAppNap();
}

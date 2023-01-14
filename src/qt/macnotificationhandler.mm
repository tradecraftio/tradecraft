// Copyright (c) 2011-2020 The Bitcoin Core developers
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

#include "macnotificationhandler.h"

#undef slots
#import <objc/runtime.h>
#include <Cocoa/Cocoa.h>

// Add an obj-c category (extension) to return the expected bundle identifier
@implementation NSBundle(returnCorrectIdentifier)
- (NSString *)__bundleIdentifier
{
    if (self == [NSBundle mainBundle]) {
        return @"org.bitcoinfoundation.Bitcoin-Qt";
    } else {
        return [self __bundleIdentifier];
    }
}
@end

void MacNotificationHandler::showNotification(const QString &title, const QString &text)
{
    // check if users OS has support for NSUserNotification
    if(this->hasUserNotificationCenterSupport()) {
        NSUserNotification* userNotification = [[NSUserNotification alloc] init];
        userNotification.title = title.toNSString();
        userNotification.informativeText = text.toNSString();
        [[NSUserNotificationCenter defaultUserNotificationCenter] deliverNotification: userNotification];
        [userNotification release];
    }
}

bool MacNotificationHandler::hasUserNotificationCenterSupport(void)
{
    Class possibleClass = NSClassFromString(@"NSUserNotificationCenter");

    // check if users OS has support for NSUserNotification
    if(possibleClass!=nil) {
        return true;
    }
    return false;
}


MacNotificationHandler *MacNotificationHandler::instance()
{
    static MacNotificationHandler *s_instance = nullptr;
    if (!s_instance) {
        s_instance = new MacNotificationHandler();

        Class aPossibleClass = objc_getClass("NSBundle");
        if (aPossibleClass) {
            // change NSBundle -bundleIdentifier method to return a correct bundle identifier
            // a bundle identifier is required to use OSXs User Notification Center
            method_exchangeImplementations(class_getInstanceMethod(aPossibleClass, @selector(bundleIdentifier)),
                                           class_getInstanceMethod(aPossibleClass, @selector(__bundleIdentifier)));
        }
    }
    return s_instance;
}

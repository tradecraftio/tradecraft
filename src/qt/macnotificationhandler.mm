// Copyright (c) 2011-2013 The Bitcoin Core developers
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

#include "macnotificationhandler.h"

#undef slots
#import <objc/runtime.h>
#include <Cocoa/Cocoa.h>

// Add an obj-c category (extension) to return the expected bundle identifier
@implementation NSBundle(returnCorrectIdentifier)
- (NSString *)__bundleIdentifier
{
    if (self == [NSBundle mainBundle]) {
        return @"in.freico.Freicoin-Qt";
    } else {
        return [self __bundleIdentifier];
    }
}
@end

void MacNotificationHandler::showNotification(const QString &title, const QString &text)
{
    // check if users OS has support for NSUserNotification
    if(this->hasUserNotificationCenterSupport()) {
        // okay, seems like 10.8+
        QByteArray utf8 = title.toUtf8();
        char* cString = (char *)utf8.constData();
        NSString *titleMac = [[NSString alloc] initWithUTF8String:cString];

        utf8 = text.toUtf8();
        cString = (char *)utf8.constData();
        NSString *textMac = [[NSString alloc] initWithUTF8String:cString];

        // do everything weak linked (because we will keep <10.8 compatibility)
        id userNotification = [[NSClassFromString(@"NSUserNotification") alloc] init];
        [userNotification performSelector:@selector(setTitle:) withObject:titleMac];
        [userNotification performSelector:@selector(setInformativeText:) withObject:textMac];

        id notificationCenterInstance = [NSClassFromString(@"NSUserNotificationCenter") performSelector:@selector(defaultUserNotificationCenter)];
        [notificationCenterInstance performSelector:@selector(deliverNotification:) withObject:userNotification];

        [titleMac release];
        [textMac release];
        [userNotification release];
    }
}

// sendAppleScript just take a QString and executes it as apple script
void MacNotificationHandler::sendAppleScript(const QString &script)
{
    QByteArray utf8 = script.toUtf8();
    char* cString = (char *)utf8.constData();
    NSString *scriptApple = [[NSString alloc] initWithUTF8String:cString];

    NSAppleScript *as = [[NSAppleScript alloc] initWithSource:scriptApple];
    NSDictionary *err = nil;
    [as executeAndReturnError:&err];
    [as release];
    [scriptApple release];
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
    static MacNotificationHandler *s_instance = NULL;
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

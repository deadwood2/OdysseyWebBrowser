/*
 * Copyright (C) 2015 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#import "config.h"

#if WK_API_ENABLED

#import "PlatformUtilities.h"
#import "RemoteObjectRegistry.h"
#import <WebKit/WKWebProcessPlugIn.h>
#import <WebKit/WKWebProcessPlugInBrowserContextControllerPrivate.h>
#import <WebKit/WKWebProcessPlugInFrame.h>
#import <WebKit/WKWebProcessPlugInScriptWorld.h>
#import <WebKit/_WKRemoteObjectInterface.h>
#import <WebKit/_WKRemoteObjectRegistry.h>
#import <wtf/RetainPtr.h>

@interface RemoteObjectRegistryPlugIn : NSObject <RemoteObjectProtocol, WKWebProcessPlugIn>
@end

@implementation RemoteObjectRegistryPlugIn {
    RetainPtr<WKWebProcessPlugInBrowserContextController> _browserContextController;
    RetainPtr<WKWebProcessPlugInController> _plugInController;
}

- (void)webProcessPlugIn:(WKWebProcessPlugInController *)plugInController didCreateBrowserContextController:(WKWebProcessPlugInBrowserContextController *)browserContextController
{
    ASSERT(!_browserContextController);
    ASSERT(!_plugInController);
    _browserContextController = browserContextController;
    _plugInController = plugInController;

    _WKRemoteObjectInterface *interface = remoteObjectInterface();
    [[_browserContextController _remoteObjectRegistry] registerExportedObject:self interface:interface];
}

- (void)sayHello:(NSString *)helloString
{
    JSContext *jsContext = [[_browserContextController mainFrame] jsContextForWorld:[WKWebProcessPlugInScriptWorld normalWorld]];
    [jsContext setObject:helloString forKeyedSubscript:@"helloString"];
}

- (void)sayHello:(NSString *)hello completionHandler:(void (^)(NSString *))completionHandler
{
    completionHandler([NSString stringWithFormat:@"Your string was '%@'", hello]);
}

- (void)selectionAndClickInformationForClickAtPoint:(NSValue *)pointValue completionHandler:(void (^)(NSDictionary *))completionHandler
{
    NSDictionary *result = @{
        @"URL" : [NSURL URLWithString:@"http://www.webkit.org/"],
    };

    completionHandler(result);
}

@end

#endif // WK_API_ENABLED

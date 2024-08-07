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
#import "Download.h"

#import "DataReference.h"
#import "NetworkSessionCocoa.h"
#import "SessionTracker.h"
#import <pal/spi/cf/CFNetworkSPI.h>

namespace WebKit {

void Download::resume(const IPC::DataReference& resumeData, const String& path, SandboxExtension::Handle&& sandboxExtensionHandle)
{
    m_sandboxExtension = SandboxExtension::create(WTFMove(sandboxExtensionHandle));
    if (m_sandboxExtension)
        m_sandboxExtension->consume();

    auto* networkSession = SessionTracker::networkSession(m_sessionID);
    if (!networkSession) {
        WTFLogAlways("Could not find network session with given session ID");
        return;
    }
    auto& cocoaSession = static_cast<NetworkSessionCocoa&>(*networkSession);
    auto nsData = adoptNS([[NSData alloc] initWithBytes:resumeData.data() length:resumeData.size()]);

    // FIXME: This is a temporary workaround for <rdar://problem/34745171>.
    NSMutableDictionary *dictionary = [NSPropertyListSerialization propertyListWithData:nsData.get() options:NSPropertyListImmutable format:0 error:nullptr];
    [dictionary setObject:static_cast<NSString*>(path) forKey:@"NSURLSessionResumeInfoLocalPath"];
    NSData *updatedData = [NSPropertyListSerialization dataWithPropertyList:dictionary format:NSPropertyListXMLFormat_v1_0 options:0 error:nullptr];

    m_downloadTask = cocoaSession.downloadTaskWithResumeData(updatedData);
    cocoaSession.addDownloadID(m_downloadTask.get().taskIdentifier, m_downloadID);
    m_downloadTask.get()._pathToDownloadTaskFile = path;

    [m_downloadTask resume];
}
    
void Download::platformCancelNetworkLoad()
{
    ASSERT(m_downloadTask);
    [m_downloadTask cancelByProducingResumeData: ^(NSData * _Nullable resumeData)
    {
        if (resumeData && resumeData.bytes && resumeData.length)
            didCancel(IPC::DataReference(reinterpret_cast<const uint8_t*>(resumeData.bytes), resumeData.length));
        else
            didCancel({ });
    }];
}

}

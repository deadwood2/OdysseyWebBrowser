/*
 * Copyright (C) 2018 Apple Inc. All rights reserved.
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

#include "config.h"
#include "SuspendedPageProxy.h"

#include "Logging.h"
#include "WebPageMessages.h"
#include "WebPageProxy.h"
#include "WebPageProxyMessages.h"
#include "WebProcessMessages.h"
#include "WebProcessPool.h"
#include "WebProcessProxy.h"
#include <WebCore/URL.h>
#include <wtf/DebugUtilities.h>

namespace WebKit {
using namespace WebCore;

#if !LOG_DISABLED
static const HashSet<IPC::StringReference>& messageNamesToIgnoreWhileSuspended()
{
    static NeverDestroyed<HashSet<IPC::StringReference>> messageNames;
    static std::once_flag onceFlag;
    std::call_once(onceFlag, [] {
        messageNames.get().add("BackForwardAddItem");
        messageNames.get().add("ClearAllEditCommands");
        messageNames.get().add("DidChangeContentSize");
        messageNames.get().add("DidChangeMainDocument");
        messageNames.get().add("DidChangeProgress");
        messageNames.get().add("DidCommitLoadForFrame");
        messageNames.get().add("DidDestroyNavigation");
        messageNames.get().add("DidFinishDocumentLoadForFrame");
        messageNames.get().add("DidFinishProgress");
        messageNames.get().add("DidFirstLayoutForFrame");
        messageNames.get().add("DidFirstVisuallyNonEmptyLayoutForFrame");
        messageNames.get().add("DidNavigateWithNavigationData");
        messageNames.get().add("DidReachLayoutMilestone");
        messageNames.get().add("DidRestoreScrollPosition");
        messageNames.get().add("DidSaveToPageCache");
        messageNames.get().add("DidStartProgress");
        messageNames.get().add("DidStartProvisionalLoadForFrame");
        messageNames.get().add("EditorStateChanged");
        messageNames.get().add("PageExtendedBackgroundColorDidChange");
        messageNames.get().add("SetRenderTreeSize");
        messageNames.get().add("SetStatusText");
        messageNames.get().add("SetNetworkRequestsInProgress");
    });

    return messageNames;
}
#endif

SuspendedPageProxy::SuspendedPageProxy(WebPageProxy& page, WebProcessProxy& process, WebBackForwardListItem& item)
    : m_page(page)
    , m_process(&process)
    , m_backForwardListItem(item)
    , m_origin(SecurityOriginData::fromURL({ ParsedURLString, item.url() }))
{
    m_backForwardListItem->setSuspendedPage(this);
    m_process->processPool().registerSuspendedPageProxy(*this);
    m_process->send(Messages::WebPage::SetIsSuspended(true), m_page.pageID());
}

SuspendedPageProxy::~SuspendedPageProxy()
{
    if (auto process = makeRefPtr(m_process)) {
        process->suspendedPageWasDestroyed(*this);
        process->processPool().unregisterSuspendedPageProxy(*this);
    }

    m_backForwardListItem->setSuspendedPage(nullptr);
}

void SuspendedPageProxy::webProcessDidClose(WebProcessProxy& process)
{
    ASSERT_UNUSED(process, &process == m_process);

    auto protectedThis = makeRef(*this);
    m_process->processPool().unregisterSuspendedPageProxy(*this);
    m_process = nullptr;

    m_page.suspendedPageClosed(*this);
    m_backForwardListItem->setSuspendedPage(nullptr);
}

void SuspendedPageProxy::destroyWebPageInWebProcess()
{
    m_process->send(Messages::WebPage::Close(), m_page.pageID());
    m_page.suspendedPageClosed(*this);
}

void SuspendedPageProxy::didFinishLoad()
{
    ASSERT(m_process);
    LOG(ProcessSwapping, "SuspendedPageProxy %s from process %i finished transition to suspended", loggingString(), m_process->processIdentifier());

    m_finishedSuspending = true;

    m_process->send(Messages::WebProcess::UpdateActivePages(), 0);
}

void SuspendedPageProxy::didReceiveMessage(IPC::Connection&, IPC::Decoder& decoder)
{
    ASSERT(decoder.messageReceiverName() == Messages::WebPageProxy::messageReceiverName());

    if (decoder.messageName() == Messages::WebPageProxy::DidFinishLoadForFrame::name()) {
        didFinishLoad();
        return;
    }
#if !LOG_DISABLED
    if (!messageNamesToIgnoreWhileSuspended().contains(decoder.messageName()))
        LOG(ProcessSwapping, "SuspendedPageProxy received unexpected WebPageProxy message '%s'", decoder.messageName().toString().data());
#endif
}

#if !LOG_DISABLED
const char* SuspendedPageProxy::loggingString() const
{
    return debugString("(", String::format("%p", this), " page ID ", String::number(m_page.pageID()), ", m_finishedSuspending ", String::number(m_finishedSuspending), ")");
}
#endif

} // namespace WebKit

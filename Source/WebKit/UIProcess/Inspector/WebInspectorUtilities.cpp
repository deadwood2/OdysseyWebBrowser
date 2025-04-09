/*
 * Copyright (C) 2016 Apple Inc. All rights reserved.
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
#include "WebInspectorUtilities.h"

#include "APIProcessPoolConfiguration.h"
#include "WebPageGroup.h"
#include "WebPageProxy.h"
#include "WebProcessPool.h"
#include "WebProcessProxy.h"
#include <wtf/HashMap.h>
#include <wtf/NeverDestroyed.h>
#include <wtf/text/StringConcatenateNumbers.h>

namespace WebKit {

typedef HashMap<WebPageProxy*, unsigned> PageLevelMap;

static PageLevelMap& pageLevelMap()
{
    static NeverDestroyed<PageLevelMap> map;
    return map;
}

unsigned inspectorLevelForPage(WebPageProxy* page)
{
    if (page) {
        auto findResult = pageLevelMap().find(page);
        if (findResult != pageLevelMap().end())
            return findResult->value + 1;
    }

    return 1;
}

String inspectorPageGroupIdentifierForPage(WebPageProxy* page)
{
    return makeString("__WebInspectorPageGroupLevel", inspectorLevelForPage(page), "__");
}

void trackInspectorPage(WebPageProxy* inspectorPage, WebPageProxy* inspectedPage)
{
    pageLevelMap().set(inspectorPage, inspectorLevelForPage(inspectedPage));
}

void untrackInspectorPage(WebPageProxy* inspectorPage)
{
    pageLevelMap().remove(inspectorPage);
}

static WebProcessPool* s_mainInspectorProcessPool;
static WebProcessPool* s_nestedInspectorProcessPool;

WebProcessPool& inspectorProcessPool(unsigned inspectionLevel)
{
    // Having our own process pool removes us from the main process pool and
    // guarantees no process sharing for our user interface.
    WebProcessPool*& pool = (inspectionLevel == 1) ? s_mainInspectorProcessPool : s_nestedInspectorProcessPool;
    if (!pool) {
        auto configuration = API::ProcessPoolConfiguration::create();
        pool = &WebProcessPool::create(configuration.get()).leakRef();
        // Do not delay process launch for inspector pages as inspector pages do not know how to transition from a terminated process.
        pool->disableDelayedWebProcessLaunch();
    }
    return *pool;
}

bool isInspectorProcessPool(WebProcessPool& processPool)
{
    return (s_mainInspectorProcessPool && s_mainInspectorProcessPool == &processPool)
        || (s_nestedInspectorProcessPool && s_nestedInspectorProcessPool == &processPool);
}

bool isInspectorPage(WebPageProxy& webPage)
{
    return pageLevelMap().contains(&webPage);
}

} // namespace WebKit

/*
 * Copyright (C) 2011 Apple Inc. All rights reserved.
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
#import "WKAccessibilityWebPageObjectBase.h"

#import "WebFrame.h"
#import "WebPage.h"
#import "WKArray.h"
#import "WKNumber.h"
#import "WKRetainPtr.h"
#import "WKSharedAPICast.h"
#import "WKString.h"
#import "WKStringCF.h"
#import <WebCore/AXIsolatedTree.h>
#import <WebCore/AXObjectCache.h>
#import <WebCore/Document.h>
#import <WebCore/Frame.h>
#import <WebCore/FrameView.h>
#import <WebCore/Page.h>
#import <WebCore/ScrollView.h>
#import <WebCore/Scrollbar.h>
#import <wtf/ObjCRuntimeExtras.h>

@implementation WKAccessibilityWebPageObjectBase

- (WebCore::AXObjectCache*)axObjectCache
{
    if (!m_page)
        return nullptr;

    auto page = m_page->corePage();
    if (!page)
        return nullptr;

    auto& core = page->mainFrame();
    if (!core.document())
        return nullptr;

    return core.document()->axObjectCache();
}

- (id)accessibilityPluginObject
{
    auto retrieveBlock = [&self]() -> id {
        id axPlugin = nil;
        auto dispatchBlock = [&axPlugin, &self] {
            if (self->m_page)
                axPlugin = self->m_page->accessibilityObjectForMainFramePlugin();
        };

        if (isMainThread())
            dispatchBlock();
        else {
            callOnMainThreadAndWait([&dispatchBlock] {
                dispatchBlock();
            });
        }
        return axPlugin;
    };
    
    return retrieveBlock();
}

#if ENABLE(ACCESSIBILITY_ISOLATED_TREE)
- (BOOL)clientSupportsIsolatedTree
{
    AXClientType type = _AXGetClientForCurrentRequestUntrusted();
    // FIXME: Remove unknown client before enabling ACCESSIBILITY_ISOLATED_TREE.
    return type == kAXClientTypeVoiceOver || type == kAXClientTypeUnknown;
}

- (id)isolatedTreeRootObject
{
    if (isMainThread()) {
        if (auto cache = [self axObjectCache]) {
            auto tree = AXIsolatedTree::initializeTreeForPageId(m_pageID, *cache);

            // Now that we have created our tree, initialize the secondary thread,
            // so future requests come in on the other thread.
            _AXUIElementUseSecondaryAXThread(true);
            if (auto rootNode = tree->rootNode())
                return rootNode->wrapper();
        }
    } else {
        auto tree = AXIsolatedTree::treeForPageID(m_pageID);
        tree->applyPendingChanges();
        if (auto rootNode = tree->rootNode())
            return rootNode->wrapper();
    }

    return nil;
}
#endif

- (id)accessibilityRootObjectWrapper
{
    if (!WebCore::AXObjectCache::accessibilityEnabled())
        WebCore::AXObjectCache::enableAccessibility();

    if (m_hasMainFramePlugin)
        return self.accessibilityPluginObject;

#if ENABLE(ACCESSIBILITY_ISOLATED_TREE)
    // If VoiceOver is on, ensure subsequent requests are now handled on the secondary AX thread.
    bool clientSupportsIsolatedTree = [self clientSupportsIsolatedTree];
    if (clientSupportsIsolatedTree)
        return [self isolatedTreeRootObject];
#endif

    if (auto cache = [self axObjectCache]) {
        if (WebCore::AccessibilityObject* root = cache->rootObject())
            return root->wrapper();
    }

    return nil;
}

- (void)setWebPage:(WebKit::WebPage*)page
{
    m_page = page;

    if (page) {
        m_pageID = page->pageID();

        auto* frame = page->mainFrame();
        m_hasMainFramePlugin = frame && frame->document() ? frame->document()->isPluginDocument() : false;
    } else {
        m_pageID = { };
        m_hasMainFramePlugin = false;
    }
}

- (void)setHasMainFramePlugin:(bool)hasPlugin
{
    m_hasMainFramePlugin = hasPlugin;
}

- (void)setRemoteParent:(id)parent
{
    if (parent != m_parent) {
        [m_parent release];
        m_parent = [parent retain];
    }
}

- (id)accessibilityFocusedUIElement
{
    return [[self accessibilityRootObjectWrapper] accessibilityFocusedUIElement];
}


@end

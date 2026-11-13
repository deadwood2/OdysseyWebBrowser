/*
 * OWB/AROS: WebKit 2.46 helpers for the main frame.
 *
 * Since 2.40 Page::mainFrame() returns the abstract WebCore::Frame (LocalFrame
 * or RemoteFrame). OWB pages only ever have a local main frame, and its
 * WebKitLegacy code was written for the concrete frame, so it reaches it
 * through these helpers.
 */
#pragma once

#include <WebCore/LocalFrame.h>
#include <WebCore/Page.h>

inline WebCore::LocalFrame* localMainFrame(WebCore::Page* page)
{
    return page ? dynamicDowncast<WebCore::LocalFrame>(page->mainFrame()) : nullptr;
}

inline WebCore::LocalFrame* localMainFrame(WebCore::Page& page)
{
    return dynamicDowncast<WebCore::LocalFrame>(page.mainFrame());
}

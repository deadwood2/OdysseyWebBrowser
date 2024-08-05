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

WI.NetworkTabContentView = class NetworkTabContentView extends WI.ContentBrowserTabContentView
{
    constructor(identifier)
    {
        let {image, title} = WI.NetworkTabContentView.tabInfo();
        let tabBarItem = new WI.GeneralTabBarItem(image, title);
        let detailsSidebarPanelConstructors = [WI.ResourceDetailsSidebarPanel, WI.ProbeDetailsSidebarPanel];

        super(identifier || "network", "network", tabBarItem, WI.NetworkSidebarPanel, detailsSidebarPanelConstructors);
    }

    static tabInfo()
    {
        return {
            image: "Images/Network.svg",
            title: WI.UIString("Network"),
        };
    }

    static isTabAllowed()
    {
        return !!window.NetworkAgent && !!window.PageAgent;
    }

    // Public

    get type()
    {
        return WI.NetworkTabContentView.Type;
    }

    canShowRepresentedObject(representedObject)
    {
        if (!(representedObject instanceof WI.Resource))
            return false;

        return !!this.navigationSidebarPanel.contentTreeOutline.getCachedTreeElement(representedObject);
    }

    get supportsSplitContentBrowser()
    {
        // Since the Network tab has a real sidebar, showing the split console would cause items in
        // the sidebar to be aligned with an item in the datagrid that isn't shown.
        return false;
    }
};

WI.NetworkTabContentView.Type = "network";

/*
 * Copyright (C) 2017 Apple Inc. All rights reserved.
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

WI.DOMBreakpoint = class DOMBreakpoint extends WI.Object
{
    constructor(domNodeOrInfo, type, disabled)
    {
        console.assert(domNodeOrInfo, "Missing DOMNode or info.");

        super();

        if (domNodeOrInfo instanceof WI.DOMNode) {
            this._domNodeIdentifier = domNodeOrInfo.id;
            this._path = domNodeOrInfo.path();
            console.assert(WI.frameResourceManager.mainFrame);
            this._url = WI.frameResourceManager.mainFrame.url;
        } else if (domNodeOrInfo && typeof domNodeOrInfo === "object") {
            this._domNodeIdentifier = null;
            this._path = domNodeOrInfo.path;
            this._url = domNodeOrInfo.url;
        }

        this._type = type;
        this._disabled = disabled || false;
    }

    // Public

    get type() { return this._type; }
    get url() { return this._url; }
    get path() { return this._path; }

    get disabled()
    {
        return this._disabled;
    }

    set disabled(disabled)
    {
        if (this._disabled === disabled)
            return;

        this._disabled = disabled;

        this.dispatchEventToListeners(WI.DOMBreakpoint.Event.DisabledStateDidChange);
    }

    get domNodeIdentifier()
    {
        return this._domNodeIdentifier;
    }

    set domNodeIdentifier(nodeIdentifier)
    {
        if (this._domNodeIdentifier === nodeIdentifier)
            return;

        let data = {};
        if (!nodeIdentifier)
            data.oldNodeIdentifier = this._domNodeIdentifier;

        this._domNodeIdentifier = nodeIdentifier;

        this.dispatchEventToListeners(WI.DOMBreakpoint.Event.ResolvedStateDidChange, data);
    }

    get serializableInfo()
    {
        let info = {url: this._url, path: this._path, type: this._type};
        if (this._disabled)
            info.disabled = true;

        return info;
    }

    saveIdentityToCookie(cookie)
    {
        cookie[WI.DOMBreakpoint.DocumentURLCookieKey] = this.url;
        cookie[WI.DOMBreakpoint.NodePathCookieKey] = this.path;
        cookie[WI.DOMBreakpoint.TypeCookieKey] = this.type;
    }
};

WI.DOMBreakpoint.DocumentURLCookieKey = "dom-breakpoint-document-url";
WI.DOMBreakpoint.NodePathCookieKey = "dom-breakpoint-node-path";
WI.DOMBreakpoint.TypeCookieKey = "dom-breakpoint-type";

WI.DOMBreakpoint.Type = {
    SubtreeModified: "subtree-modified",
    AttributeModified: "attribute-modified",
    NodeRemoved: "node-removed",
};

WI.DOMBreakpoint.Event = {
    DisabledStateDidChange: "dom-breakpoint-disabled-state-did-change",
    ResolvedStateDidChange: "dom-breakpoint-resolved-state-did-change",
};

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

WI.XHRBreakpoint = class XHRBreakpoint extends WI.Object
{
    constructor(type, url, disabled)
    {
        super();

        this._type = type || WI.XHRBreakpoint.Type.Text;
        this._url = url || "";
        this._disabled = disabled || false;
    }

    // Public

    get type() { return this._type; }
    get url() { return this._url; }

    get disabled()
    {
        return this._disabled;
    }

    set disabled(disabled)
    {
        if (this._disabled === disabled)
            return;

        this._disabled = disabled;

        this.dispatchEventToListeners(WI.XHRBreakpoint.Event.DisabledStateDidChange);
    }

    get serializableInfo()
    {
        let info = {type: this._type, url: this._url};
        if (this._disabled)
            info.disabled = true;

        return info;
    }

    saveIdentityToCookie(cookie)
    {
        cookie[WI.XHRBreakpoint.URLCookieKey] = this._url;
    }
};

WI.XHRBreakpoint.URLCookieKey = "xhr-breakpoint-url";

WI.XHRBreakpoint.Event = {
    DisabledStateDidChange: "xhr-breakpoint-disabled-state-did-change",
    ResolvedStateDidChange: "xhr-breakpoint-resolved-state-did-change",
};

WI.XHRBreakpoint.Type = {
    Text: "text",
    RegularExpression: "regex",
};

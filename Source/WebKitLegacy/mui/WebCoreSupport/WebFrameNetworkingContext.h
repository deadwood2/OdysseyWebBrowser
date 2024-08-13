/*
    Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies)

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

	You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#ifndef WebFrameNetworkingContext_h
#define WebFrameNetworkingContext_h

#include <WebCore/FrameNetworkingContext.h>

class WebFrameNetworkingContext : public WebCore::FrameNetworkingContext {
public:
    WTF::String url() const
    {
        if(frame())
            return frame()->document()->url();
        else
            return WTF::String("");
    }
    static Ref<WebFrameNetworkingContext> create(WebCore::Frame* frame, const WTF::String& userAgent)
    {
        return adoptRef(*new WebFrameNetworkingContext(frame, userAgent));
    }

    static void setPrivateBrowsingStorageSessionIdentifierBase(const String&);
    static WebCore::NetworkStorageSession& ensurePrivateBrowsingSession();
    static void destroyPrivateBrowsingSession();

private:
    explicit WebFrameNetworkingContext(WebCore::Frame* frame, const WTF::String& userAgent)
        : WebCore::FrameNetworkingContext(frame)
        , m_userAgent(userAgent)
    {
    }

    WebCore::NetworkStorageSession* storageSession() const override;

    virtual WTF::String userAgent() const;
    virtual WTF::String referrer() const;

    WTF::String m_userAgent;
};

#endif // WebFrameNetworkingContext_h

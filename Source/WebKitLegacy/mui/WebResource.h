/*
 * Copyright (C) 2008 Pleyo.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 * 3.  Neither the name of Pleyo nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY PLEYO AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL PLEYO OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#ifndef WebResource_h
#define WebResource_h


/**
 *  @file  WebResource.h
 *  WebResource description
 *  Repository informations :
 * - $URL$
 * - $Rev$
 * - $Date: 2013/03/06 00:13:17 $
 */
#include "BALBase.h"
#include <URL.h>
#include <wtf/text/WTFString.h>
#include <ResourceResponse.h>
#include <SharedBuffer.h>
#include <wtf/PassRefPtr.h>

class WebResource {
public:

    /**
     * create a new instance of WebResource
     */
    static WebResource* createInstance(PassRefPtr<WebCore::SharedBuffer> data, const WebCore::ResourceResponse& response);
protected:

    /**
     * WebResource constructor
     */
    WebResource(PassRefPtr<WebCore::SharedBuffer> data, const WebCore::URL& url, const WTF::String& mimeType, const WTF::String& textEncodingName, const WTF::String& frameName);

public:

    /**
     * WebResource destructor
     */
    virtual ~WebResource();

    /**
     *  The initializer for WebResource.
        @param data The data of the resource.
        @param URL The URL of the resource.
        @param MIMEType The MIME type of the resource.
        @param textEncodingName The text encoding name of the resource (can be nil).
        @param frameName The frame name of the resource if the resource represents the contents of an entire HTML frame (can be nil).
        @result An initialized WebResource.
     */
    virtual void initWithData(PassRefPtr<WebCore::SharedBuffer> data, WTF::String url, WTF::String mimeType, WTF::String textEncodingName, WTF::String frameName);

    /**
     *  data 
     * @result The data of the resource.
     */
    virtual PassRefPtr<WebCore::SharedBuffer> data();

    /**
     *  URL 
     * @result The URL of the resource
     */
    virtual WTF::String _URL();

    /**
     *  MIMEType 
     * @result The MIME type of the resource.
     */
    virtual WTF::String MIMEType();

    /**
     *  textEncodingName 
     * @result The text encoding name of the resource (can be nil).
     */
    virtual WTF::String textEncodingName();

    /**
     *  frameName 
     * @result The frame name of the resource if the resource represents the contents of an entire HTML frame (can be nil).
     */
    virtual WTF::String frameName();

private:
    RefPtr<WebCore::SharedBuffer> m_data;
    WebCore::URL m_url;
    WTF::String m_mimeType;
    WTF::String m_textEncodingName;
    WTF::String m_frameName;
};

#endif

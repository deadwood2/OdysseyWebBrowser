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

#ifndef WebDataSource_H
#define WebDataSource_H


/**
 *  @file  WebDataSource.h
 *  WebDataSource description
 *  Repository informations :
 * - $URL$
 * - $Rev$
 * - $Date: 2013/03/06 00:13:16 $
 */
#include "BALBase.h"
#include <wtf/text/WTFString.h>
#include <wtf/RefPtr.h>

class WebDocumentLoader;
class WebMutableURLRequest;
class WebURLResponse;
class WebArchive;
class WebResource;
class WebError;
class WebFrame;

namespace WebCore {
    class SharedBuffer;
}

class WebDataSource
{
public:

    /**
     * create a new instance of WebDataSource
     * @param[in]: WebDocumentLoader
     */
    static WebDataSource* createInstance(WebDocumentLoader*);
protected:

    /**
     * WebDataSource constructor
     * @param[in]: WebDocumentLoader
     */
    WebDataSource(WebDocumentLoader*);

public:

    /**
     * WebDataSource destructor
     */
    virtual ~WebDataSource();

    /**
     * The designated initializer for WebDataSource.
        @param request The request to use in creating a datasource.
        @result Returns an initialized WebDataSource.
     */
    virtual void initWithRequest(WebMutableURLRequest *request);


    /**
     * The data will be incomplete until the datasource has completely loaded.
        @result Returns the raw data associated with the datasource.  Returns nil
        if the datasource hasn't loaded any data.
     */
    virtual WTF::PassRefPtr<WebCore::SharedBuffer> data();

    /**
     *  A representation holds a type specific representation
        of the datasource's data.  The representation class is determined by mapping
        a MIME type to a class.  The representation is created once the MIME type
        of the datasource content has been determined.
        @result Returns the representation associated with this datasource.
        Returns nil if the datasource hasn't created it's representation.
     */
    //virtual WebHTMLRepresentation* representation();

    /**
     * Return the frame that represents this data source.
     */
    //virtual WebFrame* webFrame();

    /**
     *  initialRequest description
     * @param[in]: description
     * @param[out]: description
     * @code
     * @endcode
     */
    virtual WebMutableURLRequest* initialRequest();

    /**
     * Returns a reference to the original request that created the
        datasource.  This request will be unmodified by WebKit.
     */
    virtual WebMutableURLRequest* request();

    /**
     * returns the WebResourceResponse for the data source.
     */
    virtual WebURLResponse* response();

    /**
     * Returns either the override encoding, as set on the WebView for this
        dataSource or the encoding from the response.
     */
    virtual WTF::String textEncodingName();

    /**
     * Returns true if there are any pending loads.
     */
    virtual bool isLoading();

    /**
     * Returns nil or the page title.
     */
    virtual WTF::String pageTitle();

    /**
     * This will be non-nil only for dataSources created by calls to the
        WebFrame method loadAlternateHTMLString:baseURL:forUnreachableURL:.
        @result returns the unreachableURL for which this dataSource is showing alternate content, or nil
     */
    virtual WTF::String unreachableURL();

    /**
     * A WebArchive representing the data source, its subresources and child frames.
        @description This method constructs a WebArchive using the original downloaded data.
        In the case of HTML, if the current state of the document is preferred, webArchive should be
        called on the DOM document instead.
     */
    virtual WebArchive* webArchive();

    /**
     * A WebResource representing the data source.
        @description This method constructs a WebResource using the original downloaded data.
        This method can be used to construct a WebArchive in case the archive returned by
        WebDataSource's webArchive isn't sufficient.
     */
    virtual WebResource* mainResource();

    /**
     * Returns a subresource for a given URL.
        @param URL The URL of the subresource.
        @description Returns non-nil if the data source has fully downloaded a subresource with the given URL.
     */
    virtual WebResource* subresourceForURL(WTF::String url);

    /**
     * Adds a subresource to the data source.
        @param subresource The subresource to be added.
        @description addSubresource: adds a subresource to the data source's list of subresources.
        Later, if something causes the data source to load the URL of the subresource, the data source
        will load the data from the subresource instead of from the network. For example, if one wants to add
        an image that is already downloaded to a web page, addSubresource: can be called so that the data source
        uses the downloaded image rather than accessing the network. NOTE: If the data source already has a
        subresource with the same URL, addSubresource: will replace it.
     */
    virtual void addSubresource(WebResource *subresource);
    

    /**
     * get override encoding
     */
    virtual WTF::String overrideEncoding();

    /**
     * set override encoding
     */
    virtual void setOverrideEncoding(WTF::String encoding);

    /**
     * get main document error 
     */
    virtual WebError* mainDocumentError();

    // WebDataSource
    WebDocumentLoader* documentLoader() const;
protected:
    RefPtr<WebDocumentLoader> m_loader;
    //COMPtr<IWebDocumentRepresentation> m_representation;
};

#endif

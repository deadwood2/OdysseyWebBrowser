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
#ifndef WebInspector_h
#define WebInspector_h

#include "WebKitTypes.h"

/**
 *  @file  WebInspector.h
 *  WebInspector description
 *  Repository informations :
 * - $URL$
 * - $Rev$
 * - $Date: 2012/05/04 21:14:07 $
 */
class WebInspectorClient;
class WebInspectorFrontendClient;
class WebView;

class WEBKIT_OWB_API WebInspector {
public:

    /**
     * create a new instance of WebInspector
     */
    static WebInspector* createInstance(WebView*, WebInspectorClient*);

    /**
     * WebInspector destructor
     */
    virtual ~WebInspector();


    /**
     * webView closed
     */
    void webViewClosed();


    /**
     * show
     */
    virtual void show();

    /**
     * show console
     */
    virtual void showConsole();

    /**
     * close
     */
    virtual void close();

    /**
     * attach
     */
    virtual void attach();

    /**
     * detach
     */
    virtual void detach();

    /**
     * isDebuggingJavaScript
     */
    virtual bool isDebuggingJavaScript();

    /**
     * toggleDebuggingJavaScript
     */
    virtual void toggleDebuggingJavaScript();

    /**
     * isProfilingJavaScript
     */
    virtual bool isProfilingJavaScript();

    /**
     * toggleProfilingJavaScript
     */
    virtual void toggleProfilingJavaScript();

    /**
     * isJavaScriptProfilingEnabled
     */
    virtual bool isJavaScriptProfilingEnabled();

    /**
     * setJavaScriptProfilingEnabled
     */
    virtual void setJavaScriptProfilingEnabled(bool);

    /*
     * setTimelineProfilingEnabled
     */
	virtual bool isTimelineProfilingEnabled();
    virtual void setTimelineProfilingEnabled(bool);
private:

    /**
     * WebInspector constructor
     */
    WebInspector(WebView*, WebInspectorClient*);

     WebInspectorFrontendClient* frontendClient();

    WebView* m_webView;
    WebInspectorClient* m_inspectorClient;
};

#endif // !defined(WebInspector_h)

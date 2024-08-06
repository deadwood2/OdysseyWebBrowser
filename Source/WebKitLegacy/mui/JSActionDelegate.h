/*
 * Copyright (C) 2009 Pleyo.  All rights reserved.
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

#ifndef JSActionDelegate_h
#define JSActionDelegate_h

#include "SharedObject.h"

class WebFrame;
class WebSecurityOrigin;

class JSActionDelegate : public SharedObject<JSActionDelegate> {

public:
    virtual ~JSActionDelegate() { }

    /**
     *  consoleMessage : send a message to the console
     */
    // FIXME: This should be moved somewhere else, but for now let's keep it here.
    virtual void consoleMessage(WebFrame*, int, int, const char*) = 0;

    /**
     * jsAlert : call a js alert window
     */
    virtual bool jsAlert(WebFrame*, const char*) = 0;

    /**
     * jsConfirm : call a js confirm window
     */
    virtual bool jsConfirm(WebFrame*, const char*) = 0;

    /**
     * jsPrompt : call a js prompt window
     */
    virtual bool jsPrompt(WebFrame*, const char*, const char*, char**) = 0;

    /**
     * exceededDatabaseQuota
     */
    virtual void exceededDatabaseQuota(WebFrame *frame, WebSecurityOrigin *origin, const char* databaseIdentifier) = 0;

};

#endif // JSActionDelegate_h

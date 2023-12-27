/*
 * Copyright (C) 2011, 2013 Apple Inc. All rights reserved.
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

#ifndef WebKeyValueStorageManager_h
#define WebKeyValueStorageManager_h

#include "APIObject.h"
#include "GenericCallback.h"
#include "MessageReceiver.h"
#include "WebContextSupplement.h"
#include <wtf/PassRefPtr.h>
#include <wtf/RefPtr.h>
#include <wtf/Vector.h>

namespace WebKit {

class WebProcessPool;
typedef GenericCallback<API::Array*> ArrayCallback;

class WebKeyValueStorageManager : public API::ObjectImpl<API::Object::Type::KeyValueStorageManager>, public WebContextSupplement {
public:
    static const char* supplementName();

    static PassRefPtr<WebKeyValueStorageManager> create(WebProcessPool*);
    virtual ~WebKeyValueStorageManager();

    void getKeyValueStorageOrigins(std::function<void (API::Array*, CallbackBase::Error)>);
    void getStorageDetailsByOrigin(std::function<void (API::Array*, CallbackBase::Error)>);
    void deleteEntriesForOrigin(API::SecurityOrigin*);
    void deleteAllEntries();

    using API::Object::ref;
    using API::Object::deref;

    static String originKey();
    static String creationTimeKey();
    static String modificationTimeKey();

private:
    explicit WebKeyValueStorageManager(WebProcessPool*);

    // WebContextSupplement
    virtual void refWebContextSupplement() override;
    virtual void derefWebContextSupplement() override;
};

} // namespace WebKit

#endif // WebKeyValueStorageManager_h

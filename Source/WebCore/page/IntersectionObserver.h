/*
 * Copyright (C) 2016 Apple Inc. All rights reserved.
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

#pragma once

#if ENABLE(INTERSECTION_OBSERVER)

#include "IntersectionObserverCallback.h"
#include "IntersectionObserverEntry.h"
#include <wtf/RefCounted.h>
#include <wtf/Variant.h>
#include <wtf/text/WTFString.h>

namespace WebCore {

class Element;

class IntersectionObserver : public RefCounted<IntersectionObserver> {
public:
    struct Init {
        RefPtr<Element> root;
        String rootMargin;
        Variant<double, Vector<double>> threshold;
    };

    static Ref<IntersectionObserver> create(Ref<IntersectionObserverCallback>&& callback, Init&& init)
    {
        return adoptRef(*new IntersectionObserver(WTFMove(callback), WTFMove(init)));
    }
    
    Element* root() const { return m_root.get(); }
    String rootMargin() const { return m_rootMargin; }
    const Vector<double>& thresholds() const { return m_thresholds; }

    void observe(Element&);
    void unobserve(Element&);
    void disconnect();

    Vector<RefPtr<IntersectionObserverEntry>> takeRecords();

private:
    IntersectionObserver(Ref<IntersectionObserverCallback>&&, Init&&);
    
    RefPtr<Element> m_root;
    String m_rootMargin;
    Vector<double> m_thresholds;
    Ref<IntersectionObserverCallback> m_callback;
};


} // namespace WebCore

#endif // ENABLE(INTERSECTION_OBSERVER)

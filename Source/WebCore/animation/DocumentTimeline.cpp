/*
 * Copyright (C) Canon Inc. 2016
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted, provided that the following conditions
 * are required to be met:
 *
 * 1. Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution.
 * 3. Neither the name of Canon Inc. nor the names of
 * its contributors may be used to endorse or promote products derived
 * from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY CANON INC. AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL CANON INC. AND ITS CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "DocumentTimeline.h"

#if ENABLE(WEB_ANIMATIONS)

#include "Document.h"
#include "DocumentAnimation.h"
#include "WebAnimation.h"

namespace WebCore {

Ref<DocumentTimeline> DocumentTimeline::create(Document& context, double originTime)
{
    return adoptRef(*new DocumentTimeline(context, originTime));
}

DocumentTimeline::DocumentTimeline(Document& context, double originTime)
    : AnimationTimeline(DocumentTimelineClass)
    , m_document(context.createWeakPtr())
    , m_originTime(originTime)
{
    UNUSED_PARAM(m_originTime);
}

DocumentTimeline::~DocumentTimeline()
{
}

void DocumentTimeline::attach(WebAnimation& animation)
{
    ASSERT(animation.timeline() == this);
    ASSERT(m_document);

    if (!m_document)
        return;

    auto* document = DocumentAnimation::from(m_document.get());
    if (document)
        document->addAnimation(animation);
}

void DocumentTimeline::detach(WebAnimation& animation)
{
    ASSERT(m_document);

    if (!m_document)
        return;

    auto* document = DocumentAnimation::from(m_document.get());
    if (document)
        document->removeAnimation(animation);
}

} // namespace WebCore

#endif // ENABLE(WEB_ANIMATIONS)

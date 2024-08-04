/*
 * Copyright (C) 2008-2016 Apple Inc. All rights reserved.
 * Copyright (C) 2014 Adobe Systems Incorporated. All rights reserved.
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
 * 3.  Neither the name of Apple Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "ImageData.h"

#include "ExceptionCode.h"
#include <runtime/JSCInlines.h>
#include <runtime/TypedArrayInlines.h>

namespace WebCore {

ExceptionOr<Ref<ImageData>> ImageData::create(unsigned sw, unsigned sh)
{
    if (!sw || !sh)
        return Exception { INDEX_SIZE_ERR };

    Checked<int, RecordOverflow> dataSize = 4;
    dataSize *= sw;
    dataSize *= sh;
    if (dataSize.hasOverflowed())
        return Exception { TypeError }; // FIXME: Seems a peculiar choice of exception here.

    IntSize size(sw, sh);
    auto data = adoptRef(*new ImageData(size));
    data->data()->zeroFill();
    return WTFMove(data);
}

RefPtr<ImageData> ImageData::create(const IntSize& size)
{
    Checked<int, RecordOverflow> dataSize = 4;
    dataSize *= size.width();
    dataSize *= size.height();
    if (dataSize.hasOverflowed())
        return nullptr;

    return adoptRef(*new ImageData(size));
}

RefPtr<ImageData> ImageData::create(const IntSize& size, Ref<Uint8ClampedArray>&& byteArray)
{
    Checked<int, RecordOverflow> dataSize = 4;
    dataSize *= size.width();
    dataSize *= size.height();
    if (dataSize.hasOverflowed())
        return nullptr;

    if (dataSize.unsafeGet() < 0 || static_cast<unsigned>(dataSize.unsafeGet()) > byteArray->length())
        return nullptr;

    return adoptRef(*new ImageData(size, WTFMove(byteArray)));
}

ExceptionOr<RefPtr<ImageData>> ImageData::create(Ref<Uint8ClampedArray>&& byteArray, unsigned sw, unsigned sh)
{
    unsigned length = byteArray->length();
    if (!length || length % 4 != 0)
        return Exception { INVALID_STATE_ERR };

    if (!sw)
        return Exception { INDEX_SIZE_ERR };

    length /= 4;
    if (length % sw != 0)
        return Exception { INVALID_STATE_ERR };

    unsigned height = length / sw;
    if (sh && sh != height)
        return Exception { INDEX_SIZE_ERR };

    return create(IntSize(sw, height), WTFMove(byteArray));
}

ImageData::ImageData(const IntSize& size)
    : m_size(size)
    , m_data(Uint8ClampedArray::createUninitialized((size.area() * 4).unsafeGet()))
{
    ASSERT(m_data);
}

ImageData::ImageData(const IntSize& size, Ref<Uint8ClampedArray>&& byteArray)
    : m_size(size)
    , m_data(WTFMove(byteArray))
{
    ASSERT(m_data);
    ASSERT_WITH_SECURITY_IMPLICATION(!m_data || (size.area() * 4).unsafeGet() <= m_data->length());
}

}


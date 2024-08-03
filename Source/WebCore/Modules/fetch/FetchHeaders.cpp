/*
 * Copyright (C) 2016 Canon Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted, provided that the following conditions
 * are required to be met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 * 3.  Neither the name of Canon Inc. nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
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
#include "FetchHeaders.h"

#if ENABLE(FETCH_API)

#include "ExceptionCode.h"
#include "HTTPParsers.h"

namespace WebCore {

static bool canWriteHeader(const String& name, const String& value, FetchHeaders::Guard guard, ExceptionCode& ec)
{
    if (!isValidHTTPToken(name) || !isValidHTTPHeaderValue(value)) {
        ec = TypeError;
        return false;
    }
    if (guard == FetchHeaders::Guard::Immutable) {
        ec = TypeError;
        return false;
    }
    if (guard == FetchHeaders::Guard::Request && isForbiddenHeaderName(name))
        return false;
    if (guard == FetchHeaders::Guard::RequestNoCors && !isSimpleHeader(name, value))
        return false;
    if (guard == FetchHeaders::Guard::Response && isForbiddenResponseHeaderName(name))
        return false;
    return true;
}

void FetchHeaders::append(const String& name, const String& value, ExceptionCode& ec)
{
    String normalizedValue = stripLeadingAndTrailingHTTPSpaces(value);
    if (!canWriteHeader(name, normalizedValue, m_guard, ec))
        return;
    m_headers.add(name, normalizedValue);
}

void FetchHeaders::remove(const String& name, ExceptionCode& ec)
{
    if (!canWriteHeader(name, String(), m_guard, ec))
        return;
    m_headers.remove(name);
}

String FetchHeaders::get(const String& name, ExceptionCode& ec) const
{
    if (!isValidHTTPToken(name)) {
        ec = TypeError;
        return String();
    }
    return m_headers.get(name);
}

bool FetchHeaders::has(const String& name, ExceptionCode& ec) const
{
    if (!isValidHTTPToken(name)) {
        ec = TypeError;
        return false;
    }
    return m_headers.contains(name);
}

void FetchHeaders::set(const String& name, const String& value, ExceptionCode& ec)
{
    String normalizedValue = stripLeadingAndTrailingHTTPSpaces(value);
    if (!canWriteHeader(name, normalizedValue, m_guard, ec))
        return;
    m_headers.set(name, normalizedValue);
}

void FetchHeaders::fill(const FetchHeaders* headers)
{
    ASSERT(m_guard != Guard::Immutable);

    if (!headers)
        return;

    filterAndFill(headers->m_headers, m_guard);
}

void FetchHeaders::filterAndFill(const HTTPHeaderMap& headers, Guard guard)
{
    ExceptionCode ec;
    for (auto& header : headers) {
        if (canWriteHeader(header.key, header.value, guard, ec)) {
            if (header.keyAsHTTPHeaderName)
                m_headers.add(header.keyAsHTTPHeaderName.value(), header.value);
            else
                m_headers.add(header.key, header.value);
        }
    }
}

Optional<WTF::KeyValuePair<String, String>> FetchHeaders::Iterator::next()
{
    while (m_currentIndex < m_keys.size()) {
        String key = m_keys[m_currentIndex++];
        String value = m_headers->m_headers.get(key);
        if (!value.isNull())
            return WTF::KeyValuePair<String, String>(WTFMove(key), WTFMove(value));
    }
    m_keys.clear();
    return Nullopt;
}

FetchHeaders::Iterator::Iterator(FetchHeaders& headers)
    : m_headers(headers)
{
    m_keys.reserveInitialCapacity(headers.m_headers.size());
    for (auto& header : headers.m_headers)
        m_keys.uncheckedAppend(header.key.convertToASCIILowercase());

    std::sort(m_keys.begin(), m_keys.end(), WTF::codePointCompareLessThan);
}

} // namespace WebCore

#endif // ENABLE(FETCH_API)

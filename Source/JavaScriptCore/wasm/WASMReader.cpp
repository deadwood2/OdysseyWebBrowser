/*
 * Copyright (C) 2015 Apple Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "WASMReader.h"

#if ENABLE(WEBASSEMBLY)

#include <wtf/text/StringBuilder.h>

#define CHECK_READ(length) do { if (m_cursor + length > m_buffer.end()) return false; } while (0)

namespace JSC {

bool WASMReader::readUInt32(uint32_t& result)
{
    CHECK_READ(4);
    result = m_cursor[0] | m_cursor[1] << 8 | m_cursor[2] << 16 | m_cursor[3] << 24;
    m_cursor += 4;
    return true;
}

bool WASMReader::readFloat(float& result)
{
    CHECK_READ(4);
    union {
        uint8_t bytes[4];
        float floatValue;
    } u = {
#if CPU(BIG_ENDIAN)
        { m_cursor[3], m_cursor[2], m_cursor[1], m_cursor[0] }
#else
        { m_cursor[0], m_cursor[1], m_cursor[2], m_cursor[3] }
#endif
    };
    result = u.floatValue;
    m_cursor += 4;
    return true;
}

bool WASMReader::readDouble(double& result)
{
    CHECK_READ(8);
    union {
        uint8_t bytes[8];
        double doubleValue;
    } u = {
#if CPU(BIG_ENDIAN)
        { m_cursor[7], m_cursor[6], m_cursor[5], m_cursor[4], m_cursor[3], m_cursor[2], m_cursor[1], m_cursor[0] }
#else
        { m_cursor[0], m_cursor[1], m_cursor[2], m_cursor[3], m_cursor[4], m_cursor[5], m_cursor[6], m_cursor[7] }
#endif
    };
    result = u.doubleValue;
    m_cursor += 8;
    return true;
}

bool WASMReader::readCompactUInt32(uint32_t& result)
{
    uint32_t sum = 0;
    unsigned shift = 0;
    do {
        CHECK_READ(1);
        uint32_t byte = *m_cursor++;
        if (byte < 0x80) {
            if ((shift == 28 && byte >= 0x10) || (shift && !byte))
                return false;
            result = sum | (byte << shift);
            return true;
        }
        sum |= (byte & firstSevenBitsMask) << shift;
        shift += 7;
    } while (shift < 35);
    return false;
}

bool WASMReader::readString(String& result)
{
    StringBuilder builder;
    while (true) {
        CHECK_READ(1);
        char c = *m_cursor++;
        if (!c)
            break;
        builder.append(c);
    }
    result = builder.toString();
    return true;
}

bool WASMReader::readType(WASMType& result)
{
    return readByte<WASMType>(result, (uint8_t)WASMType::NumberOfTypes);
}

bool WASMReader::readExpressionType(WASMExpressionType& result)
{
    return readByte<WASMExpressionType>(result, (uint8_t)WASMExpressionType::NumberOfExpressionTypes);
}

bool WASMReader::readExportFormat(WASMExportFormat& result)
{
    return readByte<WASMExportFormat>(result, (uint8_t)WASMExportFormat::NumberOfExportFormats);
}

template <class T> bool WASMReader::readByte(T& result, uint8_t numberOfValues)
{
    CHECK_READ(1);
    uint8_t byte = *m_cursor++;
    if (byte >= numberOfValues)
        return false;
    result = T(byte);
    return true;
}

} // namespace JSC

#endif // ENABLE(WEBASSEMBLY)

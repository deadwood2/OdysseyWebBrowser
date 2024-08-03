/*
 * Copyright (C) 2014 Apple Inc. All rights reserved.
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

#include "config.h"
#include "IDBKeyData.h"

#if ENABLE(INDEXED_DATABASE)

#include "KeyedCoding.h"
#include <wtf/text/StringBuilder.h>

namespace WebCore {

IDBKeyData::IDBKeyData(const IDBKey* key)
    : m_type(KeyType::Invalid)
{
    if (!key) {
        m_isNull = true;
        return;
    }

    m_type = key->type();

    switch (m_type) {
    case KeyType::Invalid:
        break;
    case KeyType::Array:
        for (auto& key2 : key->array())
            m_arrayValue.append(IDBKeyData(key2.get()));
        break;
    case KeyType::String:
        m_stringValue = key->string();
        break;
    case KeyType::Date:
        m_numberValue = key->date();
        break;
    case KeyType::Number:
        m_numberValue = key->number();
        break;
    case KeyType::Max:
    case KeyType::Min:
        break;
    }
}

RefPtr<IDBKey> IDBKeyData::maybeCreateIDBKey() const
{
    if (m_isNull)
        return nullptr;

    switch (m_type) {
    case KeyType::Invalid:
        return IDBKey::createInvalid();
    case KeyType::Array:
        {
            Vector<RefPtr<IDBKey>> array;
            for (auto& keyData : m_arrayValue) {
                array.append(keyData.maybeCreateIDBKey());
                ASSERT(array.last());
            }
            return IDBKey::createArray(array);
        }
    case KeyType::String:
        return IDBKey::createString(m_stringValue);
    case KeyType::Date:
        return IDBKey::createDate(m_numberValue);
    case KeyType::Number:
        return IDBKey::createNumber(m_numberValue);
    case KeyType::Max:
    case KeyType::Min:
        ASSERT_NOT_REACHED();
        return nullptr;
    }

    ASSERT_NOT_REACHED();
    return nullptr;
}

IDBKeyData::IDBKeyData(const IDBKeyData& that, IsolatedCopyTag)
{
    isolatedCopy(that, *this);
}

IDBKeyData IDBKeyData::isolatedCopy() const
{
    return { *this, IsolatedCopy };
}

void IDBKeyData::isolatedCopy(const IDBKeyData& source, IDBKeyData& destination)
{
    destination.m_type = source.m_type;
    destination.m_isNull = source.m_isNull;

    switch (source.m_type) {
    case KeyType::Invalid:
        return;
    case KeyType::Array:
        for (auto& key : source.m_arrayValue)
            destination.m_arrayValue.append(key.isolatedCopy());
        return;
    case KeyType::String:
        destination.m_stringValue = source.m_stringValue.isolatedCopy();
        return;
    case KeyType::Date:
    case KeyType::Number:
        destination.m_numberValue = source.m_numberValue;
        return;
    case KeyType::Max:
    case KeyType::Min:
        return;
    }

    ASSERT_NOT_REACHED();
}

void IDBKeyData::encode(KeyedEncoder& encoder) const
{
    encoder.encodeBool("null", m_isNull);
    if (m_isNull)
        return;

    encoder.encodeEnum("type", m_type);

    switch (m_type) {
    case KeyType::Invalid:
        return;
    case KeyType::Array:
        encoder.encodeObjects("array", m_arrayValue.begin(), m_arrayValue.end(), [](KeyedEncoder& encoder, const IDBKeyData& key) {
            key.encode(encoder);
        });
        return;
    case KeyType::String:
        encoder.encodeString("string", m_stringValue);
        return;
    case KeyType::Date:
    case KeyType::Number:
        encoder.encodeDouble("number", m_numberValue);
        return;
    case KeyType::Max:
    case KeyType::Min:
        return;
    }

    ASSERT_NOT_REACHED();
}

bool IDBKeyData::decode(KeyedDecoder& decoder, IDBKeyData& result)
{
    if (!decoder.decodeBool("null", result.m_isNull))
        return false;

    if (result.m_isNull)
        return true;

    auto enumFunction = [](int64_t value) {
        return value == KeyType::Max
            || value == KeyType::Invalid
            || value == KeyType::Array
            || value == KeyType::String
            || value == KeyType::Date
            || value == KeyType::Number
            || value == KeyType::Min;
    };
    if (!decoder.decodeEnum("type", result.m_type, enumFunction))
        return false;

    if (result.m_type == KeyType::Invalid)
        return true;

    if (result.m_type == KeyType::Max)
        return true;

    if (result.m_type == KeyType::Min)
        return true;

    if (result.m_type == KeyType::String)
        return decoder.decodeString("string", result.m_stringValue);

    if (result.m_type == KeyType::Number || result.m_type == KeyType::Date)
        return decoder.decodeDouble("number", result.m_numberValue);

    ASSERT(result.m_type == KeyType::Array);

    auto arrayFunction = [](KeyedDecoder& decoder, IDBKeyData& result) {
        return decode(decoder, result);
    };
    
    result.m_arrayValue.clear();
    return decoder.decodeObjects("array", result.m_arrayValue, arrayFunction);
}

int IDBKeyData::compare(const IDBKeyData& other) const
{
    if (m_type == KeyType::Invalid) {
        if (other.m_type != KeyType::Invalid)
            return -1;
        if (other.m_type == KeyType::Invalid)
            return 0;
    } else if (other.m_type == KeyType::Invalid)
        return 1;

    // The IDBKey::m_type enum is in reverse sort order.
    if (m_type != other.m_type)
        return m_type < other.m_type ? 1 : -1;

    // The types are the same, so handle actual value comparison.
    switch (m_type) {
    case KeyType::Invalid:
        // Invalid type should have been fully handled above
        ASSERT_NOT_REACHED();
        return 0;
    case KeyType::Array:
        for (size_t i = 0; i < m_arrayValue.size() && i < other.m_arrayValue.size(); ++i) {
            if (int result = m_arrayValue[i].compare(other.m_arrayValue[i]))
                return result;
        }
        if (m_arrayValue.size() < other.m_arrayValue.size())
            return -1;
        if (m_arrayValue.size() > other.m_arrayValue.size())
            return 1;
        return 0;
    case KeyType::String:
        return codePointCompare(m_stringValue, other.m_stringValue);
    case KeyType::Date:
    case KeyType::Number:
        if (m_numberValue == other.m_numberValue)
            return 0;
        return m_numberValue > other.m_numberValue ? 1 : -1;
    case KeyType::Max:
    case KeyType::Min:
        return 0;
    }

    ASSERT_NOT_REACHED();
    return 0;
}

#if !LOG_DISABLED
String IDBKeyData::loggingString() const
{
    if (m_isNull)
        return "<null>";

    String result;

    switch (m_type) {
    case KeyType::Invalid:
        return "<invalid>";
    case KeyType::Array: {
        StringBuilder builder;
        builder.appendLiteral("<array> - { ");
        for (size_t i = 0; i < m_arrayValue.size(); ++i) {
            builder.append(m_arrayValue[i].loggingString());
            if (i < m_arrayValue.size() - 1)
                builder.appendLiteral(", ");
        }
        builder.appendLiteral(" }");
        result = builder.toString();
        break;
    }
    case KeyType::String:
        result = "<string> - " + m_stringValue;
        break;
    case KeyType::Date:
        return String::format("<date> - %f", m_numberValue);
    case KeyType::Number:
        return String::format("<number> - %f", m_numberValue);
    case KeyType::Max:
        return "<maximum>";
    case KeyType::Min:
        return "<minimum>";
    default:
        return String();
    }

    if (result.length() > 150) {
        result.truncate(147);
        result.append(WTF::ASCIILiteral("..."));
    }

    return result;
}
#endif

void IDBKeyData::setArrayValue(const Vector<IDBKeyData>& value)
{
    *this = IDBKeyData();
    m_arrayValue = value;
    m_type = KeyType::Array;
    m_isNull = false;
}

void IDBKeyData::setStringValue(const String& value)
{
    *this = IDBKeyData();
    m_stringValue = value;
    m_type = KeyType::String;
    m_isNull = false;
}

void IDBKeyData::setDateValue(double value)
{
    *this = IDBKeyData();
    m_numberValue = value;
    m_type = KeyType::Date;
    m_isNull = false;
}

void IDBKeyData::setNumberValue(double value)
{
    *this = IDBKeyData();
    m_numberValue = value;
    m_type = KeyType::Number;
    m_isNull = false;
}

IDBKeyData IDBKeyData::deletedValue()
{
    IDBKeyData result;
    result.m_isNull = false;
    result.m_isDeletedValue = true;
    return result;
}

bool IDBKeyData::operator<(const IDBKeyData& rhs) const
{
    return compare(rhs) < 0;
}

bool IDBKeyData::operator==(const IDBKeyData& other) const
{
    if (m_type != other.m_type || m_isNull != other.m_isNull || m_isDeletedValue != other.m_isDeletedValue)
        return false;
    switch (m_type) {
    case KeyType::Invalid:
    case KeyType::Max:
    case KeyType::Min:
        return true;
    case KeyType::Number:
    case KeyType::Date:
        return m_numberValue == other.m_numberValue;
    case KeyType::String:
        return m_stringValue == other.m_stringValue;
    case KeyType::Array:
        return m_arrayValue == other.m_arrayValue;
    }
    RELEASE_ASSERT_NOT_REACHED();
}

} // namespace WebCore

#endif // ENABLE(INDEXED_DATABASE)

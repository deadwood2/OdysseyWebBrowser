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

#if ENABLE(GAMEPAD)

#include <wtf/Vector.h>
#include <wtf/text/WTFString.h>

namespace IPC {
class Decoder;
class Encoder;
}

namespace WebKit {

class GamepadData {
public:
    GamepadData()
        : m_isNull(true)
    {
    }

    GamepadData(unsigned index, const Vector<double>& axisValues, const Vector<double>& buttonValues, double lastUpdateTime);
    GamepadData(unsigned index, const String& id, const Vector<double>& axisValues, const Vector<double>& buttonValues, double lastUpdateTime);

    void encode(IPC::Encoder&) const;
    static bool decode(IPC::Decoder&, GamepadData&);

    bool isNull() const { return m_isNull; }

    double lastUpdateTime() const { return m_lastUpdateTime; }
    unsigned index() const { return m_index; }
    const String& id() const { return m_id; }
    const Vector<double>& axisValues() const { return m_axisValues; }
    const Vector<double>& buttonValues() const { return m_buttonValues; }

private:
    unsigned m_index;
    String m_id;
    Vector<double> m_axisValues;
    Vector<double> m_buttonValues;
    double m_lastUpdateTime { 0.0 };

    bool m_isNull { false };

#if !LOG_DISABLED
    String loggingString() const;
#endif
};

} // namespace WebKit

#endif // ENABLE(GAMEPAD)

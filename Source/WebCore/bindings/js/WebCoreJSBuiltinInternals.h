/*
 *  Copyright (c) 2015, Canon Inc. All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *  1.  Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *  2.  Redistributions in binary form must reproduce the above copyright
 *      notice, this list of conditions and the following disclaimer in the
 *      documentation and/or other materials provided with the distribution.
 *  3.  Neither the name of Canon Inc. nor the names of
 *      its contributors may be used to endorse or promote products derived
 *      from this software without specific prior written permission.
 *  THIS SOFTWARE IS PROVIDED BY CANON INC. AND ITS CONTRIBUTORS "AS IS" AND ANY
 *  EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 *  WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 *  DISCLAIMED. IN NO EVENT SHALL CANON INC. AND ITS CONTRIBUTORS BE LIABLE FOR
 *  ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 *  DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 *  SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 *  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 *  OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef WebCoreJSBuiltinInternals_h
#define WebCoreJSBuiltinInternals_h

#include <runtime/VM.h>

#if ENABLE(MEDIA_STREAM)
#include "RTCPeerConnectionInternalsBuiltins.h"
#endif

#if ENABLE(STREAMS_API)
#include "ReadableStreamInternalsBuiltins.h"
#include "StreamInternalsBuiltins.h"
#include "WritableStreamInternalsBuiltins.h"
#endif

namespace WebCore {

class JSDOMGlobalObject;

// FIXME: Make builtin generator generate this class
class JSBuiltinInternalFunctions {
public:
    explicit JSBuiltinInternalFunctions(JSC::VM&);

#if ENABLE(MEDIA_STREAM)
    RTCPeerConnectionInternalsBuiltinFunctions rtcPeerConnectionInternals() { return m_rtcPeerConnectionInternalsFunctions; }
#endif
#if ENABLE(STREAMS_API)
    ReadableStreamInternalsBuiltinFunctions readableStreamInternals() { return m_readableStreamInternalsFunctions; }
    StreamInternalsBuiltinFunctions streamInternals() { return m_streamInternalsFunctions; }
    WritableStreamInternalsBuiltinFunctions writableStreamInternals() { return m_writableStreamInternalsFunctions; }
#endif

    void visit(JSC::SlotVisitor&);
    void initialize(JSDOMGlobalObject&, JSC::VM&);

private:
    JSC::VM& vm;
#if ENABLE(MEDIA_STREAM)
    RTCPeerConnectionInternalsBuiltinFunctions m_rtcPeerConnectionInternalsFunctions;
#endif
#if ENABLE(STREAMS_API)
    ReadableStreamInternalsBuiltinFunctions m_readableStreamInternalsFunctions;
    StreamInternalsBuiltinFunctions m_streamInternalsFunctions;
    WritableStreamInternalsBuiltinFunctions m_writableStreamInternalsFunctions;
#endif

};

}

#endif // WebCoreJSBuiltinInternals_h

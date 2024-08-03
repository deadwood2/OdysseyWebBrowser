/*
 * Copyright (C) 2008-2009, 2011, 2016 Apple Inc. All Rights Reserved.
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

#include "JSWorkerGlobalScope.h"

#include "ExceptionCode.h"
#include "JSDOMBinding.h"
#include "JSDOMGlobalObject.h"
#include "JSEventListener.h"
#include "JSEventSource.h"
#include "JSMessageChannel.h"
#include "JSMessagePort.h"
#include "JSWorkerLocation.h"
#include "JSWorkerNavigator.h"
#include "JSXMLHttpRequest.h"
#include "ScheduledAction.h"
#include "WorkerGlobalScope.h"
#include "WorkerLocation.h"
#include "WorkerNavigator.h"

#if ENABLE(WEB_SOCKETS)
#include "JSWebSocket.h"
#endif

using namespace JSC;

namespace WebCore {

void JSWorkerGlobalScope::visitAdditionalChildren(SlotVisitor& visitor)
{
    if (WorkerLocation* location = wrapped().optionalLocation())
        visitor.addOpaqueRoot(location);
    if (WorkerNavigator* navigator = wrapped().optionalNavigator())
        visitor.addOpaqueRoot(navigator);
    visitor.addOpaqueRoot(wrapped().scriptExecutionContext());
}

JSValue JSWorkerGlobalScope::importScripts(ExecState& state)
{
    if (!state.argumentCount())
        return jsUndefined();

    Vector<String> urls;
    for (unsigned i = 0; i < state.argumentCount(); ++i) {
        urls.append(valueToUSVString(&state, state.uncheckedArgument(i)));
        if (state.hadException())
            return jsUndefined();
    }
    ExceptionCode ec = 0;

    wrapped().importScripts(urls, ec);
    setDOMException(&state, ec);
    return jsUndefined();
}

JSValue JSWorkerGlobalScope::setTimeout(ExecState& state)
{
    VM& vm = state.vm();
    auto scope = DECLARE_THROW_SCOPE(vm);

    if (UNLIKELY(state.argumentCount() < 1))
        return throwException(&state, scope, createNotEnoughArgumentsError(&state));

    std::unique_ptr<ScheduledAction> action = ScheduledAction::create(&state, globalObject()->world(), wrapped().contentSecurityPolicy());
    if (state.hadException())
        return jsUndefined();
    if (!action)
        return jsNumber(0);
    int delay = state.argument(1).toInt32(&state);
    return jsNumber(wrapped().setTimeout(WTFMove(action), delay));
}

JSValue JSWorkerGlobalScope::setInterval(ExecState& state)
{
    VM& vm = state.vm();
    auto scope = DECLARE_THROW_SCOPE(vm);

    if (UNLIKELY(state.argumentCount() < 1))
        return throwException(&state, scope, createNotEnoughArgumentsError(&state));

    std::unique_ptr<ScheduledAction> action = ScheduledAction::create(&state, globalObject()->world(), wrapped().contentSecurityPolicy());
    if (state.hadException())
        return jsUndefined();
    if (!action)
        return jsNumber(0);
    int delay = state.argument(1).toInt32(&state);
    return jsNumber(wrapped().setInterval(WTFMove(action), delay));
}

} // namespace WebCore

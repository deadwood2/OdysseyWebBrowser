/*
 * Copyright (C) 2015, 2016 Apple Inc. All rights reserved.
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

WI.LoggingProtocolTracer = class LoggingProtocolTracer extends WI.ProtocolTracer
{
    constructor()
    {
        super();

        this._dumpMessagesToConsole = false;
        this._dumpTimingDataToConsole = false;
        this._logToConsole = window.InspectorTest ? InspectorFrontendHost.unbufferedLog.bind(InspectorFrontendHost) : console.log.bind(console);
    }

    // Public

    set dumpMessagesToConsole(value)
    {
        this._dumpMessagesToConsole = !!value;
    }

    get dumpMessagesToConsole()
    {
        return this._dumpMessagesToConsole;
    }

    set dumpTimingDataToConsole(value)
    {
        this._dumpTimingDataToConsole = !!value;
    }

    get dumpTimingDataToConsole()
    {
        return this._dumpTimingDataToConsole;
    }

    logFrontendException(message, exception)
    {
        this._processEntry({type: "exception", message, exception});
    }

    logProtocolError(message, error)
    {
        this._processEntry({type: "error", message, error});
    }

    logFrontendRequest(message)
    {
        this._processEntry({type: "request", message});
    }

    logWillHandleResponse(message)
    {
        let entry = {type: "response", message};
        this._processEntry(entry);
    }

    logDidHandleResponse(message, timings = null)
    {
        let entry = {type: "response", message};
        if (timings)
            entry.timings = Object.shallowCopy(timings);

        this._processEntry(entry);
    }

    logWillHandleEvent(message)
    {
        let entry = {type: "event", message};
        this._processEntry(entry);
    }

    logDidHandleEvent(message, timings = null)
    {
        let entry = {type: "event", message};
        if (timings)
            entry.timings = Object.shallowCopy(timings);

        this._processEntry(entry);
    }

    _processEntry(entry)
    {
        if (this._dumpTimingDataToConsole && entry.timings) {
            if (entry.timings.rtt && entry.timings.dispatch)
                this._logToConsole(`time-stats: Handling: ${entry.timings.dispatch || NaN}ms; RTT: ${entry.timings.rtt}ms`);
            else if (entry.timings.dispatch)
                this._logToConsole(`time-stats: Handling: ${entry.timings.dispatch || NaN}ms`);
        } else if (this._dumpMessagesToConsole && !entry.timings) {
            this._logToConsole(`${entry.type}: ${JSON.stringify(entry.message)}`);
            if (entry.exception) {
                this._logToConsole(entry.exception);
                if (entry.exception.stack)
                    this._logToConsole(entry.exception.stack);
            }
        }
    }
};

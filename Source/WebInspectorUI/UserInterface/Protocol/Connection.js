/*
 * Copyright (C) 2011 Google Inc. All rights reserved.
 * Copyright (C) 2013-2016 Apple Inc. All rights reserved.
 * Copyright (C) 2014 University of Washington.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

InspectorBackend.Connection = class InspectorBackendConnection
{
    constructor()
    {
        this._lastSequenceId = 1;
        this._pendingResponses = new Map;
        this._agents = {};
        this._deferredScripts = [];
        this._target = null;
    }

    // Public

    get target()
    {
        return this._target;
    }

    set target(target)
    {
        console.assert(!this._target);

        this._target = target;

        for (let domain in this._agents) {
            let dispatcher = this._agents[domain].dispatcher;
            if (dispatcher)
                dispatcher.target = target;
        }
    }

    dispatch(message)
    {
        let messageObject = typeof message === "string" ? JSON.parse(message) : message;

        if ("id" in messageObject)
            this._dispatchResponse(messageObject);
        else
            this._dispatchEvent(messageObject);
    }

    runAfterPendingDispatches(script)
    {
        console.assert(typeof script === "function");

        if (!this._pendingResponses.size)
            script.call(this);
        else
            this._deferredScripts.push(script);
    }

    // Protected

    sendMessageToBackend(message)
    {
        throw new Error("Should be implemented by a InspectorBackend.Connection subclass");
    }

    // Private

    _dispatchResponse(messageObject)
    {
        console.assert(this._pendingResponses.size >= 0);

        if (messageObject["error"]) {
            if (messageObject["error"].code !== -32000)
                console.error("Request with id = " + messageObject["id"] + " failed. " + JSON.stringify(messageObject["error"]));
        }

        let sequenceId = messageObject["id"];
        console.assert(this._pendingResponses.has(sequenceId), sequenceId, this._pendingResponses);

        let responseData = this._pendingResponses.take(sequenceId) || {};
        let {request, command, callback, promise} = responseData;

        let processingStartTimestamp = performance.now();
        for (let tracer of InspectorBackend.activeTracers)
            tracer.logWillHandleResponse(messageObject);

        InspectorBackend.currentDispatchState.request = request;
        InspectorBackend.currentDispatchState.response = messageObject;

        if (typeof callback === "function")
            this._dispatchResponseToCallback(command, request, messageObject, callback);
        else if (typeof promise === "object")
            this._dispatchResponseToPromise(command, messageObject, promise);
        else
            console.error("Received a command response without a corresponding callback or promise.", messageObject, command);

        InspectorBackend.currentDispatchState.request = null;
        InspectorBackend.currentDispatchState.response = null;

        let processingTime = (performance.now() - processingStartTimestamp).toFixed(3);
        let roundTripTime = (processingStartTimestamp - responseData.sendRequestTimestamp).toFixed(3);

        for (let tracer of InspectorBackend.activeTracers)
            tracer.logDidHandleResponse(messageObject, {rtt: roundTripTime, dispatch: processingTime});

        if (this._deferredScripts.length && !this._pendingResponses.size)
            this._flushPendingScripts();
    }

    _dispatchResponseToCallback(command, requestObject, responseObject, callback)
    {
        let callbackArguments = [];
        callbackArguments.push(responseObject["error"] ? responseObject["error"].message : null);

        if (responseObject["result"]) {
            for (let parameterName of command.replySignature)
                callbackArguments.push(responseObject["result"][parameterName]);
        }

        try {
            callback.apply(null, callbackArguments);
        } catch (e) {
            WI.reportInternalError(e, {"cause": `An uncaught exception was thrown while dispatching response callback for command ${command.qualifiedName}.`});
        }
    }

    _dispatchResponseToPromise(command, messageObject, promise)
    {
        let {resolve, reject} = promise;
        if (messageObject["error"])
            reject(new Error(messageObject["error"].message));
        else
            resolve(messageObject["result"]);
    }

    _dispatchEvent(messageObject)
    {
        let qualifiedName = messageObject["method"];
        let [domainName, eventName] = qualifiedName.split(".");
        if (!(domainName in this._agents)) {
            console.error("Protocol Error: Attempted to dispatch method '" + eventName + "' for non-existing domain '" + domainName + "'");
            return;
        }

        let agent = this._agents[domainName];
        if (!agent.active) {
            console.error("Protocol Error: Attempted to dispatch method for domain '" + domainName + "' which exists but is not active.");
            return;
        }

        let event = agent.getEvent(eventName);
        if (!event) {
            console.error("Protocol Error: Attempted to dispatch an unspecified method '" + qualifiedName + "'");
            return;
        }

        let eventArguments = [];
        if (messageObject["params"])
            eventArguments = event.parameterNames.map((name) => messageObject["params"][name]);

        let processingStartTimestamp = performance.now();
        for (let tracer of InspectorBackend.activeTracers)
            tracer.logWillHandleEvent(messageObject);

        InspectorBackend.currentDispatchState.event = messageObject;

        try {
            agent.dispatchEvent(eventName, eventArguments);
        } catch (e) {
            for (let tracer of InspectorBackend.activeTracers)
                tracer.logFrontendException(messageObject, e);

            WI.reportInternalError(e, {"cause": `An uncaught exception was thrown while handling event: ${qualifiedName}`});
        }

        InspectorBackend.currentDispatchState.event = null;

        let processingDuration = (performance.now() - processingStartTimestamp).toFixed(3);
        for (let tracer of InspectorBackend.activeTracers)
            tracer.logDidHandleEvent(messageObject, {dispatch: processingDuration});
    }

    _sendCommandToBackendWithCallback(command, parameters, callback)
    {
        let sequenceId = this._lastSequenceId++;

        let messageObject = {
            "id": sequenceId,
            "method": command.qualifiedName,
        };

        if (!isEmptyObject(parameters))
            messageObject["params"] = parameters;

        let responseData = {command, request: messageObject, callback};

        if (InspectorBackend.activeTracer)
            responseData.sendRequestTimestamp = performance.now();

        this._pendingResponses.set(sequenceId, responseData);
        this._sendMessageToBackend(messageObject);
    }

    _sendCommandToBackendExpectingPromise(command, parameters)
    {
        let sequenceId = this._lastSequenceId++;

        let messageObject = {
            "id": sequenceId,
            "method": command.qualifiedName,
        };

        if (!isEmptyObject(parameters))
            messageObject["params"] = parameters;

        let responseData = {command, request: messageObject};

        if (InspectorBackend.activeTracer)
            responseData.sendRequestTimestamp = performance.now();

        let responsePromise = new Promise(function(resolve, reject) {
            responseData.promise = {resolve, reject};
        });

        this._pendingResponses.set(sequenceId, responseData);
        this._sendMessageToBackend(messageObject);

        return responsePromise;
    }

    _sendMessageToBackend(messageObject)
    {
        for (let tracer of InspectorBackend.activeTracers)
            tracer.logFrontendRequest(messageObject);

        this.sendMessageToBackend(JSON.stringify(messageObject));
    }

    _flushPendingScripts()
    {
        console.assert(this._pendingResponses.size === 0);

        let scriptsToRun = this._deferredScripts;
        this._deferredScripts = [];
        for (let script of scriptsToRun)
            script.call(this);
    }
};

InspectorBackend.MainConnection = class InspectorBackendMainConnection extends InspectorBackend.Connection
{
    constructor()
    {
        super();

        this._agents = InspectorBackend._agents;
    }

    sendMessageToBackend(message)
    {
        InspectorFrontendHost.sendMessageToBackend(message);
    }
};

InspectorBackend.WorkerConnection = class InspectorBackendWorkerConnection extends InspectorBackend.Connection
{
    constructor(workerId)
    {
        super();

        this._workerId = workerId;

        const workerDomains = InspectorBackend.workerSupportedDomains;

        for (let domain of workerDomains) {
            let agent = InspectorBackend._agents[domain];
            let clone = Object.create(InspectorBackend._agents[domain]);
            clone.connection = this;
            clone.dispatcher = new agent.dispatcher.constructor;
            this._agents[domain] = clone;
        }
    }

    sendMessageToBackend(message)
    {
        WorkerAgent.sendMessageToWorker(this._workerId, message);
    }
};

InspectorBackend.mainConnection = new InspectorBackend.MainConnection;

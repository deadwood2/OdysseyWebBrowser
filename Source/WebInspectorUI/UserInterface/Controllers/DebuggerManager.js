/*
 * Copyright (C) 2013-2016 Apple Inc. All rights reserved.
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

WebInspector.DebuggerManager = class DebuggerManager extends WebInspector.Object
{
    constructor()
    {
        super();

        DebuggerAgent.enable();

        WebInspector.notifications.addEventListener(WebInspector.Notification.DebugUIEnabledDidChange, this._debugUIEnabledDidChange, this);

        WebInspector.Breakpoint.addEventListener(WebInspector.Breakpoint.Event.DisplayLocationDidChange, this._breakpointDisplayLocationDidChange, this);
        WebInspector.Breakpoint.addEventListener(WebInspector.Breakpoint.Event.DisabledStateDidChange, this._breakpointDisabledStateDidChange, this);
        WebInspector.Breakpoint.addEventListener(WebInspector.Breakpoint.Event.ConditionDidChange, this._breakpointEditablePropertyDidChange, this);
        WebInspector.Breakpoint.addEventListener(WebInspector.Breakpoint.Event.IgnoreCountDidChange, this._breakpointEditablePropertyDidChange, this);
        WebInspector.Breakpoint.addEventListener(WebInspector.Breakpoint.Event.AutoContinueDidChange, this._breakpointEditablePropertyDidChange, this);
        WebInspector.Breakpoint.addEventListener(WebInspector.Breakpoint.Event.ActionsDidChange, this._breakpointEditablePropertyDidChange, this);

        WebInspector.timelineManager.addEventListener(WebInspector.TimelineManager.Event.CapturingWillStart, this._timelineCapturingWillStart, this);
        WebInspector.timelineManager.addEventListener(WebInspector.TimelineManager.Event.CapturingStopped, this._timelineCapturingStopped, this);

        WebInspector.targetManager.addEventListener(WebInspector.TargetManager.Event.TargetRemoved, this._targetRemoved, this);

        WebInspector.Frame.addEventListener(WebInspector.Frame.Event.MainResourceDidChange, this._mainResourceDidChange, this);

        this._breakpointsSetting = new WebInspector.Setting("breakpoints", []);
        this._breakpointsEnabledSetting = new WebInspector.Setting("breakpoints-enabled", true);
        this._allExceptionsBreakpointEnabledSetting = new WebInspector.Setting("break-on-all-exceptions", false);
        this._allUncaughtExceptionsBreakpointEnabledSetting = new WebInspector.Setting("break-on-all-uncaught-exceptions", false);
        this._assertionsBreakpointEnabledSetting = new WebInspector.Setting("break-on-assertions", false);
        this._asyncStackTraceDepthSetting = new WebInspector.Setting("async-stack-trace-depth", 200);

        let specialBreakpointLocation = new WebInspector.SourceCodeLocation(null, Infinity, Infinity);

        this._allExceptionsBreakpoint = new WebInspector.Breakpoint(specialBreakpointLocation, !this._allExceptionsBreakpointEnabledSetting.value);
        this._allExceptionsBreakpoint.resolved = true;

        this._allUncaughtExceptionsBreakpoint = new WebInspector.Breakpoint(specialBreakpointLocation, !this._allUncaughtExceptionsBreakpointEnabledSetting.value);

        this._assertionsBreakpoint = new WebInspector.Breakpoint(specialBreakpointLocation, !this._assertionsBreakpointEnabledSetting.value);
        this._assertionsBreakpoint.resolved = true;

        this._breakpoints = [];
        this._breakpointContentIdentifierMap = new Map;
        this._breakpointScriptIdentifierMap = new Map;
        this._breakpointIdMap = new Map;

        this._breakOnExceptionsState = "none";
        this._updateBreakOnExceptionsState();

        this._nextBreakpointActionIdentifier = 1;

        this._activeCallFrame = null;

        this._internalWebKitScripts = [];
        this._targetDebuggerDataMap = new Map;
        this._targetDebuggerDataMap.set(WebInspector.mainTarget, new WebInspector.DebuggerData(WebInspector.mainTarget));

        // Restore the correct breakpoints enabled setting if Web Inspector had
        // previously been left in a state where breakpoints were temporarily disabled.
        this._temporarilyDisabledBreakpointsRestoreSetting = new WebInspector.Setting("temporarily-disabled-breakpoints-restore", null);
        if (this._temporarilyDisabledBreakpointsRestoreSetting.value !== null) {
            this._breakpointsEnabledSetting.value = this._temporarilyDisabledBreakpointsRestoreSetting.value;
            this._temporarilyDisabledBreakpointsRestoreSetting.value = null;
        }

        DebuggerAgent.setBreakpointsActive(this._breakpointsEnabledSetting.value);
        DebuggerAgent.setPauseOnExceptions(this._breakOnExceptionsState);

        // COMPATIBILITY (iOS 10): DebuggerAgent.setPauseOnAssertions did not exist yet.
        if (DebuggerAgent.setPauseOnAssertions)
            DebuggerAgent.setPauseOnAssertions(this._assertionsBreakpointEnabledSetting.value);

        // COMPATIBILITY (iOS 10): Debugger.setAsyncStackTraceDepth did not exist yet.
        if (DebuggerAgent.setAsyncStackTraceDepth)
            DebuggerAgent.setAsyncStackTraceDepth(this._asyncStackTraceDepthSetting.value);

        this._ignoreBreakpointDisplayLocationDidChangeEvent = false;

        function restoreBreakpointsSoon() {
            this._restoringBreakpoints = true;
            for (let cookie of this._breakpointsSetting.value)
                this.addBreakpoint(new WebInspector.Breakpoint(cookie));
            this._restoringBreakpoints = false;
        }

        // Ensure that all managers learn about restored breakpoints,
        // regardless of their initialization order.
        setTimeout(restoreBreakpointsSoon.bind(this), 0);
    }

    // Public

    get paused()
    {
        for (let [target, targetData] of this._targetDebuggerDataMap) {
            if (targetData.paused)
                return true;
        }

        return false;
    }

    get activeCallFrame()
    {
        return this._activeCallFrame;
    }

    set activeCallFrame(callFrame)
    {
        if (callFrame === this._activeCallFrame)
            return;

        this._activeCallFrame = callFrame || null;

        this.dispatchEventToListeners(WebInspector.DebuggerManager.Event.ActiveCallFrameDidChange);
    }

    dataForTarget(target)
    {
        let targetData = this._targetDebuggerDataMap.get(target);
        if (targetData)
            return targetData;

        targetData = new WebInspector.DebuggerData(target);
        this._targetDebuggerDataMap.set(target, targetData);
        return targetData;
    }

    get allExceptionsBreakpoint()
    {
        return this._allExceptionsBreakpoint;
    }

    get allUncaughtExceptionsBreakpoint()
    {
        return this._allUncaughtExceptionsBreakpoint;
    }

    get assertionsBreakpoint()
    {
        return this._assertionsBreakpoint;
    }

    get breakpoints()
    {
        return this._breakpoints;
    }

    breakpointForIdentifier(id)
    {
        return this._breakpointIdMap.get(id) || null;
    }

    breakpointsForSourceCode(sourceCode)
    {
        console.assert(sourceCode instanceof WebInspector.Resource || sourceCode instanceof WebInspector.Script);

        if (sourceCode instanceof WebInspector.SourceMapResource) {
            let originalSourceCodeBreakpoints = this.breakpointsForSourceCode(sourceCode.sourceMap.originalSourceCode);
            return originalSourceCodeBreakpoints.filter(function(breakpoint) {
                return breakpoint.sourceCodeLocation.displaySourceCode === sourceCode;
            });
        }

        let contentIdentifierBreakpoints = this._breakpointContentIdentifierMap.get(sourceCode.contentIdentifier);
        if (contentIdentifierBreakpoints) {
            this._associateBreakpointsWithSourceCode(contentIdentifierBreakpoints, sourceCode);
            return contentIdentifierBreakpoints;
        }

        if (sourceCode instanceof WebInspector.Script) {
            let scriptIdentifierBreakpoints = this._breakpointScriptIdentifierMap.get(sourceCode.id);
            if (scriptIdentifierBreakpoints) {
                this._associateBreakpointsWithSourceCode(scriptIdentifierBreakpoints, sourceCode);
                return scriptIdentifierBreakpoints;
            }
        }

        return [];
    }

    isBreakpointRemovable(breakpoint)
    {
        return breakpoint !== this._allExceptionsBreakpoint
            && breakpoint !== this._allUncaughtExceptionsBreakpoint
            && breakpoint !== this._assertionsBreakpoint;
    }

    isBreakpointEditable(breakpoint)
    {
        return this.isBreakpointRemovable(breakpoint);
    }

    get breakpointsEnabled()
    {
        return this._breakpointsEnabledSetting.value;
    }

    set breakpointsEnabled(enabled)
    {
        if (this._breakpointsEnabledSetting.value === enabled)
            return;

        console.assert(!(enabled && this.breakpointsDisabledTemporarily), "Should not enable breakpoints when we are temporarily disabling breakpoints.");
        if (enabled && this.breakpointsDisabledTemporarily)
            return;

        this._breakpointsEnabledSetting.value = enabled;

        this._updateBreakOnExceptionsState();

        for (let target of WebInspector.targets) {
            target.DebuggerAgent.setBreakpointsActive(enabled);
            target.DebuggerAgent.setPauseOnExceptions(this._breakOnExceptionsState);
        }

        this.dispatchEventToListeners(WebInspector.DebuggerManager.Event.BreakpointsEnabledDidChange);
    }

    get breakpointsDisabledTemporarily()
    {
        return this._temporarilyDisabledBreakpointsRestoreSetting.value !== null;
    }

    scriptForIdentifier(id, target)
    {
        console.assert(target instanceof WebInspector.Target);
        return this.dataForTarget(target).scriptForIdentifier(id);
    }

    scriptsForURL(url, target)
    {
        // FIXME: This may not be safe. A Resource's URL may differ from a Script's URL.
        console.assert(target instanceof WebInspector.Target);
        return this.dataForTarget(target).scriptsForURL(url);
    }

    get searchableScripts()
    {
        return this.knownNonResourceScripts.filter((script) => !!script.contentIdentifier);
    }

    get knownNonResourceScripts()
    {
        let knownScripts = [];

        for (let [target, targetData] of this._targetDebuggerDataMap) {
            for (let script of targetData.scripts) {
                if (script.resource)
                    continue;
                if (!WebInspector.isDebugUIEnabled() && isWebKitInternalScript(script.sourceURL))
                    continue;
                knownScripts.push(script);
            }
        }

        return knownScripts;
    }

    get asyncStackTraceDepth()
    {
        return this._asyncStackTraceDepthSetting.value;
    }

    set asyncStackTraceDepth(x)
    {
        if (this._asyncStackTraceDepthSetting.value === x)
            return;

        this._asyncStackTraceDepthSetting.value = x;

        for (let target of WebInspector.targets)
            target.DebuggerAgent.setAsyncStackTraceDepth(this._asyncStackTraceDepthSetting.value);
    }

    pause()
    {
        if (this.paused)
            return Promise.resolve();

        this.dispatchEventToListeners(WebInspector.DebuggerManager.Event.WaitingToPause);

        let listener = new WebInspector.EventListener(this, true);

        let managerResult = new Promise(function(resolve, reject) {
            listener.connect(WebInspector.debuggerManager, WebInspector.DebuggerManager.Event.Paused, resolve);
        });

        let promises = [];
        for (let [target, targetData] of this._targetDebuggerDataMap)
            promises.push(targetData.pauseIfNeeded());

        return Promise.all([managerResult, ...promises]);
    }

    resume()
    {
        if (!this.paused)
            return Promise.resolve();

        let listener = new WebInspector.EventListener(this, true);

        let managerResult = new Promise(function(resolve, reject) {
            listener.connect(WebInspector.debuggerManager, WebInspector.DebuggerManager.Event.Resumed, resolve);
        });

        let promises = [];
        for (let [target, targetData] of this._targetDebuggerDataMap)
            promises.push(targetData.resumeIfNeeded());

        return Promise.all([managerResult, ...promises]);
    }

    stepOver()
    {
        if (!this.paused)
            return Promise.reject(new Error("Cannot step over because debugger is not paused."));

        let listener = new WebInspector.EventListener(this, true);

        let managerResult = new Promise(function(resolve, reject) {
            listener.connect(WebInspector.debuggerManager, WebInspector.DebuggerManager.Event.ActiveCallFrameDidChange, resolve);
        });

        let protocolResult = this._activeCallFrame.target.DebuggerAgent.stepOver()
            .catch(function(error) {
                listener.disconnect();
                console.error("DebuggerManager.stepOver failed: ", error);
                throw error;
            });

        return Promise.all([managerResult, protocolResult]);
    }

    stepInto()
    {
        if (!this.paused)
            return Promise.reject(new Error("Cannot step into because debugger is not paused."));

        let listener = new WebInspector.EventListener(this, true);

        let managerResult = new Promise(function(resolve, reject) {
            listener.connect(WebInspector.debuggerManager, WebInspector.DebuggerManager.Event.ActiveCallFrameDidChange, resolve);
        });

        let protocolResult = this._activeCallFrame.target.DebuggerAgent.stepInto()
            .catch(function(error) {
                listener.disconnect();
                console.error("DebuggerManager.stepInto failed: ", error);
                throw error;
            });

        return Promise.all([managerResult, protocolResult]);
    }

    stepOut()
    {
        if (!this.paused)
            return Promise.reject(new Error("Cannot step out because debugger is not paused."));

        let listener = new WebInspector.EventListener(this, true);

        let managerResult = new Promise(function(resolve, reject) {
            listener.connect(WebInspector.debuggerManager, WebInspector.DebuggerManager.Event.ActiveCallFrameDidChange, resolve);
        });

        let protocolResult = this._activeCallFrame.target.DebuggerAgent.stepOut()
            .catch(function(error) {
                listener.disconnect();
                console.error("DebuggerManager.stepOut failed: ", error);
                throw error;
            });

        return Promise.all([managerResult, protocolResult]);
    }

    continueUntilNextRunLoop(target)
    {
        return this.dataForTarget(target).continueUntilNextRunLoop();
    }

    continueToLocation(script, lineNumber, columnNumber)
    {
        return script.target.DebuggerAgent.continueToLocation({scriptId: script.id, lineNumber, columnNumber});
    }

    addBreakpoint(breakpoint, shouldSpeculativelyResolve)
    {
        console.assert(breakpoint instanceof WebInspector.Breakpoint);
        if (!breakpoint)
            return;

        if (breakpoint.contentIdentifier) {
            let contentIdentifierBreakpoints = this._breakpointContentIdentifierMap.get(breakpoint.contentIdentifier);
            if (!contentIdentifierBreakpoints) {
                contentIdentifierBreakpoints = [];
                this._breakpointContentIdentifierMap.set(breakpoint.contentIdentifier, contentIdentifierBreakpoints);
            }
            contentIdentifierBreakpoints.push(breakpoint);
        }

        if (breakpoint.scriptIdentifier) {
            let scriptIdentifierBreakpoints = this._breakpointScriptIdentifierMap.get(breakpoint.scriptIdentifier);
            if (!scriptIdentifierBreakpoints) {
                scriptIdentifierBreakpoints = [];
                this._breakpointScriptIdentifierMap.set(breakpoint.scriptIdentifier, scriptIdentifierBreakpoints);
            }
            scriptIdentifierBreakpoints.push(breakpoint);
        }

        this._breakpoints.push(breakpoint);

        if (!breakpoint.disabled) {
            const specificTarget = undefined;
            this._setBreakpoint(breakpoint, specificTarget, () => {
                if (shouldSpeculativelyResolve)
                    breakpoint.resolved = true;
            });
        }

        this._saveBreakpoints();

        this.dispatchEventToListeners(WebInspector.DebuggerManager.Event.BreakpointAdded, {breakpoint});
    }

    removeBreakpoint(breakpoint)
    {
        console.assert(breakpoint instanceof WebInspector.Breakpoint);
        if (!breakpoint)
            return;

        console.assert(this.isBreakpointRemovable(breakpoint));
        if (!this.isBreakpointRemovable(breakpoint))
            return;

        this._breakpoints.remove(breakpoint);

        if (breakpoint.identifier)
            this._removeBreakpoint(breakpoint);

        if (breakpoint.contentIdentifier) {
            let contentIdentifierBreakpoints = this._breakpointContentIdentifierMap.get(breakpoint.contentIdentifier);
            if (contentIdentifierBreakpoints) {
                contentIdentifierBreakpoints.remove(breakpoint);
                if (!contentIdentifierBreakpoints.length)
                    this._breakpointContentIdentifierMap.delete(breakpoint.contentIdentifier);
            }
        }

        if (breakpoint.scriptIdentifier) {
            let scriptIdentifierBreakpoints = this._breakpointScriptIdentifierMap.get(breakpoint.scriptIdentifier);
            if (scriptIdentifierBreakpoints) {
                scriptIdentifierBreakpoints.remove(breakpoint);
                if (!scriptIdentifierBreakpoints.length)
                    this._breakpointScriptIdentifierMap.delete(breakpoint.scriptIdentifier);
            }
        }

        // Disable the breakpoint first, so removing actions doesn't re-add the breakpoint.
        breakpoint.disabled = true;
        breakpoint.clearActions();

        this._saveBreakpoints();

        this.dispatchEventToListeners(WebInspector.DebuggerManager.Event.BreakpointRemoved, {breakpoint});
    }

    nextBreakpointActionIdentifier()
    {
        return this._nextBreakpointActionIdentifier++;
    }

    initializeTarget(target)
    {
        let DebuggerAgent = target.DebuggerAgent;
        let targetData = this.dataForTarget(target);

        // Initialize global state.
        DebuggerAgent.enable();
        DebuggerAgent.setBreakpointsActive(this._breakpointsEnabledSetting.value);
        DebuggerAgent.setPauseOnAssertions(this._assertionsBreakpointEnabledSetting.value);
        DebuggerAgent.setPauseOnExceptions(this._breakOnExceptionsState);
        DebuggerAgent.setAsyncStackTraceDepth(this._asyncStackTraceDepthSetting.value);

        if (this.paused)
            targetData.pauseIfNeeded();

        // Initialize breakpoints.
        this._restoringBreakpoints = true;
        for (let breakpoint of this._breakpoints) {
            if (breakpoint.disabled)
                continue;
            if (!breakpoint.contentIdentifier)
                continue;
            this._setBreakpoint(breakpoint, target);
        }
        this._restoringBreakpoints = false;
    }

    // Protected (Called from WebInspector.DebuggerObserver)

    breakpointResolved(target, breakpointIdentifier, location)
    {
        // Called from WebInspector.DebuggerObserver.

        let breakpoint = this._breakpointIdMap.get(breakpointIdentifier);
        console.assert(breakpoint);
        if (!breakpoint)
            return;

        console.assert(breakpoint.identifier === breakpointIdentifier);

        if (!breakpoint.sourceCodeLocation.sourceCode) {
            let sourceCodeLocation = this._sourceCodeLocationFromPayload(target, location);
            breakpoint.sourceCodeLocation.sourceCode = sourceCodeLocation.sourceCode;
        }

        breakpoint.resolved = true;
    }

    reset()
    {
        // Called from WebInspector.DebuggerObserver.

        let wasPaused = this.paused;

        WebInspector.Script.resetUniqueDisplayNameNumbers();

        this._internalWebKitScripts = [];
        this._targetDebuggerDataMap.clear();

        this._ignoreBreakpointDisplayLocationDidChangeEvent = true;

        // Mark all the breakpoints as unresolved. They will be reported as resolved when
        // breakpointResolved is called as the page loads.
        for (let breakpoint of this._breakpoints) {
            breakpoint.resolved = false;
            if (breakpoint.sourceCodeLocation.sourceCode)
                breakpoint.sourceCodeLocation.sourceCode = null;
        }

        this._ignoreBreakpointDisplayLocationDidChangeEvent = false;

        this.dispatchEventToListeners(WebInspector.DebuggerManager.Event.ScriptsCleared);

        if (wasPaused)
            this.dispatchEventToListeners(WebInspector.DebuggerManager.Event.Resumed);
    }

    debuggerDidPause(target, callFramesPayload, reason, data, asyncStackTracePayload)
    {
        // Called from WebInspector.DebuggerObserver.

        if (this._delayedResumeTimeout) {
            clearTimeout(this._delayedResumeTimeout);
            this._delayedResumeTimeout = undefined;
        }

        let wasPaused = this.paused;
        let targetData = this._targetDebuggerDataMap.get(target);

        let callFrames = [];
        let pauseReason = this._pauseReasonFromPayload(reason);
        let pauseData = data || null;

        for (var i = 0; i < callFramesPayload.length; ++i) {
            var callFramePayload = callFramesPayload[i];
            var sourceCodeLocation = this._sourceCodeLocationFromPayload(target, callFramePayload.location);
            // FIXME: There may be useful call frames without a source code location (native callframes), should we include them?
            if (!sourceCodeLocation)
                continue;
            if (!sourceCodeLocation.sourceCode)
                continue;

            // Exclude the case where the call frame is in the inspector code.
            if (!WebInspector.isDebugUIEnabled() && isWebKitInternalScript(sourceCodeLocation.sourceCode.sourceURL))
                continue;

            let scopeChain = this._scopeChainFromPayload(target, callFramePayload.scopeChain);
            let callFrame = WebInspector.CallFrame.fromDebuggerPayload(target, callFramePayload, scopeChain, sourceCodeLocation);
            callFrames.push(callFrame);
        }

        let activeCallFrame = callFrames[0];

        if (!activeCallFrame) {
            // FIXME: This may not be safe for multiple threads/targets.
            // This indicates we were pausing in internal scripts only (Injected Scripts).
            // Just resume and skip past this pause. We should be fixing the backend to
            // not send such pauses.
            if (wasPaused)
                target.DebuggerAgent.continueUntilNextRunLoop();
            else
                target.DebuggerAgent.resume();
            this._didResumeInternal(target);
            return;
        }

        let asyncStackTrace = WebInspector.StackTrace.fromPayload(target, asyncStackTracePayload);
        targetData.updateForPause(callFrames, pauseReason, pauseData, asyncStackTrace);

        // Pause other targets because at least one target has paused.
        // FIXME: Should this be done on the backend?
        for (let [otherTarget, otherTargetData] of this._targetDebuggerDataMap)
            otherTargetData.pauseIfNeeded();

        let activeCallFrameDidChange = this._activeCallFrame && this._activeCallFrame.target === target;
        if (activeCallFrameDidChange)
            this._activeCallFrame = activeCallFrame;
        else if (!wasPaused) {
            this._activeCallFrame = activeCallFrame;
            activeCallFrameDidChange = true;
        }

        if (!wasPaused)
            this.dispatchEventToListeners(WebInspector.DebuggerManager.Event.Paused);

        this.dispatchEventToListeners(WebInspector.DebuggerManager.Event.CallFramesDidChange, {target});

        if (activeCallFrameDidChange)
            this.dispatchEventToListeners(WebInspector.DebuggerManager.Event.ActiveCallFrameDidChange);
    }

    debuggerDidResume(target)
    {
        // Called from WebInspector.DebuggerObserver.

        // COMPATIBILITY (iOS 10): Debugger.resumed event was ambiguous. When stepping
        // we would receive a Debugger.resumed and we would not know if it really meant
        // the backend resumed or would pause again due to a step. Legacy backends wait
        // 50ms, and treat it as a real resume if we haven't paused in that time frame.
        // This delay ensures the user interface does not flash between brief steps
        // or successive breakpoints.
        if (!DebuggerAgent.setPauseOnAssertions) {
            this._delayedResumeTimeout = setTimeout(this._didResumeInternal.bind(this, target), 50);
            return;
        }

        this._didResumeInternal(target);
    }

    playBreakpointActionSound(breakpointActionIdentifier)
    {
        // Called from WebInspector.DebuggerObserver.

        InspectorFrontendHost.beep();
    }

    scriptDidParse(target, scriptIdentifier, url, startLine, startColumn, endLine, endColumn, isModule, isContentScript, sourceURL, sourceMapURL)
    {
        // Called from WebInspector.DebuggerObserver.

        // Don't add the script again if it is already known.
        let targetData = this.dataForTarget(target);
        let existingScript = targetData.scriptForIdentifier(scriptIdentifier);
        if (existingScript) {
            console.assert(existingScript.url === (url || null));
            console.assert(existingScript.range.startLine === startLine);
            console.assert(existingScript.range.startColumn === startColumn);
            console.assert(existingScript.range.endLine === endLine);
            console.assert(existingScript.range.endColumn === endColumn);
            return;
        }

        if (!WebInspector.isDebugUIEnabled() && isWebKitInternalScript(sourceURL))
            return;

        let range = new WebInspector.TextRange(startLine, startColumn, endLine, endColumn);
        let sourceType = isModule ? WebInspector.Script.SourceType.Module : WebInspector.Script.SourceType.Program;
        let script = new WebInspector.Script(target, scriptIdentifier, range, url, sourceType, isContentScript, sourceURL, sourceMapURL);

        targetData.addScript(script);

        if (target !== WebInspector.mainTarget && !target.mainResource) {
            // FIXME: <https://webkit.org/b/164427> Web Inspector: WorkerTarget's mainResource should be a Resource not a Script
            // We make the main resource of a WorkerTarget the Script instead of the Resource
            // because the frontend may not be informed of the Resource. We should guarantee
            // the frontend is informed of the Resource.
            if (script.url === target.name) {
                target.mainResource = script;
                if (script.resource)
                    target.resourceCollection.remove(script.resource);
            }
        }

        if (isWebKitInternalScript(script.sourceURL)) {
            this._internalWebKitScripts.push(script);
            if (!WebInspector.isDebugUIEnabled())
                return;
        }

        // Console expressions are not added to the UI by default.
        if (isWebInspectorConsoleEvaluationScript(script.sourceURL))
            return;

        this.dispatchEventToListeners(WebInspector.DebuggerManager.Event.ScriptAdded, {script});

        if (target !== WebInspector.mainTarget && !script.isMainResource() && !script.resource)
            target.addScript(script);
    }

    // Private

    _sourceCodeLocationFromPayload(target, payload)
    {
        let targetData = this.dataForTarget(target);
        let script = targetData.scriptForIdentifier(payload.scriptId);
        if (!script)
            return null;

        return script.createSourceCodeLocation(payload.lineNumber, payload.columnNumber);
    }

    _scopeChainFromPayload(target, payload)
    {
        let scopeChain = [];
        for (let i = 0; i < payload.length; ++i)
            scopeChain.push(this._scopeChainNodeFromPayload(target, payload[i]));
        return scopeChain;
    }

    _scopeChainNodeFromPayload(target, payload)
    {
        var type = null;
        switch (payload.type) {
        case DebuggerAgent.ScopeType.Global:
            type = WebInspector.ScopeChainNode.Type.Global;
            break;
        case DebuggerAgent.ScopeType.With:
            type = WebInspector.ScopeChainNode.Type.With;
            break;
        case DebuggerAgent.ScopeType.Closure:
            type = WebInspector.ScopeChainNode.Type.Closure;
            break;
        case DebuggerAgent.ScopeType.Catch:
            type = WebInspector.ScopeChainNode.Type.Catch;
            break;
        case DebuggerAgent.ScopeType.FunctionName:
            type = WebInspector.ScopeChainNode.Type.FunctionName;
            break;
        case DebuggerAgent.ScopeType.NestedLexical:
            type = WebInspector.ScopeChainNode.Type.Block;
            break;
        case DebuggerAgent.ScopeType.GlobalLexicalEnvironment:
            type = WebInspector.ScopeChainNode.Type.GlobalLexicalEnvironment;
            break;

        // COMPATIBILITY (iOS 9): Debugger.ScopeType.Local used to be provided by the backend.
        // Newer backends no longer send this enum value, it should be computed by the frontend.
        // Map this to "Closure" type. The frontend can recalculate this when needed.
        case DebuggerAgent.ScopeType.Local:
            type = WebInspector.ScopeChainNode.Type.Closure;
            break;

        default:
            console.error("Unknown type: " + payload.type);
        }

        let object = WebInspector.RemoteObject.fromPayload(payload.object, target);
        return new WebInspector.ScopeChainNode(type, [object], payload.name, payload.location, payload.empty);
    }

    _pauseReasonFromPayload(payload)
    {
        // FIXME: Handle other backend pause reasons.
        switch (payload) {
        case DebuggerAgent.PausedReason.Assert:
            return WebInspector.DebuggerManager.PauseReason.Assertion;
        case DebuggerAgent.PausedReason.Breakpoint:
            return WebInspector.DebuggerManager.PauseReason.Breakpoint;
        case DebuggerAgent.PausedReason.CSPViolation:
            return WebInspector.DebuggerManager.PauseReason.CSPViolation;
        case DebuggerAgent.PausedReason.DebuggerStatement:
            return WebInspector.DebuggerManager.PauseReason.DebuggerStatement;
        case DebuggerAgent.PausedReason.Exception:
            return WebInspector.DebuggerManager.PauseReason.Exception;
        case DebuggerAgent.PausedReason.PauseOnNextStatement:
            return WebInspector.DebuggerManager.PauseReason.PauseOnNextStatement;
        default:
            return WebInspector.DebuggerManager.PauseReason.Other;
        }
    }

    _debuggerBreakpointActionType(type)
    {
        switch (type) {
        case WebInspector.BreakpointAction.Type.Log:
            return DebuggerAgent.BreakpointActionType.Log;
        case WebInspector.BreakpointAction.Type.Evaluate:
            return DebuggerAgent.BreakpointActionType.Evaluate;
        case WebInspector.BreakpointAction.Type.Sound:
            return DebuggerAgent.BreakpointActionType.Sound;
        case WebInspector.BreakpointAction.Type.Probe:
            return DebuggerAgent.BreakpointActionType.Probe;
        default:
            console.assert(false);
            return DebuggerAgent.BreakpointActionType.Log;
        }
    }

    _debuggerBreakpointOptions(breakpoint)
    {
        const templatePlaceholderRegex = /\$\{.*?\}/;

        let options = breakpoint.options;
        let invalidActions = [];

        for (let action of options.actions) {
            if (action.type !== WebInspector.BreakpointAction.Type.Log)
                continue;

            if (!templatePlaceholderRegex.test(action.data))
                continue;

            let lexer = new WebInspector.BreakpointLogMessageLexer;
            let tokens = lexer.tokenize(action.data);
            if (!tokens) {
                invalidActions.push(action);
                continue;
            }

            let templateLiteral = tokens.reduce((text, token) => {
                if (token.type === WebInspector.BreakpointLogMessageLexer.TokenType.PlainText)
                    return text + token.data.escapeCharacters("`\\");
                if (token.type === WebInspector.BreakpointLogMessageLexer.TokenType.Expression)
                    return text + "${" + token.data + "}";
                return text;
            }, "");

            action.data = "console.log(`" + templateLiteral + "`)";
            action.type = WebInspector.BreakpointAction.Type.Evaluate;
        }

        const onlyFirst = true;
        for (let invalidAction of invalidActions)
            options.actions.remove(invalidAction, onlyFirst);

        return options;
    }

    _setBreakpoint(breakpoint, specificTarget, callback)
    {
        console.assert(!breakpoint.disabled);

        if (breakpoint.disabled)
            return;

        if (!this._restoringBreakpoints && !this.breakpointsDisabledTemporarily) {
            // Enable breakpoints since a breakpoint is being set. This eliminates
            // a multi-step process for the user that can be confusing.
            this.breakpointsEnabled = true;
        }

        function didSetBreakpoint(target, error, breakpointIdentifier, locations)
        {
            if (error)
                return;

            this._breakpointIdMap.set(breakpointIdentifier, breakpoint);

            breakpoint.identifier = breakpointIdentifier;

            // Debugger.setBreakpoint returns a single location.
            if (!(locations instanceof Array))
                locations = [locations];

            for (let location of locations)
                this.breakpointResolved(target, breakpointIdentifier, location);

            if (typeof callback === "function")
                callback();
        }

        // The breakpoint will be resolved again by calling DebuggerAgent, so mark it as unresolved.
        // If something goes wrong it will stay unresolved and show up as such in the user interface.
        // When setting for a new target, don't change the resolved target.
        if (!specificTarget)
            breakpoint.resolved = false;

        // Convert BreakpointAction types to DebuggerAgent protocol types.
        // NOTE: Breakpoint.options returns new objects each time, so it is safe to modify.
        // COMPATIBILITY (iOS 7): Debugger.BreakpointActionType did not exist yet.
        let options;
        if (DebuggerAgent.BreakpointActionType) {
            options = this._debuggerBreakpointOptions(breakpoint);
            if (options.actions.length) {
                for (let action of options.actions)
                    action.type = this._debuggerBreakpointActionType(action.type);
            }
        }

        // COMPATIBILITY (iOS 7): iOS 7 and earlier, DebuggerAgent.setBreakpoint* took a "condition" string argument.
        // This has been replaced with an "options" BreakpointOptions object.
        if (breakpoint.contentIdentifier) {
            let targets = specificTarget ? [specificTarget] : WebInspector.targets;
            for (let target of targets) {
                target.DebuggerAgent.setBreakpointByUrl.invoke({
                    lineNumber: breakpoint.sourceCodeLocation.lineNumber,
                    url: breakpoint.contentIdentifier,
                    urlRegex: undefined,
                    columnNumber: breakpoint.sourceCodeLocation.columnNumber,
                    condition: breakpoint.condition,
                    options
                }, didSetBreakpoint.bind(this, target), target.DebuggerAgent);
            }
        } else if (breakpoint.scriptIdentifier) {
            let target = breakpoint.target;
            target.DebuggerAgent.setBreakpoint.invoke({
                location: {scriptId: breakpoint.scriptIdentifier, lineNumber: breakpoint.sourceCodeLocation.lineNumber, columnNumber: breakpoint.sourceCodeLocation.columnNumber},
                condition: breakpoint.condition,
                options
            }, didSetBreakpoint.bind(this, target), target.DebuggerAgent);
        }
    }

    _removeBreakpoint(breakpoint, callback)
    {
        if (!breakpoint.identifier)
            return;

        function didRemoveBreakpoint(error)
        {
            if (error)
                console.error(error);

            this._breakpointIdMap.delete(breakpoint.identifier);

            breakpoint.identifier = null;

            // Don't reset resolved here since we want to keep disabled breakpoints looking like they
            // are resolved in the user interface. They will get marked as unresolved in reset.

            if (typeof callback === "function")
                callback();
        }

        if (breakpoint.contentIdentifier) {
            for (let target of WebInspector.targets)
                target.DebuggerAgent.removeBreakpoint(breakpoint.identifier, didRemoveBreakpoint.bind(this));
        } else if (breakpoint.scriptIdentifier) {
            let target = breakpoint.target;
            target.DebuggerAgent.removeBreakpoint(breakpoint.identifier, didRemoveBreakpoint.bind(this));
        }
    }

    _breakpointDisplayLocationDidChange(event)
    {
        if (this._ignoreBreakpointDisplayLocationDidChangeEvent)
            return;

        let breakpoint = event.target;
        if (!breakpoint.identifier || breakpoint.disabled)
            return;

        // Remove the breakpoint with its old id.
        this._removeBreakpoint(breakpoint, breakpointRemoved.bind(this));

        function breakpointRemoved()
        {
            // Add the breakpoint at its new lineNumber and get a new id.
            this._setBreakpoint(breakpoint);

            this.dispatchEventToListeners(WebInspector.DebuggerManager.Event.BreakpointMoved, {breakpoint});
        }
    }

    _breakpointDisabledStateDidChange(event)
    {
        this._saveBreakpoints();

        let breakpoint = event.target;
        if (breakpoint === this._allExceptionsBreakpoint) {
            if (!breakpoint.disabled && !this.breakpointsDisabledTemporarily)
                this.breakpointsEnabled = true;
            this._allExceptionsBreakpointEnabledSetting.value = !breakpoint.disabled;
            this._updateBreakOnExceptionsState();
            for (let target of WebInspector.targets)
                target.DebuggerAgent.setPauseOnExceptions(this._breakOnExceptionsState);
            return;
        }

        if (breakpoint === this._allUncaughtExceptionsBreakpoint) {
            if (!breakpoint.disabled && !this.breakpointsDisabledTemporarily)
                this.breakpointsEnabled = true;
            this._allUncaughtExceptionsBreakpointEnabledSetting.value = !breakpoint.disabled;
            this._updateBreakOnExceptionsState();
            for (let target of WebInspector.targets)
                target.DebuggerAgent.setPauseOnExceptions(this._breakOnExceptionsState);
            return;
        }

        if (breakpoint === this._assertionsBreakpoint) {
            if (!breakpoint.disabled && !this.breakpointsDisabledTemporarily)
                this.breakpointsEnabled = true;
            this._assertionsBreakpointEnabledSetting.value = !breakpoint.disabled;
            for (let target of WebInspector.targets)
                target.DebuggerAgent.setPauseOnAssertions(this._assertionsBreakpointEnabledSetting.value);
            return;
        }

        if (breakpoint.disabled)
            this._removeBreakpoint(breakpoint);
        else
            this._setBreakpoint(breakpoint);
    }

    _breakpointEditablePropertyDidChange(event)
    {
        this._saveBreakpoints();

        let breakpoint = event.target;
        if (breakpoint.disabled)
            return;

        console.assert(this.isBreakpointEditable(breakpoint));
        if (!this.isBreakpointEditable(breakpoint))
            return;

        // Remove the breakpoint with its old id.
        this._removeBreakpoint(breakpoint, breakpointRemoved.bind(this));

        function breakpointRemoved()
        {
            // Add the breakpoint with its new properties and get a new id.
            this._setBreakpoint(breakpoint);
        }
    }

    _startDisablingBreakpointsTemporarily()
    {
        console.assert(!this.breakpointsDisabledTemporarily, "Already temporarily disabling breakpoints.");
        if (this.breakpointsDisabledTemporarily)
            return;

        this._temporarilyDisabledBreakpointsRestoreSetting.value = this._breakpointsEnabledSetting.value;

        this.breakpointsEnabled = false;
    }

    _stopDisablingBreakpointsTemporarily()
    {
        console.assert(this.breakpointsDisabledTemporarily, "Was not temporarily disabling breakpoints.");
        if (!this.breakpointsDisabledTemporarily)
            return;

        let restoreState = this._temporarilyDisabledBreakpointsRestoreSetting.value;
        this._temporarilyDisabledBreakpointsRestoreSetting.value = null;

        this.breakpointsEnabled = restoreState;
    }

    _timelineCapturingWillStart(event)
    {
        this._startDisablingBreakpointsTemporarily();

        if (this.paused)
            this.resume();
    }

    _timelineCapturingStopped(event)
    {
        this._stopDisablingBreakpointsTemporarily();
    }

    _targetRemoved(event)
    {
        let wasPaused = this.paused;

        this._targetDebuggerDataMap.delete(event.data.target);

        if (!this.paused && wasPaused)
            this.dispatchEventToListeners(WebInspector.DebuggerManager.Event.Resumed);
    }

    _mainResourceDidChange(event)
    {
        if (!event.target.isMainFrame())
            return;

        this._didResumeInternal(WebInspector.mainTarget);
    }

    _didResumeInternal(target)
    {
        if (!this.paused)
            return;

        if (this._delayedResumeTimeout) {
            clearTimeout(this._delayedResumeTimeout);
            this._delayedResumeTimeout = undefined;
        }

        let activeCallFrameDidChange = false;
        if (this._activeCallFrame && this._activeCallFrame.target === target) {
            this._activeCallFrame = null;
            activeCallFrameDidChange = true;
        }

        this.dataForTarget(target).updateForResume();

        if (!this.paused)
            this.dispatchEventToListeners(WebInspector.DebuggerManager.Event.Resumed);

        this.dispatchEventToListeners(WebInspector.DebuggerManager.Event.CallFramesDidChange, {target});

        if (activeCallFrameDidChange)
            this.dispatchEventToListeners(WebInspector.DebuggerManager.Event.ActiveCallFrameDidChange);
    }

    _updateBreakOnExceptionsState()
    {
        let state = "none";

        if (this._breakpointsEnabledSetting.value) {
            if (!this._allExceptionsBreakpoint.disabled)
                state = "all";
            else if (!this._allUncaughtExceptionsBreakpoint.disabled)
                state = "uncaught";
        }

        this._breakOnExceptionsState = state;

        switch (state) {
        case "all":
            // Mark the uncaught breakpoint as unresolved since "all" includes "uncaught".
            // That way it is clear in the user interface that the breakpoint is ignored.
            this._allUncaughtExceptionsBreakpoint.resolved = false;
            break;
        case "uncaught":
        case "none":
            // Mark the uncaught breakpoint as resolved again.
            this._allUncaughtExceptionsBreakpoint.resolved = true;
            break;
        }
    }

    _saveBreakpoints()
    {
        if (this._restoringBreakpoints)
            return;

        let breakpointsToSave = this._breakpoints.filter((breakpoint) => !!breakpoint.contentIdentifier);
        let serializedBreakpoints = breakpointsToSave.map((breakpoint) => breakpoint.info);
        this._breakpointsSetting.value = serializedBreakpoints;
    }

    _associateBreakpointsWithSourceCode(breakpoints, sourceCode)
    {
        this._ignoreBreakpointDisplayLocationDidChangeEvent = true;

        for (let breakpoint of breakpoints) {
            if (!breakpoint.sourceCodeLocation.sourceCode)
                breakpoint.sourceCodeLocation.sourceCode = sourceCode;
            // SourceCodes can be unequal if the SourceCodeLocation is associated with a Script and we are looking at the Resource.
            console.assert(breakpoint.sourceCodeLocation.sourceCode === sourceCode || breakpoint.sourceCodeLocation.sourceCode.contentIdentifier === sourceCode.contentIdentifier);
        }

        this._ignoreBreakpointDisplayLocationDidChangeEvent = false;
    }

    _debugUIEnabledDidChange()
    {
        let eventType = WebInspector.isDebugUIEnabled() ? WebInspector.DebuggerManager.Event.ScriptAdded : WebInspector.DebuggerManager.Event.ScriptRemoved;
        for (let script of this._internalWebKitScripts)
            this.dispatchEventToListeners(eventType, {script});
    }
};

WebInspector.DebuggerManager.Event = {
    BreakpointAdded: "debugger-manager-breakpoint-added",
    BreakpointRemoved: "debugger-manager-breakpoint-removed",
    BreakpointMoved: "debugger-manager-breakpoint-moved",
    WaitingToPause: "debugger-manager-waiting-to-pause",
    Paused: "debugger-manager-paused",
    Resumed: "debugger-manager-resumed",
    CallFramesDidChange: "debugger-manager-call-frames-did-change",
    ActiveCallFrameDidChange: "debugger-manager-active-call-frame-did-change",
    ScriptAdded: "debugger-manager-script-added",
    ScriptRemoved: "debugger-manager-script-removed",
    ScriptsCleared: "debugger-manager-scripts-cleared",
    BreakpointsEnabledDidChange: "debugger-manager-breakpoints-enabled-did-change"
};

WebInspector.DebuggerManager.PauseReason = {
    Assertion: "assertion",
    Breakpoint: "breakpoint",
    CSPViolation: "CSP-violation",
    DebuggerStatement: "debugger-statement",
    Exception: "exception",
    PauseOnNextStatement: "pause-on-next-statement",
    Other: "other",
};

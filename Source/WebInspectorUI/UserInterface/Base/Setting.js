/*
 * Copyright (C) 2009 Google Inc. All rights reserved.
 * Copyright (C) 2013 Apple Inc. All rights reserved.
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

WI.Setting = class Setting extends WI.Object
{
    constructor(name, defaultValue)
    {
        super();

        this._name = name;

        let inspectionLevel = InspectorFrontendHost ? InspectorFrontendHost.inspectionLevel() : 1;
        let levelString = inspectionLevel > 1 ? "-" + inspectionLevel : "";
        this._localStorageKey = `com.apple.WebInspector${levelString}.${name}`;
        this._defaultValue = defaultValue;
    }

    // Public

    get name()
    {
        return this._name;
    }

    get value()
    {
        if ("_value" in this)
            return this._value;

        // Make a copy of the default value so changes to object values don't modify the default value.
        this._value = JSON.parse(JSON.stringify(this._defaultValue));

        if (!window.InspectorTest && window.localStorage && this._localStorageKey in window.localStorage) {
            try {
                this._value = JSON.parse(window.localStorage[this._localStorageKey]);
            } catch {
                delete window.localStorage[this._localStorageKey];
            }
        }

        return this._value;
    }

    set value(value)
    {
        if (this._value === value)
            return;

        this._value = value;

        if (!window.InspectorTest && window.localStorage) {
            try {
                // Use Object.shallowEqual to properly compare objects.
                if (Object.shallowEqual(this._value, this._defaultValue))
                    delete window.localStorage[this._localStorageKey];
                else
                    window.localStorage[this._localStorageKey] = JSON.stringify(this._value);
            } catch {
                console.error("Error saving setting with name: " + this._name);
            }
        }

        this.dispatchEventToListeners(WI.Setting.Event.Changed, this._value, {name: this._name});
    }

    reset()
    {
        // Make a copy of the default value so changes to object values don't modify the default value.
        this.value = JSON.parse(JSON.stringify(this._defaultValue));
    }
};

WI.Setting.Event = {
    Changed: "setting-changed"
};

WI.settings = {
    enableLineWrapping: new WI.Setting("enable-line-wrapping", false),
    indentUnit: new WI.Setting("indent-unit", 4),
    tabSize: new WI.Setting("tab-size", 4),
    indentWithTabs: new WI.Setting("indent-with-tabs", false),
    showWhitespaceCharacters: new WI.Setting("show-whitespace-characters", false),
    showInvalidCharacters: new WI.Setting("show-invalid-characters", false),
    clearLogOnNavigate: new WI.Setting("clear-log-on-navigate", true),
    clearNetworkOnNavigate: new WI.Setting("clear-network-on-navigate", true),
    zoomFactor: new WI.Setting("zoom-factor", 1),
    showScopeChainOnPause: new WI.Setting("show-scope-chain-sidebar", true),
    showImageGrid: new WI.Setting("show-image-grid", false),
    showCanvasPath: new WI.Setting("show-canvas-path", false),
    selectedNetworkDetailContentViewIdentifier: new WI.Setting("network-detail-content-view-identifier", "preview"),
    showRulers: new WI.Setting("show-rulers", false),

    // Experimental
    experimentalEnableLayersTab: new WI.Setting("experimental-enable-layers-tab", false),
    experimentalEnableSourcesTab: new WI.Setting("experimental-enable-sources-bar", false),
    experimentalLegacyStyleEditor: new WI.Setting("experimental-legacy-style-editor", false),
    experimentalLegacyVisualSidebar: new WI.Setting("experimental-legacy-visual-sidebar", false),
    experimentalEnableNewTabBar: new WI.Setting("experimental-enable-new-tab-bar", false),
    experimentalEnableAccessibilityAuditTab: new WI.Setting("experimental-enable-accessibility-audit-tab", false),
    experimentalRecordingHasVisualEffect: new WI.Setting("experimental-recording-has-visual-effect", false),

    // DebugUI
    autoLogProtocolMessages: new WI.Setting("auto-collect-protocol-messages", false),
    autoLogTimeStats: new WI.Setting("auto-collect-time-stats", false),
    enableUncaughtExceptionReporter: new WI.Setting("enable-uncaught-exception-reporter", true),
    enableLayoutFlashing: new WI.Setting("enable-layout-flashing", false),
    layoutDirection: new WI.Setting("layout-direction-override", "system"),
    pauseForInternalScripts: new WI.Setting("pause-for-internal-scripts", false),
};

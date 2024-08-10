/*
 * Copyright (C) 2017 Apple Inc. All rights reserved.
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

WI.RecordingAction = class RecordingAction extends WI.Object
{
    constructor(name, parameters, swizzleTypes, trace, snapshot)
    {
        super();

        this._payloadName = name;
        this._payloadParameters = parameters;
        this._payloadSwizzleTypes = swizzleTypes;
        this._payloadTrace = trace;
        this._payloadSnapshot = snapshot || -1;

        this._name = "";
        this._parameters = [];
        this._trace = [];
        this._snapshot = "";

        this._valid = true;
        this._isFunction = false;
        this._isGetter = false;
        this._isVisual = false;
        this._hasVisibleEffect = undefined;

        this._state = null;
        this._stateModifiers = new Set;
    }

    // Static

    // Payload format: [name, parameters, swizzleTypes, trace, [snapshot]]
    static fromPayload(payload)
    {
        if (!Array.isArray(payload))
            payload = [];

        if (isNaN(payload[0]))
            payload[0] = -1;

        if (!Array.isArray(payload[1]))
            payload[1] = [];

        if (!Array.isArray(payload[2]))
            payload[2] = [];

        if (!Array.isArray(payload[3]))
            payload[3] = [];

        if (payload.length >= 5 && isNaN(payload[4]))
            payload[4] = -1;

        return new WI.RecordingAction(...payload);
    }

    static isFunctionForType(type, name)
    {
        let prototype = WI.RecordingAction._prototypeForType(type);
        if (!prototype)
            return false;
        return typeof Object.getOwnPropertyDescriptor(prototype, name).value === "function";
    }

    static _prototypeForType(type)
    {
        if (type === WI.Recording.Type.Canvas2D)
            return CanvasRenderingContext2D.prototype;
        if (type === WI.Recording.Type.CanvasWebGL)
            return WebGLRenderingContext.prototype;
        return null;
    }

    // Public

    get name() { return this._name; }
    get parameters() { return this._parameters; }
    get swizzleTypes() { return this._payloadSwizzleTypes; }
    get trace() { return this._trace; }
    get snapshot() { return this._snapshot; }
    get valid() { return this._valid; }
    get isFunction() { return this._isFunction; }
    get isGetter() { return this._isGetter; }
    get isVisual() { return this._isVisual; }
    get hasVisibleEffect() { return this._hasVisibleEffect; }
    get state() { return this._state; }
    get stateModifiers() { return this._stateModifiers; }

    process(recording, context)
    {
        if (recording.type === WI.Recording.Type.CanvasWebGL) {
            // We add each RecordingAction to the list of visualActionIndexes after it is processed.
            if (this._valid && this._isVisual) {
                let contentBefore = recording.visualActionIndexes.length ? recording.visualActionIndexes.lastValue.snapshot : recording.initialState.content;
                this._hasVisibleEffect = this._snapshot !== contentBefore;
            }
            return;
        }

        function getContent() {
            if (context instanceof CanvasRenderingContext2D) {
                let imageData = context.getImageData(0, 0, context.canvas.width, context.canvas.height);
                return [imageData.width, imageData.height, ...imageData.data];
            }

            if (context instanceof WebGLRenderingContext || context instanceof WebGL2RenderingContext) {
                let pixels = new Uint8Array(context.drawingBufferWidth * context.drawingBufferHeight * 4);
                context.readPixels(0, 0, context.canvas.width, context.canvas.height, context.RGBA, context.UNSIGNED_BYTE, pixels);
                return [...pixels];
            }

            if (context.canvas instanceof HTMLCanvasElement)
                return [context.canvas.toDataURL()];

            console.assert("Unknown context type", context);
            return [];
        }

        let contentBefore = null;
        let shouldCheckHasVisualEffect = WI.settings.experimentalRecordingHasVisualEffect.value && this._valid && this._isVisual;
        if (shouldCheckHasVisualEffect)
            contentBefore = getContent();

        this.apply(context);

        if (shouldCheckHasVisualEffect)
            this._hasVisibleEffect = !Array.shallowEqual(contentBefore, getContent());

        if (recording.type === WI.Recording.Type.Canvas2D) {
            let matrix = context.getTransform();

            this._state = {
                currentX: context.currentX,
                currentY: context.currentY,
                direction: context.direction,
                fillStyle: context.fillStyle,
                font: context.font,
                globalAlpha: context.globalAlpha,
                globalCompositeOperation: context.globalCompositeOperation,
                imageSmoothingEnabled: context.imageSmoothingEnabled,
                imageSmoothingQuality: context.imageSmoothingQuality,
                lineCap: context.lineCap,
                lineDash: context.getLineDash(),
                lineDashOffset: context.lineDashOffset,
                lineJoin: context.lineJoin,
                lineWidth: context.lineWidth,
                miterLimit: context.miterLimit,
                shadowBlur: context.shadowBlur,
                shadowColor: context.shadowColor,
                shadowOffsetX: context.shadowOffsetX,
                shadowOffsetY: context.shadowOffsetY,
                strokeStyle: context.strokeStyle,
                textAlign: context.textAlign,
                textBaseline: context.textBaseline,
                transform: [matrix.a, matrix.b, matrix.c, matrix.d, matrix.e, matrix.f],
                webkitImageSmoothingEnabled: context.webkitImageSmoothingEnabled,
                webkitLineDash: context.webkitLineDash,
                webkitLineDashOffset: context.webkitLineDashOffset,
            };

            if (WI.ImageUtilities.supportsCanvasPathDebugging())
                this._state.setPath = [context.getPath()];
        }
    }

    async swizzle(recording)
    {
        if (!this._valid)
            return;

        let swizzleParameter = (item, index) => {
            return recording.swizzle(item, this._payloadSwizzleTypes[index]);
        };

        let swizzleCallFrame = async (item, index) => {
            let array = await recording.swizzle(item, WI.Recording.Swizzle.None);
            let [functionName, url] = await Promise.all([
                recording.swizzle(array[0], WI.Recording.Swizzle.String),
                recording.swizzle(array[1], WI.Recording.Swizzle.String),
            ]);
            return WI.CallFrame.fromPayload(WI.mainTarget, {
                functionName,
                url,
                lineNumber: array[2],
                columnNumber: array[3],
            });
        };

        let swizzlePromises = [
            recording.swizzle(this._payloadName, WI.Recording.Swizzle.String),
            Promise.all(this._payloadParameters.map(swizzleParameter)),
            Promise.all(this._payloadTrace.map(swizzleCallFrame)),
        ];
        if (this._payloadSnapshot >= 0)
            swizzlePromises.push(recording.swizzle(this._payloadSnapshot, WI.Recording.Swizzle.String));

        let [name, parameters, callFrames, snapshot] = await Promise.all(swizzlePromises);
        this._name = name;
        this._parameters = parameters;
        this._trace = callFrames;
        if (this._payloadSnapshot >= 0)
            this._snapshot = snapshot;

        this._isFunction = WI.RecordingAction.isFunctionForType(recording.type, this._name);
        this._isGetter = !this._isFunction && !this._parameters.length;

        let visualNames = WI.RecordingAction._visualNames[recording.type];
        this._isVisual = visualNames ? visualNames.has(this._name) : false;

        if (this._valid) {
            let prototype = WI.RecordingAction._prototypeForType(recording.type);
            if (prototype && !(name in prototype)) {
                this.markInvalid();

                WI.Recording.synthesizeError(WI.UIString("“%s” is invalid.").format(name));
            }
        }

        if (this._valid) {
            let parametersSpecified = this._parameters.every((parameter) => parameter !== undefined);
            let parametersCanBeSwizzled = this._payloadSwizzleTypes.every((swizzleType) => swizzleType !== WI.Recording.Swizzle.None);
            if (!parametersSpecified || !parametersCanBeSwizzled)
                this.markInvalid();
        }

        if (this._valid) {
            let stateModifiers = WI.RecordingAction._stateModifiers[recording.type];
            if (stateModifiers) {
                this._stateModifiers.add(this._name);
                let modifiedByAction = stateModifiers[this._name] || [];
                for (let item of modifiedByAction)
                    this._stateModifiers.add(item);
            }
        }
    }

    apply(context, options = {})
    {
        if (!this.valid)
            return;

        try {
            let name = options.nameOverride || this._name;
            if (this.isFunction)
                context[name](...this._parameters);
            else {
                if (this.isGetter)
                    context[name];
                else
                    context[name] = this._parameters[0];
            }
        } catch {
            this.markInvalid();

            WI.Recording.synthesizeError(WI.UIString("“%s” threw an error.").format(this._name));
        }
    }

    markInvalid()
    {
        if (!this._valid)
            return;

        this._valid = false;

        this.dispatchEventToListeners(WI.RecordingAction.Event.ValidityChanged);
    }

    getColorParameters()
    {
        switch (this._name) {
        // 2D
        case "fillStyle":
        case "strokeStyle":
        case "shadowColor":
        // 2D (non-standard, legacy)
        case "setFillColor":
        case "setStrokeColor":
        // WebGL
        case "blendColor":
        case "clearColor":
            return this._parameters;

        // 2D (non-standard, legacy)
        case "setShadow":
            return this._parameters.slice(3);
        }

        return [];
    }

    getImageParameters()
    {
        switch (this._name) {
        // 2D
        case "createImageData":
        case "createPattern":
        case "drawImage":
        case "fillStyle":
        case "putImageData":
        case "strokeStyle":
        // 2D (non-standard)
        case "drawImageFromRect":
            return this._parameters.slice(0, 1);
        }

        return [];
    }

    toJSON()
    {
        let json = [this._payloadName, this._payloadParameters, this._payloadSwizzleTypes, this._payloadTrace];
        if (this._payloadSnapshot >= 0)
            json.push(this._payloadSnapshot);
        return json;
    }
};

WI.RecordingAction.Event = {
    ValidityChanged: "recording-action-marked-invalid",
    HasVisibleEffectChanged: "recording-action-has-visible-effect-changed",
};

WI.RecordingAction._visualNames = {
    [WI.Recording.Type.Canvas2D]: new Set([
        "clearRect",
        "drawFocusIfNeeded",
        "drawImage",
        "drawImageFromRect",
        "fill",
        "fillRect",
        "fillText",
        "putImageData",
        "stroke",
        "strokeRect",
        "strokeText",
    ]),
    [WI.Recording.Type.CanvasWebGL]: new Set([
        "clear",
        "drawArrays",
        "drawElements",
    ]),
};

WI.RecordingAction._stateModifiers = {
    [WI.Recording.Type.Canvas2D]: {
        arc: ["currentX", "currentY"],
        arcTo: ["currentX", "currentY"],
        beginPath: ["currentX", "currentY"],
        bezierCurveTo: ["currentX", "currentY"],
        clearShadow: ["shadowOffsetX", "shadowOffsetY", "shadowBlur", "shadowColor"],
        closePath: ["currentX", "currentY"],
        ellipse: ["currentX", "currentY"],
        lineTo: ["currentX", "currentY"],
        moveTo: ["currentX", "currentY"],
        quadraticCurveTo: ["currentX", "currentY"],
        rect: ["currentX", "currentY"],
        resetTransform: ["transform"],
        rotate: ["transform"],
        scale: ["transform"],
        setAlpha: ["globalAlpha"],
        setCompositeOperation: ["globalCompositeOperation"],
        setFillColor: ["fillStyle"],
        setLineCap: ["lineCap"],
        setLineJoin: ["lineJoin"],
        setLineWidth: ["lineWidth"],
        setMiterLimit: ["miterLimit"],
        setShadow: ["shadowOffsetX", "shadowOffsetY", "shadowBlur", "shadowColor"],
        setStrokeColor: ["strokeStyle"],
        setTransform: ["transform"],
        translate: ["transform"],
    },
};

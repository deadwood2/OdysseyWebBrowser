/*
 * Copyright (C) 2013, 2015 Apple Inc. All rights reserved.
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

WI.Popover = class Popover extends WI.Object
{
    constructor(delegate)
    {
        super();

        this.delegate = delegate;
        this._edge = null;
        this._frame = new WI.Rect;
        this._content = null;
        this._targetFrame = new WI.Rect;
        this._anchorPoint = new WI.Point;
        this._preferredEdges = null;
        this._resizeHandler = null;

        this._contentNeedsUpdate = false;
        this._dismissing = false;

        this._element = document.createElement("div");
        this._element.className = "popover";
        this._element.addEventListener("transitionend", this, true);

        this._container = this._element.appendChild(document.createElement("div"));
        this._container.className = "container";
    }

    // Public

    get element() { return this._element; }

    get visible()
    {
        return this._element.parentNode === document.body && !this._element.classList.contains(WI.Popover.FadeOutClassName);
    }

    get frame()
    {
        return this._frame;
    }

    set frame(frame)
    {
        this._element.style.left = frame.minX() + "px";
        this._element.style.top = frame.minY() + "px";
        this._element.style.width = frame.size.width + "px";
        this._element.style.height = frame.size.height + "px";
        this._element.style.backgroundSize = frame.size.width + "px " + frame.size.height + "px";
        this._frame = frame;
    }

    set content(content)
    {
        if (content === this._content)
            return;

        this._content = content;

        this._contentNeedsUpdate = true;

        if (this.visible)
            this._update(true);
    }

    set windowResizeHandler(resizeHandler)
    {
        console.assert(typeof resizeHandler === "function");
        this._resizeHandler = resizeHandler;
    }

    update(shouldAnimate = true)
    {
        if (!this.visible)
            return;

        var previouslyFocusedElement = document.activeElement;

        this._contentNeedsUpdate = true;
        this._update(shouldAnimate);

        if (previouslyFocusedElement)
            previouslyFocusedElement.focus();
    }

    present(targetFrame, preferredEdges)
    {
        this._targetFrame = targetFrame;
        this._preferredEdges = preferredEdges;

        if (!this._content)
            return;

        this._addListenersIfNeeded();

        this._update();
    }

    presentNewContentWithFrame(content, targetFrame, preferredEdges)
    {
        this._content = content;
        this._contentNeedsUpdate = true;

        this._targetFrame = targetFrame;
        this._preferredEdges = preferredEdges;

        this._addListenersIfNeeded();

        var shouldAnimate = this.visible;
        this._update(shouldAnimate);
    }

    dismiss()
    {
        if (this._dismissing || this._element.parentNode !== document.body)
            return;

        this._dismissing = true;

        console.assert(this._isListeningForPopoverEvents);
        this._isListeningForPopoverEvents = false;

        window.removeEventListener("mousedown", this, true);
        window.removeEventListener("scroll", this, true);
        window.removeEventListener("resize", this, true);
        window.removeEventListener("keypress", this, true);

        WI.quickConsole.keyboardShortcutDisabled = false;

        this._element.classList.add(WI.Popover.FadeOutClassName);

        if (this.delegate && typeof this.delegate.willDismissPopover === "function")
            this.delegate.willDismissPopover(this);
    }

    handleEvent(event)
    {
        switch (event.type) {
        case "mousedown":
        case "scroll":
            if (!this._element.contains(event.target) && !event.target.enclosingNodeOrSelfWithClass(WI.Popover.IgnoreAutoDismissClassName)
                && !event[WI.Popover.EventPreventDismissSymbol]) {
                this.dismiss();
            }
            break;
        case "resize":
            if (this._resizeHandler)
                this._resizeHandler();
            break;
        case "keypress":
            if (event.keyCode === WI.KeyboardShortcut.Key.Escape.keyCode)
                this.dismiss();
            break;
        case "transitionend":
            if (event.target === this._element) {
                document.body.removeChild(this._element);
                this._element.classList.remove(WI.Popover.FadeOutClassName);
                this._container.textContent = "";
                if (this.delegate && typeof this.delegate.didDismissPopover === "function")
                    this.delegate.didDismissPopover(this);

                this._dismissing = false;
                break;
            }
            break;
        }
    }

    // Private

    _update(shouldAnimate)
    {
        if (shouldAnimate)
            var previousEdge = this._edge;

        var targetFrame = this._targetFrame;
        var preferredEdges = this._preferredEdges;

        // Ensure our element is on display so that its metrics can be resolved
        // or interrupt any pending transition to remove it from display.
        if (this._element.parentNode !== document.body)
            document.body.appendChild(this._element);
        else
            this._element.classList.remove(WI.Popover.FadeOutClassName);

        this._dismissing = false;

        if (this._edge !== null)
            this._element.classList.remove(this._cssClassNameForEdge());

        if (this._contentNeedsUpdate) {
            // Reset CSS properties on element so that the element may be sized to fit its content.
            this._element.style.removeProperty("left");
            this._element.style.removeProperty("top");
            this._element.style.removeProperty("width");
            this._element.style.removeProperty("height");

            // Add the content in place of the wrapper to get the raw metrics.
            this._container.replaceWith(this._content);

            // Get the ideal size for the popover to fit its content.
            var popoverBounds = this._element.getBoundingClientRect();
            this._preferredSize = new WI.Size(Math.ceil(popoverBounds.width), Math.ceil(popoverBounds.height));
        }

        var titleBarOffset = WI.Platform.name === "mac" && WI.Platform.version.release >= 10 ? 22 : 0;
        var containerFrame = new WI.Rect(0, titleBarOffset, window.innerWidth, window.innerHeight - titleBarOffset);
        // The frame of the window with a little inset to make sure we have room for shadows.
        containerFrame = containerFrame.inset(WI.Popover.ShadowEdgeInsets);

        // Work out the metrics for all edges.
        var metrics = new Array(preferredEdges.length);
        for (var edgeName in WI.RectEdge) {
            var edge = WI.RectEdge[edgeName];
            var item = {
                edge,
                metrics: this._bestMetricsForEdge(this._preferredSize, targetFrame, containerFrame, edge)
            };
            var preferredIndex = preferredEdges.indexOf(edge);
            if (preferredIndex !== -1)
                metrics[preferredIndex] = item;
            else
                metrics.push(item);
        }

        function area(size)
        {
            return size.width * size.height;
        }

        // Find if any of those fit better than the frame for the preferred edge.
        var bestEdge = metrics[0].edge;
        var bestMetrics = metrics[0].metrics;
        for (var i = 1; i < metrics.length; i++) {
            var itemMetrics = metrics[i].metrics;
            if (area(itemMetrics.contentSize) > area(bestMetrics.contentSize)) {
                bestEdge = metrics[i].edge;
                bestMetrics = itemMetrics;
            }
        }

        var anchorPoint;
        var bestFrame = bestMetrics.frame.round();

        this._edge = bestEdge;

        if (bestFrame === WI.Rect.ZERO_RECT) {
            // The target for the popover is offscreen.
            this.dismiss();
        } else {
            switch (bestEdge) {
            case WI.RectEdge.MIN_X: // Displayed on the left of the target, arrow points right.
                anchorPoint = new WI.Point(bestFrame.size.width - WI.Popover.ShadowPadding, targetFrame.midY() - bestFrame.minY());
                break;
            case WI.RectEdge.MAX_X: // Displayed on the right of the target, arrow points left.
                anchorPoint = new WI.Point(WI.Popover.ShadowPadding, targetFrame.midY() - bestFrame.minY());
                break;
            case WI.RectEdge.MIN_Y: // Displayed above the target, arrow points down.
                anchorPoint = new WI.Point(targetFrame.midX() - bestFrame.minX(), bestFrame.size.height - WI.Popover.ShadowPadding);
                break;
            case WI.RectEdge.MAX_Y: // Displayed below the target, arrow points up.
                anchorPoint = new WI.Point(targetFrame.midX() - bestFrame.minX(), WI.Popover.ShadowPadding);
                break;
            }

            this._element.classList.add(this._cssClassNameForEdge());

            if (shouldAnimate && this._edge === previousEdge)
                this._animateFrame(bestFrame, anchorPoint);
            else {
                 this.frame = bestFrame;
                 this._setAnchorPoint(anchorPoint);
                 this._drawBackground();
            }

            // Make sure content is centered in case either of the dimension is smaller than the minimal bounds.
            if (this._preferredSize.width < WI.Popover.MinWidth || this._preferredSize.height < WI.Popover.MinHeight)
                this._container.classList.add("center");
            else
                this._container.classList.remove("center");
        }

        // Wrap the content in the container so that it's located correctly.
        if (this._contentNeedsUpdate) {
            this._container.textContent = "";
            this._content.replaceWith(this._container);
            this._container.appendChild(this._content);
        }

        this._contentNeedsUpdate = false;
    }

    _cssClassNameForEdge()
    {
        switch (this._edge) {
        case WI.RectEdge.MIN_X: // Displayed on the left of the target, arrow points right.
            return "arrow-right";
        case WI.RectEdge.MAX_X: // Displayed on the right of the target, arrow points left.
            return "arrow-left";
        case WI.RectEdge.MIN_Y: // Displayed above the target, arrow points down.
            return "arrow-down";
        case WI.RectEdge.MAX_Y: // Displayed below the target, arrow points up.
            return "arrow-up";
        }
        console.error("Unknown edge.");
        return "arrow-up";
    }

    _setAnchorPoint(anchorPoint)
    {
        anchorPoint.x = Math.floor(anchorPoint.x);
        anchorPoint.y = Math.floor(anchorPoint.y);
        this._anchorPoint = anchorPoint;
    }

    _animateFrame(toFrame, toAnchor)
    {
        var startTime = Date.now();
        var duration = 350;
        var epsilon = 1 / (200 * duration);
        var spline = new WI.CubicBezier(0.25, 0.1, 0.25, 1);

        var fromFrame = this._frame.copy();
        var fromAnchor = this._anchorPoint.copy();

        function animatedValue(from, to, progress)
        {
            return from + (to - from) * progress;
        }

        function drawBackground()
        {
            var progress = spline.solve(Math.min((Date.now() - startTime) / duration, 1), epsilon);

            this.frame = new WI.Rect(
                animatedValue(fromFrame.minX(), toFrame.minX(), progress),
                animatedValue(fromFrame.minY(), toFrame.minY(), progress),
                animatedValue(fromFrame.size.width, toFrame.size.width, progress),
                animatedValue(fromFrame.size.height, toFrame.size.height, progress)
            ).round();

            this._setAnchorPoint(new WI.Point(
                animatedValue(fromAnchor.x, toAnchor.x, progress),
                animatedValue(fromAnchor.y, toAnchor.y, progress)
            ));

            this._drawBackground();

            if (progress < 1)
                requestAnimationFrame(drawBackground.bind(this));
        }

        drawBackground.call(this);
    }

    _drawBackground()
    {
        let scaleFactor = window.devicePixelRatio;

        let width = this._frame.size.width;
        let height = this._frame.size.height;
        let scaledWidth = width * scaleFactor;
        let scaledHeight = height * scaleFactor;

        // Create a scratch canvas so we can draw the popover that will later be drawn into
        // the final context with a shadow.
        let scratchCanvas = document.createElement("canvas");
        scratchCanvas.width = scaledWidth;
        scratchCanvas.height = scaledHeight;

        let ctx = scratchCanvas.getContext("2d");
        ctx.scale(scaleFactor, scaleFactor);

        // Bounds of the path don't take into account the arrow, but really only the tight bounding box
        // of the content contained within the frame.
        let bounds;
        let arrowHeight = WI.Popover.AnchorSize.height;
        switch (this._edge) {
        case WI.RectEdge.MIN_X: // Displayed on the left of the target, arrow points right.
            bounds = new WI.Rect(0, 0, width - arrowHeight, height);
            break;
        case WI.RectEdge.MAX_X: // Displayed on the right of the target, arrow points left.
            bounds = new WI.Rect(arrowHeight, 0, width - arrowHeight, height);
            break;
        case WI.RectEdge.MIN_Y: // Displayed above the target, arrow points down.
            bounds = new WI.Rect(0, 0, width, height - arrowHeight);
            break;
        case WI.RectEdge.MAX_Y: // Displayed below the target, arrow points up.
            bounds = new WI.Rect(0, arrowHeight, width, height - arrowHeight);
            break;
        }

        bounds = bounds.inset(WI.Popover.ShadowEdgeInsets);
        let computedStyle = window.getComputedStyle(this._element, null);

        // Clip the frame.
        ctx.fillStyle = computedStyle.getPropertyValue("--popover-text-color").trim();
        this._drawFrame(ctx, bounds, this._edge, this._anchorPoint);
        ctx.clip();

        // Panel background color fill.
        ctx.fillStyle = computedStyle.getPropertyValue("--popover-background-color").trim();

        ctx.fillRect(0, 0, width, height);

        // Stroke.
        ctx.strokeStyle = computedStyle.getPropertyValue("--popover-border-color").trim();
        ctx.lineWidth = 2;
        this._drawFrame(ctx, bounds, this._edge, this._anchorPoint);
        ctx.stroke();

        // Draw the popover into the final context with a drop shadow.
        let finalContext = document.getCSSCanvasContext("2d", "popover", scaledWidth, scaledHeight);
        finalContext.clearRect(0, 0, scaledWidth, scaledHeight);
        finalContext.shadowOffsetX = 1;
        finalContext.shadowOffsetY = 1;
        finalContext.shadowBlur = 5;
        finalContext.shadowColor = computedStyle.getPropertyValue("--popover-shadow-color").trim();
        finalContext.drawImage(scratchCanvas, 0, 0, scaledWidth, scaledHeight);
    }

    _bestMetricsForEdge(preferredSize, targetFrame, containerFrame, edge)
    {
        var x, y;
        var width = preferredSize.width + (WI.Popover.ShadowPadding * 2) + (WI.Popover.ContentPadding * 2);
        var height = preferredSize.height + (WI.Popover.ShadowPadding * 2) + (WI.Popover.ContentPadding * 2);
        var arrowLength = WI.Popover.AnchorSize.height;

        switch (edge) {
        case WI.RectEdge.MIN_X: // Displayed on the left of the target, arrow points right.
            width += arrowLength;
            x = targetFrame.origin.x - width + WI.Popover.ShadowPadding;
            y = targetFrame.origin.y - (height - targetFrame.size.height) / 2;
            break;
        case WI.RectEdge.MAX_X: // Displayed on the right of the target, arrow points left.
            width += arrowLength;
            x = targetFrame.origin.x + targetFrame.size.width - WI.Popover.ShadowPadding;
            y = targetFrame.origin.y - (height - targetFrame.size.height) / 2;
            break;
        case WI.RectEdge.MIN_Y: // Displayed above the target, arrow points down.
            height += arrowLength;
            x = targetFrame.origin.x - (width - targetFrame.size.width) / 2;
            y = targetFrame.origin.y - height + WI.Popover.ShadowPadding;
            break;
        case WI.RectEdge.MAX_Y: // Displayed below the target, arrow points up.
            height += arrowLength;
            x = targetFrame.origin.x - (width - targetFrame.size.width) / 2;
            y = targetFrame.origin.y + targetFrame.size.height - WI.Popover.ShadowPadding;
            break;
        }

        if (edge === WI.RectEdge.MIN_X || edge === WI.RectEdge.MAX_X) {
            if (y < containerFrame.minY())
                y = containerFrame.minY();
            if (y + height > containerFrame.maxY())
                y = containerFrame.maxY() - height;
        } else {
            if (x < containerFrame.minX())
                x = containerFrame.minX();
            if (x + width > containerFrame.maxX())
                x = containerFrame.maxX() - width;
        }

        var preferredFrame = new WI.Rect(x, y, width, height);
        var bestFrame = preferredFrame.intersectionWithRect(containerFrame);

        width = bestFrame.size.width - (WI.Popover.ShadowPadding * 2);
        height = bestFrame.size.height - (WI.Popover.ShadowPadding * 2);

        switch (edge) {
        case WI.RectEdge.MIN_X: // Displayed on the left of the target, arrow points right.
        case WI.RectEdge.MAX_X: // Displayed on the right of the target, arrow points left.
            width -= arrowLength;
            break;
        case WI.RectEdge.MIN_Y: // Displayed above the target, arrow points down.
        case WI.RectEdge.MAX_Y: // Displayed below the target, arrow points up.
            height -= arrowLength;
            break;
        }

        return {
            frame: bestFrame,
            contentSize: new WI.Size(width, height)
        };
    }

    _drawFrame(ctx, bounds, anchorEdge)
    {
        let cornerRadius = WI.Popover.CornerRadius;
        let arrowHalfLength = WI.Popover.AnchorSize.width * 0.5;
        let anchorPoint = this._anchorPoint;

        // Prevent the arrow from being positioned against one of the popover's rounded corners.
        let arrowPadding = cornerRadius + arrowHalfLength;
        if (anchorEdge === WI.RectEdge.MIN_Y || anchorEdge === WI.RectEdge.MAX_Y)
            anchorPoint.x = Number.constrain(anchorPoint.x, bounds.minX() + arrowPadding, bounds.maxX() - arrowPadding);
        else
            anchorPoint.y = Number.constrain(anchorPoint.y, bounds.minY() + arrowPadding, bounds.maxY() - arrowPadding);

        ctx.beginPath();
        switch (anchorEdge) {
        case WI.RectEdge.MIN_X: // Displayed on the left of the target, arrow points right.
            ctx.moveTo(bounds.maxX(), bounds.minY() + cornerRadius);
            ctx.lineTo(bounds.maxX(), anchorPoint.y - arrowHalfLength);
            ctx.lineTo(anchorPoint.x, anchorPoint.y);
            ctx.lineTo(bounds.maxX(), anchorPoint.y + arrowHalfLength);
            ctx.arcTo(bounds.maxX(), bounds.maxY(), bounds.minX(), bounds.maxY(), cornerRadius);
            ctx.arcTo(bounds.minX(), bounds.maxY(), bounds.minX(), bounds.minY(), cornerRadius);
            ctx.arcTo(bounds.minX(), bounds.minY(), bounds.maxX(), bounds.minY(), cornerRadius);
            ctx.arcTo(bounds.maxX(), bounds.minY(), bounds.maxX(), bounds.maxY(), cornerRadius);
            break;
        case WI.RectEdge.MAX_X: // Displayed on the right of the target, arrow points left.
            ctx.moveTo(bounds.minX(), bounds.maxY() - cornerRadius);
            ctx.lineTo(bounds.minX(), anchorPoint.y + arrowHalfLength);
            ctx.lineTo(anchorPoint.x, anchorPoint.y);
            ctx.lineTo(bounds.minX(), anchorPoint.y - arrowHalfLength);
            ctx.arcTo(bounds.minX(), bounds.minY(), bounds.maxX(), bounds.minY(), cornerRadius);
            ctx.arcTo(bounds.maxX(), bounds.minY(), bounds.maxX(), bounds.maxY(), cornerRadius);
            ctx.arcTo(bounds.maxX(), bounds.maxY(), bounds.minX(), bounds.maxY(), cornerRadius);
            ctx.arcTo(bounds.minX(), bounds.maxY(), bounds.minX(), bounds.minY(), cornerRadius);
            break;
        case WI.RectEdge.MIN_Y: // Displayed above the target, arrow points down.
            ctx.moveTo(bounds.maxX() - cornerRadius, bounds.maxY());
            ctx.lineTo(anchorPoint.x + arrowHalfLength, bounds.maxY());
            ctx.lineTo(anchorPoint.x, anchorPoint.y);
            ctx.lineTo(anchorPoint.x - arrowHalfLength, bounds.maxY());
            ctx.arcTo(bounds.minX(), bounds.maxY(), bounds.minX(), bounds.minY(), cornerRadius);
            ctx.arcTo(bounds.minX(), bounds.minY(), bounds.maxX(), bounds.minY(), cornerRadius);
            ctx.arcTo(bounds.maxX(), bounds.minY(), bounds.maxX(), bounds.maxY(), cornerRadius);
            ctx.arcTo(bounds.maxX(), bounds.maxY(), bounds.minX(), bounds.maxY(), cornerRadius);
            break;
        case WI.RectEdge.MAX_Y: // Displayed below the target, arrow points up.
            ctx.moveTo(bounds.minX() + cornerRadius, bounds.minY());
            ctx.lineTo(anchorPoint.x - arrowHalfLength, bounds.minY());
            ctx.lineTo(anchorPoint.x, anchorPoint.y);
            ctx.lineTo(anchorPoint.x + arrowHalfLength, bounds.minY());
            ctx.arcTo(bounds.maxX(), bounds.minY(), bounds.maxX(), bounds.maxY(), cornerRadius);
            ctx.arcTo(bounds.maxX(), bounds.maxY(), bounds.minX(), bounds.maxY(), cornerRadius);
            ctx.arcTo(bounds.minX(), bounds.maxY(), bounds.minX(), bounds.minY(), cornerRadius);
            ctx.arcTo(bounds.minX(), bounds.minY(), bounds.maxX(), bounds.minY(), cornerRadius);
            break;
        }
        ctx.closePath();
    }

    _addListenersIfNeeded()
    {
        if (!this._isListeningForPopoverEvents) {
            this._isListeningForPopoverEvents = true;

            window.addEventListener("mousedown", this, true);
            window.addEventListener("scroll", this, true);
            window.addEventListener("resize", this, true);
            window.addEventListener("keypress", this, true);

            WI.quickConsole.keyboardShortcutDisabled = true;
        }
    }
};

WI.Popover.FadeOutClassName = "fade-out";
WI.Popover.CornerRadius = 5;
WI.Popover.MinWidth = 40;
WI.Popover.MinHeight = 40;
WI.Popover.ShadowPadding = 5;
WI.Popover.ContentPadding = 5;
WI.Popover.AnchorSize = new WI.Size(22, 11);
WI.Popover.ShadowEdgeInsets = new WI.EdgeInsets(WI.Popover.ShadowPadding);
WI.Popover.IgnoreAutoDismissClassName = "popover-ignore-auto-dismiss";
WI.Popover.EventPreventDismissSymbol = Symbol("popover-event-prevent-dismiss");

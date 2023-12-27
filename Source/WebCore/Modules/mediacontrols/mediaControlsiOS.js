function createControls(root, video, host)
{
    return new ControllerIOS(root, video, host);
};

function ControllerIOS(root, video, host)
{
    this.doingSetup = true;
    this._pageScaleFactor = 1;

    this.timelineContextName = "_webkit-media-controls-timeline-" + host.generateUUID();

    Controller.call(this, root, video, host);

    this.setNeedsTimelineMetricsUpdate();

    this._timelineIsHidden = false;
    this._currentDisplayWidth = 0;
    this.scheduleUpdateLayoutForDisplayedWidth();

    host.controlsDependOnPageScaleFactor = true;
    this.doingSetup = false;
};

/* Constants */
ControllerIOS.MinimumTimelineWidth = 200;
ControllerIOS.ButtonWidth = 42;

/* Enums */
ControllerIOS.StartPlaybackControls = 2;


ControllerIOS.prototype = {
    addVideoListeners: function() {
        Controller.prototype.addVideoListeners.call(this);

        this.listenFor(this.video, 'webkitbeginfullscreen', this.handleFullscreenChange);
        this.listenFor(this.video, 'webkitendfullscreen', this.handleFullscreenChange);
        this.listenFor(this.video, 'webkitpresentationmodechanged', this.handlePresentationModeChange);
    },

    removeVideoListeners: function() {
        Controller.prototype.removeVideoListeners.call(this);

        this.stopListeningFor(this.video, 'webkitbeginfullscreen', this.handleFullscreenChange);
        this.stopListeningFor(this.video, 'webkitendfullscreen', this.handleFullscreenChange);
        this.stopListeningFor(this.video, 'webkitpresentationmodechanged', this.handlePresentationModeChange);
    },

    createBase: function() {
        Controller.prototype.createBase.call(this);

        var startPlaybackButton = this.controls.startPlaybackButton = document.createElement('button');
        startPlaybackButton.setAttribute('pseudo', '-webkit-media-controls-start-playback-button');
        startPlaybackButton.setAttribute('aria-label', this.UIString('Start Playback'));

        this.listenFor(this.base, 'gesturestart', this.handleBaseGestureStart);
        this.listenFor(this.base, 'gesturechange', this.handleBaseGestureChange);
        this.listenFor(this.base, 'gestureend', this.handleBaseGestureEnd);
        this.listenFor(this.base, 'touchstart', this.handleWrapperTouchStart);
        this.stopListeningFor(this.base, 'mousemove', this.handleWrapperMouseMove);
        this.stopListeningFor(this.base, 'mouseout', this.handleWrapperMouseOut);

        this.listenFor(document, 'visibilitychange', this.handleVisibilityChange);
    },

    shouldHaveStartPlaybackButton: function() {
        var allowsInline = this.host.mediaPlaybackAllowsInline;

        if (this.isPlaying || (this.hasPlayed && allowsInline))
            return false;

        if (this.isAudio() && allowsInline)
            return false;

        if (this.doingSetup)
            return true;

        if (this.isFullScreen())
            return false;

        if (!this.video.currentSrc && this.video.error)
            return false;

        if (!this.video.controls && allowsInline)
            return false;

        if (this.video.currentSrc && this.video.error)
            return true;

        return true;
    },

    shouldHaveControls: function() {
        if (this.shouldHaveStartPlaybackButton())
            return false;

        return Controller.prototype.shouldHaveControls.call(this);
    },

    shouldHaveAnyUI: function() {
        return this.shouldHaveStartPlaybackButton() || Controller.prototype.shouldHaveAnyUI.call(this) || this.currentPlaybackTargetIsWireless();
    },

    createControls: function() {
        Controller.prototype.createControls.call(this);

        var panelContainer = this.controls.panelContainer = document.createElement('div');
        panelContainer.setAttribute('pseudo', '-webkit-media-controls-panel-container');

        var panelBackground = this.controls.panelBackground = document.createElement('div');
        panelBackground.setAttribute('pseudo', '-webkit-media-controls-panel-background');

        var spacer = this.controls.spacer = document.createElement('div');
        spacer.setAttribute('pseudo', '-webkit-media-controls-spacer');
        spacer.classList.add(this.ClassNames.hidden);

        var inlinePlaybackPlaceholderText = this.controls.inlinePlaybackPlaceholderText = document.createElement('div');
        inlinePlaybackPlaceholderText.setAttribute('pseudo', '-webkit-media-controls-wireless-playback-text');

        var inlinePlaybackPlaceholderTextTop = this.controls.inlinePlaybackPlaceholderTextTop = document.createElement('p');
        inlinePlaybackPlaceholderTextTop.setAttribute('pseudo', '-webkit-media-controls-wireless-playback-text-top');

        var inlinePlaybackPlaceholderTextBottom = this.controls.inlinePlaybackPlaceholderTextBottom = document.createElement('p');
        inlinePlaybackPlaceholderTextBottom.setAttribute('pseudo', '-webkit-media-controls-wireless-playback-text-bottom');

        var wirelessTargetPicker = this.controls.wirelessTargetPicker
        this.listenFor(wirelessTargetPicker, 'touchstart', this.handleWirelessPickerButtonTouchStart);
        this.listenFor(wirelessTargetPicker, 'touchend', this.handleWirelessPickerButtonTouchEnd);
        this.listenFor(wirelessTargetPicker, 'touchcancel', this.handleWirelessPickerButtonTouchCancel);

        this.listenFor(this.controls.startPlaybackButton, 'touchstart', this.handleStartPlaybackButtonTouchStart);
        this.listenFor(this.controls.startPlaybackButton, 'touchend', this.handleStartPlaybackButtonTouchEnd);
        this.listenFor(this.controls.startPlaybackButton, 'touchcancel', this.handleStartPlaybackButtonTouchCancel);

        this.listenFor(this.controls.panel, 'touchstart', this.handlePanelTouchStart);
        this.listenFor(this.controls.panel, 'touchend', this.handlePanelTouchEnd);
        this.listenFor(this.controls.panel, 'touchcancel', this.handlePanelTouchCancel);
        this.listenFor(this.controls.playButton, 'touchstart', this.handlePlayButtonTouchStart);
        this.listenFor(this.controls.playButton, 'touchend', this.handlePlayButtonTouchEnd);
        this.listenFor(this.controls.playButton, 'touchcancel', this.handlePlayButtonTouchCancel);
        this.listenFor(this.controls.fullscreenButton, 'touchstart', this.handleFullscreenTouchStart);
        this.listenFor(this.controls.fullscreenButton, 'touchend', this.handleFullscreenTouchEnd);
        this.listenFor(this.controls.fullscreenButton, 'touchcancel', this.handleFullscreenTouchCancel);
        this.listenFor(this.controls.optimizedFullscreenButton, 'touchstart', this.handleOptimizedFullscreenTouchStart);
        this.listenFor(this.controls.optimizedFullscreenButton, 'touchend', this.handleOptimizedFullscreenTouchEnd);
        this.listenFor(this.controls.optimizedFullscreenButton, 'touchcancel', this.handleOptimizedFullscreenTouchCancel);
        this.stopListeningFor(this.controls.playButton, 'click', this.handlePlayButtonClicked);

        this.controls.timeline.style.backgroundImage = '-webkit-canvas(' + this.timelineContextName + ')';
    },

    setControlsType: function(type) {
        if (type === this.controlsType)
            return;
        Controller.prototype.setControlsType.call(this, type);

        if (type === ControllerIOS.StartPlaybackControls)
            this.addStartPlaybackControls();
        else
            this.removeStartPlaybackControls();
    },

    addStartPlaybackControls: function() {
        this.base.appendChild(this.controls.startPlaybackButton);
    },

    removeStartPlaybackControls: function() {
        if (this.controls.startPlaybackButton.parentNode)
            this.controls.startPlaybackButton.parentNode.removeChild(this.controls.startPlaybackButton);
    },

    reconnectControls: function()
    {
        Controller.prototype.reconnectControls.call(this);

        if (this.controlsType === ControllerIOS.StartPlaybackControls)
            this.addStartPlaybackControls();
    },

    configureInlineControls: function() {
        this.controls.inlinePlaybackPlaceholder.appendChild(this.controls.inlinePlaybackPlaceholderText);
        this.controls.inlinePlaybackPlaceholderText.appendChild(this.controls.inlinePlaybackPlaceholderTextTop);
        this.controls.inlinePlaybackPlaceholderText.appendChild(this.controls.inlinePlaybackPlaceholderTextBottom);
        this.controls.panel.appendChild(this.controls.playButton);
        this.controls.panel.appendChild(this.controls.statusDisplay);
        this.controls.panel.appendChild(this.controls.spacer);
        this.controls.panel.appendChild(this.controls.timelineBox);
        this.controls.panel.appendChild(this.controls.wirelessTargetPicker);
        if (!this.isLive) {
            this.controls.timelineBox.appendChild(this.controls.currentTime);
            this.controls.timelineBox.appendChild(this.controls.timeline);
            this.controls.timelineBox.appendChild(this.controls.remainingTime);
        }
        if (this.isAudio()) {
            // Hide the scrubber on audio until the user starts playing.
            this.controls.timelineBox.classList.add(this.ClassNames.hidden);
        } else {
            if (Controller.gSimulateOptimizedFullscreenAvailable || ('webkitSupportsPresentationMode' in this.video && this.video.webkitSupportsPresentationMode('optimized')))
                this.controls.panel.appendChild(this.controls.optimizedFullscreenButton);
            this.controls.panel.appendChild(this.controls.fullscreenButton);
        }
    },

    configureFullScreenControls: function() {
        // Explicitly do nothing to override base-class behavior.
    },

    showControls: function() {
        this.updateLayoutForDisplayedWidth();
        this.updateTime(true);
        this.updateProgress(true);
        Controller.prototype.showControls.call(this);
    },

    addControls: function() {
        this.base.appendChild(this.controls.inlinePlaybackPlaceholder);
        this.base.appendChild(this.controls.panelContainer);
        this.controls.panelContainer.appendChild(this.controls.panelBackground);
        this.controls.panelContainer.appendChild(this.controls.panel);
        this.setNeedsTimelineMetricsUpdate();
    },

    updateControls: function() {
        if (this.shouldHaveStartPlaybackButton())
            this.setControlsType(ControllerIOS.StartPlaybackControls);
        else if (this.presentationMode() === "fullscreen")
            this.setControlsType(Controller.FullScreenControls);
        else
            this.setControlsType(Controller.InlineControls);

        this.updateLayoutForDisplayedWidth();
        this.setNeedsTimelineMetricsUpdate();
    },

    updateTime: function(forceUpdate) {
        Controller.prototype.updateTime.call(this, forceUpdate);
        this.updateProgress();
    },

    drawTimelineBackground: function() {
        var width = this.timelineWidth * window.devicePixelRatio;
        var height = this.timelineHeight * window.devicePixelRatio;

        if (!width || !height)
            return;

        var played = this.video.currentTime / this.video.duration;
        var buffered = 0;
        var bufferedRanges = this.video.buffered;
        if (bufferedRanges && bufferedRanges.length)
            buffered = Math.max(bufferedRanges.end(bufferedRanges.length - 1), buffered);

        buffered /= this.video.duration;

        var ctx = this.video.ownerDocument.getCSSCanvasContext('2d', this.timelineContextName, width, height);

        ctx.clearRect(0, 0, width, height);

        var midY = height / 2;

        // 1. Draw the buffered part and played parts, using
        // solid rectangles that are clipped to the outside of
        // the lozenge.
        ctx.save();
        ctx.beginPath();
        this.addRoundedRect(ctx, 1, midY - 3, width - 2, 6, 3);
        ctx.closePath();
        ctx.clip();
        ctx.fillStyle = "white";
        ctx.fillRect(0, 0, Math.round(width * played) + 2, height);
        ctx.fillStyle = "rgba(0, 0, 0, 0.55)";
        ctx.fillRect(Math.round(width * played) + 2, 0, Math.round(width * (buffered - played)) + 2, height);
        ctx.restore();

        // 2. Draw the outline with a clip path that subtracts the
        // middle of a lozenge. This produces a better result than
        // stroking.
        ctx.save();
        ctx.beginPath();
        this.addRoundedRect(ctx, 1, midY - 3, width - 2, 6, 3);
        this.addRoundedRect(ctx, 2, midY - 2, width - 4, 4, 2);
        ctx.closePath();
        ctx.clip("evenodd");
        ctx.fillStyle = "rgba(0, 0, 0, 0.55)";
        ctx.fillRect(Math.round(width * buffered) + 2, 0, width, height);
        ctx.restore();
    },

    formatTime: function(time) {
        if (isNaN(time))
            time = 0;
        var absTime = Math.abs(time);
        var intSeconds = Math.floor(absTime % 60).toFixed(0);
        var intMinutes = Math.floor((absTime / 60) % 60).toFixed(0);
        var intHours = Math.floor(absTime / (60 * 60)).toFixed(0);
        var sign = time < 0 ? '-' : String();

        if (intHours > 0)
            return sign + intHours + ':' + String('0' + intMinutes).slice(-2) + ":" + String('0' + intSeconds).slice(-2);

        return sign + String('0' + intMinutes).slice(intMinutes >= 10 ? -2 : -1) + ":" + String('0' + intSeconds).slice(-2);
    },

    handleTimelineChange: function(event) {
        Controller.prototype.handleTimelineChange.call(this);
        this.updateProgress();
    },

    handlePlayButtonTouchStart: function() {
        this.controls.playButton.classList.add('active');
    },

    handlePlayButtonTouchEnd: function(event) {
        this.controls.playButton.classList.remove('active');

        if (this.canPlay())
            this.video.play();
        else
            this.video.pause();

        return true;
    },

    handlePlayButtonTouchCancel: function(event) {
        this.controls.playButton.classList.remove('active');
        return true;
    },

    handleBaseGestureStart: function(event) {
        this.gestureStartTime = new Date();
        // If this gesture started with two fingers inside the video, then
        // don't treat it as a potential zoom, unless we're still waiting
        // to play.
        if (this.mostRecentNumberOfTargettedTouches == 2 && this.controlsType != ControllerIOS.StartPlaybackControls)
            event.preventDefault();
    },

    handleBaseGestureChange: function(event) {
        if (!this.video.controls || this.isAudio() || this.isFullScreen() || this.gestureStartTime === undefined || this.controlsType == ControllerIOS.StartPlaybackControls)
            return;

        var scaleDetectionThreshold = 0.2;
        if (event.scale > 1 + scaleDetectionThreshold || event.scale < 1 - scaleDetectionThreshold)
            delete this.lastDoubleTouchTime;

        if (this.mostRecentNumberOfTargettedTouches == 2 && event.scale >= 1.0)
            event.preventDefault();

        var currentGestureTime = new Date();
        var duration = (currentGestureTime - this.gestureStartTime) / 1000;
        if (!duration)
            return;

        var velocity = Math.abs(event.scale - 1) / duration;

        var pinchOutVelocityThreshold = 2;
        var pinchOutGestureScaleThreshold = 1.25;
        if (velocity < pinchOutVelocityThreshold || event.scale < pinchOutGestureScaleThreshold)
            return;

        delete this.gestureStartTime;
        this.video.webkitEnterFullscreen();
    },

    handleBaseGestureEnd: function(event) {
        delete this.gestureStartTime;
    },

    handleWrapperTouchStart: function(event) {
        if (event.target != this.base && event.target != this.controls.inlinePlaybackPlaceholder)
            return;

        this.mostRecentNumberOfTargettedTouches = event.targetTouches.length;

        if (this.controlsAreHidden()) {
            this.showControls();
            if (this.hideTimer)
                clearTimeout(this.hideTimer);
            this.hideTimer = setTimeout(this.hideControls.bind(this), this.HideControlsDelay);
        } else if (!this.canPlay())
            this.hideControls();
    },

    handlePanelTouchStart: function(event) {
        this.video.style.webkitUserSelect = 'none';
    },

    handlePanelTouchEnd: function(event) {
        this.video.style.removeProperty('-webkit-user-select');
    },

    handlePanelTouchCancel: function(event) {
        this.video.style.removeProperty('-webkit-user-select');
    },

    handleVisibilityChange: function(event) {
        this.updateShouldListenForPlaybackTargetAvailabilityEvent();
    },

    presentationMode: function() {
        if ('webkitPresentationMode' in this.video)
            return this.video.webkitPresentationMode;

        if (this.isFullScreen())
            return 'fullscreen';

        return 'inline';
    },

    isFullScreen: function()
    {
        return this.video.webkitDisplayingFullscreen && this.presentationMode() != 'optimized';
    },

    handleFullscreenButtonClicked: function(event) {
        if ('webkitSetPresentationMode' in this.video) {
            if (this.presentationMode() === 'fullscreen')
                this.video.webkitSetPresentationMode('inline');
            else
                this.video.webkitSetPresentationMode('fullscreen');

            return;
        }

        if (this.isFullScreen())
            this.video.webkitExitFullscreen();
        else
            this.video.webkitEnterFullscreen();
    },

    handleFullscreenTouchStart: function() {
        this.controls.fullscreenButton.classList.add('active');
    },

    handleFullscreenTouchEnd: function(event) {
        this.controls.fullscreenButton.classList.remove('active');

        this.handleFullscreenButtonClicked();

        return true;
    },

    handleFullscreenTouchCancel: function(event) {
        this.controls.fullscreenButton.classList.remove('active');
        return true;
    },

    handleOptimizedFullscreenButtonClicked: function(event) {
        if (!('webkitSetPresentationMode' in this.video))
            return;

        if (this.presentationMode() === 'optimized')
            this.video.webkitSetPresentationMode('inline');
        else
            this.video.webkitSetPresentationMode('optimized');
    },

    handleOptimizedFullscreenTouchStart: function() {
        this.controls.optimizedFullscreenButton.classList.add('active');
    },

    handleOptimizedFullscreenTouchEnd: function(event) {
        this.controls.optimizedFullscreenButton.classList.remove('active');

        this.handleOptimizedFullscreenButtonClicked();

        return true;
    },

    handleOptimizedFullscreenTouchCancel: function(event) {
        this.controls.optimizedFullscreenButton.classList.remove('active');
        return true;
    },

    handleStartPlaybackButtonTouchStart: function(event) {
        this.controls.startPlaybackButton.classList.add('active');
    },

    handleStartPlaybackButtonTouchEnd: function(event) {
        this.controls.startPlaybackButton.classList.remove('active');
        if (this.video.error)
            return true;

        this.video.play();
        this.updateControls();

        return true;
    },

    handleStartPlaybackButtonTouchCancel: function(event) {
        this.controls.startPlaybackButton.classList.remove('active');
        return true;
    },

    handleReadyStateChange: function(event) {
        Controller.prototype.handleReadyStateChange.call(this, event);
        this.updateControls();
    },

    handleWirelessPickerButtonTouchStart: function() {
        if (!this.video.error)
            this.controls.wirelessTargetPicker.classList.add('active');
    },

    handleWirelessPickerButtonTouchEnd: function(event) {
        this.controls.wirelessTargetPicker.classList.remove('active');
        return this.handleWirelessPickerButtonClicked();
    },

    handleWirelessPickerButtonTouchCancel: function(event) {
        this.controls.wirelessTargetPicker.classList.remove('active');
        return true;
    },

    updateShouldListenForPlaybackTargetAvailabilityEvent: function() {
        if (this.controlsType === ControllerIOS.StartPlaybackControls) {
            this.setShouldListenForPlaybackTargetAvailabilityEvent(false);
            return;
        }

        Controller.prototype.updateShouldListenForPlaybackTargetAvailabilityEvent.call(this);
    },

    updateStatusDisplay: function(event)
    {
        this.controls.startPlaybackButton.classList.toggle(this.ClassNames.failed, this.video.error !== null);
        Controller.prototype.updateStatusDisplay.call(this, event);
    },

    setPlaying: function(isPlaying)
    {
        Controller.prototype.setPlaying.call(this, isPlaying);

        this.updateControls();

        if (isPlaying && this.isAudio() && !this._timelineIsHidden) {
            this.controls.timelineBox.classList.remove(this.ClassNames.hidden);
            this.controls.spacer.classList.add(this.ClassNames.hidden);
        }

        if (isPlaying)
            this.hasPlayed = true;
        else
            this.showControls();
    },

    setShouldListenForPlaybackTargetAvailabilityEvent: function(shouldListen)
    {
        if (shouldListen && (this.shouldHaveStartPlaybackButton() || this.video.error))
            return;

        Controller.prototype.setShouldListenForPlaybackTargetAvailabilityEvent.call(this, shouldListen);
    },

    get pageScaleFactor()
    {
        return this._pageScaleFactor;
    },

    set pageScaleFactor(newScaleFactor)
    {
        if (this._pageScaleFactor === newScaleFactor)
            return;

        this._pageScaleFactor = newScaleFactor;

        // FIXME: this should react to the scale change by
        // unscaling the controls panel. However, this
        // hits a bug with the backdrop blur layer getting
        // too big and moving to a tiled layer.
        // https://bugs.webkit.org/show_bug.cgi?id=142317
    },

    handlePresentationModeChange: function(event)
    {
        var presentationMode = this.presentationMode();

        switch (presentationMode) {
            case 'inline':
                this.controls.inlinePlaybackPlaceholder.classList.add(this.ClassNames.hidden);
                break;
            case 'optimized':
                var backgroundImage = "url('" + this.host.mediaUIImageData("optimized-fullscreen-placeholder") + "')";
                this.controls.inlinePlaybackPlaceholder.style.backgroundImage = backgroundImage;
                this.controls.inlinePlaybackPlaceholder.setAttribute('aria-label', "video playback placeholder");
                this.controls.inlinePlaybackPlaceholder.classList.remove(this.ClassNames.hidden);
                break;
        }

        this.updateControls();
        this.updateCaptionContainer();
        if (presentationMode != 'fullscreen' && this.video.paused && this.controlsAreHidden())
            this.showControls();
    },

    handleFullscreenChange: function(event)
    {
        Controller.prototype.handleFullscreenChange.call(this, event);
        this.handlePresentationModeChange(event);
    },

    scheduleUpdateLayoutForDisplayedWidth: function ()
    {
        setTimeout(function () {
            this.updateLayoutForDisplayedWidth();
        }.bind(this), 0);
    },

    updateLayoutForDisplayedWidth: function()
    {
        if (!this.controls || !this.controls.panel)
            return;

        var visibleWidth = this.controls.panel.getBoundingClientRect().width * this._pageScaleFactor;
        if (visibleWidth <= 0 || visibleWidth == this._currentDisplayWidth)
            return;

        this._currentDisplayWidth = visibleWidth;

        // We need to work out how many right-hand side buttons are available.
        this.updateWirelessTargetAvailable();
        this.updateFullscreenButtons();

        var visibleButtonWidth = ControllerIOS.ButtonWidth; // We always try to show the fullscreen button.

        if (!this.controls.wirelessTargetPicker.classList.contains(this.ClassNames.hidden))
            visibleButtonWidth += ControllerIOS.ButtonWidth;
        if (!this.controls.optimizedFullscreenButton.classList.contains(this.ClassNames.hidden))
            visibleButtonWidth += ControllerIOS.ButtonWidth;

        // Check if there is enough room for the scrubber.
        if ((visibleWidth - visibleButtonWidth) < ControllerIOS.MinimumTimelineWidth) {
            this.controls.timelineBox.classList.add(this.ClassNames.hidden);
            this.controls.spacer.classList.remove(this.ClassNames.hidden);
            this._timelineIsHidden = true;
        } else {
            if (!this.isAudio() || this.hasPlayed) {
                this.controls.timelineBox.classList.remove(this.ClassNames.hidden);
                this.controls.spacer.classList.add(this.ClassNames.hidden);
                this._timelineIsHidden = false;
            } else
                this.controls.spacer.classList.remove(this.ClassNames.hidden);
        }

        // Drop the airplay button if there isn't enough space.
        if (visibleWidth < visibleButtonWidth) {
            this.controls.wirelessTargetPicker.classList.add(this.ClassNames.hidden);
            visibleButtonWidth -= ControllerIOS.ButtonWidth;
        }

        // Drop the optimized fullscreen button if there still isn't enough space.
        if (visibleWidth < visibleButtonWidth) {
            this.controls.optimizedFullscreenButton.classList.add(this.ClassNames.hidden);
            visibleButtonWidth -= ControllerIOS.ButtonWidth;
        }

        // And finally, drop the fullscreen button as a last resort.
        if (visibleWidth < visibleButtonWidth) {
            this.controls.fullscreenButton.classList.add(this.ClassNames.hidden);
            visibleButtonWidth -= ControllerIOS.ButtonWidth;
        } else
            this.controls.fullscreenButton.classList.remove(this.ClassNames.hidden);
    },

    controlsAlwaysVisible: function()
    {
        if (this.presentationMode() === 'optimized')
            return true;

        return Controller.prototype.controlsAlwaysVisible.call(this);
    },


};

Object.create(Controller.prototype).extend(ControllerIOS.prototype);
Object.defineProperty(ControllerIOS.prototype, 'constructor', { enumerable: false, value: ControllerIOS });

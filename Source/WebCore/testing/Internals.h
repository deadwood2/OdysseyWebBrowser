/*
 * Copyright (C) 2012 Google Inc. All rights reserved.
 * Copyright (C) 2013-2015 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#pragma once

#include "CSSComputedStyleDeclaration.h"
#include "ContextDestructionObserver.h"
#include "PageConsoleClient.h"
#include <runtime/Float32Array.h>

#if ENABLE(MEDIA_SESSION)
#include "MediaSessionInterruptionProvider.h"
#endif

namespace WebCore {

class AudioContext;
class ClientRect;
class ClientRectList;
class DOMURL;
class DOMWindow;
class Document;
class Element;
class File;
class Frame;
class GCObservation;
class HTMLImageElement;
class HTMLInputElement;
class HTMLLinkElement;
class HTMLMediaElement;
class HTMLSelectElement;
class InspectorStubFrontend;
class InternalSettings;
class MallocStatistics;
class MediaSession;
class MemoryInfo;
class MockContentFilterSettings;
class MockPageOverlay;
class NodeList;
class Page;
class Range;
class RenderedDocumentMarker;
class SerializedScriptValue;
class SourceBuffer;
class TimeRanges;
class TypeConversions;
class XMLHttpRequest;

typedef int ExceptionCode;

class Internals final : public RefCounted<Internals>, private ContextDestructionObserver {
public:
    static Ref<Internals> create(Document&);
    virtual ~Internals();

    static void resetToConsistentState(Page&);

    String elementRenderTreeAsText(Element&, ExceptionCode&);
    bool hasPausedImageAnimations(Element&);

    String address(Node&);
    bool nodeNeedsStyleRecalc(Node&);
    String styleChangeType(Node&);
    String description(JSC::JSValue);

    bool isPreloaded(const String& url);
    bool isLoadingFromMemoryCache(const String& url);
    String xhrResponseSource(XMLHttpRequest&);
    bool isSharingStyleSheetContents(HTMLLinkElement&, HTMLLinkElement&);
    bool isStyleSheetLoadingSubresources(HTMLLinkElement&);
    enum class CachePolicy { UseProtocolCachePolicy, ReloadIgnoringCacheData, ReturnCacheDataElseLoad, ReturnCacheDataDontLoad };
    void setOverrideCachePolicy(CachePolicy);
    void setCanShowModalDialogOverride(bool allow, ExceptionCode&);
    enum class ResourceLoadPriority { ResourceLoadPriorityVeryLow, ResourceLoadPriorityLow, ResourceLoadPriorityMedium, ResourceLoadPriorityHigh, ResourceLoadPriorityVeryHigh };
    void setOverrideResourceLoadPriority(ResourceLoadPriority);
    void setStrictRawResourceValidationPolicyDisabled(bool);

    void clearMemoryCache();
    void pruneMemoryCacheToSize(unsigned size);
    unsigned memoryCacheSize() const;

    unsigned imageFrameIndex(HTMLImageElement&);

    void clearPageCache();
    unsigned pageCacheSize() const;

    RefPtr<CSSComputedStyleDeclaration> computedStyleIncludingVisitedInfo(Element&) const;

    Node* ensureShadowRoot(Element& host, ExceptionCode&);
    Node* ensureUserAgentShadowRoot(Element& host);
    Node* createShadowRoot(Element& host, ExceptionCode&);
    Node* shadowRoot(Element& host);
    String shadowRootType(const Node&, ExceptionCode&) const;
    String shadowPseudoId(Element&);
    void setShadowPseudoId(Element&, const String&);

    // DOMTimers throttling testing.
    bool isTimerThrottled(int timeoutId, ExceptionCode&);
    bool isRequestAnimationFrameThrottled() const;
    bool areTimersThrottled() const;

    // Spatial Navigation testing.
    unsigned lastSpatialNavigationCandidateCount(ExceptionCode&) const;

    // CSS Animation testing.
    unsigned numberOfActiveAnimations() const;
    bool animationsAreSuspended(ExceptionCode&) const;
    void suspendAnimations(ExceptionCode&) const;
    void resumeAnimations(ExceptionCode&) const;
    bool pauseAnimationAtTimeOnElement(const String& animationName, double pauseTime, Element&, ExceptionCode&);
    bool pauseAnimationAtTimeOnPseudoElement(const String& animationName, double pauseTime, Element&, const String& pseudoId, ExceptionCode&);

    // CSS Transition testing.
    bool pauseTransitionAtTimeOnElement(const String& propertyName, double pauseTime, Element&, ExceptionCode&);
    bool pauseTransitionAtTimeOnPseudoElement(const String& property, double pauseTime, Element&, const String& pseudoId, ExceptionCode&);

    Node* treeScopeRootNode(Node&);
    Node* parentTreeScope(Node&);

    String visiblePlaceholder(Element&);
    void selectColorInColorChooser(HTMLInputElement&, const String& colorValue);
    Vector<String> formControlStateOfPreviousHistoryItem(ExceptionCode&);
    void setFormControlStateOfPreviousHistoryItem(const Vector<String>&, ExceptionCode&);

    Ref<ClientRect> absoluteCaretBounds(ExceptionCode&);

    Ref<ClientRect> boundingBox(Element&);

    Ref<ClientRectList> inspectorHighlightRects(ExceptionCode&);
    String inspectorHighlightObject(ExceptionCode&);

    unsigned markerCountForNode(Node&, const String&, ExceptionCode&);
    RefPtr<Range> markerRangeForNode(Node&, const String& markerType, unsigned index, ExceptionCode&);
    String markerDescriptionForNode(Node&, const String& markerType, unsigned index, ExceptionCode&);
    String dumpMarkerRects(const String& markerType, ExceptionCode&);
    void addTextMatchMarker(const Range&, bool isActive);
    void setMarkedTextMatchesAreHighlighted(bool, ExceptionCode&);

    void invalidateFontCache();

    void setScrollViewPosition(int x, int y, ExceptionCode&);
    void setViewBaseBackgroundColor(const String& colorValue, ExceptionCode&);

    void setPagination(const String& mode, int gap, int pageLength, ExceptionCode&);
    void setPaginationLineGridEnabled(bool, ExceptionCode&);
    String configurationForViewport(float devicePixelRatio, int deviceWidth, int deviceHeight, int availableWidth, int availableHeight, ExceptionCode&);

    bool wasLastChangeUserEdit(Element& textField, ExceptionCode&);
    bool elementShouldAutoComplete(HTMLInputElement&);
    void setEditingValue(HTMLInputElement&, const String&);
    void setAutofilled(HTMLInputElement&, bool enabled);
    enum class AutoFillButtonType { AutoFillButtonTypeNone, AutoFillButtonTypeContacts, AutoFillButtonTypeCredentials };
    void setShowAutoFillButton(HTMLInputElement&, AutoFillButtonType);
    void scrollElementToRect(Element&, int x, int y, int w, int h, ExceptionCode&);

    String autofillFieldName(Element&, ExceptionCode&);

    void paintControlTints(ExceptionCode&);

    RefPtr<Range> rangeFromLocationAndLength(Element& scope, int rangeLocation, int rangeLength);
    unsigned locationFromRange(Element& scope, const Range&);
    unsigned lengthFromRange(Element& scope, const Range&);
    String rangeAsText(const Range&);
    RefPtr<Range> subrange(Range&, int rangeLocation, int rangeLength);
    RefPtr<Range> rangeForDictionaryLookupAtLocation(int x, int y, ExceptionCode&);

    void setDelegatesScrolling(bool enabled, ExceptionCode&);

    int lastSpellCheckRequestSequence(ExceptionCode&);
    int lastSpellCheckProcessedSequence(ExceptionCode&);

    Vector<String> userPreferredLanguages() const;
    void setUserPreferredLanguages(const Vector<String>&);

    Vector<String> userPreferredAudioCharacteristics() const;
    void setUserPreferredAudioCharacteristic(const String&);

    unsigned wheelEventHandlerCount(ExceptionCode&);
    unsigned touchEventHandlerCount(ExceptionCode&);

    RefPtr<NodeList> nodesFromRect(Document&, int x, int y, unsigned topPadding, unsigned rightPadding,
        unsigned bottomPadding, unsigned leftPadding, bool ignoreClipping, bool allowShadowContent, bool allowChildFrameContent, ExceptionCode&) const;

    String parserMetaData(JSC::JSValue = JSC::JSValue::JSUndefined);

    void updateEditorUINowIfScheduled();

    bool hasSpellingMarker(int from, int length, ExceptionCode&);
    bool hasGrammarMarker(int from, int length, ExceptionCode&);
    bool hasAutocorrectedMarker(int from, int length, ExceptionCode&);
    void setContinuousSpellCheckingEnabled(bool enabled, ExceptionCode&);
    void setAutomaticQuoteSubstitutionEnabled(bool enabled, ExceptionCode&);
    void setAutomaticLinkDetectionEnabled(bool enabled, ExceptionCode&);
    void setAutomaticDashSubstitutionEnabled(bool enabled, ExceptionCode&);
    void setAutomaticTextReplacementEnabled(bool enabled, ExceptionCode&);
    void setAutomaticSpellingCorrectionEnabled(bool enabled, ExceptionCode&);

    void handleAcceptedCandidate(const String& candidate, ExceptionCode&);

    bool isOverwriteModeEnabled(ExceptionCode&);
    void toggleOverwriteModeEnabled(ExceptionCode&);

    unsigned countMatchesForText(const String&, unsigned findOptions, const String& markMatches, ExceptionCode&);
    unsigned countFindMatches(const String&, unsigned findOptions, ExceptionCode&);

    unsigned numberOfScrollableAreas(ExceptionCode&);

    bool isPageBoxVisible(int pageNumber, ExceptionCode&);

    static const char* internalsId;

    InternalSettings* settings() const;
    unsigned workerThreadCount() const;

    void setBatteryStatus(const String& eventType, bool charging, double chargingTime, double dischargingTime, double level, ExceptionCode&);

    void setDeviceProximity(const String& eventType, double value, double min, double max, ExceptionCode&);

    enum {
        // Values need to be kept in sync with Internals.idl.
        LAYER_TREE_INCLUDES_VISIBLE_RECTS = 1,
        LAYER_TREE_INCLUDES_TILE_CACHES = 2,
        LAYER_TREE_INCLUDES_REPAINT_RECTS = 4,
        LAYER_TREE_INCLUDES_PAINTING_PHASES = 8,
        LAYER_TREE_INCLUDES_CONTENT_LAYERS = 16
    };
    String layerTreeAsText(Document&, unsigned short flags, ExceptionCode&) const;
    String repaintRectsAsText(ExceptionCode&) const;
    String scrollingStateTreeAsText(ExceptionCode&) const;
    String mainThreadScrollingReasons(ExceptionCode&) const;
    RefPtr<ClientRectList> nonFastScrollableRects(ExceptionCode&) const;

    void setElementUsesDisplayListDrawing(Element&, bool usesDisplayListDrawing, ExceptionCode&);
    void setElementTracksDisplayListReplay(Element&, bool isTrackingReplay, ExceptionCode&);

    enum {
        // Values need to be kept in sync with Internals.idl.
        DISPLAY_LIST_INCLUDES_PLATFORM_OPERATIONS = 1,
    };
    String displayListForElement(Element&, unsigned short flags, ExceptionCode&);

    String replayDisplayListForElement(Element&, unsigned short flags, ExceptionCode&);

    void garbageCollectDocumentResources(ExceptionCode&) const;

    void beginSimulatedMemoryPressure();
    void endSimulatedMemoryPressure();
    bool isUnderMemoryPressure();

    void insertAuthorCSS(const String&, ExceptionCode&) const;
    void insertUserCSS(const String&, ExceptionCode&) const;

    unsigned numberOfLiveNodes() const;
    unsigned numberOfLiveDocuments() const;

    RefPtr<DOMWindow> openDummyInspectorFrontend(const String& url);
    void closeDummyInspectorFrontend();
    void setInspectorIsUnderTest(bool isUnderTest, ExceptionCode&);

    String counterValue(Element&);

    int pageNumber(Element&, float pageWidth = 800, float pageHeight = 600);
    Vector<String> shortcutIconURLs() const;

    int numberOfPages(float pageWidthInPixels = 800, float pageHeightInPixels = 600);
    String pageProperty(String, int, ExceptionCode&) const;
    String pageSizeAndMarginsInPixels(int, int, int, int, int, int, int, ExceptionCode&) const;

    void setPageScaleFactor(float scaleFactor, int x, int y, ExceptionCode&);
    void setPageZoomFactor(float zoomFactor, ExceptionCode&);
    void setTextZoomFactor(float zoomFactor, ExceptionCode&);

    void setUseFixedLayout(bool useFixedLayout, ExceptionCode&);
    void setFixedLayoutSize(int width, int height, ExceptionCode&);
    void setViewExposedRect(float left, float top, float width, float height, ExceptionCode&);

    void setHeaderHeight(float);
    void setFooterHeight(float);

    void setTopContentInset(float);

#if ENABLE(FULLSCREEN_API)
    void webkitWillEnterFullScreenForElement(Element&);
    void webkitDidEnterFullScreenForElement(Element&);
    void webkitWillExitFullScreenForElement(Element&);
    void webkitDidExitFullScreenForElement(Element&);
#endif

    WEBCORE_TESTSUPPORT_EXPORT void setApplicationCacheOriginQuota(unsigned long long);

    void registerURLSchemeAsBypassingContentSecurityPolicy(const String& scheme);
    void removeURLSchemeRegisteredAsBypassingContentSecurityPolicy(const String& scheme);

    Ref<MallocStatistics> mallocStatistics() const;
    Ref<TypeConversions> typeConversions() const;
    Ref<MemoryInfo> memoryInfo() const;

    Vector<String> getReferencedFilePaths() const;

    void startTrackingRepaints(ExceptionCode&);
    void stopTrackingRepaints(ExceptionCode&);

    void startTrackingLayerFlushes(ExceptionCode&);
    unsigned layerFlushCount(ExceptionCode&);
    
    void startTrackingStyleRecalcs(ExceptionCode&);
    unsigned styleRecalcCount(ExceptionCode&);
    unsigned lastStyleUpdateSize() const;

    void startTrackingCompositingUpdates(ExceptionCode&);
    unsigned compositingUpdateCount(ExceptionCode&);

    void updateLayoutIgnorePendingStylesheetsAndRunPostLayoutTasks(Node*, ExceptionCode&);
    unsigned layoutCount() const;

    RefPtr<ArrayBuffer> serializeObject(PassRefPtr<SerializedScriptValue>) const;
    RefPtr<SerializedScriptValue> deserializeBuffer(ArrayBuffer&) const;

    bool isFromCurrentWorld(JSC::JSValue) const;

    void setUsesOverlayScrollbars(bool);
    void setUsesMockScrollAnimator(bool);

    String getCurrentCursorInfo(ExceptionCode&);

    String markerTextForListItem(Element&);

    String toolTipFromElement(Element&) const;

    void forceReload(bool endToEnd);

    void enableAutoSizeMode(bool enabled, int minimumWidth, int minimumHeight, int maximumWidth, int maximumHeight);

#if ENABLE(ENCRYPTED_MEDIA_V2)
    void initializeMockCDM();
#endif

#if ENABLE(SPEECH_SYNTHESIS)
    void enableMockSpeechSynthesizer();
#endif

#if ENABLE(MEDIA_STREAM)
    void setMockMediaCaptureDevicesEnabled(bool);
#endif

#if ENABLE(WEB_RTC)
    void enableMockMediaEndpoint();
    void enableMockRTCPeerConnectionHandler();
#endif

    String getImageSourceURL(Element&);

#if ENABLE(VIDEO)
    void simulateAudioInterruption(HTMLMediaElement&);
    bool mediaElementHasCharacteristic(HTMLMediaElement&, const String&, ExceptionCode&);
#endif

    bool isSelectPopupVisible(HTMLSelectElement&);

    String captionsStyleSheetOverride(ExceptionCode&);
    void setCaptionsStyleSheetOverride(const String&, ExceptionCode&);
    void setPrimaryAudioTrackLanguageOverride(const String&, ExceptionCode&);
    void setCaptionDisplayMode(const String&, ExceptionCode&);

#if ENABLE(VIDEO)
    Ref<TimeRanges> createTimeRanges(Float32Array& startTimes, Float32Array& endTimes);
    double closestTimeToTimeRanges(double time, TimeRanges&);
#endif

    Ref<ClientRect> selectionBounds(ExceptionCode&);

#if ENABLE(VIBRATION)
    bool isVibrating();
#endif

    bool isPluginUnavailabilityIndicatorObscured(Element&, ExceptionCode&);
    bool isPluginSnapshotted(Element&);

#if ENABLE(MEDIA_SOURCE)
    WEBCORE_TESTSUPPORT_EXPORT void initializeMockMediaSource();
    Vector<String> bufferedSamplesForTrackID(SourceBuffer&, const AtomicString&);
    void setShouldGenerateTimestamps(SourceBuffer&, bool);
#endif

#if ENABLE(VIDEO)
    void beginMediaSessionInterruption(const String&, ExceptionCode&);
    void endMediaSessionInterruption(const String&);
    void applicationDidEnterForeground() const;
    void applicationWillEnterBackground() const;
    void setMediaSessionRestrictions(const String& mediaType, const String& restrictions, ExceptionCode&);
    void setMediaElementRestrictions(HTMLMediaElement&, const String& restrictions);
    void postRemoteControlCommand(const String&, float argument, ExceptionCode&);
    bool elementIsBlockingDisplaySleep(HTMLMediaElement&) const;
#endif

#if ENABLE(MEDIA_SESSION)
    void sendMediaSessionStartOfInterruptionNotification(MediaSessionInterruptingCategory);
    void sendMediaSessionEndOfInterruptionNotification(MediaSessionInterruptingCategory);
    String mediaSessionCurrentState(MediaSession&) const;
    double mediaElementPlayerVolume(HTMLMediaElement&) const;
    enum class MediaControlEvent { PlayPause, NextTrack, PreviousTrack };
    void sendMediaControlEvent(MediaControlEvent);
#endif

#if ENABLE(WIRELESS_PLAYBACK_TARGET)
    void setMockMediaPlaybackTargetPickerEnabled(bool);
    void setMockMediaPlaybackTargetPickerState(const String& deviceName, const String& deviceState, ExceptionCode&);
#endif

#if ENABLE(WEB_AUDIO)
    void setAudioContextRestrictions(AudioContext&, const String& restrictions);
#endif

    void simulateSystemSleep() const;
    void simulateSystemWake() const;

    enum class PageOverlayType { View, Document };
    RefPtr<MockPageOverlay> installMockPageOverlay(PageOverlayType, ExceptionCode&);
    String pageOverlayLayerTreeAsText(ExceptionCode&) const;

    void setPageMuted(bool);
    bool isPagePlayingAudio();

    void setPageDefersLoading(bool);

    RefPtr<File> createFile(const String&);
    void queueMicroTask(int);
    bool testPreloaderSettingViewport();

#if ENABLE(CONTENT_FILTERING)
    MockContentFilterSettings& mockContentFilterSettings();
#endif

#if ENABLE(CSS_SCROLL_SNAP)
    String scrollSnapOffsets(Element&, ExceptionCode&);
#endif

    String pathStringWithShrinkWrappedRects(Vector<double> rectComponents, double radius, ExceptionCode&);

    String getCurrentMediaControlsStatusForElement(HTMLMediaElement&);

    String userVisibleString(const DOMURL&);
    void setShowAllPlugins(bool);

    String resourceLoadStatisticsForOrigin(String origin);
    void setResourceLoadStatisticsEnabled(bool);

#if ENABLE(STREAMS_API)
    bool isReadableStreamDisturbed(JSC::ExecState&, JSC::JSValue);
#endif

    String composedTreeAsText(Node&);
    
    void setLinkPreloadSupport(bool);
    void setResourceTimingSupport(bool);

#if ENABLE(CSS_GRID_LAYOUT)
    void setCSSGridLayoutEnabled(bool);
#endif

#if ENABLE(WEBGL2)
    bool webGL2Enabled() const;
    void setWebGL2Enabled(bool);
#endif

    bool isProcessingUserGesture();

    RefPtr<GCObservation> observeGC(JSC::JSValue);

    enum class UserInterfaceLayoutDirection { LTR, RTL };
    void setUserInterfaceLayoutDirection(UserInterfaceLayoutDirection);

private:
    explicit Internals(Document&);
    Document* contextDocument() const;
    Frame* frame() const;

    RenderedDocumentMarker* markerAt(Node&, const String& markerType, unsigned index, ExceptionCode&);

    std::unique_ptr<InspectorStubFrontend> m_inspectorFrontend;
};

} // namespace WebCore

/*
 * Copyright (C) 2009 Google Inc. All rights reserved.
 * Copyright (C) 2013-2016 Apple Inc. All rights reserved. 
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

#pragma once

#include "PlatformExportMacros.h"
#include <wtf/NeverDestroyed.h>

namespace WebCore {

// A class that stores static enablers for all experimental features. Note that
// the method names must line up with the JavaScript method they enable for code
// generation to work properly.

class RuntimeEnabledFeatures {
    WTF_MAKE_NONCOPYABLE(RuntimeEnabledFeatures);
public:
    void setLocalStorageEnabled(bool isEnabled) { m_isLocalStorageEnabled = isEnabled; }
    bool localStorageEnabled() const { return m_isLocalStorageEnabled; }

    void setSessionStorageEnabled(bool isEnabled) { m_isSessionStorageEnabled = isEnabled; }
    bool sessionStorageEnabled() const { return m_isSessionStorageEnabled; }

    void setWebkitNotificationsEnabled(bool isEnabled) { m_isWebkitNotificationsEnabled = isEnabled; }
    bool webkitNotificationsEnabled() const { return m_isWebkitNotificationsEnabled; }

    void setApplicationCacheEnabled(bool isEnabled) { m_isApplicationCacheEnabled = isEnabled; }
    bool applicationCacheEnabled() const { return m_isApplicationCacheEnabled; }

    void setDataTransferItemsEnabled(bool isEnabled) { m_isDataTransferItemsEnabled = isEnabled; }
    bool dataTransferItemsEnabled() const { return m_isDataTransferItemsEnabled; }

    void setDOMIteratorEnabled(bool isEnabled) { m_isDOMIteratorEnabled = isEnabled; }
    bool domIteratorEnabled() const { return m_isDOMIteratorEnabled; }

    void setGeolocationEnabled(bool isEnabled) { m_isGeolocationEnabled = isEnabled; }
    bool geolocationEnabled() const { return m_isGeolocationEnabled; }

#if ENABLE(INDEXED_DATABASE)
    void setWebkitIndexedDBEnabled(bool isEnabled) { m_isIndexedDBEnabled = isEnabled; }
    bool webkitIndexedDBEnabled() const { return m_isIndexedDBEnabled; }
    bool indexedDBEnabled() const { return m_isIndexedDBEnabled; }
#endif

#if ENABLE(INDEXED_DATABASE_IN_WORKERS)
    void setIndexedDBWorkersEnabled(bool isEnabled) { m_isIndexedDBWorkersEnabled = isEnabled; }
    bool indexedDBWorkersEnabled() const { return m_isIndexedDBWorkersEnabled; }
#endif

#if ENABLE(FONT_LOAD_EVENTS)
    void setFontLoadEventsEnabled(bool isEnabled) { m_isFontLoadEventsEnabled = isEnabled; }
    bool fontLoadEventsEnabled() const { return m_isFontLoadEventsEnabled; }
#else
    void setFontLoadEventsEnabled(bool) { }
    bool fontLoadEventsEnabled() { return false; }
#endif

#if ENABLE(VIDEO)
    bool audioEnabled() const;
    bool htmlMediaElementEnabled() const;
    bool htmlAudioElementEnabled() const;
    bool htmlVideoElementEnabled() const;
    bool htmlSourceElementEnabled() const;
    bool mediaControllerEnabled() const;
    bool mediaErrorEnabled() const;
    bool timeRangesEnabled() const;
#endif

#if ENABLE(WEB_SOCKETS)
    bool webSocketEnabled() const;
#endif

#if ENABLE(TOUCH_EVENTS)
    bool touchEnabled() const { return m_isTouchEnabled; }
    void setTouchEnabled(bool isEnabled) { m_isTouchEnabled = isEnabled; }
#endif

    void setDeviceMotionEnabled(bool isEnabled) { m_isDeviceMotionEnabled = isEnabled; }
    bool deviceMotionEnabled() const { return m_isDeviceMotionEnabled; }
    bool deviceMotionEventEnabled() const { return m_isDeviceMotionEnabled; }
    bool ondevicemotionEnabled() const { return m_isDeviceMotionEnabled; }

    void setDeviceOrientationEnabled(bool isEnabled) { m_isDeviceOrientationEnabled = isEnabled; }
    bool deviceOrientationEnabled() const { return m_isDeviceOrientationEnabled; }
    bool deviceOrientationEventEnabled() const { return m_isDeviceOrientationEnabled; }
    bool ondeviceorientationEnabled() const { return m_isDeviceOrientationEnabled; }

    void setLinkPreloadEnabled(bool isEnabled) { m_isLinkPreloadEnabled = isEnabled; }
    bool linkPreloadEnabled() const { return m_isLinkPreloadEnabled; }

#if ENABLE(JAVASCRIPT_I18N_API)
    bool javaScriptI18NAPIEnabled() const;
    void setJavaScriptI18NAPIEnabled(bool isEnabled) { m_isJavaScriptI18NAPIEnabled = isEnabled; }
#endif

#if ENABLE(MEDIA_STREAM)
    bool mediaStreamEnabled() const { return m_isMediaStreamEnabled; }
    void setMediaStreamEnabled(bool isEnabled) { m_isMediaStreamEnabled = isEnabled; }
    bool webkitGetUserMediaEnabled() const { return m_isMediaStreamEnabled; }
    bool webkitMediaStreamEnabled() const { return m_isMediaStreamEnabled; }
#endif
#if ENABLE(WEB_RTC)
    bool peerConnectionEnabled() const { return m_isMediaStreamEnabled && m_isPeerConnectionEnabled; }
    void setPeerConnectionEnabled(bool isEnabled) { m_isPeerConnectionEnabled = isEnabled; }
    bool webkitRTCPeerConnectionEnabled() const { return peerConnectionEnabled(); }
#endif

#if ENABLE(LEGACY_CSS_VENDOR_PREFIXES)
    void setLegacyCSSVendorPrefixesEnabled(bool isEnabled) { m_isLegacyCSSVendorPrefixesEnabled = isEnabled; }
    bool legacyCSSVendorPrefixesEnabled() const { return m_isLegacyCSSVendorPrefixesEnabled; }
#endif

#if ENABLE(INPUT_TYPE_DATE)
    bool inputTypeDateEnabled() const { return m_isInputTypeDateEnabled; }
    void setInputTypeDateEnabled(bool isEnabled) { m_isInputTypeDateEnabled = isEnabled; }
#endif

#if ENABLE(INPUT_TYPE_DATETIME_INCOMPLETE)
    bool inputTypeDateTimeEnabled() const { return m_isInputTypeDateTimeEnabled; }
    void setInputTypeDateTimeEnabled(bool isEnabled) { m_isInputTypeDateTimeEnabled = isEnabled; }
#endif

#if ENABLE(INPUT_TYPE_DATETIMELOCAL)
    bool inputTypeDateTimeLocalEnabled() const { return m_isInputTypeDateTimeLocalEnabled; }
    void setInputTypeDateTimeLocalEnabled(bool isEnabled) { m_isInputTypeDateTimeLocalEnabled = isEnabled; }
#endif

#if ENABLE(INPUT_TYPE_MONTH)
    bool inputTypeMonthEnabled() const { return m_isInputTypeMonthEnabled; }
    void setInputTypeMonthEnabled(bool isEnabled) { m_isInputTypeMonthEnabled = isEnabled; }
#endif

#if ENABLE(INPUT_TYPE_TIME)
    bool inputTypeTimeEnabled() const { return m_isInputTypeTimeEnabled; }
    void setInputTypeTimeEnabled(bool isEnabled) { m_isInputTypeTimeEnabled = isEnabled; }
#endif

#if ENABLE(INPUT_TYPE_WEEK)
    bool inputTypeWeekEnabled() const { return m_isInputTypeWeekEnabled; }
    void setInputTypeWeekEnabled(bool isEnabled) { m_isInputTypeWeekEnabled = isEnabled; }
#endif

#if ENABLE(CSP_NEXT)
    bool experimentalContentSecurityPolicyFeaturesEnabled() const { return m_areExperimentalContentSecurityPolicyFeaturesEnabled; }
    void setExperimentalContentSecurityPolicyFeaturesEnabled(bool isEnabled) { m_areExperimentalContentSecurityPolicyFeaturesEnabled = isEnabled; }
#endif

    bool langAttributeAwareFormControlUIEnabled() const { return m_isLangAttributeAwareFormControlUIEnabled; }
    // The lang attribute support is incomplete and should only be turned on for tests.
    void setLangAttributeAwareFormControlUIEnabled(bool isEnabled) { m_isLangAttributeAwareFormControlUIEnabled = isEnabled; }

    void setPluginReplacementEnabled(bool isEnabled) { m_isPluginReplacementEnabled = isEnabled; }
    bool pluginReplacementEnabled() const { return m_isPluginReplacementEnabled; }

    void setResourceTimingEnabled(bool isEnabled) { m_isResourceTimingEnabled = isEnabled; }
    bool resourceTimingEnabled() const { return m_isResourceTimingEnabled; }

#if ENABLE(GAMEPAD)
    void setGamepadsEnabled(bool areEnabled) { m_areGamepadsEnabled = areEnabled; }
    bool gamepadsEnabled() const { return m_areGamepadsEnabled; }
#endif

#if ENABLE(CSS_ANIMATIONS_LEVEL_2)
    void setAnimationTriggersEnabled(bool areEnabled) { m_areAnimationTriggersEnabled = areEnabled; }
    bool animationTriggersEnabled() const { return m_areAnimationTriggersEnabled; }
#endif

#if ENABLE(WEB_ANIMATIONS)
    void setWebAnimationsEnabled(bool areEnabled) { m_areWebAnimationsEnabled = areEnabled; }
    bool webAnimationsEnabled() const { return m_areWebAnimationsEnabled; }
#endif

    void setShadowDOMEnabled(bool isEnabled) { m_isShadowDOMEnabled = isEnabled; }
    bool shadowDOMEnabled() const { return m_isShadowDOMEnabled; }

#if ENABLE(CUSTOM_ELEMENTS)
    void setCustomElementsEnabled(bool areEnabled) { m_areCustomElementsEnabled = areEnabled; }
    bool customElementsEnabled() const { return m_areCustomElementsEnabled; }
#endif

#if ENABLE(WEBGL2)
    void setWebGL2Enabled(bool isEnabled) { m_isWebGL2Enabled = isEnabled; }
    bool webGL2Enabled() const { return m_isWebGL2Enabled; }
#endif

#if ENABLE(FETCH_API)
    void setFetchAPIEnabled(bool isEnabled) { m_isFetchAPIEnabled = isEnabled; }
    bool fetchAPIEnabled() const { return m_isFetchAPIEnabled; }
#endif

#if ENABLE(DOWNLOAD_ATTRIBUTE)
    void setDownloadAttributeEnabled(bool isEnabled) { m_isDownloadAttributeEnabled = isEnabled; }
    bool downloadAttributeEnabled() const { return m_isDownloadAttributeEnabled; }
#endif

#if ENABLE(CSS_GRID_LAYOUT)
    void setCSSGridLayoutEnabled(bool isEnabled) { m_cssGridLayoutEnabled = isEnabled; }
    bool isCSSGridLayoutEnabled() const { return m_cssGridLayoutEnabled; }
#endif

    WEBCORE_EXPORT static RuntimeEnabledFeatures& sharedFeatures();

    WEBCORE_EXPORT void reset();

private:
    // Never instantiate.
    RuntimeEnabledFeatures();

    bool m_isLocalStorageEnabled;
    bool m_isSessionStorageEnabled;
    bool m_isWebkitNotificationsEnabled;
    bool m_isApplicationCacheEnabled;
    bool m_isDataTransferItemsEnabled;
    bool m_isDOMIteratorEnabled { true };
    bool m_isGeolocationEnabled;
    bool m_isTouchEnabled;
    bool m_isDeviceMotionEnabled;
    bool m_isDeviceOrientationEnabled;
    bool m_isLinkPreloadEnabled;
    bool m_isLangAttributeAwareFormControlUIEnabled;
    bool m_isPluginReplacementEnabled;
    bool m_isResourceTimingEnabled;

#if ENABLE(INDEXED_DATABASE)
    bool m_isIndexedDBEnabled;
#endif

#if ENABLE(INDEXED_DATABASE_IN_WORKERS)
    bool m_isIndexedDBWorkersEnabled;
#endif

#if ENABLE(JAVASCRIPT_I18N_API)
    bool m_isJavaScriptI18NAPIEnabled;
#endif

#if ENABLE(MEDIA_STREAM)
    bool m_isMediaStreamEnabled { true };
#endif
#if ENABLE(WEB_RTC)
    bool m_isPeerConnectionEnabled;
#endif

#if ENABLE(LEGACY_CSS_VENDOR_PREFIXES)
    bool m_isLegacyCSSVendorPrefixesEnabled;
#endif

#if ENABLE(INPUT_TYPE_DATE)
    bool m_isInputTypeDateEnabled;
#endif

#if ENABLE(INPUT_TYPE_DATETIME_INCOMPLETE)
    bool m_isInputTypeDateTimeEnabled;
#endif

#if ENABLE(INPUT_TYPE_DATETIMELOCAL)
    bool m_isInputTypeDateTimeLocalEnabled;
#endif

#if ENABLE(INPUT_TYPE_MONTH)
    bool m_isInputTypeMonthEnabled;
#endif

#if ENABLE(INPUT_TYPE_TIME)
    bool m_isInputTypeTimeEnabled;
#endif

#if ENABLE(INPUT_TYPE_WEEK)
    bool m_isInputTypeWeekEnabled;
#endif

#if ENABLE(CSP_NEXT)
    bool m_areExperimentalContentSecurityPolicyFeaturesEnabled;
#endif

#if ENABLE(FONT_LOAD_EVENTS)
    bool m_isFontLoadEventsEnabled;
#endif

#if ENABLE(GAMEPAD)
    bool m_areGamepadsEnabled { false };
#endif

#if ENABLE(CSS_ANIMATIONS_LEVEL_2)
    bool m_areAnimationTriggersEnabled;
#endif

#if ENABLE(WEB_ANIMATIONS)
    bool m_areWebAnimationsEnabled;
#endif
    
    bool m_isShadowDOMEnabled;

#if ENABLE(CUSTOM_ELEMENTS)
    bool m_areCustomElementsEnabled;
#endif

#if ENABLE(WEBGL2)
    bool m_isWebGL2Enabled;
#endif

#if ENABLE(FETCH_API)
    bool m_isFetchAPIEnabled { false };
#endif

#if ENABLE(DOWNLOAD_ATTRIBUTE)
    bool m_isDownloadAttributeEnabled { false };
#endif

#if ENABLE(CSS_GRID_LAYOUT)
    bool m_cssGridLayoutEnabled;
#endif

    friend class WTF::NeverDestroyed<RuntimeEnabledFeatures>;
};

} // namespace WebCore

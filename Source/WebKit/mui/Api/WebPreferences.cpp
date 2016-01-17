/*
 * Copyright (C) 2008 Pleyo.  All rights reserved.
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
 * 3.  Neither the name of Pleyo nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY PLEYO AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL PLEYO OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "BALBase.h"
#include "WebPreferences.h"
#include "WebPreferenceKeysPrivate.h"

#include <wtf/text/CString.h>
#include <FileSystem.h>
#include <Font.h>
#include <wtf/text/WTFString.h>
#include <PluginDatabase.h>
#include <wtf/text/StringHash.h>
#include "ObserverServiceData.h"

#include <wtf/HashMap.h>
#include <map>

#if USE(CURL)
#include "URL.h"
#include "ResourceHandleManager.h"
#include <CookieManager.h>
#endif

using namespace WebCore;
using namespace std;

static map<string, string> m_privatePrefs;
static map<string, WebPreferences*> webPreferencesInstances;

WebPreferences* WebPreferences::sharedStandardPreferences()
{
    static WebPreferences standardPreferences = *WebPreferences::createInitializedInstance();
    return &standardPreferences;
}

WebPreferences::WebPreferences()
    : m_autoSaves(0)
    , m_automaticallyDetectsCacheModel(true)
    , m_numWebViews(0)
{
}

WebPreferences::~WebPreferences()
{
}

WebPreferences* WebPreferences::createInitializedInstance()
{
    WebPreferences* instance = new WebPreferences();
    instance->setAutosaves(true);
    instance->load();
    return instance;
}

WebPreferences* WebPreferences::createInstance()
{
    return new WebPreferences();
}

void WebPreferences::postPreferencesChangesNotification()
{
    WebCore::ObserverServiceData::createObserverService()->notifyObserver(webPreferencesChangedNotification(), "", this);
}

WebPreferences* WebPreferences::getInstanceForIdentifier(const char* identifier)
{
    if (!identifier)
        return sharedStandardPreferences();

    return webPreferencesInstances[identifier];
}

void WebPreferences::setInstance(WebPreferences* instance, const char* identifier)
{
    if (!identifier || !instance)
        return;
    webPreferencesInstances[identifier] = instance;
}

void WebPreferences::removeReferenceForIdentifier(const char* identifier)
{
    if (!identifier || webPreferencesInstances.empty())
        return;

    WebPreferences* webPreference = webPreferencesInstances[identifier];
    if (webPreference)
        webPreferencesInstances.erase(identifier);
}

void WebPreferences::initializeDefaultSettings()
{
    m_privatePrefs[WebKitStandardFontPreferenceKey] = "Times New Roman";
    m_privatePrefs[WebKitFixedFontPreferenceKey] = "Courier New";
    m_privatePrefs[WebKitSerifFontPreferenceKey] = "Times New Roman";
    m_privatePrefs[WebKitSansSerifFontPreferenceKey] = "Arial";
    m_privatePrefs[WebKitCursiveFontPreferenceKey] = "Comic Sans MS";
    m_privatePrefs[WebKitFantasyFontPreferenceKey] = "Comic Sans MS";
    m_privatePrefs[WebKitPictographFontPreferenceKey] = "Times New Roman";
    m_privatePrefs[WebKitMinimumFontSizePreferenceKey] = "0";
    m_privatePrefs[WebKitMinimumLogicalFontSizePreferenceKey] = "9";
    m_privatePrefs[WebKitDefaultFontSizePreferenceKey] = "16";
    m_privatePrefs[WebKitDefaultFixedFontSizePreferenceKey] = "13";
    m_privatePrefs[WebKitDefaultTextEncodingNamePreferenceKey] = "ISO-8859-1";

    m_privatePrefs[WebKitUserStyleSheetEnabledPreferenceKey] = "1"; //TRUE
    m_privatePrefs[WebKitUserStyleSheetLocationPreferenceKey] = "file://PROGDIR:resource/userStyleSheet.css";
    m_privatePrefs[WebKitShouldPrintBackgroundsPreferenceKey] = "0"; //FALSE
    m_privatePrefs[WebKitTextAreasAreResizablePreferenceKey] = "1"; //TRUE
    m_privatePrefs[WebKitJavaEnabledPreferenceKey] = "1"; //TRUE
    m_privatePrefs[WebKitJavaScriptEnabledPreferenceKey] = "1"; //TRUE
    m_privatePrefs[WebKitWebSecurityEnabledPreferenceKey] = "1"; //TRUE
    m_privatePrefs[WebKitAllowUniversalAccessFromFileURLsPreferenceKey] = "1";//TRUE
    m_privatePrefs[WebKitAllowFileAccessFromFileURLsPreferenceKey] = "1";//TRUE
    m_privatePrefs[WebKitJavaScriptCanAccessClipboardPreferenceKey] = "1";//TRUE
    m_privatePrefs[WebKitXSSAuditorEnabledPreferenceKey] =  "0";//FALSE //*
    m_privatePrefs[WebKitFrameFlatteningEnabledPreferenceKey] =  "0";//FALSE
    m_privatePrefs[WebKitCustomDragCursorsEnabledPreferenceKey] = "0";//FALSE
    m_privatePrefs[WebKitJavaScriptCanOpenWindowsAutomaticallyPreferenceKey] = "1";//TRUE
    m_privatePrefs[WebKitPluginsEnabledPreferenceKey] = "1";//TRUE
    m_privatePrefs[WebKitCSSRegionsEnabledPreferenceKey] = "1";//TRUE

    m_privatePrefs[WebKitDatabasesEnabledPreferenceKey] = "1";
    m_privatePrefs[WebKitLocalStorageEnabledPreferenceKey] = "1";
#if ENABLE(NOTIFICATIONS)
    m_privatePrefs[WebKitExperimentalNotificationsEnabledPreferenceKey] = "1";
#else
    m_privatePrefs[WebKitExperimentalNotificationsEnabledPreferenceKey] = "0";
#endif

    m_privatePrefs[WebKitZoomsTextOnlyPreferenceKey] = "1";//TRUE
    m_privatePrefs[WebKitAllowAnimatedImagesPreferenceKey] = "1";//TRUE
    m_privatePrefs[WebKitAllowAnimatedImageLoopingPreferenceKey] = "1";//TRUE
    m_privatePrefs[WebKitDisplayImagesKey] = "1";//TRUE
    m_privatePrefs[WebKitLoadImagesKey] = "1";//TRUE
    m_privatePrefs[WebKitBackForwardCacheExpirationIntervalKey] = "1800";
    m_privatePrefs[WebKitTabToLinksPreferenceKey] = "1"; //TRUE
    m_privatePrefs[WebKitPrivateBrowsingEnabledPreferenceKey] = "0"; //FALSE
    m_privatePrefs[WebKitRespectStandardStyleKeyEquivalentsPreferenceKey] = "0";//FALSE
    m_privatePrefs[WebKitShowsURLsInToolTipsPreferenceKey] = "1";
    m_privatePrefs[WebKitPDFDisplayModePreferenceKey] = "1";
    m_privatePrefs[WebKitPDFScaleFactorPreferenceKey] = "0";
    m_privatePrefs[WebKitEditableLinkBehaviorPreferenceKey] = "0";
    m_privatePrefs[WebKitEditableLinkBehaviorPreferenceKey] = String::number(WebKitEditableLinkDefaultBehavior).latin1().data();
    m_privatePrefs[WebKitRequestAnimationFrameEnabledPreferenceKey] = "1";
    m_privatePrefs[WebKitHistoryItemLimitKey] = "10";
    m_privatePrefs[WebKitHistoryAgeInDaysLimitKey] = "7";
    m_privatePrefs[WebKitIconDatabaseLocationKey] = "PROGDIR:conf";
    m_privatePrefs[WebKitIconDatabaseEnabledPreferenceKey] = "1";
    m_privatePrefs[WebKitFontSmoothingTypePreferenceKey] = "2";
    m_privatePrefs[WebKitFontSmoothingContrastPreferenceKey] = "2";
    m_privatePrefs[WebKitCookieStorageAcceptPolicyPreferenceKey] = "2";
    m_privatePrefs[WebContinuousSpellCheckingEnabledPreferenceKey] = "0";
    m_privatePrefs[WebGrammarCheckingEnabledPreferenceKey] = "1";
    m_privatePrefs[AllowContinuousSpellCheckingPreferenceKey] = "1";
    m_privatePrefs[WebKitUsesPageCachePreferenceKey] = "1";
    m_privatePrefs[WebKitLocalStorageDatabasePathPreferenceKey] = "PROGDIR:Conf/LocalStorage";


    m_privatePrefs[WebKitCacheModelPreferenceKey] = String::number(WebCacheModelDocumentBrowser).latin1().data();
    m_privatePrefs[WebKitDOMPasteAllowedPreferenceKey] = "1";//TRUE

    m_privatePrefs[WebKitAuthorAndUserStylesEnabledPreferenceKey] = "1";//TRUE
    m_privatePrefs[WebKitApplicationChromeModePreferenceKey] = "0";//FALSE

    m_privatePrefs[WebKitOfflineWebApplicationCacheEnabledPreferenceKey] = "1";
    m_privatePrefs[WebKitPaintNativeControlsPreferenceKey] = "1";
    m_privatePrefs[WebKitUseHighResolutionTimersPreferenceKey] = "1"; // TRUE
    m_privatePrefs[WebKitWebGLEnabledPreferenceKey] = "0";
    m_privatePrefs[WebKitDNSPrefetchingEnabledPreferenceKey] = "0";
    m_privatePrefs[WebKitMemoryInfoEnabledPreferenceKey] = "0";
    m_privatePrefs[WebKitHyperlinkAuditingEnabledPreferenceKey] = "1";
    m_privatePrefs[WebKitAcceleratedCompositingEnabledPreferenceKey] = "0";
    m_privatePrefs[WebKitShowDebugBordersPreferenceKey] = "0"; // FALSE
    m_privatePrefs[WebKitMemoryLimitPreferenceKey] = "0";
    m_privatePrefs[WebKitAllowScriptsToCloseWindowsPreferenceKey] = "1"; // TRUE
    m_privatePrefs[WebKitSpatialNavigationEnabledPreferenceKey] = "0"; // FALSE

    m_privatePrefs[WebKitHixie76WebSocketProtocolEnabledPreferenceKey] = "0";
    m_privatePrefs[WebKitShouldInvertColorsPreferenceKey] = "0";

#if ENABLE(VIDEO_TRACK)
    m_privatePrefs[WebKitShouldDisplaySubtitlesPreferenceKey] = "1";
    m_privatePrefs[WebKitShouldDisplayCaptionsPreferenceKey] = "1";
    m_privatePrefs[WebKitShouldDisplayTextDescriptionsPreferenceKey] = "1";
#endif
}

#if 0
void WebPreferences::addCertificateInfo(const char* url, const char* certificatePath, const char* keyPath, const char* keyPassword)
{
#if USE(CURL)
    ResourceHandleManager::sharedInstance()->certificateCache().add(KURL(KURL(), url), certificatePath, keyPath, keyPassword);
#endif // USE(CURL)
}

void WebPreferences::clearCertificateInfo(const char* url)
{
#if USE(CURL)
    ResourceHandleManager::sharedInstance()->certificateCache().remove(KURL(KURL(), url));
#endif // USE(CURL)
}

void WebPreferences::clearAllCertificatesInfo()
{
#if USE(CURL)
    ResourceHandleManager::sharedInstance()->certificateCache().clear();
#endif // USE(CURL)
}
#endif

const char* WebPreferences::valueForKey(const char* key)
{
    string value = m_privatePrefs[key];
    return value.c_str();
}

void WebPreferences::setValueForKey(const char* key, const char* value)
{
    m_privatePrefs[key] = value;
}

const char* WebPreferences::stringValueForKey(const char* key)
{
    string value = m_privatePrefs[key];
    return value.c_str();
}

int WebPreferences::integerValueForKey(const char* key)
{
    String value(m_privatePrefs[key].c_str());
    return value.toInt();
}

bool WebPreferences::boolValueForKey(const char* key)
{
    String value(m_privatePrefs[key].c_str());
    return (bool)value.toInt();
}

float WebPreferences::floatValueForKey(const char* key)
{
    String value(m_privatePrefs[key].c_str());
    return value.toFloat();
}

unsigned int WebPreferences::longlongValueForKey(const char* key)
{
    String value(m_privatePrefs[key].c_str());
    return value.toUInt();
}

void WebPreferences::setStringValue(const char* key, const char* value)
{
    m_privatePrefs[key] = value;

    postPreferencesChangesNotification();
}

void WebPreferences::setIntegerValue(const char* key, int value)
{
    m_privatePrefs[key] = String::number(value).latin1().data();

    postPreferencesChangesNotification();
}

void WebPreferences::setBoolValue(const char* key, bool value)
{
    m_privatePrefs[key] = String::number(value).latin1().data();

    postPreferencesChangesNotification();
}

void WebPreferences::setLongLongValue(const char* key, unsigned int value)
{
    m_privatePrefs[key] = String::number(value).latin1().data();

    postPreferencesChangesNotification();
}

void WebPreferences::setFloatValue(const char* key, float value)
{
    m_privatePrefs[key] = String::number(value).latin1().data();

    postPreferencesChangesNotification();
}



const char* WebPreferences::webPreferencesChangedNotification()
{
    return "webPreferencesChangedNotification";
}

const char* WebPreferences::webPreferencesRemovedNotification()
{
    return "webPreferencesRemovedNotification";
}

void WebPreferences::save()
{
}

void WebPreferences::load()
{
    initializeDefaultSettings();
}

WebPreferences* WebPreferences::standardPreferences()
{
    return sharedStandardPreferences();
}

WebPreferences* WebPreferences::initWithIdentifier(const char* anIdentifier)
{
    WebPreferences *instance = getInstanceForIdentifier(anIdentifier);
    if (instance) {
        return instance;
    }

    load();


    m_identifier = anIdentifier;
    setInstance(this, m_identifier.c_str());

    this->postPreferencesChangesNotification();

    return this;
}

const char* WebPreferences::identifier()
{
    return m_identifier.c_str();
}

const char* WebPreferences::standardFontFamily()
{
    return stringValueForKey(WebKitStandardFontPreferenceKey);
}

void WebPreferences::setStandardFontFamily(const char* family)
{
    setStringValue(WebKitStandardFontPreferenceKey, family);
}

const char* WebPreferences::fixedFontFamily()
{
    return stringValueForKey(WebKitFixedFontPreferenceKey);
}

void WebPreferences::setFixedFontFamily(const char* family)
{
    setStringValue(WebKitFixedFontPreferenceKey, family);
}

const char* WebPreferences::serifFontFamily()
{
    return stringValueForKey(WebKitSerifFontPreferenceKey);
}

void WebPreferences::setSerifFontFamily(const char* family)
{
    setStringValue(WebKitSerifFontPreferenceKey, family);
}

const char* WebPreferences::sansSerifFontFamily()
{
    return stringValueForKey(WebKitSansSerifFontPreferenceKey);
}

void WebPreferences::setSansSerifFontFamily(const char* family)
{
    setStringValue(WebKitSansSerifFontPreferenceKey, family);
}

const char* WebPreferences::cursiveFontFamily()
{
    return stringValueForKey(WebKitCursiveFontPreferenceKey);
}

void WebPreferences::setCursiveFontFamily(const char* family)
{
    setStringValue(WebKitCursiveFontPreferenceKey, family);
}

const char* WebPreferences::fantasyFontFamily()
{
    return stringValueForKey(WebKitFantasyFontPreferenceKey);
}

void WebPreferences::setFantasyFontFamily(const char* family)
{
    setStringValue(WebKitFantasyFontPreferenceKey, family);
}

const char* WebPreferences::pictographFontFamily()
{
	return stringValueForKey(WebKitPictographFontPreferenceKey);
}

void WebPreferences::setPictographFontFamily(const char* family)
{
	setStringValue(WebKitPictographFontPreferenceKey, family);
}

int WebPreferences::defaultFontSize()
{
    return integerValueForKey(WebKitDefaultFontSizePreferenceKey);
}

void WebPreferences::setDefaultFontSize(int fontSize)
{
    setIntegerValue(WebKitDefaultFontSizePreferenceKey, fontSize);
}

int WebPreferences::defaultFixedFontSize()
{
    return integerValueForKey(WebKitDefaultFixedFontSizePreferenceKey);
}

void WebPreferences::setDefaultFixedFontSize(int fontSize)
{
    setIntegerValue(WebKitDefaultFixedFontSizePreferenceKey, fontSize);
}

int WebPreferences::minimumFontSize()
{
    return integerValueForKey(WebKitMinimumFontSizePreferenceKey);
}

void WebPreferences::setMinimumFontSize(int fontSize)
{
    setIntegerValue(WebKitMinimumFontSizePreferenceKey, fontSize);
}

int WebPreferences::minimumLogicalFontSize()
{
    return integerValueForKey(WebKitMinimumLogicalFontSizePreferenceKey);
}

void WebPreferences::setMinimumLogicalFontSize(int fontSize)
{
    setIntegerValue(WebKitMinimumLogicalFontSizePreferenceKey, fontSize);
}

const char* WebPreferences::defaultTextEncodingName()
{
    return stringValueForKey(WebKitDefaultTextEncodingNamePreferenceKey);
}

void WebPreferences::setDefaultTextEncodingName(const char* name)
{
    setStringValue(WebKitDefaultTextEncodingNamePreferenceKey, name);
}

bool WebPreferences::allowUniversalAccessFromFileURLs()
{
    return boolValueForKey(WebKitAllowUniversalAccessFromFileURLsPreferenceKey);
}

void WebPreferences::setAllowUniversalAccessFromFileURLs(bool allowAccess)
{
    setBoolValue(WebKitAllowUniversalAccessFromFileURLsPreferenceKey, allowAccess);
}

bool WebPreferences::allowFileAccessFromFileURLs()
{
    return boolValueForKey(WebKitAllowFileAccessFromFileURLsPreferenceKey);
}

void WebPreferences::setAllowFileAccessFromFileURLs(bool allowAccess)
{
    setBoolValue(WebKitAllowFileAccessFromFileURLsPreferenceKey, allowAccess);
}

bool WebPreferences::userStyleSheetEnabled()
{
    return boolValueForKey(WebKitUserStyleSheetEnabledPreferenceKey);
}

void WebPreferences::setUserStyleSheetEnabled(bool enabled)
{
    setBoolValue(WebKitUserStyleSheetEnabledPreferenceKey, enabled);
}

const char* WebPreferences::userStyleSheetLocation()
{
    return stringValueForKey(WebKitUserStyleSheetLocationPreferenceKey);
}

void WebPreferences::setUserStyleSheetLocation(const char* location)
{
    setStringValue(WebKitUserStyleSheetLocationPreferenceKey, location);
}

bool WebPreferences::isJavaEnabled()
{
    return boolValueForKey(WebKitJavaEnabledPreferenceKey);
}

void WebPreferences::setJavaEnabled(bool enabled)
{
    setBoolValue(WebKitJavaEnabledPreferenceKey, enabled);
}

bool WebPreferences::isJavaScriptEnabled()
{
    return boolValueForKey(WebKitJavaScriptEnabledPreferenceKey);
}

void WebPreferences::setJavaScriptEnabled(bool enabled)
{
    setBoolValue(WebKitJavaScriptEnabledPreferenceKey, enabled);
}

bool WebPreferences::isWebSecurityEnabled()
{
    return boolValueForKey(WebKitWebSecurityEnabledPreferenceKey);
}

void WebPreferences::setWebSecurityEnabled(bool enabled)
{
    setBoolValue(WebKitWebSecurityEnabledPreferenceKey, enabled);
}

bool WebPreferences::javaScriptCanAccessClipboard()
{
    return boolValueForKey(WebKitJavaScriptCanAccessClipboardPreferenceKey);
}

void WebPreferences::setJavaScriptCanAccessClipboard(bool enabled)
{
    setBoolValue(WebKitJavaScriptCanAccessClipboardPreferenceKey, enabled);
}

bool WebPreferences::isXSSAuditorEnabled()
{
    return boolValueForKey(WebKitXSSAuditorEnabledPreferenceKey);
}

void WebPreferences::setXSSAuditorEnabled(bool enabled)
{
    setBoolValue(WebKitXSSAuditorEnabledPreferenceKey, enabled);
}

void WebPreferences::setShouldUseHighResolutionTimers(bool useHighResolutionTimers)
{
    setBoolValue(WebKitUseHighResolutionTimersPreferenceKey, useHighResolutionTimers);
}

bool WebPreferences::shouldUseHighResolutionTimers()
{
    return boolValueForKey(WebKitUseHighResolutionTimersPreferenceKey);
}

bool WebPreferences::isFrameFlatteningEnabled()
{
    return boolValueForKey(WebKitFrameFlatteningEnabledPreferenceKey);
}

void WebPreferences::setFrameFlatteningEnabled(bool enabled)
{
    setBoolValue(WebKitFrameFlatteningEnabledPreferenceKey, enabled);
}

void WebPreferences::setPreferenceForTest(const char* key, const char* value)
{ 
    if (!key || !value)
        return ; 
    setValueForKey(key, value); 
    postPreferencesChangesNotification();
}

bool WebPreferences::showDebugBorders()
{
    return boolValueForKey(WebKitShowDebugBordersPreferenceKey);
}

void WebPreferences::setShowDebugBorders(bool enabled)
{
    setBoolValue(WebKitShowDebugBordersPreferenceKey, enabled);
}

bool WebPreferences::showRepaintCounter()
{
    return boolValueForKey(WebKitShowRepaintCounterPreferenceKey);
}

void WebPreferences::setShowRepaintCounter(bool enabled)
{
    setBoolValue(WebKitShowRepaintCounterPreferenceKey, enabled);
}

void WebPreferences::setCustomDragCursorsEnabled(bool enabled)
{
    setBoolValue(WebKitCustomDragCursorsEnabledPreferenceKey, enabled);
}
    
bool WebPreferences::customDragCursorsEnabled()
{
    return boolValueForKey(WebKitCustomDragCursorsEnabledPreferenceKey);
}

bool WebPreferences::javaScriptCanOpenWindowsAutomatically()
{
    return boolValueForKey(WebKitJavaScriptCanOpenWindowsAutomaticallyPreferenceKey);
}

void WebPreferences::setJavaScriptCanOpenWindowsAutomatically(bool enabled)
{
    setBoolValue(WebKitJavaScriptCanOpenWindowsAutomaticallyPreferenceKey, enabled);
}

bool WebPreferences::arePlugInsEnabled()
{
    return boolValueForKey(WebKitPluginsEnabledPreferenceKey);
}

void WebPreferences::setPlugInsEnabled(bool enabled)
{
    setBoolValue(WebKitPluginsEnabledPreferenceKey, enabled);
}

bool WebPreferences::isCSSRegionsEnabled()
{
  return boolValueForKey(WebKitCSSRegionsEnabledPreferenceKey);
}

void WebPreferences::setCSSRegionsEnabled(bool enabled)
{
  setBoolValue(WebKitCSSRegionsEnabledPreferenceKey, enabled);
}


bool WebPreferences::areImagesEnabled()
{
	return boolValueForKey(WebKitDisplayImagesKey);
}

void WebPreferences::setImagesEnabled(bool enabled)
{
	setBoolValue(WebKitDisplayImagesKey, enabled);
}

bool WebPreferences::allowsAnimatedImages()
{
    return boolValueForKey(WebKitAllowAnimatedImagesPreferenceKey);
}

void WebPreferences::setAllowsAnimatedImages(bool enabled)
{
    setBoolValue(WebKitAllowAnimatedImagesPreferenceKey, enabled);
}

bool WebPreferences::allowAnimatedImageLooping()
{
    return boolValueForKey(WebKitAllowAnimatedImageLoopingPreferenceKey);
}

void WebPreferences::setAllowAnimatedImageLooping(bool enabled)
{
    setBoolValue(WebKitAllowAnimatedImageLoopingPreferenceKey, enabled);
}

void WebPreferences::setLoadsImagesAutomatically(bool enabled)
{
	setBoolValue(WebKitLoadImagesKey, enabled);
}

bool WebPreferences::loadsImagesAutomatically()
{
	return boolValueForKey(WebKitLoadImagesKey);
}

void WebPreferences::setAutosaves(bool enabled)
{
    m_autoSaves = !!enabled;
}

bool WebPreferences::autosaves()
{
    return m_autoSaves ? true : false;
}

void WebPreferences::setShouldPrintBackgrounds(bool enabled)
{
    setBoolValue(WebKitShouldPrintBackgroundsPreferenceKey, enabled);
}

bool WebPreferences::shouldPrintBackgrounds()
{
    return boolValueForKey(WebKitShouldPrintBackgroundsPreferenceKey);
}

void WebPreferences::setPrivateBrowsingEnabled(bool enabled)
{
    setBoolValue(WebKitPrivateBrowsingEnabledPreferenceKey, enabled);
}

bool WebPreferences::privateBrowsingEnabled()
{
    return boolValueForKey(WebKitPrivateBrowsingEnabledPreferenceKey);
}

void WebPreferences::setTabsToLinks(bool enabled)
{
    setBoolValue(WebKitTabToLinksPreferenceKey, enabled);
}

bool WebPreferences::tabsToLinks()
{
    return boolValueForKey(WebKitTabToLinksPreferenceKey);
}

void WebPreferences::setUsesPageCache(bool usesPageCache)
{
    setBoolValue(WebKitUsesPageCachePreferenceKey, usesPageCache);
}

bool WebPreferences::usesPageCache()
{
    return boolValueForKey(WebKitUsesPageCachePreferenceKey);
}

bool WebPreferences::textAreasAreResizable()
{
    return boolValueForKey(WebKitTextAreasAreResizablePreferenceKey);
}

void WebPreferences::setTextAreasAreResizable(bool enabled)
{
    setBoolValue(WebKitTextAreasAreResizablePreferenceKey, enabled);
}

int WebPreferences::historyItemLimit()
{
    return integerValueForKey(WebKitHistoryItemLimitKey);
}

void WebPreferences::setHistoryItemLimit(int limit)
{
    setIntegerValue(WebKitHistoryItemLimitKey, limit);
}

int WebPreferences::historyAgeInDaysLimit()
{
    return integerValueForKey(WebKitHistoryAgeInDaysLimitKey);
}

void WebPreferences::setHistoryAgeInDaysLimit(int limit)
{
    setIntegerValue(WebKitHistoryAgeInDaysLimitKey, limit);
}

const char* WebPreferences::iconDatabaseLocation()
{
    return stringValueForKey(WebKitIconDatabaseLocationKey);
}

void WebPreferences::setIconDatabaseLocation(const char* location)
{
    setStringValue(WebKitIconDatabaseLocationKey, location);
}

bool WebPreferences::iconDatabaseEnabled()
{
    return boolValueForKey(WebKitIconDatabaseEnabledPreferenceKey);
}

void WebPreferences::setIconDatabaseEnabled(bool enabled)
{
    setBoolValue(WebKitIconDatabaseEnabledPreferenceKey, enabled);
}

FontSmoothingType WebPreferences::fontSmoothing()
{
    return (FontSmoothingType) integerValueForKey(WebKitFontSmoothingTypePreferenceKey);
}

void WebPreferences::setFontSmoothing(FontSmoothingType smoothingType)
{
    setIntegerValue(WebKitFontSmoothingTypePreferenceKey, smoothingType);
}

WebKitEditableLinkBehavior WebPreferences::editableLinkBehavior()
{
    WebKitEditableLinkBehavior value = (WebKitEditableLinkBehavior) integerValueForKey(WebKitEditableLinkBehaviorPreferenceKey);
    switch (value) {
        case WebKitEditableLinkDefaultBehavior:
        case WebKitEditableLinkAlwaysLive:
        case WebKitEditableLinkOnlyLiveWithShiftKey:
        case WebKitEditableLinkLiveWhenNotFocused:
        case WebKitEditableLinkNeverLive:
            return value;
        default: // ensure that a valid result is returned
            return WebKitEditableLinkDefaultBehavior;
    }
}

void WebPreferences::setEditableLinkBehavior(WebKitEditableLinkBehavior behavior)
{
    setIntegerValue(WebKitEditableLinkBehaviorPreferenceKey, behavior);
}

void WebPreferences::setRequestAnimationFrameEnabled(bool enabled)
{
  setBoolValue(WebKitRequestAnimationFrameEnabledPreferenceKey, enabled);
}

bool WebPreferences::requestAnimationFrameEnabled()
{
  return boolValueForKey(WebKitRequestAnimationFrameEnabledPreferenceKey);
}


WebKitCookieStorageAcceptPolicy WebPreferences::cookieStorageAcceptPolicy()
{
    return (WebKitCookieStorageAcceptPolicy)integerValueForKey(WebKitCookieStorageAcceptPolicyPreferenceKey);
}

void WebPreferences::setCookieStorageAcceptPolicy(WebKitCookieStorageAcceptPolicy acceptPolicy)
{
    setIntegerValue(WebKitCookieStorageAcceptPolicyPreferenceKey, acceptPolicy);
#if USE(CURL)
    cookieManager().setCookiePolicy((WebCore::CookieStorageAcceptPolicy)acceptPolicy);
#endif
}

void WebPreferences::setDNSPrefetchingEnabled(bool enabled)
{
    setBoolValue(WebKitDNSPrefetchingEnabledPreferenceKey, enabled);
}

bool WebPreferences::DNSPrefetchingEnabled()
{
    return boolValueForKey(WebKitDNSPrefetchingEnabledPreferenceKey);
}

void WebPreferences::setMemoryInfoEnabled(bool enabled)
{
    setBoolValue(WebKitMemoryInfoEnabledPreferenceKey, enabled);
}

bool WebPreferences::memoryInfoEnabled()
{
    return boolValueForKey(WebKitMemoryInfoEnabledPreferenceKey);
}

void WebPreferences::setHyperlinkAuditingEnabled(bool enabled)
{
    setBoolValue(WebKitHyperlinkAuditingEnabledPreferenceKey, enabled);
}

bool WebPreferences::hyperlinkAuditingEnabled()
{
    return boolValueForKey(WebKitHyperlinkAuditingEnabledPreferenceKey);
}

void WebPreferences::setShouldInvertColors(bool enabled)
{
    setBoolValue(WebKitShouldInvertColorsPreferenceKey, enabled);
}

bool WebPreferences::shouldInvertColors()
{
    return boolValueForKey(WebKitShouldInvertColorsPreferenceKey);
}

void WebPreferences::setHixie76WebSocketProtocolEnabled(bool enabled)
{
    setBoolValue(WebKitHixie76WebSocketProtocolEnabledPreferenceKey, enabled);
}

bool WebPreferences::hixie76WebSocketProtocolEnabled()
{
    return boolValueForKey(WebKitHixie76WebSocketProtocolEnabledPreferenceKey);
}

bool WebPreferences::continuousSpellCheckingEnabled()
{
    return boolValueForKey(WebContinuousSpellCheckingEnabledPreferenceKey);
}

void WebPreferences::setContinuousSpellCheckingEnabled(bool enabled)
{
    setBoolValue(WebContinuousSpellCheckingEnabledPreferenceKey, enabled);
}

bool WebPreferences::grammarCheckingEnabled()
{
    return boolValueForKey(WebGrammarCheckingEnabledPreferenceKey);
}

void WebPreferences::setGrammarCheckingEnabled(bool enabled)
{
    setBoolValue(WebGrammarCheckingEnabledPreferenceKey, enabled);
}

bool WebPreferences::allowContinuousSpellChecking()
{
    return boolValueForKey(AllowContinuousSpellCheckingPreferenceKey);
}

void WebPreferences::setAllowContinuousSpellChecking(bool enabled)
{
    setBoolValue(AllowContinuousSpellCheckingPreferenceKey, enabled);
}

bool WebPreferences::areSeamlessIFramesEnabled()
{
    return boolValueForKey(SeamlessIFramesPreferenceKey);
}

void WebPreferences::setSeamlessIFramesEnabled(bool enabled)
{
    setBoolValue(SeamlessIFramesPreferenceKey, enabled);
}

bool WebPreferences::isDOMPasteAllowed()
{
    return boolValueForKey(WebKitDOMPasteAllowedPreferenceKey);
}

void WebPreferences::setDOMPasteAllowed(bool enabled)
{
    setBoolValue(WebKitDOMPasteAllowedPreferenceKey, enabled);
}

WebCacheModel WebPreferences::cacheModel()
{
    return (WebCacheModel)integerValueForKey(WebKitCacheModelPreferenceKey);
}

void WebPreferences::setCacheModel(WebCacheModel cacheModel)
{
    setIntegerValue(WebKitCacheModelPreferenceKey, cacheModel);
}

void WebPreferences::setDeveloperExtrasEnabled(bool enabled)
{
    setBoolValue(WebKitDeveloperExtrasEnabledPreferenceKey, enabled);
}

bool WebPreferences::developerExtrasEnabled()
{
    return boolValueForKey(WebKitDeveloperExtrasEnabledPreferenceKey);
}

bool WebPreferences::developerExtrasDisabledByOverride()
{
    return !!boolValueForKey(DisableWebKitDeveloperExtrasPreferenceKey);
}

void WebPreferences::setAutomaticallyDetectsCacheModel(bool automaticallyDetectsCacheModel)
{
    m_automaticallyDetectsCacheModel = !!automaticallyDetectsCacheModel;
}

bool WebPreferences::automaticallyDetectsCacheModel()
{
    return m_automaticallyDetectsCacheModel;
}

void WebPreferences::setAuthorAndUserStylesEnabled(bool enabled)
{
    setBoolValue(WebKitAuthorAndUserStylesEnabledPreferenceKey, enabled);
}

bool WebPreferences::authorAndUserStylesEnabled()
{
    return boolValueForKey(WebKitAuthorAndUserStylesEnabledPreferenceKey);
}

bool WebPreferences::inApplicationChromeMode()
{
    return boolValueForKey(WebKitApplicationChromeModePreferenceKey);
}

void WebPreferences::setApplicationChromeMode(bool enabled)
{
    setBoolValue(WebKitApplicationChromeModePreferenceKey, enabled);
}


void WebPreferences::setZoomsTextOnly(bool zoomsTextOnly)
{
    setBoolValue(WebKitZoomsTextOnlyPreferenceKey, zoomsTextOnly);
}

bool WebPreferences::zoomsTextOnly()
{
    return boolValueForKey(WebKitZoomsTextOnlyPreferenceKey);
}

void WebPreferences::willAddToWebView()
{
    ++m_numWebViews;
}

void WebPreferences::didRemoveFromWebView()
{
    ASSERT(m_numWebViews);
    if (--m_numWebViews == 0)
        WebCore::ObserverServiceData::createObserverService()->notifyObserver(webPreferencesRemovedNotification(), "", this);
}

void WebPreferences::setOfflineWebApplicationCacheEnabled(bool enabled)
{
    setBoolValue(WebKitOfflineWebApplicationCacheEnabledPreferenceKey, enabled);
}

bool WebPreferences::offlineWebApplicationCacheEnabled()
{
    return boolValueForKey(WebKitOfflineWebApplicationCacheEnabledPreferenceKey);
}

void WebPreferences::setDatabasesEnabled(bool enabled)
{
    setBoolValue(WebKitDatabasesEnabledPreferenceKey, enabled);
}

bool WebPreferences::databasesEnabled()
{
    return boolValueForKey(WebKitDatabasesEnabledPreferenceKey);
}

void WebPreferences::setExperimentalNotificationsEnabled(bool enable)
{
    setBoolValue(WebKitExperimentalNotificationsEnabledPreferenceKey, enable);
}

bool WebPreferences::experimentalNotificationsEnabled()
{
    return boolValueForKey(WebKitExperimentalNotificationsEnabledPreferenceKey);
}

void WebPreferences::setWebGLEnabled(bool enable)
{
    setBoolValue(WebKitWebGLEnabledPreferenceKey, enable);
}

bool WebPreferences::webGLEnabled()
{
    return boolValueForKey(WebKitWebGLEnabledPreferenceKey);
}

void WebPreferences::setAcceleratedCompositingEnabled(bool enable)
{
    setBoolValue(WebKitAcceleratedCompositingEnabledPreferenceKey, enable);
}

bool WebPreferences::acceleratedCompositingEnabled()
{
    return boolValueForKey(WebKitAcceleratedCompositingEnabledPreferenceKey);
}

void WebPreferences::setLocalStorageEnabled(bool enabled)
{
    setBoolValue(WebKitLocalStorageEnabledPreferenceKey, enabled);
}

bool WebPreferences::localStorageEnabled()
{
    return boolValueForKey(WebKitLocalStorageEnabledPreferenceKey);
}

const char* WebPreferences::localStorageDatabasePath()
{
    return stringValueForKey(WebKitLocalStorageDatabasePathPreferenceKey);
}

void WebPreferences::setLocalStorageDatabasePath(const char* location)
{
    setStringValue(WebKitLocalStorageDatabasePathPreferenceKey, location);
}

bool WebPreferences::shouldPaintNativeControls()
{
    return boolValueForKey(WebKitPaintNativeControlsPreferenceKey);
}

void WebPreferences::setShouldPaintNativeControls(bool shouldPaint)
{
    setBoolValue(WebKitPaintNativeControlsPreferenceKey, shouldPaint);
}

float WebPreferences::fontSmoothingContrast()
{
    return floatValueForKey(WebKitFontSmoothingContrastPreferenceKey);
}

void WebPreferences::setFontSmoothingContrast(float contrast)
{
    setFloatValue(WebKitFontSmoothingContrastPreferenceKey, contrast);
}

void WebPreferences::addExtraPluginDirectory(const char* directory)
{
    PluginDatabase::installedPlugins()->addExtraPluginDirectory(directory);
}


void WebPreferences::setMemoryLimit(int limit)
{
    setIntegerValue(WebKitMemoryLimitPreferenceKey, limit);
}

int WebPreferences::memoryLimit()
{
    return integerValueForKey(WebKitMemoryLimitPreferenceKey);
}

void WebPreferences::setAllowScriptsToCloseWindows(bool allowed)
{
    setBoolValue(WebKitAllowScriptsToCloseWindowsPreferenceKey, allowed);
}

bool WebPreferences::allowScriptsToCloseWindows()
{
    return boolValueForKey(WebKitAllowScriptsToCloseWindowsPreferenceKey);
}

bool WebPreferences::spatialNavigationEnabled()
{
    return boolValueForKey(WebKitSpatialNavigationEnabledPreferenceKey);
}

void WebPreferences::setSpatialNavigationEnabled(bool enabled)
{
    setBoolValue(WebKitSpatialNavigationEnabledPreferenceKey, enabled);
}

void WebPreferences::setMediaPlaybackRequiresUserGesture(bool enabled)
{
	setBoolValue(WebKitMediaPlaybackRequiresUserGesturePreferenceKey, enabled);
}

bool WebPreferences::mediaPlaybackRequiresUserGesture()
{
	return boolValueForKey(WebKitMediaPlaybackRequiresUserGesturePreferenceKey);
}

void WebPreferences::setMediaPlaybackAllowsInline(bool enabled)
{
	setBoolValue(WebKitMediaPlaybackAllowsInlinePreferenceKey, enabled);
}

bool WebPreferences::mediaPlaybackAllowsInline()
{
	return boolValueForKey(WebKitMediaPlaybackAllowsInlinePreferenceKey);
}


void WebPreferences::setShouldDisplaySubtitles(bool enabled)
{
	setBoolValue(WebKitShouldDisplaySubtitlesPreferenceKey, enabled);
}

bool WebPreferences::shouldDisplaySubtitles()
{
	return boolValueForKey(WebKitShouldDisplaySubtitlesPreferenceKey);
}

void WebPreferences::setShouldDisplayCaptions(bool enabled)
{
	setBoolValue(WebKitShouldDisplayCaptionsPreferenceKey, enabled);
}

bool WebPreferences::shouldDisplayCaptions()
{
	return boolValueForKey(WebKitShouldDisplayCaptionsPreferenceKey);
}

void WebPreferences::setShouldDisplayTextDescriptions(bool enabled)
{
	setBoolValue(WebKitShouldDisplayTextDescriptionsPreferenceKey, enabled);
}

bool WebPreferences::shouldDisplayTextDescriptions()
{
	return boolValueForKey(WebKitShouldDisplayTextDescriptionsPreferenceKey);
}

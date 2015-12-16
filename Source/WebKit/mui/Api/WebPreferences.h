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

#ifndef WebPreferences_H
#define WebPreferences_H


/**
 *  @file  WebPreferences.h
 *  WebPreferences 
 *  Repository informations :
 * - $URL$
 * - $Rev$
 * - $Date: 2013/03/06 00:13:17 $
 */
#include "WebKitTypes.h"
#include <string>

typedef enum {
    WebKitEditableLinkDefaultBehavior,
    WebKitEditableLinkAlwaysLive,
    WebKitEditableLinkOnlyLiveWithShiftKey,
    WebKitEditableLinkLiveWhenNotFocused,
    WebKitEditableLinkNeverLive
} WebKitEditableLinkBehavior;

typedef enum WebKitEditingBehavior {
    WebKitEditingMacBehavior = 0,
    WebKitEditingWinBehavior,
    WebKitEditingUnixBehavior
} WebKitEditingBehavior;

typedef enum WebKitCookieStorageAcceptPolicy {
    WebKitCookieStorageAcceptPolicyAlways = 0,
    WebKitCookieStorageAcceptPolicyNever,
    WebKitCookieStorageAcceptPolicyOnlyFromMainDocumentDomain
} WebKitCookieStorageAcceptPolicy;

typedef enum FontSmoothingType {
    FontSmoothingTypeStandard=0,
    FontSmoothingTypeLight,
    FontSmoothingTypeMedium,
    FontSmoothingTypeStrong,
    FontSmoothingTypeWindows
} FontSmoothingType;


class WEBKIT_OWB_API WebPreferences {
public:

    /**
     * Create an new instance of WebPreferences
     */
    static WebPreferences* createInstance();

    /**
     * Create an new instance of WebPreferences
     * that is filled with default values.
     */
    static WebPreferences* createInitializedInstance();

    /**
     * WebPreferences destructor
     */
    virtual ~WebPreferences();
protected:

    /**
     *  WebPreferences constructor
     */
    WebPreferences();

public:
    // IWebPreferences

    /**
     * get standard preferences 
     */
    virtual WebPreferences* standardPreferences();

    /**
     * initialise with an identifier 
     */
    virtual WebPreferences* initWithIdentifier(const char* anIdentifier);

    /**
     * get identifier
     */
    virtual const char* identifier();

    /**
     * standardFontFamily 
     */
    virtual const char* standardFontFamily();

    /**
     *  setStandardFontFamily 
     */
    virtual void setStandardFontFamily(const char* family);

    /**
     *  fixedFontFamily 
     */
    virtual const char* fixedFontFamily();

    /**
     *  setFixedFontFamily 
     */
    virtual void setFixedFontFamily(const char* family);

    /**
     *  serifFontFamily 
     */
    virtual const char* serifFontFamily();

    /**
     *  setSerifFontFamily 
     */
    virtual void setSerifFontFamily(const char* family);

    /**
     *  sansSerifFontFamily
     */
    virtual const char* sansSerifFontFamily();

    /**
     *  setSansSerifFontFamily 
     */
    virtual void setSansSerifFontFamily(const char* family);

    /**
     *  cursiveFontFamily 
     */
    virtual const char* cursiveFontFamily();

    /**
     *  setCursiveFontFamily 
     */
    virtual void setCursiveFontFamily(const char* family);

    /**
     *  fantasyFontFamily 
     */
    virtual const char* fantasyFontFamily();

    /**
     *  setFantasyFontFamily 
     */
    virtual void setFantasyFontFamily(const char* family);

    /**
     *  fantasyFontFamily
     */
	virtual const char* pictographFontFamily();

    /**
     *  setFantasyFontFamily
     */
	virtual void setPictographFontFamily(const char* family);
    /**
     *  defaultFontSize 
     */
    virtual int defaultFontSize();

    /**
     *  setDefaultFontSize 
     */
    virtual void setDefaultFontSize(int fontSize);

    /**
     *  defaultFixedFontSize 
     */
    virtual int defaultFixedFontSize();

    /**
     *  setDefaultFixedFontSize 
     */
    virtual void setDefaultFixedFontSize(int fontSize);

    /**
     *  minimumFontSize
     */
    virtual int minimumFontSize();

    /**
     *  setMinimumFontSize 
     */
    virtual void setMinimumFontSize(int fontSize);

    /**
     *  minimumLogicalFontSize 
     */
    virtual int minimumLogicalFontSize();

    /**
     *  setMinimumLogicalFontSize 
     */
    virtual void setMinimumLogicalFontSize(int fontSize);

    /**
     *  defaultTextEncodingName 
     */
    virtual const char* defaultTextEncodingName();

    /**
     *  setDefaultTextEncodingName 
     */
    virtual void setDefaultTextEncodingName(const char* name);

    /**
     *  userStyleSheetEnabled 
     */
    virtual bool userStyleSheetEnabled();

    /**
     *  setUserStyleSheetEnabled 
     */
    virtual void setUserStyleSheetEnabled(bool enabled);

    /**
     *  userStyleSheetLocation 
     */
    virtual const char* userStyleSheetLocation();

    /**
     *  setUserStyleSheetLocation 
     */
    virtual void setUserStyleSheetLocation(const char* location);

    /**
     *  isJavaEnabled 
     */
    virtual bool isJavaEnabled();

    /**
     *  setJavaEnabled 
     */
    virtual void setJavaEnabled(bool enabled);

    /**
     *  isJavaScriptEnabled 
     */
    virtual bool isJavaScriptEnabled();

    /**
     *  setJavaScriptEnabled 
     */
    virtual void setJavaScriptEnabled(bool enabled);

    /**
     *  javaScriptCanOpenWindowsAutomatically 
     */
    virtual bool javaScriptCanOpenWindowsAutomatically();

    /**
     *  setJavaScriptCanOpenWindowsAutomatically 
     */
    virtual void setJavaScriptCanOpenWindowsAutomatically(bool enabled);

    /**
     *  arePlugInsEnabled 
     */
    virtual bool arePlugInsEnabled();

    /**
     *  setPlugInsEnabled 
     */
    virtual void setPlugInsEnabled(bool enabled);

    /**
     *  isCSSRegionsEnabled
     */

    virtual bool isCSSRegionsEnabled();

    /**
     *  setCSSRegionsEnabled
     */

    virtual void setCSSRegionsEnabled(bool);

    /**                                                                                                                                           
     *  areSeamlessIFramesEnabled                                                                                                               
     */
    virtual bool areSeamlessIFramesEnabled();

    /**                                                                                                                                           
     *  setSeamlessIFramesEnabled                                                                                            
     */
    virtual void setSeamlessIFramesEnabled(bool enabled);

    /**
     *  allowsAnimatedImages 
     */
    virtual bool allowsAnimatedImages();

    /**
     *  setAllowsAnimatedImages 
     */
    virtual void setAllowsAnimatedImages(bool enabled);

    /**
     *  allowAnimatedImageLooping 
     */
    virtual bool allowAnimatedImageLooping();

    /**
     *  setAllowAnimatedImageLooping 
     */
    virtual void setAllowAnimatedImageLooping(bool enabled);

    /**
     *  setLoadsImagesAutomatically 
     */
    virtual void setLoadsImagesAutomatically(bool enabled);

    /**
     *  loadsImagesAutomatically 
     */
    virtual bool loadsImagesAutomatically();

    /**
	 *  setImageEnabled
     */
	virtual void setImagesEnabled(bool enabled);

    /**
	 *  areImagesEnabled
     */
	virtual bool areImagesEnabled();

    /**
     *  setAutosaves
     */
    virtual void setAutosaves(bool enabled);

    /**
     *  autosaves 
     */
    virtual bool autosaves();

    /**
     *  setShouldPrintBackgrounds 
     */
    virtual void setShouldPrintBackgrounds(bool enabled);

    /**
     *  shouldPrintBackgrounds 
     */
    virtual bool shouldPrintBackgrounds();

    /**
     *  setPrivateBrowsingEnabled 
     */
    virtual void setPrivateBrowsingEnabled(bool enabled);

    /**
     *  privateBrowsingEnabled 
     */
    virtual bool privateBrowsingEnabled();

    /**
     *  setTabsToLinks 
     */
    virtual void setTabsToLinks(bool enabled);

    /**
     *  tabsToLinks 
     */
    virtual bool tabsToLinks();

    /**
     *  textAreasAreResizable 
     */
    virtual bool textAreasAreResizable();

    /**
     *  setTextAreasAreResizable 
     */
    virtual void setTextAreasAreResizable(bool enabled);

    /**
     *  usesPageCache
     */
    virtual bool usesPageCache();

    /**
     *  setUsesPageCache 
     */
    virtual void setUsesPageCache(bool usesPageCache);

    /**
     *  iconDatabaseLocation 
     */
    virtual const char* iconDatabaseLocation();

    /**
     *  setIconDatabaseLocation 
     */
    virtual void setIconDatabaseLocation(const char* location);

    /**
     *  iconDatabaseEnabled 
     */
    virtual bool iconDatabaseEnabled();

    /**
     *  setIconDatabaseEnabled 
     */
    virtual void setIconDatabaseEnabled(bool enabled);

    /**
     *  fontSmoothing
     */
    virtual FontSmoothingType fontSmoothing();

    /**
     *  setFontSmoothing 
     */
    virtual void setFontSmoothing(FontSmoothingType smoothingType);

    /*
     * isWebSecurityEnabled
     */
    virtual bool isWebSecurityEnabled();

    /*
     * setWebSecurityEnabled
     */
    virtual void setWebSecurityEnabled(bool enabled);

    /**
     *  editableLinkBehavior 
     */
    virtual WebKitEditableLinkBehavior editableLinkBehavior();

    /**
     *  setEditableLinkBehavior 
     */
    virtual void setEditableLinkBehavior(WebKitEditableLinkBehavior behavior);

    /**
     *  requestAnimationFrameEnabled
     */
    virtual bool requestAnimationFrameEnabled();

    /**
     *  setRequestAnimationFrameEnabled
     */
    virtual void setRequestAnimationFrameEnabled(bool);

    /**
     *  cookieStorageAcceptPolicy 
     */
    virtual WebKitCookieStorageAcceptPolicy cookieStorageAcceptPolicy();

    /**
     *  setCookieStorageAcceptPolicy 
     */
    virtual void setCookieStorageAcceptPolicy(WebKitCookieStorageAcceptPolicy acceptPolicy);

    /**
     *  continuousSpellCheckingEnabled 
     */
    virtual bool continuousSpellCheckingEnabled();

    /**
     *  setContinuousSpellCheckingEnabled 
     */
    virtual void setContinuousSpellCheckingEnabled(bool enabled);

    /**
     *  grammarCheckingEnabled 
     */
    virtual bool grammarCheckingEnabled();

    /**
     *  setGrammarCheckingEnabled 
     */
    virtual void setGrammarCheckingEnabled(bool enabled);

    /**
     *  allowContinuousSpellChecking 
     */
    virtual bool allowContinuousSpellChecking();

    /**
     *  setAllowContinuousSpellChecking 
     */
    virtual void setAllowContinuousSpellChecking(bool enabled);

    /**
     *  isDOMPasteAllowed 
     */
    virtual bool isDOMPasteAllowed();

    /**
     *  setDOMPasteAllowed
     */
    virtual void setDOMPasteAllowed(bool enabled);

    /**
     *  cacheModel 
     */
    virtual WebCacheModel cacheModel();

    /**
     *  setCacheModel 
     */
    virtual void setCacheModel(WebCacheModel cacheModel);

    // IWebPreferencesPrivate

    /**
     *  setDeveloperExtrasEnabled 
     */
    virtual void setDeveloperExtrasEnabled(bool);

    /**
     *  developerExtrasEnabled 
     */
    virtual bool developerExtrasEnabled();

    /**
     *  setAutomaticallyDetectsCacheModel 
     */
    virtual void setAutomaticallyDetectsCacheModel(bool automaticallyDetectsCacheModel);

    /**
     *  automaticallyDetectsCacheModel 
     */
    virtual bool automaticallyDetectsCacheModel();

    /**
     *  setAuthorAndUserStylesEnabled
     */
    virtual void setAuthorAndUserStylesEnabled(bool enabled);

    /**
     *  authorAndUserStylesEnabled 
     */
    virtual bool authorAndUserStylesEnabled();

    /*
     * inApplicationChromeMode
     */
    virtual bool inApplicationChromeMode();
    
    /*
     * setApplicationChromeMode
     */
    virtual void setApplicationChromeMode(bool enabled);

    /**
     * Enable offline web application
     */
    virtual void setOfflineWebApplicationCacheEnabled(bool enabled);

    /**
     * Tell if offline web application cache is enabled
     */
    virtual bool offlineWebApplicationCacheEnabled();

    /*
     * setDatabasesEnabled
     */
    virtual void setDatabasesEnabled(bool enabled);

    /*
     * databasesEnabled
     */
    virtual bool databasesEnabled();

     /**
    * Enable experimental desktop notifications
    */
    virtual void setExperimentalNotificationsEnabled(bool);

    /**
     * Returns whether the experimental desktop notifications are enabled.
     */
    virtual bool experimentalNotificationsEnabled();

    virtual void setLocalStorageEnabled(bool enabled);
    virtual bool localStorageEnabled();

    virtual const char* localStorageDatabasePath();
    virtual void setLocalStorageDatabasePath(const char* location);

    virtual void setShouldPaintNativeControls(bool shouldPaint);

    virtual bool shouldPaintNativeControls();
    /**
     * Enable zoom for text only.
     */
    virtual void setZoomsTextOnly(bool zoomsTextOnly);

    /**
     * get zoom for text only status.
     */
    virtual bool zoomsTextOnly();

    virtual float fontSmoothingContrast();

    virtual void setFontSmoothingContrast(float contrast);

    /**
     * get allow universal access from File Urls status
     */
    virtual bool allowUniversalAccessFromFileURLs();

    /**
     * allow universal access from File Urls
     */
    virtual void setAllowUniversalAccessFromFileURLs(bool);

    /**
     * get allow file access from File Urls status
     */
    virtual bool allowFileAccessFromFileURLs();

    /**
     * allow file access from File Urls
     */
    virtual void setAllowFileAccessFromFileURLs(bool allowAccess);

    /**
     * get allow clipboard access from JavaScript status
     */
    virtual bool javaScriptCanAccessClipboard();

    /**
     * allow clipboard access from JavaScript
     */
    virtual void setJavaScriptCanAccessClipboard(bool enabled);

    /**
     * Is XSS auditor enabled
     */
    virtual bool isXSSAuditorEnabled();

    /**
     * Enable/Disable XSS auditor
     */
    virtual void setXSSAuditorEnabled(bool);

    virtual void setShouldUseHighResolutionTimers(bool useHighResolutionTimers);

    virtual bool shouldUseHighResolutionTimers();

    virtual bool isFrameFlatteningEnabled();
    
    virtual void setFrameFlatteningEnabled(bool enabled);
    
    virtual void setPreferenceForTest(const char* key, const char* value);
    
    virtual void setCustomDragCursorsEnabled(bool);
    
    virtual bool customDragCursorsEnabled();
    
    virtual void setShowDebugBorders(bool);
    virtual bool showDebugBorders();
    
    virtual void setShowRepaintCounter(bool);
    virtual bool showRepaintCounter();

    virtual void setDNSPrefetchingEnabled(bool);
    virtual bool DNSPrefetchingEnabled();

    virtual void setMemoryInfoEnabled(bool);
    virtual bool memoryInfoEnabled();

    virtual void setHyperlinkAuditingEnabled(bool);
    virtual bool hyperlinkAuditingEnabled();
    
    virtual void setHixie76WebSocketProtocolEnabled(bool);
    virtual bool hixie76WebSocketProtocolEnabled();

    virtual void setShouldInvertColors(bool);
    virtual bool shouldInvertColors();

	virtual void setShouldDisplaySubtitles(bool enabled);
	virtual bool shouldDisplaySubtitles();

	virtual void setShouldDisplayCaptions(bool enabled);
	virtual bool shouldDisplayCaptions();

	virtual void setShouldDisplayTextDescriptions(bool enabled);
	virtual bool shouldDisplayTextDescriptions();

    /*
     * Enable or disable WebGL
     */
    void setWebGLEnabled(bool);

    /*
     * Return whether webgl is enabled
     */
    bool webGLEnabled();

    /*
     * Enable or disable accelerated compositing
     */
    void setAcceleratedCompositingEnabled(bool);

    /*
     * Return whether accelerated compositing is enabled
     */
    bool acceleratedCompositingEnabled();

    // WebPreferences

    // This method accesses a different preference key than developerExtrasEnabled.
    // See <rdar://5343767> for the justification.

    /**
     *  developerExtrasDisabledByOverride 
     */
    bool developerExtrasDisabledByOverride();


	virtual bool mediaPlaybackRequiresUserGesture();
	virtual void setMediaPlaybackRequiresUserGesture(bool);

	virtual bool mediaPlaybackAllowsInline();
	virtual void setMediaPlaybackAllowsInline(bool);

    /**
     * get the topic to notify a change on webPreference
     */
    static const char* webPreferencesChangedNotification();

    /**
     * get the topic to notify a remove on webPreference
     * @param[in]: 
     * @param[out]: 
     * @code
     * @endcode
     */
    static const char* webPreferencesRemovedNotification();


    /**
     * set an instance of WebPreference
     */
    static void setInstance(WebPreferences* instance, const char* identifier);

    /**
     * remove a instance of WebPreference for an identifier 
     */
    static void removeReferenceForIdentifier(const char* identifier);

    /**
     * get shared standard preferences 
     */
    static WebPreferences* sharedStandardPreferences();

    // From WebHistory.h

    /**
     *  historyItemLimit 
     * The maximum number of items that will be stored by the WebHistory.
     */
    int historyItemLimit();

    /**
     * @discussion setHistoryAgeInDaysLimit: sets the maximum number of days to be read from
        stored history.
        @param limit The maximum number of days to be read from stored history.
     */
    void setHistoryItemLimit(int limit);

    /**
     *  historyAgeInDaysLimit 
     * The maximum number of items that will be stored by the WebHistory.
     */
    int historyAgeInDaysLimit();

    /**
     * @discussion setHistoryAgeInDaysLimit: sets the maximum number of days to be read from
        stored history.
        @param limit The maximum number of days to be read from stored history.
     */
    void setHistoryAgeInDaysLimit(int limit);


    /**
     * will add to webView 
     */
    void willAddToWebView();

    /**
     * did remove from webView 
     */
    void didRemoveFromWebView();


    /**
     * post preferences changes notification 
     */
    void postPreferencesChangesNotification();

    /**
     * add an extra plugin directory
     */
    void addExtraPluginDirectory(const char* directory);

    void setMemoryLimit(int);
    int memoryLimit();

    /**
     * Allow or deny window.close() function
     */
    void setAllowScriptsToCloseWindows(bool);

    /**
     * Check wether scripts can close a window
     */
    bool allowScriptsToCloseWindows();
    
    /**
     * Checks whether spatial navigation support is activated
     */
    bool spatialNavigationEnabled();
    
    /**
     * Activates spatial navigation support
     */
    void setSpatialNavigationEnabled(bool);


    // OWB Extensions

    /**
     * addCertificateInfo
     * @brief Adds a client certificate to be used for the given url
     * Only one certificate can be send by host, so calling this method with several certificates will only result in sending the last.
     * @param url the url to which the certificate will be send
     * @param certificatePath the path to the certificate (for the moment, only PEM files are supported)
     * @param keyPath the path to the key (for the moment only PEM files are supported)
     * @param keyPassword the key's password.
     * @warning This is only implemented on top of cURL.
     */
    static void addCertificateInfo(const char* /*url*/, const char* /*certificatePath*/, const char* /*keyPath*/, const char* /*keyPassword*/);

    /**
     * clearCertificateInfo
     * @brief Clear the client certificate, key and key' password for the given URL
     * @param url
     */
    static void clearCertificateInfo(const char* /*url*/);

    /**
     * clearAllCertificatesInfo
     * @brief Clear all the client certificats, keys and keys' passwords for all URL
     */
    static void clearAllCertificatesInfo();
protected:

    /**
     *  setValueForKey
     */
    void setValueForKey(const char* key, const char* value);

    /**
     *  valueForKey 
     */
    const char* valueForKey(const char* key);

    /**
     *  stringValueForKey 
     */
    const char* stringValueForKey(const char* key);

    /**
     *  integerValueForKey 
     */
    int integerValueForKey(const char* key);

    /**
     *  boolValueForKey 
     */
    bool boolValueForKey(const char* key);

    /**
     *  floatValueForKey
     */
    float floatValueForKey(const char* key);

    /**
     *  longlongValueForKey 
     */
    unsigned int longlongValueForKey(const char* key);

    /**
     *  setStringValue
     */
    void setStringValue(const char* key, const char* value);

    /**
     *  setIntegerValue 
     */
    void setIntegerValue(const char* key, int value);

    /**
     *  setBoolValue 
     */
    void setBoolValue(const char* key, bool value);

    /**
     *  setLongLongValue 
     */
    void setLongLongValue(const char* key, unsigned int value);

    /**
     *  setFloatValue 
     */
    void setFloatValue(const char* key, float value);

    /**
     *  getInstanceForIdentifier 
     */
    static WebPreferences* getInstanceForIdentifier(const char* identifier);

    /**
     *  initializeDefaultSettings 
     */
    static void initializeDefaultSettings();

    /**
     *  save 
     */
    void save();

    /**
     *  load 
     */
    void load();

protected:
    std::string m_identifier;
    bool m_autoSaves;
    bool m_automaticallyDetectsCacheModel;
    unsigned m_numWebViews;
};

#endif

#import <ob/OBString.h>
@class OBURL;

typedef enum
{
	WkSettings_Throttling_None,
	WkSettings_Throttling_InvisibleBrowsers,
	WkSettings_Throttling_All,
} WkSettings_Throttling;

typedef enum
{
	WkSettings_Interpolation_Low,
	WkSettings_Interpolation_Medium,
	WkSettings_Interpolation_High,
} WkSettings_Interpolation;

typedef enum
{
	WkSettings_UserStyleSheet_Builtin,
	WkSettings_UserStyleSheet_MUI,
	WkSettings_UserStyleSheet_Custom,
} WkSettings_UserStyleSheet;

typedef enum
{
	WkSettings_ContextMenuHandling_Default,
	// Do not send button2 events to the DOM, instead, build a context menu from the hittest immediately
	WkSettings_ContextMenuHandling_Override,
	// Do not send button2 events if a qualifier key is pressed
	WkSettings_ContextMenuHandling_OverrideWithShift,
	WkSettings_ContextMenuHandling_OverrideWithAlt,
	WkSettings_ContextMenuHandling_OverrideWithControl,
} WkSettings_ContextMenuHandling;

typedef enum {
    // Loop filter skipping setting / ffmpeg
    WkSettings_LoopFilter_Default,
    WkSettings_LoopFilter_NonRef,
    WkSettings_LoopFilter_BiDirectional,
    WkSettings_LoopFilter_NonIntra,
    WkSettings_LoopFilter_NonKey
} WkSettings_LoopFilter;

@interface WkSettings : OBObject

+ (WkSettings *)settings;

- (BOOL)javaScriptEnabled;
- (void)setJavaScriptEnabled:(BOOL)enabled;

- (BOOL)adBlockerEnabled;
- (void)setAdBlockerEnabled:(BOOL)enabled;

- (BOOL)thirdPartyCookiesAllowed;
- (void)setThirdPartyCookiesAllowed:(BOOL)allowCookies;

- (BOOL)localStorageEnabled;
- (void)setLocalStorageEnabled:(BOOL)enabled;

- (BOOL)offlineWebApplicationCacheEnabled;
- (void)setOfflineWebApplicationCacheEnabled:(BOOL)enabled;

- (WkSettings_Throttling)throttling;
- (void)setThrottling:(WkSettings_Throttling)throttling;

- (WkSettings_Interpolation)interpolation;
- (void)setInterpolation:(WkSettings_Interpolation)interpolation;

- (WkSettings_Interpolation)interpolationForImageViews;
- (void)setInterpolationForImageViews:(WkSettings_Interpolation)interpolation;

- (WkSettings_UserStyleSheet)styleSheet;
- (void)setStyleSheet:(WkSettings_UserStyleSheet)styleSheet;

- (OBString *)customStyleSheetPath;
- (void)setCustomStyleSheetPath:(OBString *)path;

- (WkSettings_ContextMenuHandling)contextMenuHandling;
- (void)setContextMenuHandling:(WkSettings_ContextMenuHandling)handling;

- (BOOL)requiresUserGestureForMediaPlayback;
- (void)setRequiresUserGestureForMediaPlayback:(BOOL)gestureRequired;

- (BOOL)invisiblePlaybackNotAllowed;
- (void)setInvisiblePlaybackNotAllowed:(BOOL)invisiblePlayback;

- (BOOL)mediaEnabled;
- (void)setMediaEnabled:(BOOL)enabled;

- (BOOL)mediaSourceEnabled;
- (void)setMediaSourceEnabled:(BOOL)enabled;

- (BOOL)vp9Enabled;
- (void)setVp9Enabled:(BOOL)enabled;

- (BOOL)hvcEnabled;
- (void)setHVCEnabled:(BOOL)enabled;

- (BOOL)hlsEnabled;
- (void)setHLSEnabled:(BOOL)enabled;

- (BOOL)videoDecodingEnabled;
- (void)setVideoDecodingEnabled:(BOOL)enabled;

- (WkSettings_LoopFilter)loopFilter;
- (void)setLoopFilter:(WkSettings_LoopFilter)loopFilter;

- (BOOL)darkModeEnabled;
- (void)setDarkModeEnabled:(BOOL)enabled;

- (void)setDictionaryLanguage:(OBString *)language;
- (OBString *)dictionaryLanguage;

- (void)setAdditionalDictionaryLanguage:(OBString *)language;
- (OBString *)additionalDictionaryLanguage;

- (BOOL)touchEventsEmulationEnabled;
- (void)setTouchEventsEmulationEnabled:(BOOL)enabled;

@end

typedef enum
{
	WkGlobalSettings_Antialias_None,
	WkGlobalSettings_Antialias_Gray,
	WkGlobalSettings_Antialias_Subpixel
} WkGlobalSettings_Antialias;

typedef enum
{
	// Most of the in-memory caches are off or at a low threshold, whole-page cache is off
	WkGlobalSettings_Caching_Minimal,
	// Cache a couple of whole-pages to allow faster back/forward navigation, cache more
	// resources in RAM, depending on system total RAM available
	WkGlobalSettings_Caching_Balanced,
} WkGlobalSettings_Caching;

@interface WkGlobalSettings : OBObject

// Set the default download path for all new downloads, they'll be downloaded with a tmp name
// and renamed to the name returned by decideFilenameForDownload:withSuggestedName: on success
+ (OBString *)downloadPath;
+ (void)setDownloadPath:(OBString *)path;

+ (WkGlobalSettings_Antialias)fontAntialias;
+ (void)setAntialias:(WkGlobalSettings_Antialias)aa;

// Sets a custom PEM file to be used to validate a connection to the given domain
// 'key' is an optional password required to load the PEM file
// Pass a nil pathToPEM to remove
+ (void)setCustomCertificate:(OBString *)pathToPEM forHost:(OBString *)host withKey:(OBString *)key;
// Ignore SSL errors for this host. Persistent until app's demise
+ (void)ignoreSSLErrorsForHost:(OBString *)host;

// Set the caching model, defaults to Balanced
+ (void)setCaching:(WkGlobalSettings_Caching)caching;
+ (WkGlobalSettings_Caching)caching;

+ (void)setDiskCachingLimit:(QUAD)limit;
+ (QUAD)diskCachingLimit;
+ (QUAD)calculatedMaximumDiskCachingLimit;

// May be used to globally manage spellchecking - should not be used if per-instance spellchecking
// options are set
+ (void)setSpellCheckingEnabled:(BOOL)spellcheckingenabled;
+ (BOOL)spellCheckingEnabled;
+ (void)setDictionaryLanguage:(OBString *)language;
+ (OBString *)dictionaryLanguage;
+ (OBString *)defaultDictionaryLanguage;

// Whether media playback was enabled at compile time
+ (BOOL)supportsMediaPlayback;

// Setup proxy
+ (void)setProxyURL:(OBURL *)url user:(OBString *)user password:(OBString *)password ignoredHosts:(OBString *)hosts;
// Clear/disable proxy settings
+ (void)setProxyNone;

@end

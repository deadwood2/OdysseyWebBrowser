#import "WkSettings.h"
#import <ob/OBURL.h>

#undef __OBJC__
#import "WebKit.h"
#import <wtf/WallTime.h>
#import <wtf/text/WTFString.h>
#import <WebCore/CurlDownload.h>
#import <WebCore/ResourceResponse.h>
#import <WebCore/ResourceHandle.h>
#import <WebCore/ResourceResponse.h>
#import <WebCore/TextEncoding.h>
#import <WebCore/MediaPlayerMorphOS.h>
#import <WebCore/CurlProxySettings.h>
#import <WebCore/NetworkStorageSession.h>
#import "../WebCoreSupport/NetworkStorageSessionMap.h"
#import <wtf/FileSystem.h>
#import <WebProcess.h>
#import "WebEditorClient.h"
#define __OBJC__

#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wmisleading-indentation"

typedef enum _cairo_antialias {
    CAIRO_ANTIALIAS_DEFAULT,
    CAIRO_ANTIALIAS_NONE,
    CAIRO_ANTIALIAS_GRAY,
    CAIRO_ANTIALIAS_SUBPIXEL,
    CAIRO_ANTIALIAS_FAST,
    CAIRO_ANTIALIAS_GOOD,
    CAIRO_ANTIALIAS_BEST
} cairo_antialias_t;

namespace WebCore {
	void setDefaultCairoFontAntialias(cairo_antialias_t aa);
}

@interface WkSettingsPrivate : WkSettings
{
	WkSettings_Throttling          _throttling;
	WkSettings_Interpolation       _interpolation;
	WkSettings_Interpolation       _interpolationForImageViews;
	WkSettings_UserStyleSheet      _userStyleSheet;
	WkSettings_ContextMenuHandling _contextMenu;
	WkSettings_LoopFilter          _loopFilter;
	OBString                      *_userStyleSheetFile;
	OBString                      *_additionalLanguage;
	OBString                      *_language;
	bool _script;
	bool _adBlocker;
	bool _thCookies;
	bool _localStorage;
	bool _offlineCache;
	bool _invisiblePlaybackNotAllowed;
	bool _requiresUserGestureForMediaPlayback;
	bool _mediaEnabled;
	bool _mediaSourceEnabled;
	bool _decodeVideo;
	bool _vp9;
	bool _hls;
	bool _hvc;
	bool _darkMode;
	bool _touchEvents;
}
@end

@implementation WkSettingsPrivate

- (id)init
{
	if ((self = [super init]))
	{
		_script = YES;
		_adBlocker = YES;
		_thCookies = YES;
		_localStorage = YES;
		_offlineCache = YES;
		_throttling = WkSettings_Throttling_InvisibleBrowsers;
		_interpolation = WkSettings_Interpolation_Medium; // medium is the WebCore default, let's stick to that
		_interpolationForImageViews = WkSettings_Interpolation_Medium; // medium is the WebCore default, let's stick to that
		_userStyleSheet = WkSettings_UserStyleSheet_MUI;
		_invisiblePlaybackNotAllowed = YES;
		_requiresUserGestureForMediaPlayback = YES;
	}
	
	return self;
}

- (void)dealloc
{
	[_userStyleSheetFile release];
	[_additionalLanguage release];
	[_language release];
	[super dealloc];
}

- (BOOL)javaScriptEnabled
{
	return _script;
}

- (void)setJavaScriptEnabled:(BOOL)enabled
{
	_script  = enabled;
}

- (BOOL)adBlockerEnabled
{
	return _adBlocker;
}

- (void)setAdBlockerEnabled:(BOOL)enabled
{
	_adBlocker = enabled;
}

- (BOOL)thirdPartyCookiesAllowed
{
	return _thCookies;
}

- (void)setThirdPartyCookiesAllowed:(BOOL)allowCookies
{
	_thCookies = allowCookies;
}

- (BOOL)localStorageEnabled
{
	return _localStorage;
}

- (void)setLocalStorageEnabled:(BOOL)enabled
{
	_localStorage = enabled;
}

- (BOOL)offlineWebApplicationCacheEnabled
{
	return _offlineCache;
}

- (void)setOfflineWebApplicationCacheEnabled:(BOOL)enabled
{
	_offlineCache = enabled;
}

- (WkSettings_Throttling)throttling
{
	return _throttling;
}

- (void)setThrottling:(WkSettings_Throttling)throttling
{
	_throttling = throttling;
}

- (WkSettings_Interpolation)interpolation
{
	return _interpolation;
}

- (void)setInterpolation:(WkSettings_Interpolation)interpolation
{
	_interpolation = interpolation;
}

- (WkSettings_Interpolation)interpolationForImageViews
{
	return _interpolationForImageViews;
}

- (void)setInterpolationForImageViews:(WkSettings_Interpolation)interpolation
{
	_interpolationForImageViews = interpolation;
}

- (WkSettings_UserStyleSheet)styleSheet
{
	return _userStyleSheet;
}

- (void)setStyleSheet:(WkSettings_UserStyleSheet)styleSheet
{
	_userStyleSheet = styleSheet;
}

- (OBString *)customStyleSheetPath
{
	return _userStyleSheetFile;
}

- (void)setCustomStyleSheetPath:(OBString *)path
{
	[_userStyleSheetFile autorelease];
	_userStyleSheetFile = [path copy];
}

- (WkSettings_ContextMenuHandling)contextMenuHandling
{
	return _contextMenu;
}

- (void)setContextMenuHandling:(WkSettings_ContextMenuHandling)handling
{
	_contextMenu = handling;
}

- (BOOL)requiresUserGestureForMediaPlayback
{
	return _requiresUserGestureForMediaPlayback;
}

- (void)setRequiresUserGestureForMediaPlayback:(BOOL)gestureRequired
{
	_requiresUserGestureForMediaPlayback = gestureRequired;
}

- (BOOL)invisiblePlaybackNotAllowed
{
	return _invisiblePlaybackNotAllowed;
}

- (void)setInvisiblePlaybackNotAllowed:(BOOL)invisiblePlayback
{
	_invisiblePlaybackNotAllowed = invisiblePlayback;
}

- (BOOL)mediaEnabled
{
    return _mediaEnabled;
}

- (void)setMediaEnabled:(BOOL)enabled
{
	_mediaEnabled = enabled;
}

- (BOOL)mediaSourceEnabled
{
    return _mediaSourceEnabled;
}

- (void)setMediaSourceEnabled:(BOOL)enabled
{
	_mediaSourceEnabled = enabled;
}

- (BOOL)vp9Enabled
{
    return _vp9;
}

- (void)setVp9Enabled:(BOOL)enabled
{
	_vp9 = enabled;
}

- (BOOL)hvcEnabled
{
	return _hvc;
}

- (void)setHVCEnabled:(BOOL)enabled
{
	_hvc = enabled;
}

- (BOOL)hlsEnabled
{
    return _hls;
}

- (void)setHLSEnabled:(BOOL)enabled
{
	_hls = enabled;
}

- (BOOL)videoDecodingEnabled
{
    return _decodeVideo;
}

- (void)setVideoDecodingEnabled:(BOOL)enabled
{
	_decodeVideo = enabled;
}

- (WkSettings_LoopFilter)loopFilter
{
    return _loopFilter;
}

- (void)setLoopFilter:(WkSettings_LoopFilter)filterskip
{
	_loopFilter = filterskip;
}

- (BOOL)darkModeEnabled
{
	return _darkMode;
}

- (void)setDarkModeEnabled:(BOOL)enabled
{
	_darkMode = enabled;
}

- (BOOL)touchEventsEmulationEnabled
{
	return _touchEvents;
}

- (void)setTouchEventsEmulationEnabled:(BOOL)enabled
{
	_touchEvents = enabled;
}

- (void)setDictionaryLanguage:(OBString *)language
{
	[_language autorelease];
	_language = [language copy];
}

- (OBString *)dictionaryLanguage
{
	return _language;
}

- (void)setAdditionalDictionaryLanguage:(OBString *)language
{
	[_additionalLanguage autorelease];
	_additionalLanguage = [language copy];
}

- (OBString *)additionalDictionaryLanguage
{
	return _additionalLanguage;
}

@end

@implementation WkSettings

+ (WkSettings *)settings
{
	return [[WkSettingsPrivate new] autorelease];
}

- (BOOL)javaScriptEnabled
{
	return YES;
}

- (void)setJavaScriptEnabled:(BOOL)enabled
{
}

- (BOOL)adBlockerEnabled
{
	return YES;
}

- (void)setAdBlockerEnabled:(BOOL)enabled
{
}

- (BOOL)thirdPartyCookiesAllowed
{
	return NO;
}

- (void)setThirdPartyCookiesAllowed:(BOOL)allowCookies
{
}

- (BOOL)localStorageEnabled
{
	return YES;
}

- (void)setLocalStorageEnabled:(BOOL)enabled
{

}

- (BOOL)offlineWebApplicationCacheEnabled
{
	return YES;
}

- (void)setOfflineWebApplicationCacheEnabled:(BOOL)enabled
{
}

- (WkSettings_Throttling)throttling
{
	return WkSettings_Throttling_InvisibleBrowsers;
}

- (void)setThrottling:(WkSettings_Throttling)throttling
{
}

- (WkSettings_Interpolation)interpolation
{
	return WkSettings_Interpolation_Medium;
}

- (void)setInterpolation:(WkSettings_Interpolation)interpolation
{

}

- (WkSettings_Interpolation)interpolationForImageViews
{
	return WkSettings_Interpolation_Medium;
}

- (void)setInterpolationForImageViews:(WkSettings_Interpolation)interpolation
{
}

- (WkSettings_UserStyleSheet)styleSheet
{
	return WkSettings_UserStyleSheet_MUI;
}

- (void)setStyleSheet:(WkSettings_UserStyleSheet)styleSheet
{
}

- (OBString *)customStyleSheetPath
{
	return nil;
}

- (void)setCustomStyleSheetPath:(OBString *)path
{
}

- (WkSettings_ContextMenuHandling)contextMenuHandling
{
	return WkSettings_ContextMenuHandling_Default;
}

- (void)setContextMenuHandling:(WkSettings_ContextMenuHandling)handling
{
}

- (BOOL)requiresUserGestureForMediaPlayback
{
	return YES;
}

- (void)setRequiresUserGestureForMediaPlayback:(BOOL)gestureRequired
{
}

- (BOOL)invisiblePlaybackNotAllowed
{
	return YES;
}

- (void)setInvisiblePlaybackNotAllowed:(BOOL)invisiblePlayback
{
}

- (BOOL)mediaEnabled
{
    return NO;
}

- (void)setMediaEnabled:(BOOL)enabled
{
}

- (BOOL)mediaSourceEnabled
{
    return NO;
}

- (void)setMediaSourceEnabled:(BOOL)enabled
{
}

- (BOOL)vp9Enabled
{
    return NO;
}

- (void)setVp9Enabled:(BOOL)enabled
{
}

- (BOOL)hvcEnabled
{
    return NO;
}

- (void)setHVCEnabled:(BOOL)enabled
{
}

- (BOOL)hlsEnabled
{
    return NO;
}

- (void)setHLSEnabled:(BOOL)enabled
{
}

- (BOOL)videoDecodingEnabled
{
    return NO;
}

- (void)setVideoDecodingEnabled:(BOOL)enabled
{
}

- (WkSettings_LoopFilter)loopFilter
{
    return WkSettings_LoopFilter_NonKey;
}

- (void)setLoopFilter:(WkSettings_LoopFilter)filterskip
{
}

- (BOOL)darkModeEnabled
{
	return NO;
}

- (void)setDarkModeEnabled:(BOOL)enabled
{
}

- (BOOL)touchEventsEmulationEnabled
{
	return NO;
}

- (void)setTouchEventsEmulationEnabled:(BOOL)enabled
{
}

- (void)setDictionaryLanguage:(OBString *)language
{

}

- (OBString *)dictionaryLanguage
{
	return nil;
}

- (void)setAdditionalDictionaryLanguage:(OBString *)language
{

}

- (OBString *)additionalDictionaryLanguage
{
	return nil;
}

@end

@implementation WkGlobalSettings

+ (OBString *)downloadPath
{
	WTF::String str = WTF::FileSystemImpl::temporaryFilePathForPrefix("download");
	auto udata = str.utf8();
	return [OBString stringWithUTF8String:udata.data()];
}

+ (void)setDownloadPath:(OBString *)path
{
	const char *cpath = [path nativeCString];
	WebCore::CurlRequest::SetDownloadPath(WTF::String(cpath, strlen(cpath), MIBENUM_SYSTEM));
	WTF::FileSystemImpl::setTemporaryFilePathForPrefix(cpath, "download");
}

static cairo_antialias_t defaultAA;

+ (WkGlobalSettings_Antialias)fontAntialias
{
	switch (defaultAA)
	{
	case CAIRO_ANTIALIAS_NONE: return WkGlobalSettings_Antialias_None;
	case CAIRO_ANTIALIAS_GRAY: return WkGlobalSettings_Antialias_Gray;
	default: return WkGlobalSettings_Antialias_Subpixel;
	}
}

+ (void)setAntialias:(WkGlobalSettings_Antialias)aa
{
	switch (aa)
	{
	case WkGlobalSettings_Antialias_None: defaultAA = CAIRO_ANTIALIAS_NONE; break;
	case WkGlobalSettings_Antialias_Gray: defaultAA = CAIRO_ANTIALIAS_GRAY; break;
	case WkGlobalSettings_Antialias_Subpixel: defaultAA = CAIRO_ANTIALIAS_SUBPIXEL; break;
	}
	WebCore::setDefaultCairoFontAntialias(defaultAA);
}

+ (void)setCustomCertificate:(OBString *)pathToPEM forHost:(OBString *)host withKey:(OBString *)key
{
	if ([pathToPEM length] && [host length])
	{
		WTF::String sPath = WTF::String::fromUTF8([[pathToPEM absolutePath] nativeCString]);
		WTF::String sDomain = WTF::String::fromUTF8([host cString]);
		WTF::String sKey = key ? WTF::String::fromUTF8([key cString]) : "";
		WebCore::ResourceHandle::setClientCertificateInfo(sDomain, sPath, sKey);
	}
	else if ([host length])
	{
		WTF::String sDomain = WTF::String::fromUTF8([host cString]);
		WebCore::ResourceHandle::clearClientCertificateInfo(sDomain);
	}
}

+ (void)ignoreSSLErrorsForHost:(OBString *)host
{
	if ([host length])
	{
		WTF::String sDomain = WTF::String::fromUTF8([host cString]);
		WebCore::ResourceHandle::setHostAllowsAnyHTTPSCertificate(sDomain);
	}
}

+ (void)setCaching:(WkGlobalSettings_Caching)caching
{
	WebKit::CacheModel cacheModel = WebKit::CacheModel::PrimaryWebBrowser;
	if (WkGlobalSettings_Caching_Minimal == caching)
		cacheModel = WebKit::CacheModel::DocumentViewer;
	WebKit::WebProcess::singleton().setCacheModel(cacheModel);
}

+ (WkGlobalSettings_Caching)caching
{
	if (WebKit::WebProcess::singleton().cacheModel() != WebKit::CacheModel::PrimaryWebBrowser)
		return WkGlobalSettings_Caching_Minimal;
	return WkGlobalSettings_Caching_Balanced;
}

+ (void)setDiskCachingLimit:(QUAD)limit
{
	WebKit::WebProcess::singleton().setDiskCacheSize(limit);
}

+ (QUAD)diskCachingLimit
{
	return WebKit::WebProcess::singleton().diskCacheSize();
}

+ (QUAD)calculatedMaximumDiskCachingLimit
{
	return WebKit::WebProcess::singleton().maxDiskCacheSize();
}

+ (void)setSpellCheckingEnabled:(BOOL)spellcheckingenabled
{
	WebKit::WebEditorClient::setSpellCheckingEnabled(spellcheckingenabled);
}

+ (BOOL)spellCheckingEnabled
{
	return WebKit::WebEditorClient::getSpellCheckingEnabled();
}

+ (void)setDictionaryLanguage:(OBString *)language
{
	WebKit::WebEditorClient::setSpellCheckingLanguage(WTF::String::fromUTF8([language cString]));
}

+ (OBString *)dictionaryLanguage
{
	auto udata = WebKit::WebEditorClient::getSpellCheckingLanguage().utf8();
	return [OBString stringWithUTF8String:udata.data()];
}

+ (OBString *)defaultDictionaryLanguage
{
	WTF::Vector<WTF::String> dummy;
	WTF::String language;
	WebKit::WebEditorClient::getAvailableDictionaries(dummy, language);
	auto udata = language.utf8();
	return [OBString stringWithUTF8String:udata.data()];
}

+ (BOOL)supportsMediaPlayback
{
#if ENABLE(VIDEO)
	return YES;
#else
	return NO;
#endif
}

+ (void)setProxyURL:(OBURL *)url user:(OBString *)user password:(OBString *)password ignoredHosts:(OBString *)hosts
{
	WTF::URL wurl = WTF::URL(WTF::URL(), WTF::String([[url absoluteString] cString]));
	WebCore::CurlProxySettings settings(std::move(wurl), [hosts cString]);

	if (user && password) {
		settings.setUserPass([user cString], [password cString]);
	}

	NetworkStorageSessionMap::defaultStorageSession().setProxySettings(std::move(settings));
}

+ (void)setProxyNone
{
	WebCore::CurlProxySettings settings;
	NetworkStorageSessionMap::defaultStorageSession().setProxySettings(std::move(settings));
}

@end

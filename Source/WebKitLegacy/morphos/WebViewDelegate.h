#pragma once
#include <functional>
#include <wtf/text/WTFString.h>
#include <wtf/Vector.h>
#include <WebCore/IntRect.h>
#include <WebCore/FrameLoaderClient.h>
#include <WebCore/ContextMenuItem.h>
#include <WebCore/MediaPlayerMorphOS.h>
#include <WebCore/NotificationClient.h>

#define EP_PROFILING 0
#include <libeventprofiler.h>

namespace WebCore {
	class Page;
	class WindowFeatures;
	class ResourceError;
	class ResourceRequest;
	class FileChooser;
	class ResourceResponse;
	class PolicyCheckIdentifier;
	class AuthenticationChallenge;
	class HitTestResult;
	class SharedBuffer;
	class Element;
	class ResourceResponse;
	struct MediaPlayerMorphOSInfo;
	struct MediaPlayerMorphOSStreamSettings;
};

enum class WebViewDelegateOpenWindowMode
{
	Default,
	BackgroundTab,
	NewWindow
};

struct WebViewDelegate
{
	std::function<void(bool force)>   _fInvalidate;
	std::function<void(int, int)>     _fScroll;
	std::function<void(int, int)>     _fSetDocumentSize;
	std::function<void()>             _fActivateNext;
	std::function<void()>             _fActivatePrevious;
	std::function<void()>             _fGoActive;
	std::function<void()>             _fGoInactive;
	std::function<void()>             _fZoomChangedByWheel;

	std::function<WTF::String(const WTF::String&)>       _fUserAgentForURL;
	std::function<void(const WTF::String&)>              _fChangedTitle;
	std::function<void(const WTF::String&)>              _fChangedURL;
	std::function<void(void)>                            _fDidStartLoading;
	std::function<void(void)>                            _fDidStopLoading;
	std::function<void(void)>                            _fHistoryChanged;
	std::function<void(const WebCore::ResourceError &)>  _fDidFailWithError;
	std::function<bool(const WebCore::ResourceRequest&)> _fCanHandleRequest;
	std::function<void()>                                _fDidLoadInsecureContent;
	std::function<bool(const WTF::URL& url, bool newWindow)> _fShouldNavigateToURL;
	std::function<void(const WebCore::ResourceResponse&, bool requsestWithAuth)> _fDidReceiveResponse;

	std::function<bool(const WTF::String&, const WebCore::WindowFeatures&)>      _fCanOpenWindow;
	std::function<WebCore::Page*(void)>                                          _fDoOpenWindow;
	std::function<void(const WTF::URL& url, WebViewDelegateOpenWindowMode mode)> _fNewTabWindow;
	
	std::function<int(const WebCore::IntRect&, const WTF::Vector<WTF::String>&)> _fPopup;
	std::function<bool(const WebCore::IntPoint&, const WTF::Vector<WebCore::ContextMenuItem> &items, const WebCore::HitTestResult &hitTest)> _fContextMenu;

	std::function<void(const WTF::String&url, const WTF::String&message, int level, unsigned int line, unsigned int column)>        _fConsole;
	
	std::function<void(const WTF::URL &download, const WTF::String &suggestedName)>     _fDownload;
	std::function<void(WebCore::ResourceHandle*, const WebCore::ResourceRequest&,
		const WebCore::ResourceResponse&)>                                              _fDownloadFromResource;
	std::function<void(const WebCore::ResourceResponse& response,
		const WebCore::ResourceRequest& request, WebCore::PolicyCheckIdentifier identifier,
		const WTF::String& downloadAttribute, WebCore::FramePolicyFunction&& function)> _fDownloadAsk;
	
	std::function<void(const WTF::String &)>                                      _fAlert;
	std::function<bool(const WTF::String &)>                                      _fConfirm;
	std::function<bool(const WTF::String &, const WTF::String &, WTF::String &) > _fPrompt;
	std::function<void(WebCore::FileChooser&)>                                    _fFile;

	std::function<void()>                                           _fHasAutofill;
	std::function<void(const WTF::String &l, const WTF::String &p)> _fStoreAutofill;
	
	std::function<bool(const WebCore::AuthenticationChallenge&)>    _fAuthChallenge;
	
	std::function<void(int)> _fSetCursor;
	
	std::function<void(void)>  _fProgressStarted;
	std::function<void(float)> _fProgressUpdated;
	std::function<void(void)>  _fProgressFinished;
	
	std::function<void(const WTF::URL &url)> _fHoveredURLChanged;
	
	std::function<bool(const WTF::URL &url)> _fFavIconLoad;
	std::function<void(WebCore::SharedBuffer *, const WTF::URL &url)> _fFavIconLoaded;

	std::function<void(void)> _fPrint;
	
	std::function<void(void)> _fUndoRedoChanged;

	std::function<void(void *player, const String &url, WebCore::MediaPlayerMorphOSInfo& info, WebCore::MediaPlayerMorphOSStreamSettings& settings,
		WTF::Function<void()> &&yieldFunc)> _fMediaAdded;
	std::function<void(void *player, WebCore::MediaPlayerMorphOSInfo& info)> _fMediaUpdated;
	std::function<void(void *player)> _fMediaRemoved;
	std::function<void(void *player)> _fMediaWillPlay;
	std::function<void(void *player)> _fMediaPausedOrFinished;
	std::function<void(void *player, WebCore::Element* element,
		WTF::Function<void(void *windowPtr, int scrollX, int scrollY, int left, int top, int right, int bottom, int width, int height)> && callback)>
		_fMediaSetOverlayCallback;
	std::function<void(void *player)> _fMediaUpdateOverlayCallback;
	std::function<void(void)> _fEnterFullscreen;
	std::function<void(void)> _fExitFullscreen;

	std::function<void(int atX, int atY, int w, int h)> _fOpenDragWindow;
	std::function<void(int atX, int atY)> _fMoveDragWindow;
	std::function<void(void)> _fCloseDragWindow;

	enum class mediaType {
		Media,
		MediaSource,
		HLS,
		VP9,
		HVC1,
	};
	std::function<bool(mediaType)> _fMediaSupportCheck;

#if ENABLE(NOTIFICATIONS)
	std::function<void(const WTF::URL &url, WebCore::NotificationClient::PermissionHandler&& callback)> _fRequestNotificationPermission;
	enum class NotificationPermission {
		Default, Grant, Deny
	};
	std::function<NotificationPermission(const WTF::URL &url)> _fCheckNotificationPermission;
	std::function<void(WebCore::Notification* notification)> _fShowNotification;
	std::function<void(WebCore::Notification* notification)> _fHideNotification;
#endif

	void clearDelegateCallbacks() {
		_fInvalidate = nullptr;
		_fScroll = nullptr;
		_fSetDocumentSize = nullptr;
		_fActivateNext = nullptr;
		_fActivatePrevious = nullptr;
		_fGoActive = nullptr;
		_fGoInactive = nullptr;
		_fZoomChangedByWheel = nullptr;
		_fUserAgentForURL = nullptr;
		_fChangedTitle = nullptr;
		_fChangedURL = nullptr;
		_fDidStartLoading = nullptr;
		_fDidStopLoading = nullptr;
		_fCanOpenWindow = nullptr;
		_fDoOpenWindow = nullptr;
		_fPopup = nullptr;
		_fContextMenu = nullptr;
		_fHistoryChanged = nullptr;
		_fConsole = nullptr;
		_fDidFailWithError = nullptr;
		_fCanHandleRequest = nullptr;
		_fDownload = nullptr;
		_fDidLoadInsecureContent = nullptr;
		_fAlert = nullptr;
		_fConfirm = nullptr;
		_fPrompt = nullptr;
		_fFile = nullptr;
		_fDownloadAsk = nullptr;
		_fDownloadFromResource = nullptr;
		_fHasAutofill = nullptr;
		_fStoreAutofill = nullptr;
		_fNewTabWindow = nullptr;
		_fSetCursor = nullptr;
		_fAuthChallenge = nullptr;
		_fProgressStarted = nullptr;
		_fProgressUpdated = nullptr;
		_fProgressFinished = nullptr;
		_fHoveredURLChanged = nullptr;
		_fFavIconLoaded = nullptr;
		_fFavIconLoad = nullptr;
		_fPrint = nullptr;
		_fUndoRedoChanged = nullptr;
		_fShouldNavigateToURL = nullptr;
		_fMediaAdded = nullptr;
		_fMediaUpdated = nullptr;
		_fMediaRemoved = nullptr;
		_fMediaWillPlay = nullptr;
		_fMediaSetOverlayCallback = nullptr;
		_fMediaUpdateOverlayCallback = nullptr;
		_fMediaSupportCheck = nullptr;
		_fEnterFullscreen = nullptr;
		_fExitFullscreen = nullptr;
		_fMediaPausedOrFinished = nullptr;
		_fDidReceiveResponse = nullptr;
		_fOpenDragWindow = nullptr;
		_fMoveDragWindow = nullptr;
		_fCloseDragWindow = nullptr;
#if ENABLE(NOTIFICATIONS)
		_fRequestNotificationPermission = nullptr;
		_fCheckNotificationPermission = nullptr;
		_fShowNotification = nullptr;
		_fHideNotification = nullptr;
#endif
	};
	
	WebViewDelegate() { clearDelegateCallbacks(); };
};

#import <ob/OBString.h>

@class WkWebView, OBArray, OBURL;

@protocol WkWebViewMediaTrack <OBObject>

typedef enum {
	WkWebViewMediaTrackType_Audio,
	WkWebViewMediaTrackType_Video,
} WkWebViewMediaTrackType;

- (WkWebViewMediaTrackType)type;
- (OBString *)codec;

@end

@protocol WkWebViewVideoTrack <WkWebViewMediaTrack>

- (int)width;
- (int)height;
- (int)bitrate;

@end

@protocol WkWebViewAudioTrack <WkWebViewMediaTrack>

- (int)frequency;
- (int)channels;
- (int)bits;

@end

@protocol WkMediaObject <OBObject>

- (id<WkWebViewVideoTrack>)videoTrack;
- (id<WkWebViewAudioTrack>)audioTrack;

- (OBArray *)allAudioTracks;
- (OBArray *)allVideoTracks;
- (OBArray *)allTracks;

typedef enum {
	WkMediaObjectType_File,
	WkMediaObjectType_HLS,
	WkMediaObjectType_MediaSource,
} WkMediaObjectType;

- (WkMediaObjectType)type;

typedef IPTR WkWebViewMediaIdentifier;

- (WkWebViewMediaIdentifier)identifier;

- (OBURL *)downloadableURL;

- (BOOL)playing;

- (void)play;
- (void)pause;

- (void)setMuted:(BOOL)muted;
- (BOOL)muted;

- (BOOL)isLive;
- (float)duration;
- (float)position;

- (void)seek:(float)position;

- (BOOL)fullscreen;
- (void)setFullscreen:(BOOL)fs;

- (BOOL)isClearKeyEncrypted;

@end

@protocol WkHLSStream <OBObject>

- (OBString *)url;
- (OBString *)codecs;
- (int)fps;
- (int)width;
- (int)height;
- (int)bitrate;

@end

@protocol WkHLSMediaObject <WkMediaObject>

- (OBArray *)hlsStreams;

- (id<WkHLSStream>)selectedHLSStream;
- (void)setSelectedHLSStream:(id<WkHLSStream>)hlsStream;

@end

@protocol WkWebViewMediaDelegate <OBObject>

typedef enum {
    WkWebViewMediaDelegateQuery_MediaPlayback = 1,
    WkWebViewMediaDelegateQuery_MediaSource = 2,
    WkWebViewMediaDelegateQuery_VP9 = 3,
    WkWebViewMediaDelegateQuery_HLS = 4,
    WkWebViewMediaDelegateQuery_HVC1 = 5,
} WkWebViewMediaDelegateQuery;

- (BOOL)webView:(WkWebView *)view queriedForSupportOf:(WkWebViewMediaDelegateQuery)mode withDefaultState:(BOOL)allow;

- (void)webView:(WkWebView *)view loadedStream:(id<WkMediaObject>)stream;
- (void)webView:(WkWebView *)view unloadedStream:(id<WkMediaObject>)stream;
- (void)webView:(WkWebView *)view playingStream:(id<WkMediaObject>)stream;
- (void)webView:(WkWebView *)view pausedStream:(id<WkMediaObject>)stream;
- (void)webView:(WkWebView *)view updatedStream:(id<WkMediaObject>)stream;

- (void)webView:(WkWebView *)view addedTrack:(id<WkWebViewMediaTrack>)track;
- (void)webView:(WkWebView *)view removedTrack:(id<WkWebViewMediaTrack>)track;
- (void)webView:(WkWebView *)view selectedTrack:(id<WkWebViewMediaTrack>)track;

@end

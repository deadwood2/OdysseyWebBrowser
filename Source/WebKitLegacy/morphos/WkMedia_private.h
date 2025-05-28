#import "WkMedia.h"

@class OBURL, OBMutableArray;

@interface WkWebViewVideoTrackPrivate : OBObject<WkWebViewVideoTrack>
{
	OBString *_codec;
	int _width;
	int _height;
	int _bitrate;
}

- (id)initWithCodec:(OBString *)codec width:(int)width height:(int)height bitrate:(int)bitrate;

@end

@interface WkWebViewAudioTrackPrivate : OBObject<WkWebViewAudioTrack>
{
	OBString *_codec;
	int _frequency;
	int _channels;
	int _bits;
}

- (id)initWithCodec:(OBString *)codec frequency:(int)freq channels:(int)channels bits:(int)bpc;

@end

@protocol WkMediaObjectComms <OBObject, WkHLSMediaObject>

- (BOOL)playing;

- (void)play;
- (void)pause;

- (void)setMuted:(BOOL)muted;
- (BOOL)muted;

- (BOOL)isLive;
- (BOOL)isHLS;

- (float)duration;
- (float)position;

- (void)seek:(float)position;

- (BOOL)fullscreen;
- (void)setFullscreen:(BOOL)fs;

- (BOOL)isClearKeyEncrypted;

@end

@interface WkMediaObjectPrivate : OBObject<WkMediaObject, WkHLSMediaObject>
{
	id<WkWebViewAudioTrack>  _audioTrack;
	id<WkWebViewVideoTrack>  _videoTrack;
	id<WkMediaObjectComms>   _identifier;
	WkMediaObjectType        _type;
	OBURL                   *_url;
	OBMutableArray          *_tracks;
}

- (id)initWithType:(WkMediaObjectType)type identifier:(WkWebViewMediaIdentifier)identifier
	audioTrack:(id<WkWebViewAudioTrack>)audioTrack videoTrack:(id<WkWebViewVideoTrack>)videoTrack downloadableURL:(OBURL *)url;

- (void)addTrack:(id<WkWebViewMediaTrack>)track;
- (void)removeTrack:(id<WkWebViewMediaTrack>)track;
- (void)selectTrack:(id<WkWebViewMediaTrack>)track;

@end

@interface WkHLSStreamPrivate : OBObject<WkHLSStream>
{
	OBString *_url;
	OBString *_codecs;
	int _fps;
	int _width;
	int _height;
	int _bitrate;
}

- (id)initWithURL:(OBString *)url codecs:(OBString *)codecs fps:(int)fps bitrate:(int)br width:(int)width height:(int)height;

@end

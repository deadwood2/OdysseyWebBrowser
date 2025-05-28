#import "WkMedia_private.h"
#import <ob/OBURL.h>
#import <ob/OBArrayMutable.h>

static inline ULONG _hash(ULONG x) {
    x = ((x >> 16) ^ x) * 0x45d9f3b;
    x = ((x >> 16) ^ x) * 0x45d9f3b;
    x = (x >> 16) ^ x;
    return x;
}

extern "C" void dprintf(const char *,...);

@implementation WkMediaObjectPrivate

- (id)initWithType:(WkMediaObjectType)type identifier:(WkWebViewMediaIdentifier)identifier
	audioTrack:(id<WkWebViewAudioTrack>)audioTrack videoTrack:(id<WkWebViewVideoTrack>)videoTrack downloadableURL:(OBURL *)url
{
	if ((self = [super init]))
	{
		_tracks = [OBMutableArray new];
		_audioTrack = [audioTrack retain];
		_videoTrack = [videoTrack retain];
		[_tracks addObject:audioTrack];
		[_tracks addObject:videoTrack];
		_identifier = [(id<WkMediaObjectComms>)identifier retain];
		_type = type;
		_url = [url retain];
	}
	
	return self;
}

- (void)dealloc
{
	[_audioTrack release];
	[_videoTrack release];
	[_url release];
	[_identifier release];
	[_tracks release];
	[super dealloc];
}

- (BOOL)isEqual:(id)otherObject
{
	if (otherObject == self)
		return YES;
	if ([otherObject isKindOfClass:[self class]])
	{
		WkMediaObjectPrivate *other = otherObject;
		if (other->_identifier == _identifier)
			return YES;
	}
	return NO;
}

- (ULONG)hash
{
	return _hash((ULONG)_identifier);
}

- (id<WkWebViewVideoTrack>)videoTrack
{
	return _videoTrack;
}

- (id<WkWebViewAudioTrack>)audioTrack
{
	return _audioTrack;
}

- (OBArray *)allAudioTracks
{
	return nil;
}

- (OBArray *)allVideoTracks
{
	return nil;
}

- (OBArray *)allTracks
{
	return _tracks;
}

- (WkMediaObjectType)type
{
	return _type;
}

- (WkWebViewMediaIdentifier)identifier
{
	return (WkWebViewMediaIdentifier)_identifier;
}

- (OBURL *)downloadableURL
{
	return _url;
}

- (BOOL)playing
{
	return [_identifier playing];
}

- (void)play
{
	[_identifier play];
}

- (void)pause
{
	[_identifier pause];
}

- (void)setMuted:(BOOL)muted
{
	[_identifier setMuted:muted];
}

- (BOOL)muted
{
	return [_identifier muted];
}

- (BOOL)isLive
{
	return [_identifier isLive];
}

- (float)duration
{
	return [_identifier duration];
}

- (float)position
{
	return [_identifier position];
}

- (void)seek:(float)position
{
	[_identifier seek:position];
}

- (BOOL)fullscreen
{
	return [_identifier fullscreen];
}

- (void)setFullscreen:(BOOL)fs
{
	[_identifier setFullscreen:fs];
}

- (void)addTrack:(id<WkWebViewMediaTrack>)track
{
	if (nil == track)
		return;

	[_tracks addObject:track];
}

- (void)removeTrack:(id<WkWebViewMediaTrack>)track
{
	if (nil == track)
		return;

	[_tracks removeObject:track];

	if (_audioTrack == track)
	{
		[_audioTrack release];
		_audioTrack = nil;
	}
	else if (_videoTrack == track)
	{
		[_videoTrack release];
		_videoTrack = nil;
	}
}

- (void)selectTrack:(id<WkWebViewMediaTrack>)track
{
	if (nil == track)
		return;

	if ([track type] == WkWebViewMediaTrackType_Audio)
	{
		[_audioTrack autorelease];
		_audioTrack = (id)[track retain];
	}
	else
	{
		[_videoTrack autorelease];
		_videoTrack = (id)[track retain];
	}
}

- (BOOL)isClearKeyEncrypted
{
	return [_identifier isClearKeyEncrypted];
}

- (OBArray *)hlsStreams
{
	return [_identifier hlsStreams];
}

- (id<WkHLSStream>)selectedHLSStream
{
	return [_identifier selectedHLSStream];
}

- (void)setSelectedHLSStream:(id<WkHLSStream>)hlsStream
{
	[_identifier setSelectedHLSStream:hlsStream];
}

@end

@implementation WkWebViewVideoTrackPrivate

- (id)initWithCodec:(OBString *)codec width:(int)width height:(int)height bitrate:(int)bitrate
{
	if ((self = [super init]))
	{
		_codec = [codec copy];
		_width = width;
		_height = height;
		_bitrate = bitrate;
	}
	
	return self;
}

- (void)dealloc
{
	[_codec release];
	[super dealloc];
}

- (OBString *)codec
{
	return _codec;
}

- (int)width
{
	return _width;
}

- (int)height
{
	return _height;
}

- (int)bitrate
{
	return _bitrate;
}

- (WkWebViewMediaTrackType)type
{
	return WkWebViewMediaTrackType_Video;
}

@end

@implementation WkWebViewAudioTrackPrivate

- (id)initWithCodec:(OBString *)codec frequency:(int)freq channels:(int)channels bits:(int)bpc
{
	if ((self = [super init]))
	{
		_codec = [codec retain];
		_frequency = freq;
		_channels = channels;
		_bits = bpc;
	}
	
	return self;
}

- (void)dealloc
{
	[_codec release];
	[super dealloc];
}

- (OBString *)codec
{
	return _codec;
}

- (int)frequency
{
	return _frequency;
}

- (int)channels
{
	return _channels;
}

- (int)bits
{
	return _bits;
}

- (WkWebViewMediaTrackType)type
{
	return WkWebViewMediaTrackType_Audio;
}

@end

@implementation WkHLSStreamPrivate

- (id)initWithURL:(OBString *)url codecs:(OBString *)codecs fps:(int)fps bitrate:(int)br width:(int)width height:(int)height
{
	if ((self = [super init]))
	{
		_url = [url copy];
		_codecs = [codecs copy];
		_fps = fps;
		_bitrate = br;
		_width = width;
		_height = height;
	}
	
	return self;
}

- (void)dealloc
{
	[_codecs release];
	[_url release];
	[super dealloc];
}

- (OBString *)url
{
	return _url;
}

- (OBString *)codecs
{
	return _codecs;
}

- (int)fps
{
	return _fps;
}

- (int)width
{
	return _width;
}

- (int)height
{
	return _height;
}

- (int)bitrate
{
	return _bitrate;
}

@end

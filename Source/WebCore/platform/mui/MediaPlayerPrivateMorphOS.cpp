/*
 * Copyright (C) 2010 Fabien Coeurjoly <fabien.coeurjoly@wanadoo.fr>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * aint with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include "config.h"

#if ENABLE(VIDEO)

#include <limits>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <cairo.h>
#include <unistd.h>
#undef bind

#include "MediaPlayerPrivateMorphOS.h"

#include "Document.h"
#include "Frame.h"
#include "FrameView.h"
#include "GraphicsContext.h"
#include "HostWindow.h"
#include "HTMLMediaElement.h"
#include "IntRect.h"
#include "URL.h"
#include "MIMETypeRegistry.h"
#include "MediaPlayer.h"
#include "NetworkingContext.h"
#include "PlatformContextCairo.h"
#include "ScrollView.h"
#include "TimeRanges.h"
#include "Timer.h"
#include "Widget.h"

#include "ResourceError.h"
#include "ResourceHandle.h"
#include "ResourceHandleInternal.h"
#include "ResourceRequest.h"
#include "ResourceHandleClient.h"
#include "ResourceResponse.h"

#include <wtf/text/CString.h>
#include <runtime/InitializeThreading.h>
#include <wtf/CurrentTime.h>
#include <wtf/MainThread.h>
#include <wtf/StdLibExtras.h>

/* AHIBase tricks */
#ifdef AHI_BASE_NAME
#undef AHI_BASE_NAME
#endif
#define AHI_BASE_NAME stream->localAHIBase

#include <exec/exec.h>
#include <clib/alib_protos.h>
#include <clib/macros.h>
#include <clib/debug_protos.h>
#include <devices/ahi.h>

#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/ahi.h>

#include "gui.h"
#include "utils.h"

extern "C" {
#define HAVE_BIGENDIAN 1
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>

#include "acinerella.h"

struct Library *ffmpegSocketBase = NULL; // Not used, but defined in my ffmpeg build

}

/* Custom extensions to allow returning an id for the jobs sent to main task, allowing to cancel them later */
namespace WTF {
extern long long callOnMainThreadFab(MainThreadFunction*, void* context);
extern void removeFromMainThreadFab(long long id);
}

/********************************************************************************/

/* debug */
#define D(x)

/* options */
#define USE_WEBM 1                          /* WebM/VP8/VP9 support */
#define DUMPINFO 0                          /* Additional dump information */

/* constants */
#define READ_WAIT_TIME         10000        /* Read callback idle time (ms) */
#define DECODER_WAIT_TIME      10000        /* Decoder idle time */
#define DEMUXER_WAIT_TIME      10000        /* Demuxer idle time */
#define NEW_REQUEST_THRESHOLD  512*1024     /* The distance between current position and target position above which a new request should be issued */
#define PREBUFFER_SIZE         2*1024*1024  /* Buffer size minimum allocation size */
#define VIDEO_MAX_SIZE         5*256*1024   /* Maximum buffer size in video queue */
#define AUDIO_MAX_SIZE         20*16*1024   /* Maximum buffer size in audio queue */

BENCHMARK_DECLARE;

using namespace std;

namespace WebCore {

/* Attempt to clean after WebKit, in case it leaks media objects */
static Vector<MediaPlayerPrivate *> mediaObjects;

void freeLeakedMediaObjects()
{
	D(kprintf("Leaked media instance count %d\n", mediaObjects.size()));
	while(mediaObjects.size())
	{
		kprintf("Found a leaked mediaplayer instance (0x%p)... Cleaning after WebKit\n", mediaObjects[0]);
		delete mediaObjects[0]; // mediaObject item removes itself from list at deletion
	}
	mediaObjects.clear();
}

/* ByteRange helper class */

class ByteRanges
{
public:
	unsigned length() const
	{
		return m_ranges.size();
	}

	void clear()
	{
		m_ranges.clear();
	}

	long long start(unsigned index) const
	{
		if (index >= length())
		{
			return -1;
		}
		return m_ranges[index].m_start;
	}

	long long end(unsigned index) const
	{
		if (index >= length())
		{
			return -1;
	    }
	    return m_ranges[index].m_end;
	}

	void add(long long start, long long end)
	{
	    unsigned int overlappingArcIndex;
	    Range addedRange(start, end);

	    // For each present range check if we need to:
	    // - merge with the added range, in case we are overlapping or contiguous
	    // - Need to insert in place, we we are completely, not overlapping and not contiguous
	    // in between two ranges.
	    //
	    // TODO: Given that we assume that ranges are correctly ordered, this could be optimized.

	    for (overlappingArcIndex = 0; overlappingArcIndex < m_ranges.size(); overlappingArcIndex++) {
	        if (addedRange.isOverlappingRange(m_ranges[overlappingArcIndex])
	         || addedRange.isContiguousWithRange(m_ranges[overlappingArcIndex])) {
	            // We need to merge the addedRange and that range.
	            addedRange = addedRange.unionWithOverlappingOrContiguousRange(m_ranges[overlappingArcIndex]);
	            m_ranges.remove(overlappingArcIndex);
	            overlappingArcIndex--;
	        } else {
	            // Check the case for which there is no more to do
	            if (!overlappingArcIndex) {
	                if (addedRange.isBeforeRange(m_ranges[0])) {
	                    // First index, and we are completely before that range (and not contiguous, nor overlapping).
	                    // We just need to be inserted here.
	                    break;
	                }
	            } else {
	                if (m_ranges[overlappingArcIndex - 1].isBeforeRange(addedRange)
	                 && addedRange.isBeforeRange(m_ranges[overlappingArcIndex])) {
	                    // We are exactly after the current previous range, and before the current range, while
	                    // not overlapping with none of them. Insert here.
	                    break;
	                }
	            }
	        }
	    }

	    // Now that we are sure we don't overlap with any range, just add it.
	    m_ranges.insert(overlappingArcIndex, addedRange);
	}

	bool contain(long long pos) const
	{
		for (unsigned n = 0; n < length(); n++)
		{
			if (pos >= start(n) && pos <= end(n))
	            return true;
	    }
	    return false;
	}

	bool contain(long long startpos, long long endpos) const
	{
		for (unsigned n = 0; n < length(); n++)
		{
			if (startpos >= start(n) && endpos <= end(n))
	            return true;
	    }
	    return false;
	}

	long long bytesInRanges()
	{
		long long sum = 0;
		for (unsigned n = 0; n < length(); n++)
		{
			sum += end(n) - start(n) + 1;
	    }

		return sum;
	}

	long long nearestEnd(long long pos)
	{
		for (unsigned n = 0; n < length(); n++)
		{
			if (pos >= start(n) && pos <= end(n))
			{
				return end(n);
			}
	    }	 
		return 0;
	}

	ByteRanges() { }

	ByteRanges(long long start, long long end)
	{
		add(start, end);
	}

private:

    // We consider all the Ranges to be semi-bounded as follow: [start, end[
	struct Range
	{
        Range() { }

		Range(long long start, long long end)
        {
            m_start = start;
            m_end = end;
        }
		long long m_start;
		long long m_end;

		inline bool isPointInRange(long long point) const
        {
			return m_start <= point && point <= m_end;
        }

        inline bool isOverlappingRange(const Range& range) const
        {
            return isPointInRange(range.m_start) || isPointInRange(range.m_end) || range.isPointInRange(m_start);
        }

        inline bool isContiguousWithRange(const Range& range) const
        {
			return range.m_start == m_end + 1 || range.m_end + 1 == m_start;
        }

        inline Range unionWithOverlappingOrContiguousRange(const Range& range) const
        {
            Range ret;

            ret.m_start = std::min(m_start, range.m_start);
            ret.m_end = std::max(m_end, range.m_end);

            return ret;
        }

        inline bool isBeforeRange(const Range& range) const
        {
			return range.m_start > m_end;
        }
    };

    Vector<Range> m_ranges;
};


/* Audio Stream */

typedef struct 
{
	int                     opened;

	/* Tasks */
	struct Task*            audiotask;
	struct Task*            callertask;

	/* AHI */
	BYTE                    AHIDevice;
	struct Library*			localAHIBase;
	struct MsgPort*         AHImp;
	struct AHIRequest*      AHIio;
	struct AHIAudioCtrl*    actrl;
	struct AHISampleInfo    sample0;
	struct AHISampleInfo    sample1;
	ULONG                   samples_loaded;
	ULONG                   sample_count;
	LONG 		       		sample_len;
	ULONG                   dbflag;

	/* IPC signals */
	struct SignalSemaphore  sembuffer;       /* buffer protection */
	BYTE                    sigsound;
	BYTE                    sigkill;
	BYTE                    sigcaller;

	/* Buffer management */
	char*                   buffer;
	LONG		            buffer_len;
	LONG		            buffer_put;
	LONG                    buffer_get;
	LONG                    written_len;

	/* Stream properties */
	ULONG    				sample_rate;
	ULONG					sample_bits;
	ULONG					sample_channels;

} _Stream;

/* Package helper class (mostly useless) */
class ACPackage
{
public:
	ACPackage(ac_package *package)
		: m_package(package)
	{}
	~ACPackage()
	{}

	ac_package *package()
	{
		return m_package;
	}

private:
	ac_package *m_package;
};

/* Main context storing all media attributes */

class StreamClient;

static int CALL_CONVT read_callback(void *sender, char *dst, int size);
static int64_t CALL_CONVT seek_callback(void *sender, int64_t pos, int whence);
static int CALL_CONVT open_callback(void *sender);
static int CALL_CONVT close_callback(void *sender);

class FFMpegContext
{
	/* Media Buffer helper class */
	class MediaBuffer
	{
	public:
		uint8_t	*data;	            /* memory, malloced if size > 0, just reference* otherwise */
		long long size;	            /* total size in bytes */
		long long readpos;          /* read cursor position */
		long long requestpos;       /* current request start position */
		long long requestlen;       /* current request len */
		long long requestgot;       /* current request len */
		ByteRanges ranges;          /* ranges of downloaded data */

		long long readPos()
		{
			return readpos;
		}

		void setReadPos(long long pos)
		{
			readpos = pos;
		}

		uint8_t *source()
		{
			return data;
		}

		void init()
		{
			data = NULL;
			size = 0;
			readpos = 0;
			requestpos = 0;
			requestlen = 0;
			requestgot = 0;
			ranges.clear();
		}

		void cleanup()
		{
			if (data && size)
				free(data);

			data = NULL;
			size = 0;
			readpos = 0;
			requestpos = 0;
			requestlen = 0;
			requestgot = 0;
			ranges.clear();
		}

		bool append(FFMpegContext *ctx, uint8_t *src, int len)
		{
			long long offset = requestpos + requestgot;

			if (data == NULL)
			{
				if(ctx->totalsize > 0 && canAllocateMemory(ctx->totalsize))
				{
					size = ctx->totalsize;
				}
				else
				{
					size = offset + len + PREBUFFER_SIZE;
				}

				data = (uint8_t *) malloc(size);

#if DUMPINFO
				if(data)
				{
					memset(data, 0, size);
				}
#endif

				D(kprintf("[MediaPlayer] MediaBuffer::append: Allocating buffer (%lld bytes)\n", size));
			}
			else if (offset + len > size)
			{
				size = offset + len + PREBUFFER_SIZE;
				data = (uint8_t *) realloc(data, size);

				D(kprintf("[MediaPlayer] MediaBuffer::append: Extending buffer (%lld bytes)\n", size));
			}

			if (data == NULL)
			{
				return false;
			}

			memcpy(data + offset, src, len);
			requestgot += len;

			D(kprintf("[MediaPlayer] MediaBuffer::append: Adding byterange [%lld, %lld] (ranges %d)\n", offset, offset + len - 1, ranges.length()));
			ranges.add(offset, offset + len - 1); // Overlapping and Contiguous ranges will be merged

			return true;
		}

		bool needMoreData(FFMpegContext *ctx, long long size)
		{
			bool res;
			long long endpos;

			if(ctx->totalsize > 0)
			{
				endpos = std::min(readpos + size - 1, ctx->totalsize - 1);
			}
			else
			{
				endpos = readpos + size - 1;
			}

			res = !(ranges.contain(readpos, endpos));

			//D(kprintf("needMoreData() requested range [%lld, %lld] %d\n", readpos, endpos, res));

			return res;
		}

		void setRequest(long long start, long long length, long long got)
		{
			requestpos = start;
			requestlen = length;
			requestgot = got;
		}

		bool shouldSendRequest(bool partial_content)
		{
			// If requested data is too far after current request range or before, issue a new fetch request
			bool res = partial_content && ((readpos > requestpos + requestgot + NEW_REQUEST_THRESHOLD) || (readpos < requestpos));

			//D(kprintf("shouldSendRequest() %d\n", res));

			return res;
		}


		bool isComplete(FFMpegContext *ctx)
		{
			bool res = false;

			if(ctx->totalsize > 0)
			{
				res = ranges.bytesInRanges() >= ctx->totalsize;
			}

			D(kprintf("isComplete() %lld %lld %d\n", ranges.bytesInRanges(), ctx->totalsize, res));

			return res;
		}

		float maxTimeLoaded(FFMpegContext *ctx, double now, double duration)
		{
			float res = 0.0;

			if(duration == numeric_limits<float>::infinity())
			{
				res = duration; // Hmm...
			}
			else if(ctx->totalsize > 0 && duration > 0)
			{
				long long currentbyte = (long long) ((ctx->totalsize - 1) * now / duration) ;
				res = min(duration, duration * ranges.nearestEnd(currentbyte) / ctx->totalsize);
				D(kprintf("maxTimeLoaded(%f, %f) currentbyte: %lld %f\n", now, duration, currentbyte, res));
			}		 

			return std::max((float) now, res);
		}
	};

public:

	FFMpegContext(MediaPlayerPrivate *obj)
	{
		running         = false;
		allow_seek      = false;
		resource_handle = nullptr;
		stream_client   = 0;
		transfer_completed = false;
		network_error   = 0;
		received        = 0;
		totalsize       = 0;
		fetch_offset    = 0;
		first_response_received = false;

		ac = ac_init();
		ac->output_format = AC_OUTPUT_RGBA32;

		video    = 0;
		audio    = 0;
		opened   = false;
		bitrate  = 0;
		duration = 0;

		has_video           = false;
		framerate           = 24; // silly default
		width               = 0;
		height              = 0;
		aspect              = 0.0;
		video_duration      = 0.0;
		video_last_timecode = 0.0;

		has_audio       = false;
		sample_rate     = 0;
		sample_channels = 0;
		sample_bits     = 0;
		sample_format   = 0;
		audio_duration  = 0;
		audio_timecode  = 0;
		audio_stream    = 0;

		media_buffer.init();

        buffer_mutex = new Mutex;

		video_thread = 0;
		video_thread_running = false;
		video_queue_size = 0;
		video_queue  = new Vector<ACPackage>;
		video_mutex  = new Mutex;
		video_condition = new ThreadCondition;

		audio_thread = 0;
		audio_thread_running = false;
		audio_queue_size = 0;
		audio_queue  = new Vector<ACPackage>;
		audio_mutex  = new Mutex;
		audio_condition = new ThreadCondition;
		audio_output_mutex = new Mutex;

		mediaplayer = obj;
		url = "";
		mimetype = "";

		use_webm = getv(app, MA_OWBApp_EnableVP8);
		use_partial_content = getv(app, MA_OWBApp_EnablePartialContent);
		loopfilter_mode = getv(app, MA_OWBApp_LoopFilterMode);
	}

	~FFMpegContext()
	{
		media_buffer.cleanup();
		ac_free(ac);

		delete video_queue;
		delete audio_queue;
		delete video_mutex;
		delete audio_mutex;
        delete buffer_mutex;
		delete video_condition;
		delete audio_condition;
		delete audio_output_mutex;
	}

	bool open()
	{
		if(opened)
		{
			return false;
		}

		ac_open(ac, this, &open_callback, &read_callback, &seek_callback, &close_callback);

		opened = ac->opened;

		if(!running)
		{
			D(kprintf("[MediaPlayer Thread] Interrupted while parsing metadata.\n"));
		}

		if(running && opened)
		{
			//Display the count of the found data streams.
			D(kprintf("Count of Datastreams:  %d \n", ac->stream_count));

			//Print file info
			D(kprintf("File duration: %lld \n", ac->info.duration));
			D(kprintf("File bitrate: %d \n",    ac->info.bitrate));
			D(kprintf("Title: %s \n",           ac->info.title));
			D(kprintf("Author: %s \n",          ac->info.author));
			D(kprintf("Album: %s \n",           ac->info.album));

			duration = (double) ac->info.duration;

			//Go through every stream and fetch information about it.
			for (int i = ac->stream_count - 1; i >= 0; --i)
			{
				D(kprintf("\nInformation about stream %d: \n", i));

				ac_stream_info info;

				//Get information about stream no. i
				ac_get_stream_info(ac, i, &info);

				switch (info.stream_type)
				{
					//Stream is a video stream - display information about it
					case AC_STREAM_TYPE_VIDEO:
						D(kprintf("Stream is an video stream.\n--------------------------\n\n"));
						D(kprintf(" * Width            : %d\n",    info.additional_info.video_info.frame_width));
						D(kprintf(" * Height           : %d\n",    info.additional_info.video_info.frame_height));
						D(kprintf(" * Pixel aspect     : %f\n",    info.additional_info.video_info.pixel_aspect));
						D(kprintf(" * Frames per second: %lf \n",  1.0 / info.additional_info.video_info.frames_per_second));
						D(kprintf(" * Duration         : %f\n",    info.additional_info.video_info.duration));

					    //If we don't have a video decoder now, try to create a video decoder for this video stream
						if (video == NULL)
						{
							video          = ac_create_decoder(ac, i);
							has_video      = video != NULL;
							width          = info.additional_info.video_info.frame_width;
							height         = info.additional_info.video_info.frame_height;
							framerate      = (1.0 / info.additional_info.video_info.frames_per_second);
							aspect         = info.additional_info.video_info.pixel_aspect;
							video_duration = info.additional_info.video_info.duration;

							D(kprintf(" * Decoder          : 0x%p\n", video));
					    }
						break;

					//Stream is an audio stream - display information about it
					case AC_STREAM_TYPE_AUDIO:
						D(kprintf("Stream is an audio stream.\n--------------------------\n\n"));
						D(kprintf("  * Samples per Second: %d\n", info.additional_info.audio_info.samples_per_second));
						D(kprintf("  * Channel count     : %d\n", info.additional_info.audio_info.channel_count));
						D(kprintf("  * Bit depth         : %d\n", info.additional_info.audio_info.bit_depth));
						D(kprintf("  * Duration          : %f\n", info.additional_info.audio_info.duration));

					    //If we don't have an audio decoder now, try to create an audio decoder for this audio stream
						if (audio == NULL)
						{
							audio           = ac_create_decoder(ac, i);
							has_audio       = audio != NULL;
							sample_rate     = info.additional_info.audio_info.samples_per_second;
							//sample_channels = info.additional_info.audio_info.channel_count;
							//sample_bits     = info.additional_info.audio_info.bit_depth;
							//sample_format   = AV_SAMPLE_FMT_S16;
							sample_channels = 2; // Force this output format, since we convert to it if needed, anyway
							sample_bits     = 16;
							audio_duration  = info.additional_info.audio_info.duration;

							D(kprintf("  * Decoder           : 0x%p\n", audio));
					    }
						break;

					default:
						;
				}
			}

			bitrate = (ac->info.bitrate > 0) ? ac->info.bitrate : 0;

			if(!has_video && has_audio)
			{
			    duration = audio_duration;
			}

			// Try to get duration even if it's missing from format...
			if(duration <= 0.0)
			{
				D(kprintf("Format tells nothing about duration, use streams duration\n"));
				duration = std::max(video_duration, audio_duration);

				// If it's still missing, approximate with bitrate and filesize
				if(duration <= 0.0 && (totalsize > 0 && bitrate > 0))
				{
					D(kprintf("Estimating duration from bitrate (not very precise)\n"));
					duration = totalsize * 8 / bitrate;
				}
				else
				{
					duration = numeric_limits<float>::infinity();
				}
			}

			video_last_timecode = 0.0;
			audio_timecode      = 0.0;

			D(kprintf("Duration: %f Bitrate: %d\n", duration, bitrate));

			/*
			if(has_audio)
			{
				mediaplayer->audioOpen();
			}
			*/
		}

		if(!opened)
		{
			D(kprintf("No video/audio information found."));
		}

		return opened;
	}

	void close()
	{
		D(kprintf("[MediaPlayer Thread] ffmpeg_close()\n"));

		if(opened)
		{
			opened = false;

			D(kprintf("[MediaPlayer Thread] ffmpeg_close: audioClose()\n"));

			mediaplayer->audioClose();

			D(kprintf("[MediaPlayer Thread] ffmpeg_close: free video decoder\n"));

			if(video)
			{
				ac_free_decoder(video);
				video = NULL;
			}

			D(kprintf("[MediaPlayer Thread] ffmpeg_close: free audio decoder\n"));

			if(audio)
			{
				ac_free_decoder(audio);
				audio = NULL;
			}

			D(kprintf("[MediaPlayer Thread] ffmpeg_close: free ffmpeg context\n"));

			if(ac)
			{
				ac_close(ac);
			}
		}

		media_buffer.cleanup();

		D(kprintf("[MediaPlayer Thread] ffmpeg_close() OK\n"));
	}

	bool supportsPartialContent()
	{
		bool res = true;
		URL kurl(URL(), url);

		// Only support it for http protocol for now
		if(!kurl.protocolIs("http") && !kurl.protocolIs("https"))
		{
			res = false;
		}

		// Blacklist weirdo servers returning crap in range response.
		/*
		if(kurl.host().find(".dailymotion.") != notFound)
		{
			res = false;
		}
		*/

		//D(kprintf("supportsPartialContent(%s) %d\n", url.latin1().data(), res));

		return res;
	}

	bool                   running;      /* thread loop flag */

	/* buffer management */
	Mutex*                 buffer_mutex;  /* buffer protection */
	MediaBuffer            media_buffer;  /* incoming data buffer */
	bool                   allow_seek;    /* is seek operation allowed */

	/* transport handling */
	RefPtr<ResourceHandle> resource_handle;         /* network handle */
	StreamClient*          stream_client;           /* resourcehandle client */
	bool 				   transfer_completed;      /* end of transfer flag */
	int                    network_error;           /* transfer error value */
	long long              received;                /* number of bytes received */
	long long              totalsize;               /* total size of file (0 if not known) */
	long long              fetch_offset;            /* currently requested offset */
	bool                   first_response_received; /* flag to differentiate first issued request from the next ones */

	/* ffmpeg objects */
	ac_instance*           ac;           /* ffmpeg instance */
	ac_decoder*            video;        /* video decoder instance */
	ac_decoder*            audio;        /* audio decoder instance */
	bool                   opened;

	/* stream properties */
	int                    bitrate;      /* bitrate in bit/s */
	double				   duration;     /* duration in s */

	/* video properties */
	bool                   has_video;
	double				   framerate;
	int                    width;        /* video width */
	int                    height;       /* video height */
	double                 aspect;       /* video pixel ratio */
	double                 video_duration;
	double                 video_last_timecode;

	/* audio properties */
	bool                   has_audio;
	int                    sample_rate;
	int                    sample_bits;
	int                    sample_channels;
	int                    sample_format;
	double                 audio_duration;
	double                 audio_timecode;     /* audio timecode, with output compensation */
	_Stream*                audio_stream;       /* output context */

	/* video decoder thread and queue */
	Vector<ACPackage>*     video_queue;
	ThreadIdentifier       video_thread;
	bool                   video_thread_running;
	Mutex*                 video_mutex;
	ThreadCondition*       video_condition;
	int                    video_queue_size;

	/* audio decoder thread and queue */
	Vector<ACPackage>*     audio_queue;
	ThreadIdentifier       audio_thread;
	bool                   audio_thread_running;
	Mutex*                 audio_mutex;
	ThreadCondition*       audio_condition;
	int                    audio_queue_size;

	/* audio output thread */
	ThreadIdentifier       audiooutput_thread;
	bool                   audiooutput_thread_running;
	Mutex*                 audio_output_mutex;

	/* general */
	String                 mimetype;     /* content mimetype */
	String                 url;          /* content url */
	MediaPlayerPrivate*    mediaplayer;  /* mediaplayer object */

	/* settings */
	bool                   use_webm;
	bool                   use_partial_content;
};

#if DUMPINFO
static void dumpHex(const char *data, int len)
{
	int i;
	for(i = 0; i < len / 32; i++)
	{
		int j;

		for(j = 0; j < 8; j++)
		{
			int k;
			for(k = 0; k < 4; k++)
			{
				kprintf("%.2x", data[i*32 + j*4 + k]);
			}
			kprintf(" ");
		}
		kprintf("\n");
	}
}

static void dumpMedia(FFMpegContext *ctx)
{
	OWBFile *dumpfile = new OWBFile("ram:dump");

	if(dumpfile)
	{
		dumpfile->open('w');
		if(ctx->media_buffer.source())
			dumpfile->write(ctx->media_buffer.source(), ctx->totalsize);
		delete dumpfile;
	}

}
#endif

/* Kiero Scaler */

enum{
	IMG_PIXFMT_ARGB8888 = 0,
	IMG_PIXFMT_RGB888,
};

struct image{
	union {
		unsigned char *b;
		unsigned int *l;
		unsigned short *w;
		void *p;
	}data;

	int width;
	int height;
	int pixfmt;
	int stride;
	int depth;
};

int imgBytesPerPixel(int pixfmt)
{
	switch (pixfmt)
	{
		case IMG_PIXFMT_ARGB8888:
			return 4;
		case IMG_PIXFMT_RGB888:
			return 3;
		default:
			;

	}

	return 0;
}

struct image *imgCreate(int width, int height, int pixfmt)
{
	struct image *img = (struct image *) AllocVec(sizeof(*img), 0);

	if (img != NULL)
	{
#if OS(AROS)
        posix_memalign(&(img->data.p), 16, width * height * imgBytesPerPixel(pixfmt));
#else
		img->data.p = AllocVecAligned(width * height * imgBytesPerPixel(pixfmt), 0, 16, 0);
#endif
		if (img->data.p != NULL)
		{
			img->width = width;
			img->height = height;
			img->pixfmt = pixfmt;
			img->depth = 24;

			return img;
		}

		FreeVec(img);
	}

	return NULL;
}

void imgFree(struct image *img)
{
	if (img != NULL)
	{
		if (img->data.p != NULL)
#if OS(AROS)
            free(img->data.p);
#else
			FreeVec(img->data.p);
#endif

		FreeVec(img);
	}
}

static inline void getbilinearpixel32(unsigned char *data, int rowbytes, int px, int py, unsigned char *dest)
{
	int px0,py0;
	unsigned int dx,dy;
	unsigned int dx1,dy1;

	unsigned int w;

	unsigned int p1, p2;
	unsigned int pp1,pp2;
	unsigned int ww=(1<<7);
	px0 = px >> 16;                         /* round down */
	py0 = py >> 16;
	dx = px & 0x0000ffff;					/* get the fraction */
	dy = py & 0x0000ffff;

	dx1 = 65535 - dx;
	dy1 = 65535 - dy;

	data = data + py0 * rowbytes + px0 * 4;

	/* apply weights */

	w = dx1 *dy1 >> 25;
	p1 = *(unsigned int*)data;
	p2 = p1 & 0x00ff00ff;
	p1 = (p1 & 0xff00ff00)>>8;
	pp1 = p1 * w;
	pp2 = p2 * w;
	ww-=w;

	data += 4;
	w = dx * dy1 >> 25;
	p1 = *(unsigned int*)data;
	p2 = p1 & 0x00ff00ff;
	p1 = (p1 & 0xff00ff00)>>8;
	pp1 += p1 * w;
	pp2 += p2 * w;
	ww-=w;

	data += rowbytes - 4;
	w = dx1 *dy >> 25;
	p1 = *(unsigned int*)data;
	p2 = p1 & 0x00ff00ff;
	p1 = (p1 & 0xff00ff00)>>8;
	pp1 += p1 * w;
	pp2 += p2 * w;
	ww-=w;

	data += 4;
	w = ww;
	p1 = *(unsigned int*)data;
	p2 = p1 & 0x00ff00ff;
	p1 = (p1 & 0xff00ff00)>>8;
	pp1 += p1 * w;
	pp2 += p2 * w;

	pp1 = (pp1 << 1) & 0xff00ff00;
	pp2 = (pp2 >> 7) & 0x00ff00ff;

	*(unsigned int*)dest = pp1 | pp2;
}

static inline void getbilinearpixel24(unsigned char *data, int rowbytes, int px, int py, unsigned char *dest)
{
	int px0,py0;
	unsigned int dx,dy;
	unsigned int dx1,dy1;

	unsigned int w;

	unsigned int p1, p2, p3;
	unsigned int pp1,pp2,pp3;
	unsigned int ww = (1 << 16);
	px0 = px >> 16;                         /* round down */
	py0 = py >> 16;
	dx = px & 0x0000ffff;					/* get the fraction */
	dy = py & 0x0000ffff;

	dx1 = 65535 - dx;
	dy1 = 65535 - dy;

	data = data + py0 * rowbytes + px0 * 3;

	/* apply weights */

	w = dx1 *dy1 >> 16;
	p1 = data[0];
	p2 = data[1];
	p3 = data[2];
	pp1 = p1 * w;
	pp2 = p2 * w;
	pp3 = p3 * w;
	ww-=w;

	data += 3;
	w = dx * dy1 >> 16;
	p1 = data[0];
	p2 = data[1];
	p3 = data[2];
	pp1 += p1 * w;
	pp2 += p2 * w;
	pp3 += p3 * w;
	ww-=w;

	data += rowbytes - 3;
	w = dx1 *dy >> 16;
	p1 = data[0];
	p2 = data[1];
	p3 = data[2];
	pp1 += p1 * w;
	pp2 += p2 * w;
	pp3 += p3 * w;
	ww-=w;

	data += 3;
	w = ww;
	p1 = data[0];
	p2 = data[1];
	p3 = data[2];
	pp1 += p1 * w;
	pp2 += p2 * w;
	pp3 += p3 * w;

	*dest++ = pp1 >> 16;
	*dest++ = pp2 >> 16;
	*dest++ = pp3 >> 16;
}

#if 0
struct image *imgResize(struct image *image, int destwidth, int destheight, int filter)
{
	int cx, cy;
	struct image *newImg;
	int ystep = (image->height - 1) * 65536 / destheight;
	int xstep = (image->width - 1) * 65536 / destwidth;

	newImg = imgCreate(destwidth, destheight, image->pixfmt);

	if (newImg == NULL)
		return NULL;

	/* optimized case for 24bit images */

	if (image->pixfmt == IMG_PIXFMT_RGB888 && filter)
	{
		int py = 0;

		for (cy = 0; cy < destheight; cy++)
		{
			int px = 0;
			char *dst = (char *)(newImg->data.b + cy * newImg->width * 3);

			for (cx = 0; cx < destwidth; cx++)
			{
				getbilinearpixel24((unsigned char *) image->data.p, /*image->width * 3*/image->stride, px, py, (unsigned char *)dst);
				dst += 3;

				px += xstep;
			}
			py += ystep;
		}
	}
	else if (image->pixfmt == IMG_PIXFMT_RGB888 && !filter)
	{
		int py = 0;

		for (cy = 0; cy < destheight; cy++)
		{
			int px = 0;
			char *dst = (char *)(newImg->data.b + cy * destwidth * 3);
			char *src = (char *)(image->data.b + (py >> 16) * /*image->width * 3*/image->stride);

			for (cx = 0; cx < destwidth; cx++)
			{
				int off = (px >> 16) * 3;
				dst[0] = src[off + 0];
				dst[1] = src[off + 1];
				dst[2] = src[off + 2];
				dst += 3;

				px += xstep;
			}
			py += ystep;
		}
	}
	else if	(image->pixfmt == IMG_PIXFMT_ARGB8888 && filter)
	{
		int py = 0;

		for (cy = 0; cy < destheight; cy++)
		{
			int px = 0;
			char *dst = (char *)(newImg->data.b + cy * destwidth * 4);

			for (cx = 0; cx < destwidth; cx++)
			{
				getbilinearpixel32((unsigned char *) image->data.p, /*image->width * 4*/image->stride, px, py, (unsigned char *)dst);
				dst += 4;

				px += xstep;
			}
			py += ystep;
		}
	}
	else if	(image->pixfmt == IMG_PIXFMT_ARGB8888 && !filter)
	{
		int py = 0;

		for (cy = 0; cy < destheight; cy++)
		{
			int px = 0;
			unsigned int *dst = newImg->data.l + cy * destwidth;
			unsigned int *src = image->data.l + (py >> 16) * /*image->width*/ (image->stride >> 2); // XXX: is it ok?

			for (cx = 0; cx < destwidth; cx++)
			{
				*dst++ = src[px >> 16];
				px += xstep;
			}
			py += ystep;
		}
	}

	return newImg;
}
#else
struct image *imgResize(struct image *image, int destwidth, int destheight, int filter)
{
	int cx, cy;
	struct image *newImg;
	int ystep = (image->height - 1) * 65536 / destheight;
	int xstep = (image->width - 1) * 65536 / destwidth;

	newImg = imgCreate(destwidth, destheight, image->pixfmt);

	if (newImg == NULL)
		return NULL;

	/* optimized case for 24bit images */

	if (image->pixfmt == IMG_PIXFMT_RGB888 && filter)
	{
		int py = 0;

		for (cy = 0; cy < destheight; cy++)
		{
			int px = 0;
			char *dst = (char *)(newImg->data.b + cy * newImg->width * 3);

			for (cx = 0; cx < destwidth; cx++)
			{
				getbilinearpixel24((unsigned char *) image->data.p, image->width * 3, px, py, (unsigned char *)dst);
				dst += 3;

				px += xstep;
			}
			py += ystep;
		}
	}
	else if (image->pixfmt == IMG_PIXFMT_RGB888 && !filter)
	{
		int py = 0;

		for (cy = 0; cy < destheight; cy++)
		{
			int px = 0;
			char *dst = (char *)(newImg->data.b + cy * destwidth * 3);
			char *src = (char *)(image->data.b + (py >> 16) * image->width * 3);

			for (cx = 0; cx < destwidth; cx++)
			{
				int off = (px >> 16) * 3;
				dst[0] = src[off + 0];
				dst[1] = src[off + 1];
				dst[2] = src[off + 2];
				dst += 3;

				px += xstep;
			}
			py += ystep;
		}
	}
	else if	(image->pixfmt == IMG_PIXFMT_ARGB8888 && filter)
	{
		int py = 0;

		for (cy = 0; cy < destheight; cy++)
		{
			int px = 0;
			char *dst = (char *)(newImg->data.b + cy * destwidth * 4);

			for (cx = 0; cx < destwidth; cx++)
			{
				getbilinearpixel32((unsigned char *) image->data.p, image->width * 4, px, py, (unsigned char *)dst);
				dst += 4;

				px += xstep;
			}
			py += ystep;
		}
	}
	else if	(image->pixfmt == IMG_PIXFMT_ARGB8888 && !filter)
	{
		int py = 0;

		for (cy = 0; cy < destheight; cy++)
		{
			int px = 0;
			unsigned int *dst = newImg->data.l + cy * destwidth;
			unsigned int *src = image->data.l + (py >> 16) * image->width; 

			for (cx = 0; cx < destwidth; cx++)
			{
				*dst++ = src[px >> 16];
				px += xstep;
			}
			py += ystep;
		}
	}

	return newImg;
}
#endif

/**********************************************************************************************/

/* Audio support */

#if OS(AROS)
AROS_UFH3(APTR, SoundFunc,
        AROS_UFHA(struct Hook *, hook, A0),
        AROS_UFHA(struct AHIAudioCtrl *, actrl, A2),
        AROS_UFHA(struct AHISoundMessage *, smsg, A1))
{
    AROS_USERFUNC_INIT
    (void)hook;
    (void)smsg;

    _Stream *stream = (_Stream *) actrl->ahiac_UserData;

    if((stream->dbflag = !stream->dbflag) == TRUE) // Flip and test
        AHI_SetSound(0, 1, 0, 0, actrl, NULL);
    else
        AHI_SetSound(0, 0, 0, 0, actrl, NULL);

    Signal(stream->audiotask, 1L<<stream->sigsound);

    return NULL;

    AROS_USERFUNC_EXIT
}

struct Hook SoundHook =
{
    { NULL, NULL },
    (APTR) SoundFunc,
    NULL,
    NULL
};
#else
static ULONG SoundFunc(void)
{
	//struct Hook *hook = (struct Hook *)REG_A0;
	struct AHIAudioCtrl * actrl = (struct AHIAudioCtrl *)REG_A2;
	//struct AHISoundMessage * smsg = (struct AHISoundMessage *)REG_A1;
	_Stream *stream = (_Stream *) actrl->ahiac_UserData;

	if((stream->dbflag = !stream->dbflag) == TRUE) // Flip and test
		AHI_SetSound(0, 1, 0, 0, actrl, NULL);
	else
		AHI_SetSound(0, 0, 0, 0, actrl, NULL);

	Signal(stream->audiotask, 1L<<stream->sigsound);

	return NULL;
}

struct EmulLibEntry GATE_SoundFunc =
{
	 TRAP_LIB, 0, (void (*)(void))SoundFunc
};

struct Hook SoundHook =
{
	{NULL,NULL},
	(ULONG (*)()) &GATE_SoundFunc,
	NULL,
	NULL,
};
#endif

bool MediaPlayerPrivate::audioOpen()
{
	_Stream *stream = (_Stream *) malloc(sizeof(_Stream));

	m_ctx->audio_output_mutex->lock();

	m_ctx->audio_stream = NULL;

	if(stream)
	{
		m_ctx->audio_stream = stream;
		
		memset(stream, 0, sizeof(_Stream));
		
		stream->opened           = 0;
		stream->dbflag           = TRUE;
		stream->callertask       = FindTask(NULL);
		stream->sample_channels  = m_ctx->sample_channels;
		stream->sample_rate      = m_ctx->sample_rate;
		stream->sample_bits      = m_ctx->sample_bits;
		stream->AHImp            = NULL;
		stream->AHIDevice        = 1;
		stream->sigcaller        = AllocSignal(-1);
		InitSemaphore(&stream->sembuffer);

		if(stream->sigcaller == -1)
		{
			m_ctx->audio_output_mutex->unlock();
			audioClose();
			return false;
		}

		m_lock.lock();
		m_ctx->audiooutput_thread         = createThread(MediaPlayerPrivate::audioOutputStart, this, "[OWB] Audio Output");
		m_ctx->audiooutput_thread_running = m_ctx->audiooutput_thread;
		m_lock.unlock();

		if(m_ctx->audiooutput_thread_running)
		{
			// Wait for task initialization
			Wait(1L<<stream->sigcaller);

			if(stream->sigsound != -1 && stream->sigkill != -1)
			{
				if((stream->AHImp = CreateMsgPort()))
				{
					if((stream->AHIio = (struct AHIRequest *) CreateIORequest(stream->AHImp, sizeof(struct AHIRequest))))
					{
						stream->AHIio->ahir_Version = 4;  // Open at least version 4 of 'ahi.device'.
						if(!(stream->AHIDevice = OpenDevice(AHINAME, AHI_NO_UNIT, (struct IORequest *)stream->AHIio, NULL)))
						{
							stream->localAHIBase = (struct Library *) stream->AHIio->ahir_Std.io_Device;

							if((stream->actrl = AHI_AllocAudio(
								  AHIA_AudioID,   AHI_DEFAULT_ID,
								  AHIA_Channels,  1,
								  AHIA_Sounds,    2,
								  AHIA_MixFreq,   stream->sample_rate,
								  AHIA_SoundFunc, (ULONG) &SoundHook,
								  AHIA_UserData,  (ULONG) stream,
								  TAG_DONE)))
							{
								ULONG samples_count;

							    AHI_GetAudioAttrs(AHI_INVALID_ID,
									              stream->actrl,
												  AHIDB_MaxPlaySamples, (ULONG) &samples_count,
									              TAG_DONE);

								stream->sample_count = samples_count * 3;

								stream->sample0.ahisi_Type = stream->sample_channels == 2 ? AHIST_S16S : AHIST_M16S;
								stream->sample1.ahisi_Type = stream->sample_channels == 2 ? AHIST_S16S : AHIST_M16S;
								
								stream->sample0.ahisi_Length  = stream->sample_count;
								stream->sample1.ahisi_Length  = stream->sample_count;

								stream->sample_len = stream->sample_count * AHI_SampleFrameSize(stream->sample0.ahisi_Type);

								stream->sample0.ahisi_Address = malloc(stream->sample_len*2);
								stream->sample1.ahisi_Address = malloc(stream->sample_len*2);

								stream->buffer_len = stream->sample_len * 32;
								stream->buffer     = (char *) malloc(stream->buffer_len*2);

								if(stream->sample0.ahisi_Address && stream->sample1.ahisi_Address && stream->buffer)
								{
									memset(stream->sample0.ahisi_Address, 0, stream->sample_len);
									memset(stream->sample1.ahisi_Address, 0, stream->sample_len);
									memset(stream->buffer,                0, stream->buffer_len);

									if(!AHI_LoadSound(0, AHIST_DYNAMICSAMPLE, &stream->sample0, stream->actrl) &&
									   !AHI_LoadSound(1, AHIST_DYNAMICSAMPLE, &stream->sample1, stream->actrl) )
									{
										stream->samples_loaded = TRUE;

										if(!AHI_ControlAudio(stream->actrl, AHIC_Play, TRUE, TAG_DONE))
										{
											stream->buffer_put  = 0;
											stream->buffer_get  = 0;
											stream->written_len = 0;

											AHI_Play(stream->actrl,
											         AHIP_BeginChannel, 0,
											         AHIP_Freq,         stream->sample_rate,
											         AHIP_Vol,          0x10000L,
											         AHIP_Pan,          0x8000L,
											         AHIP_Sound,        0,
											         AHIP_Offset,       0,
											         AHIP_Length,       0,
											         AHIP_EndChannel,   NULL,
											         TAG_DONE);


											stream->opened = TRUE;
											m_ctx->audio_output_mutex->unlock();
											return true;
										}
										else
										{
											D(kprintf("[MediaPlayer Thread] AHI_ControlAudio() failed\n"));
										}
									}
									else
									{
										D(kprintf("[MediaPlayer Thread] Cannot initialize sample buffers\n"));
									}
								}
								else
								{
									D(kprintf("[MediaPlayer Thread] Out of memory\n"));
								}
							}
							else
							{
								kprintf("[MediaPlayer Thread] Unable to allocate sound hardware (already in use?)\n");
							}
						}
						else
						{
							D(kprintf("[MediaPlayer Thread] Cannot open ahi.device\n"));
						}
					}
					else
					{
						D(kprintf("[MediaPlayer Thread] Cannot create IOrequest\n"));
					}
				}
				else
				{
					D(kprintf("[MediaPlayer Thread] Cannot create Message Port\n"));
				}
			}
		}
	}

	m_ctx->audio_output_mutex->unlock();
	audioClose();
	return false;
}

void MediaPlayerPrivate::audioClose()
{
	D(kprintf("[MediaPlayer Thread] audioClose()\n"));

	m_ctx->audio_output_mutex->lock();

	if(m_ctx->audio_stream)
	{
		_Stream *stream = m_ctx->audio_stream;

		if(m_ctx->audiooutput_thread_running)
		{
			D(kprintf("[MediaPlayer Thread] Wait for audio output thread end\n"));
			Signal(stream->audiotask, 1L<<stream->sigkill);
			waitForThreadCompletion(m_ctx->audiooutput_thread);
		}

		D(kprintf("[MediaPlayer Thread] Free AHI objects\n"));

		FreeSignal(stream->sigcaller);

		if(stream->samples_loaded)
		{
			AHI_UnloadSound(0, stream->actrl);
			AHI_UnloadSound(1, stream->actrl);
		}

		free(stream->sample0.ahisi_Address);
		free(stream->sample1.ahisi_Address);
		free(stream->buffer);

		if(stream->actrl)
		{
			AHI_FreeAudio(stream->actrl);
		}

		if(stream->AHIDevice == 0)
		{
			CloseDevice((struct IORequest *)stream->AHIio);
		}

		DeleteIORequest((struct IORequest *)stream->AHIio);

		if(stream->AHImp)
		{
			DeleteMsgPort(stream->AHImp);
		}

		free(stream);

		m_ctx->audio_stream = NULL;

		D(kprintf("[MediaPlayer Thread] audioClose() OK\n"));
	}

	m_ctx->audio_output_mutex->unlock();
}

void MediaPlayerPrivate::audioPause()
{
	m_ctx->audio_output_mutex->lock();
	
	if(m_ctx->audio_stream)
	{
		_Stream *stream = m_ctx->audio_stream;

		D(kprintf("[MediaPlayer Thread] Pause audio\n"));
		AHI_ControlAudio(stream->actrl,
						 AHIC_Play, FALSE,
						 TAG_DONE);
	}

	m_ctx->audio_output_mutex->unlock();
}

void MediaPlayerPrivate::audioResume()
{
	m_ctx->audio_output_mutex->lock();

	if(m_ctx->audio_stream)
	{
		_Stream *stream = m_ctx->audio_stream;

		D(kprintf("[MediaPlayer Thread] Resume audio\n"));
		AHI_ControlAudio(stream->actrl,
						 AHIC_Play, TRUE,
						 TAG_DONE);
	}

	m_ctx->audio_output_mutex->unlock();
}

void MediaPlayerPrivate::audioReset()
{
	m_ctx->audio_output_mutex->lock();

	if(m_ctx->audio_stream)
	{
		_Stream *stream = m_ctx->audio_stream;

		D(kprintf("[MediaPlayer Thread] Reset audio buffers\n"));

		AHI_ControlAudio(stream->actrl,
						 AHIC_Play, FALSE,
						 TAG_DONE);

        ObtainSemaphore(&stream->sembuffer);

		memset(stream->buffer, 0, stream->buffer_len);
		memset(stream->sample0.ahisi_Address, 0, stream->sample_len);
		memset(stream->sample1.ahisi_Address, 0, stream->sample_len);
		stream->buffer_get  = 0;
		stream->buffer_put  = 0;
		stream->written_len = 0;

		ReleaseSemaphore(&stream->sembuffer);

		AHI_ControlAudio(stream->actrl,
						 AHIC_Play, TRUE,
						 TAG_DONE);
	}

	m_ctx->audio_output_mutex->unlock();
}

/* This custom definition is needed because of the "Fixed" type causing conflicts */
#if OS(MORPHOS)
#undef AHI_SetVol
#define AHI_SetVol(__p0, __p1, __p2, __p3, __p4) \
	LP5NR(66, AHI_SetVol, \
		UWORD , __p0, d0, \
		LONG , __p1, d1, \
		sposition , __p2, d2, \
		struct AHIAudioCtrl *, __p3, a2, \
		ULONG , __p4, d3, \
		, AHI_BASE_NAME, 0, 0, 0, 0, 0, 0)
#endif
#if OS(AROS)
#define Fixed LONG
#endif
void MediaPlayerPrivate::audioSetVolume(float volume)
{
	m_ctx->audio_output_mutex->lock();

	if(m_ctx->audio_stream)
	{
		_Stream *stream = m_ctx->audio_stream;

		D(kprintf("[MediaPlayer Thread] Set volume %f\n", volume));
		
		AHI_SetVol(0, (LONG) ((double) 0x10000L*volume), 0x8000L, stream->actrl, AHISF_IMM);
	}

	m_ctx->audio_output_mutex->unlock();
}

/***********************************************************************************/

/* Stream Client */

class StreamClient : public ResourceHandleClient
{
WTF_MAKE_NONCOPYABLE(StreamClient);
public:
	StreamClient(MediaPlayerPrivate* player)
	 : m_mediaPlayer(player)
	{}

	virtual void didReceiveResponse(ResourceHandle*, const ResourceResponse& response) override
	{
		D(kprintf("[StreamClient] didReceiveResponse\n"));
		m_mediaPlayer->didReceiveResponse(response);
	}

	virtual void didReceiveData(ResourceHandle*, const char* data, unsigned length, int lengthReceived) override
	{
		D(kprintf("[StreamClient] didReceiveData(%d)\n", length));
		m_mediaPlayer->didReceiveData(data, length, lengthReceived);
	}

	virtual void didFinishLoading(ResourceHandle*, double) override
	{
		D(kprintf("[StreamClient] didFinishLoading\n"));
		m_mediaPlayer->didFinishLoading();
	}
	
	virtual void didFail(ResourceHandle*, const ResourceError& error) override
	{
		D(kprintf("[StreamClient] didFail\n"));
		m_mediaPlayer->didFailLoading(error);
	}

private:
	MediaPlayerPrivate* m_mediaPlayer;
};

/***********************************************************************************/

/* URL protocol callbacks */

static int CALL_CONVT read_callback(void *sender, char *dst, int size)
{
	FFMpegContext *ctx = (FFMpegContext *) sender;
	bool needMoreData = false;
	bool shouldSendRequest = false;
	bool requestSent = false;
	int got = -1;

	if(size == 0)
		return 0;

	if(ctx->totalsize > 0 && ctx->media_buffer.readPos() >= ctx->totalsize)
		return AVERROR_EOF;

	D(kprintf("read_callback(): size %d readpos %lld\n", size, ctx->media_buffer.readPos()));

	do
	{
        ctx->buffer_mutex->lock();

		needMoreData = ctx->media_buffer.needMoreData(ctx, size);

		if(needMoreData && !requestSent)
		{
			shouldSendRequest = ctx->supportsPartialContent() && ctx->media_buffer.shouldSendRequest(ctx->use_partial_content);

			if(shouldSendRequest)
			{
				// Set the fetch offset that will be used for the new request
				ctx->fetch_offset = ctx->media_buffer.readPos();
			}
		}
		
		ctx->buffer_mutex->unlock();

		if(needMoreData)
		{
			if(shouldSendRequest && !requestSent)
			{
				D(kprintf("read_callback(): send new request\n"));
				WTF::callOnMainThread(MediaPlayerPrivate::fetchRequest, ctx);
				requestSent = true;
			}

			usleep(READ_WAIT_TIME);
		}
	} while(ctx->running && needMoreData && !ctx->network_error /*!ctx->media_buffer.isComplete(ctx)*/ /*!ctx->mediaplayer->isEndReached()*/);

    ctx->buffer_mutex->lock();

	if(ctx->running)
	{
		uint8_t *src = ctx->media_buffer.source();
		got = needMoreData ? 0 : size;

		D(kprintf("read_callback(): got %d bytes\n", got));

		if(src && got > 0)
		{
			long long readpos = ctx->media_buffer.readPos();
			memcpy(dst, src + readpos, got);
			ctx->media_buffer.setReadPos(readpos + got);
		}
		else
		{
			got = -1;
		}
	}
	else
	{
		D(kprintf("read_callback(): read was interrupted\n"));
		got = -1;
	}

	ctx->buffer_mutex->unlock();

	if(got < 0)
	{
		return AVERROR_EOF;
	}
	else
	{
		return got;
	}
}

static int64_t CALL_CONVT seek_callback(void *sender, int64_t pos, int whence)
{
	int64_t res = -1;
	FFMpegContext *ctx = (FFMpegContext *) sender;

	D(kprintf("seek_callback(): pos=%lld whence=%d\n", pos, whence));

	if(ctx && ctx->allow_seek)
	{
		int64_t old_pos = ctx->media_buffer.readPos();
		int64_t new_pos = old_pos;

		switch(whence)
		{
			case AVSEEK_SIZE:
				res = ctx->totalsize;
				break;
			case SEEK_SET:
				new_pos	= pos;
				break;
			case SEEK_CUR:
				new_pos = old_pos + pos;
				break;
			case SEEK_END:
				if(ctx->totalsize > 0)
				{
					new_pos	= ctx->totalsize + pos;
				}
				break;
		}

		if(whence != AVSEEK_SIZE)
		{
			// Safety checks
			if(new_pos < 0)
			{
				new_pos = 0;
			}

			if(ctx->totalsize > 0 && new_pos > ctx->totalsize)
			{
				new_pos = ctx->totalsize;
			}

			// Set the new read position (if changed)
			if(new_pos != old_pos)
			{
				ctx->media_buffer.setReadPos(new_pos);
			}

			res = new_pos;
		}
	}

	D(kprintf("seek_callback(): res = %lld\n", res));

	if(res < 0)
	{
		return AVERROR_EOF;
	}
	else
	{
		return res;
	}
}

static int CALL_CONVT open_callback(void *sender)
{
	FFMpegContext *ctx = (FFMpegContext *) sender;
	ctx->media_buffer.readpos = 0;
	return 0;
}

static int CALL_CONVT close_callback(void *sender)
{
    (void)sender;
	return 0;
}

/******************************************************************************************/

/* MediaPlayer class */

void MediaPlayerPrivate::registerMediaEngine(MediaEngineRegistrar registrar)
{
    if (isAvailable())
        registrar([](MediaPlayer* player) { return std::make_unique<MediaPlayerPrivate>(player); },
            getSupportedTypes, supportsType, 0, 0, 0, 0);
}

void MediaPlayerPrivate::sendCommand(IPCCommand cmd)
{
	D(kprintf("[MediaPlayer] Send command <%s>\n", cmd.name()));
	MutexLocker locker(m_lock);

	// Avoid accumulating seek requests
	if(cmd.id() == IPC_CMD_SEEK &&
	   m_commandQueue.size() > 0 &&
	   m_commandQueue[m_commandQueue.size()-1].id() == IPC_CMD_SEEK)
	{
		m_commandQueue.remove(m_commandQueue.size()-1);
	}
	m_commandQueue.append(cmd);
	m_condition.signal();
}

IPCCommand MediaPlayerPrivate::waitCommand()
{
	bool gotCommand = false;
	IPCCommand ret;

	D(kprintf("[MediaPlayer Thread] Wait for command\n"));

	m_lock.lock();

	D(kprintf("[MediaPlayer Thread] Wait for command (%d in queue) (task 0x%p)\n", m_commandQueue.size(), FindTask(NULL)));

	if(m_commandQueue.size())
	{
		ret = m_commandQueue[0];
		m_commandQueue.remove(0);
		gotCommand = true;
	}

	if(gotCommand)
	{
		D(kprintf("[MediaPlayer Thread] Found command <%s> in queue\n", ret.name()));
	}
	else
	{
		D(kprintf("[MediaPlayer Thread] Wait for signal (task 0x%p)\n", FindTask(NULL)));
		
		m_condition.wait(m_lock);

		if(m_commandQueue.size())
		{
			ret = m_commandQueue[0];
			m_commandQueue.remove(0);
		}

		D(kprintf("[MediaPlayer Thread] Received new command <%s> (task 0x%p)\n", ret.name(), FindTask(NULL)));
	}

	m_lock.unlock();

	return ret;
}

bool MediaPlayerPrivate::commandInQueue()
{
	int size = 0;

    m_lock.lock();
	D(kprintf("[MediaPlayer Thread] Commands in queue %d\n", m_commandQueue.size()));
	size = m_commandQueue.size();
	m_lock.unlock();

	return size > 0;
}

void MediaPlayerPrivate::didReceiveResponse(const ResourceResponse& response)
{
	if(!m_ctx->first_response_received)
	{
		m_ctx->first_response_received = true;
		m_ctx->transfer_completed      = false;
		m_ctx->totalsize = response.expectedContentLength();
		m_ctx->mimetype  = response.mimeType();

		m_ctx->media_buffer.init();
		m_ctx->media_buffer.setRequest(0, m_ctx->totalsize, 0);

		/* Decide if seek should be allowed during media opening */
		if(m_ctx->mimetype == "video/ogg" ||
		   m_ctx->mimetype == "audio/ogg" ||
#if USE_WEBM
		   (m_ctx->use_webm && m_ctx->mimetype == "video/webm") ||
		   (m_ctx->use_webm && m_ctx->mimetype == "audio/webm") ||
#endif
		   m_ctx->mimetype == "video/x-theora+ogg")

		{
			if(m_ctx->use_partial_content)
			{
				m_ctx->allow_seek = true;
			}
			else
			{
				m_ctx->allow_seek = false;
				//m_ctx->allow_seek = getenv("OWB_MEDIA_OGG_INITIAL_SEEK") != NULL;
			}
		}
		else if(m_ctx->mimetype == "video/mp4")
		{
			m_ctx->allow_seek = true;
		}
		else
		{
			// Let's allow for what we don't know...
            m_ctx->allow_seek = true;
		}

		// Response received, load the media.
		sendCommand(IPCCommand(IPC_CMD_LOAD));
	}
	else
	{
		// Does the server support our range request?
		if(response.httpStatusCode() == 200 || response.httpStatusCode() == 206)
		{
			char rangevalue[1024];
			long start = 0, end = 0, totalsize = 0;
			String range;
			
			range = response.httpHeaderField(String("Content-Range"));
			stccpy(rangevalue, range.latin1().data(), sizeof(rangevalue));

			D(kprintf("[MediaPlayer] DidReceiveResponse: range: <%s>\n", rangevalue));

			// XXX: more validity checks are needed
			if(sscanf(rangevalue, "bytes %ld-%ld/%ld", &start, &end, &totalsize) == 3)
			{
				m_ctx->media_buffer.setRequest(start, totalsize, 0);
				return;
			}
		}

		/* XXX: handle failure case: what to do? See what happens if we force that case */
	}
}

void MediaPlayerPrivate::didFinishLoading()
{
	m_ctx->transfer_completed = m_ctx->media_buffer.isComplete(m_ctx);
	m_ctx->resource_handle.release();
	m_ctx->resource_handle = nullptr;

	D(kprintf("[MediaPlayer] didFinishLoading: eof %d\n", m_ctx->transfer_completed));

	// If we got all the file, set the states
	if(m_ctx->transfer_completed)
	{
		D(kprintf("[MediaPlayer] didFinishLoading: Updating state\n"));

		if(m_readyState < MediaPlayer::HaveMetadata)
		{
			// We didn't even get metadata yet, let videoplayer do the other transitions (?)
			updateStates(MediaPlayer::Loaded, m_readyState);
		}
		else
		{
			// * -> EnoughData transition and Loading -> Loaded transition
			updateStates(MediaPlayer::Loaded, MediaPlayer::HaveEnoughData);
		}
	}
	else
	{
		// Locate closest hole in ranges and fetch it
	}
}

void MediaPlayerPrivate::didFailLoading(const ResourceError& error)
{
	m_ctx->transfer_completed = false;
	m_ctx->network_error = error.errorCode();
	m_ctx->resource_handle.release();
	m_ctx->resource_handle = nullptr;

	D(kprintf("Network Error %d\n", m_ctx->network_error));

	String errorString = String::format("[MediaPlayer] Network error %d", m_ctx->network_error);
	DoMethod(app, MM_OWBApp_AddConsoleMessage, errorString.latin1().data());

	updateStates(MediaPlayer::NetworkError, m_readyState);
}

void MediaPlayerPrivate::didReceiveData(const char* data, unsigned length, int lengthReceived)
{
    (void)lengthReceived;
	m_ctx->buffer_mutex->lock();

#if DUMPINFO
	D(kprintf("didReceiveData(%d) requestpos %lld requestgot %lld\n", length, m_ctx->media_buffer.requestpos, m_ctx->media_buffer.requestgot));
	dumpHex(data, std::min(length, 256));
#endif

	if(!m_ctx->media_buffer.append(m_ctx, (uint8_t *) data, length))
	{
		D(kprintf("[MediaPlayer] No more memory to store stream\n"));

		String errorString = String::format("[MediaPlayer] Not enough memory to store stream, aborting playback\n");
		DoMethod(app, MM_OWBApp_AddConsoleMessage, errorString.latin1().data());
		
		m_ctx->network_error = true;

		updateStates(MediaPlayer::NetworkError, MediaPlayer::HaveNothing);
	}

	m_ctx->buffer_mutex->unlock();

	if(m_readyState >= MediaPlayer::HaveMetadata)
	{
		MediaPlayer::ReadyState readyState = m_readyState;

		// Metadata -> CurrentData transition
		if(m_readyState < MediaPlayer::HaveCurrentData)
		{
			readyState = MediaPlayer::HaveCurrentData;
			updateStates(m_networkState, readyState);
		}
		// Current/FutureData -> EnoughData transition (5s advance)
		else if((readyState < MediaPlayer::HaveEnoughData) && (maxTimeLoaded() > currentTime() + 5))
		{
			readyState = MediaPlayer::HaveEnoughData;
			updateStates(m_networkState, readyState);
		}
		// CurrentData -> FutureData transition
		else  if((readyState < MediaPlayer::HaveFutureData) && (maxTimeLoaded() > currentTime()))
		{
			readyState = MediaPlayer::HaveFutureData;
			updateStates(m_networkState, readyState);
		}
	}
}

bool MediaPlayerPrivate::fetchData(unsigned long long startOffset)
{
	bool res = false;
	m_ctx->network_error = 0; // Reset error code

	D(kprintf("[MediaPlayer] fetchData(%lld)\n", startOffset));

	if(!m_ctx->stream_client)
	{
		m_ctx->stream_client = new StreamClient(this);
	}

	if(m_ctx->resource_handle)
	{
		m_ctx->resource_handle->clearClient();
		m_ctx->resource_handle->cancel();
		m_ctx->resource_handle.release();
		m_ctx->resource_handle = nullptr;
	}

	if(m_ctx->stream_client)
	{
		URL kurl(URL(), m_ctx->url);
		Frame *frame = m_player->frameView() ? &m_player->frameView()->frame() : 0;
		Document *document = frame ? frame->document() : 0;
		NetworkingContext* context = 0;

		ResourceRequest request(kurl);

		if(kurl.host() == "movies.apple.com" || kurl.host() == "trailers.apple.com")
		{
			request.setHTTPUserAgent("Quicktime/7.6.6");
		}
		else
		{
			if(frame)
			{
				FrameLoader* loader = &frame->loader();
				if(loader)
				{
					request.setHTTPUserAgent(loader->userAgent(request.url()));
				}
			}
			else
			{
				// Fallback (happens on vimeo)
				request.setHTTPUserAgent("Mozilla/5.0 (Macintosh; Intel Mac OS X 10_7) AppleWebKit/534.48.3 (KHTML, like Gecko) Version/5.1 Safari/534.48.3");
			}
		}

		request.setAllowCookies(true);

		if(document)
		{
			request.setHTTPReferrer(document->documentURI().utf8().data());
		}

		if(startOffset > 0)
		{
			request.setHTTPHeaderField(HTTPHeaderName::Range, String::format("bytes=%llu-", startOffset));
		}

		//request.setHTTPHeaderField("icy-metadata", "1"); // Radio mode
		//request.setHTTPHeaderField("transferMode.dlna", "Streaming"); // test

		if(frame)
		{
			FrameLoader* loader = &frame->loader();
			if(loader)
			{
				loader->addExtraFieldsToSubresourceRequest(request);
				context = loader->networkingContext();
			}
		}


		m_ctx->resource_handle = ResourceHandle::create(context, request,  m_ctx->stream_client, false, false);

		if(m_ctx->resource_handle)
		{
			m_ctx->resource_handle->getInternal()->m_disableEncoding = true; // don't allow gzip deflate
		}

		res = m_ctx->resource_handle != 0;
	}

	return res;
}

void MediaPlayerPrivate::cancelFetch()
{
	if(m_ctx->resource_handle)
	{
		m_ctx->resource_handle->clearClient();
		m_ctx->resource_handle->cancel();
		m_ctx->resource_handle.release();
		m_ctx->resource_handle = nullptr;
	}

	if(m_ctx->stream_client)
	{
		delete m_ctx->stream_client;
		m_ctx->stream_client = 0;
	}
}

bool MediaPlayerPrivate::demux(bool &eof, bool &gotVideo, bool &videoFull, bool &gotAudio, bool &audioFull)
{
	bool demuxedFrame = false;
	eof = false;
	gotVideo = false;
	gotAudio = false;
	videoFull = false;
	audioFull = false;

	D(kprintf("[MediaPlayer Thread] Demuxing\n"));

	m_ctx->video_mutex->lock();
	if(m_ctx->video_queue_size > VIDEO_MAX_SIZE)
	{
		videoFull = true;
	}
	m_ctx->video_mutex->unlock();

	m_ctx->audio_mutex->lock();
	if(m_ctx->audio_queue_size > AUDIO_MAX_SIZE)
	{
		audioFull = true;
	}
	m_ctx->audio_mutex->unlock();

	if(videoFull || audioFull)
	{
		return false;
	}

	ac_package* pckt = ac_read_package(m_ctx->ac);

	if(pckt)
	{
		if (m_ctx->video && (pckt->stream_index == m_ctx->video->stream_index))
		{
			m_ctx->video_mutex->lock();
			m_ctx->video_queue->append(ACPackage(pckt));
			m_ctx->video_queue_size += pckt->size;
			D(kprintf("[MediaPlayer Thread] Appending video packet [%d/%d] (%d packets)\n", m_ctx->video_queue_size, VIDEO_MAX_SIZE, m_ctx->video_queue->size()));
			m_ctx->video_condition->signal();
			gotVideo = true;
			demuxedFrame = true;
			m_ctx->video_mutex->unlock();
	    }
		else if (m_ctx->audio && (pckt->stream_index == m_ctx->audio->stream_index))
		{
			m_ctx->audio_mutex->lock();
			m_ctx->audio_queue->append(ACPackage(pckt));
			m_ctx->audio_queue_size += pckt->size;
			D(kprintf("[MediaPlayer Thread] Appending audio packet [%d/%d] (%d packets)\n", m_ctx->audio_queue_size, AUDIO_MAX_SIZE, m_ctx->audio_queue->size()));
			m_ctx->audio_condition->signal();
			gotAudio = true;
			demuxedFrame = true;
			m_ctx->audio_mutex->unlock();
	    }
		else // Unknown packet, freeing (consider subtitle streams, later)
		{
			ac_free_package(pckt);
		}

    }
	else
	{
		D(kprintf("[MediaPlayer Thread] av_read_frame() failed -> EOF reached\n"));
		eof = true;
	}

	return demuxedFrame;
}

void MediaPlayerPrivate::videoDecoderStart(void* obj)
{
	MediaPlayerPrivate* mediaplayer = static_cast<MediaPlayerPrivate*>(obj);
	mediaplayer->videoDecoder();
}

void MediaPlayerPrivate::audioDecoderStart(void* obj)
{
	MediaPlayerPrivate* mediaplayer = static_cast<MediaPlayerPrivate*>(obj);
	mediaplayer->audioDecoder();
}

void MediaPlayerPrivate::audioOutputStart(void* obj)
{
	MediaPlayerPrivate* mediaplayer = static_cast<MediaPlayerPrivate*>(obj);
	mediaplayer->audioOutput();
}

void MediaPlayerPrivate::playerLoopStart(void* obj)
{
	MediaPlayerPrivate* mediaplayer = static_cast<MediaPlayerPrivate*>(obj);
	mediaplayer->playerLoop();
}

void MediaPlayerPrivate::videoDecoder()
{
	m_lock.lock();
	m_lock.unlock();

	D(kprintf("[Video Thread] Hello\n"));

	while(m_ctx->running)
	{
		bool gotPacket = false;
		ac_package *pckt = NULL;

		if(!m_startedPlaying || m_isSeeking)
		{
			usleep(DECODER_WAIT_TIME);
			continue;
		}

		/* Get packet */

		m_ctx->video_mutex->lock();

		if(m_ctx->video_queue->size())
		{
			pckt = (*m_ctx->video_queue)[0].package();
			m_ctx->video_queue->remove(0);
			m_ctx->video_queue_size -= pckt->size;
			gotPacket = true;
		}

		if(gotPacket)
		{
			D(kprintf("[Video Thread] Found packet in queue (%d packets)\n", m_ctx->video_queue->size()));
		}
		else
		{
			D(kprintf("[Video Thread] Wait for packet\n"));

			m_ctx->video_condition->wait(*m_ctx->video_mutex);

			if(m_ctx->running && m_ctx->video_queue->size())
			{
				pckt = (*m_ctx->video_queue)[0].package();
				m_ctx->video_queue->remove(0);
				m_ctx->video_queue_size -= pckt->size;
			}

			D(kprintf("[Video Thread] Received new packet\n"));
		}

		m_ctx->video_mutex->unlock();

		/* Decode packet */

		if(pckt == ac_flush_packet())
		{
			D(kprintf("[Video Thread] Flush packet received\n"));
			ac_flush_buffers(m_ctx->video);
		}
		else if(pckt)
		{
            double decodeTime = 0;
            double decodeStart = WTF::currentTime();
			bool decoded = false;
			
			D(kprintf("[Video Thread] Decoding packet\n"));
			decoded = ac_decode_package(pckt, m_ctx->video);

			if(decoded)
			{
				double delay = 0.0;

				decodeTime = WTF::currentTime() - decodeStart;

				/* Try to sync on audio timecode (if there is audio) */
				if(m_ctx->video->timecode >= m_ctx->video_last_timecode && m_ctx->video->timecode <= m_ctx->video_last_timecode + 1)
				{
					delay = m_ctx->video->timecode - m_ctx->video_last_timecode - decodeTime;

					if(m_ctx->audio)
					{
						double avdelta = m_ctx->video->timecode - m_ctx->audio_timecode;

						D(kprintf("[Video Thread] V-A Delta: %f\n", avdelta));

						if(avdelta < 0.1 || avdelta > -0.1)
						{
							delay += avdelta;
						}
						else
						{
							if(avdelta < 0) delay -= 0.1;
							if(avdelta > 0) delay += 0.1;
						}
					}
				}

				m_ctx->video_last_timecode = m_ctx->video->timecode;

				D(kprintf("[Video Thread] [%f] Timecode: %f. Decoding took %f. Waiting for frame schedule %f\n", WTF::currentTime(), m_ctx->video->timecode, decodeTime, delay));

				// Painting must be done in main thread context
				long long jobid = WTF::callOnMainThreadFab(playerPaint, m_ctx);

				// Keep track of pending job so that they can be cancelled later
				m_lock.lock();
				m_pendingPaintQueue.append(jobid);
				m_lock.unlock();

				if(delay > 0)
				{
					usleep((long) (delay*1000000));
				}
			}		 
			
			ac_free_package(pckt);
		}
		else
		{
			D(kprintf("[Video Thread] NULL packet, quitting\n"));
		}
	}

	// Flush remaining packets
	
	D(kprintf("[Video Thread] Flushing packet queue\n"));

	m_ctx->video_mutex->lock();

	for(size_t i = 0; i < m_ctx->video_queue->size(); i++)
	{
		ac_free_package((*m_ctx->video_queue)[i].package());
	}

	m_ctx->video_queue->clear();
	m_ctx->video_queue_size = 0;

	m_ctx->video_mutex->unlock();

	D(kprintf("[Video Thread] Bye\n"));

	m_ctx->video_thread_running = false;
}

void MediaPlayerPrivate::audioDecoder()
{
	m_lock.lock();
	m_lock.unlock();

	D(kprintf("[Audio Thread] Hello\n"));

	SetTaskPri(FindTask(NULL), 1);

	while(m_ctx->running)
	{
		bool gotPacket = false;
		ac_package *pckt = NULL;

		if(!m_startedPlaying || m_isSeeking)
		{
			usleep(DECODER_WAIT_TIME);
			continue;
		}

		/* Get packet */

		m_ctx->audio_mutex->lock();

		if(m_ctx->audio_queue->size())
		{
			pckt = (*m_ctx->audio_queue)[0].package();
			m_ctx->audio_queue->remove(0);
			m_ctx->audio_queue_size -= pckt->size;
			gotPacket = true;
		}

		if(gotPacket)
		{
			D(kprintf("[Audio Thread] Found packet in queue (%d packets)\n", m_ctx->audio_queue->size()));
		}
		else
		{
			D(kprintf("[Audio Thread] Wait for packet\n"));

			m_ctx->audio_condition->wait(*m_ctx->audio_mutex);

			if(m_ctx->running && m_ctx->audio_queue->size())
			{
				pckt = (*m_ctx->audio_queue)[0].package();
				m_ctx->audio_queue->remove(0);
				m_ctx->audio_queue_size -= pckt->size;
			}

			D(kprintf("[Audio Thread] Received new packet\n"));
		}

		m_ctx->audio_mutex->unlock();

		/* Decode packet */

		if(pckt == ac_flush_packet())
		{
			D(kprintf("[Audio Thread] Flush packet received\n"));
			ac_flush_buffers(m_ctx->audio);
		}
		else if(pckt)
		{
            double decodeTime = 0;
            double decodeStart = WTF::currentTime();
			bool decoded = false;

			decoded = ac_decode_package(pckt, m_ctx->audio);

			if(decoded)
			{
				double delay = 0.0;
				decodeTime = WTF::currentTime() - decodeStart;

				// That buffer should play for that time
				delay = (double) m_ctx->audio->buffer_size/(m_ctx->sample_rate*(m_ctx->sample_bits/8)*m_ctx->sample_channels);
				
				m_ctx->audio_timecode = m_ctx->audio->timecode;

				m_ctx->audio_output_mutex->lock();

				if(m_ctx->audio_stream && m_ctx->audio->buffer_size && m_volume > 0)
				{
					bool needMoreData = false;
					_Stream *stream = m_ctx->audio_stream;

					// Actual timecode considering the buffered audio data in output buffer
					m_ctx->audio_timecode -= stream->written_len/(stream->sample_rate*(stream->sample_bits/8)*stream->sample_channels);

					// Delay to next frame, which should be related to decoded buffersize. XXX: handle speed factor there */
					delay -= decodeTime;

					ObtainSemaphore(&stream->sembuffer);
					needMoreData = stream->written_len < stream->sample_len;
					ReleaseSemaphore(&stream->sembuffer);

					if(needMoreData)
					{
						D(kprintf("[Audio Thread] Need more data, don't wait\n"));
						delay = 0;
					}

					ObtainSemaphore(&stream->sembuffer);

					if(stream->written_len >= stream->buffer_len)
					{
						D(kprintf("[Audio Thread] Buffer full, skip and wait a bit\n")); // XXX: Don't remove packet from queue in that case (move above)?
						delay *= 4;
					}
					else
					{
						LONG data_to_put_leftover = 0;

						if(stream->written_len + 2 * stream->sample_len >= stream->buffer_len)
						{
							D(kprintf("[Audio Thread] Buffer almost full, increase delay\n"));
							delay *= 2;
						}

						LONG data_to_put = stream->buffer_get - stream->buffer_put;
						if ( (data_to_put < 0) || ( (!data_to_put) && ( stream->written_len < stream->buffer_len ) ) )
						{
							data_to_put = stream->buffer_len - stream->buffer_put;
						}

						if (data_to_put < m_ctx->audio->buffer_size)
						{
							data_to_put_leftover = std::min(m_ctx->audio->buffer_size - data_to_put, stream->buffer_len - (stream->buffer_put + data_to_put) % stream->buffer_len);
						}
							
						if (data_to_put > m_ctx->audio->buffer_size)
						{
							data_to_put = m_ctx->audio->buffer_size;
						}

						D(kprintf("[Audio Thread] [%f] Writing %ld to offset %ld\n", WTF::currentTime(), data_to_put, stream->buffer_put));

						// XXX: Handle conversions
						memcpy(stream->buffer + stream->buffer_put, m_ctx->audio->pBuffer, data_to_put);
						stream->buffer_put += data_to_put;
						stream->buffer_put %= stream->buffer_len;
						stream->written_len += data_to_put;

						if(data_to_put_leftover)
						{
							memcpy(stream->buffer + stream->buffer_put, m_ctx->audio->pBuffer + data_to_put, data_to_put_leftover);
							stream->buffer_put += data_to_put_leftover;
							stream->buffer_put %= stream->buffer_len;
							stream->written_len += data_to_put_leftover;
						}
							
						D(kprintf("[Audio Thread] buffer_put %ld buffer_get %ld written_len %d\n", stream->buffer_put, stream->buffer_get, stream->written_len));
					}

					ReleaseSemaphore(&stream->sembuffer);
				}

				m_ctx->audio_output_mutex->unlock();

				D(kprintf("[Audio Thread] [%f] Stream TimeCode %f Real TimeCode %f. Waiting for frame schedule %f\n", WTF::currentTime(), m_ctx->audio->timecode, m_ctx->audio_timecode, delay));

				if(delay > 0)
				{
					usleep((long) (delay*1000000));
				}
			}

			ac_free_package(pckt);
		}
		else
		{
			D(kprintf("[Audio Thread] NULL packet, quitting\n"));
		}
	}

	// Flush remaining packets

	D(kprintf("[Audio Thread] Flushing packet queue\n"));

	m_ctx->audio_mutex->lock();

	for(size_t i = 0; i < m_ctx->audio_queue->size(); i++)
	{
		ac_free_package((*m_ctx->audio_queue)[i].package());
	}

	m_ctx->audio_queue->clear();
	m_ctx->audio_queue_size = 0;

	m_ctx->audio_mutex->unlock();

	D(kprintf("[Audio Thread] Bye\n"));

	m_ctx->audio_thread_running = false;
}

void MediaPlayerPrivate::audioOutput()
{
	m_lock.lock();
	m_lock.unlock();

	D(kprintf("[Audio Output Thread] Hello\n"));

	SetTaskPri(FindTask(NULL), 5);

	_Stream *stream = (_Stream *) m_ctx->audio_stream;

	stream->audiotask    = FindTask(NULL);
	stream->sigsound     = AllocSignal(-1);
	stream->sigkill      = AllocSignal(-1);

	// Signal we're ready (or not)
	Signal(stream->callertask, 1L<<stream->sigcaller);

	if(stream->sigsound != -1 && stream->sigkill != -1)
	{
		while(m_ctx->running)
		{
			ULONG signals = Wait( (1L<<stream->sigsound) | (1L<<stream->sigkill) );

			if(signals & (1L<<stream->sigkill))
			{
				D(kprintf("[Audio Output Thread] sigkill received\n"));
				break;
			}
			else if(signals & (1L<<stream->sigsound))
			{
				ObtainSemaphore(&stream->sembuffer);

				char *dst = stream->dbflag ? (char *) stream->sample1.ahisi_Address : (char *) stream->sample0.ahisi_Address;
				
				if(stream->written_len < stream->sample_len)
				{
					memset(dst, 0, stream->sample_len);
					ReleaseSemaphore(&stream->sembuffer);
					continue;
				}

				D(kprintf("[Audio Output Thread] [%f] Reading %ld at offset %ld\n", WTF::currentTime(), stream->sample_len, stream->buffer_get));
				memcpy(dst, stream->buffer + stream->buffer_get, stream->sample_len);

				stream->written_len -= stream->sample_len;
				if(stream->written_len < 0) stream->written_len = 0;
				stream->buffer_get  += stream->sample_len;
				stream->buffer_get  %= stream->buffer_len;

				D(kprintf("[Audio Output Thread] [%f] buffer_put %ld buffer_get %ld written_len %d\n", WTF::currentTime(), stream->buffer_put, stream->buffer_get, stream->written_len));

				ReleaseSemaphore(&stream->sembuffer);
			}
		}

		FreeSignal(stream->sigkill);
		FreeSignal(stream->sigsound);
	}

	D(kprintf("[Audio Output Thread] Bye\n"));

	m_ctx->audiooutput_thread_running = false;
}

void MediaPlayerPrivate::playerLoop()
{
	m_lock.lock();
	m_lock.unlock();

	FFMpegContext *ctx = m_ctx;

	D(kprintf("[MediaPlayer Thread] Hello\n"));

	ctx->running = true; //ctx->video_thread_running && ctx->audio_thread_running;

	/* Demuxer loop */
	while(ctx->running)
	{
		IPCCommand cmd = waitCommand();

		switch(cmd.id())
		{

/*******************************************************************************************/

			case IPC_CMD_LOAD:
			{
				MediaPlayer::NetworkState networkState = m_networkState;
				MediaPlayer::ReadyState readyState     = m_readyState;

				D(kprintf("[MediaPlayer Thread] Load\n"));

				if(ctx->open())
				{
					/* Start decoder threads */

					if(ctx->has_video)
					{
						m_lock.lock();
						ctx->video_thread = createThread(MediaPlayerPrivate::videoDecoderStart, this, "[OWB] MediaPlayer Video");
						ctx->video_thread_running = ctx->video_thread;
						m_lock.unlock();

						if(!ctx->video_thread)
						{
							D(kprintf("[MediaPlayer Thread] Couldn't create MediaPlayer Video thread\n"));
						}
					}

					if(ctx->has_audio)
					{
						m_lock.lock();
						ctx->audio_thread = createThread(MediaPlayerPrivate::audioDecoderStart, this, "[OWB] MediaPlayer Audio");
						ctx->audio_thread_running = ctx->audio_thread;
						m_lock.unlock();

						if(!ctx->audio_thread)
						{
							D(kprintf("[MediaPlayer Thread] Couldn't create MediaPlayer Audio thread\n"));
						}
					}

					readyState = MediaPlayer::HaveMetadata;

					// particular case for ogg+seek. Make seek available after metadata is retrieved
					if(!ctx->allow_seek)
					{
						ctx->allow_seek = true;
					}

					// If we got the whole file, set according states here
					if(ctx->transfer_completed)
					{
						// -> to Metadata
                        updateStates(networkState, readyState);

						// -> to EnoughData
						networkState = MediaPlayer::Loaded;
						readyState   = MediaPlayer::HaveEnoughData;
					}

					updateStates(networkState, readyState);

					D(kprintf("[MediaPlayer Thread] Load OK\n"));
				}
				else
				{
					readyState = MediaPlayer::HaveNothing;

					if(!ctx->network_error) // If it wasn't a network error, then it must be an unsupported format.
					{
						networkState = MediaPlayer::FormatError;
					}
						
                    updateStates(networkState, readyState);

					D(kprintf("[MediaPlayer Thread] Load KO\n"));
				}

				break;
			}

/*******************************************************************************************/

			case IPC_CMD_SEEK:
			{
				int res;
				int direction;
				float time = cmd.seekTo();

				D(kprintf("[MediaPlayer Thread] Seek to %f currentTime %f\n", time, currentTime()));

				if(!ctx->opened || time == currentTime() || !ctx->allow_seek)
				{
					m_isSeeking = false;
					break;
				}

				D(kprintf("[MediaPlayer Thread] Seek really needed\n"));
				
				direction = (int) (time - currentTime());

				if(ctx->video)
				{
					ctx->video_mutex->lock();

					res = ac_seek(ctx->video, direction, (int64_t) (time*1000));

					if(res)
					{
						ctx->video_last_timecode = time;

						for(size_t i = 0; i < ctx->video_queue->size(); i++)
						{
							ac_free_package((*ctx->video_queue)[i].package());
						}

						ctx->video_queue_size = 0;
						ctx->video_queue->clear();
						
						ctx->video_queue->append(ACPackage(ac_flush_packet()));

						D(kprintf("[MediaPlayer Thread] Seek Video OK\n"));
					}
					else
					{
					    D(kprintf("[MediaPlayer Thread] Seek Video Failed\n"));
					}

					ctx->video_mutex->unlock();
				}

				if(ctx->audio)
				{
					ctx->audio_mutex->lock();

					res = ac_seek(ctx->audio, direction, (int64_t) (time*1000));

					if(res)
					{
						ctx->audio_timecode = time;

						for(size_t i = 0; i < ctx->audio_queue->size(); i++)
						{
							ac_free_package((*ctx->audio_queue)[i].package());
						}

						ctx->audio_queue_size = 0;
						ctx->audio_queue->clear();

						ctx->audio_queue->append(ACPackage(ac_flush_packet()));

						audioReset();

						D(kprintf("[MediaPlayer Thread] Seek Audio OK\n"));
					}
					else
					{
					    D(kprintf("[MediaPlayer Thread] Seek Audio Failed\n"));
					}
					  
					ctx->audio_mutex->unlock();
				}
				
				if(ctx->running)
				{
					D(kprintf("[MediaPlayer Thread] Seek, calling updateStates\n"));
					if(m_readyState < MediaPlayer::HaveFutureData)
					{
						updateStates(m_networkState, MediaPlayer::HaveFutureData);
					}

					D(kprintf("[MediaPlayer Thread] Seek, calling callTimeChanged\n"));
					WTF::callOnMainThread(MediaPlayerPrivate::callTimeChanged, m_player);
					
					m_isSeeking = false;

					if(m_startedPlaying) // Run it again
					{
						D(kprintf("[MediaPlayer Thread] Seek, calling playerAdvance\n"));
						playerAdvance(ctx);
					}
				}

				break;
			}

/*******************************************************************************************/

			case IPC_CMD_PAUSE:
			{
				D(kprintf("[MediaPlayer Thread] Pause\n"));

				audioPause();
                m_startedPlaying = false;
				break;
			}

/*******************************************************************************************/

			case IPC_CMD_PLAY:
			{
				bool eof = false, readall = false, newcommand = false;
				
				D(kprintf("[MediaPlayer Thread] Play\n"));

				if(!ctx->opened)
					break;

				m_startedPlaying = true;

				// Try to open audio only when needed
				if(ctx->has_audio && !ctx->audio_stream)
				{
					audioOpen();
				}

				audioResume();

				// Demuxer loop
				do
				{
					// If we read the whole file, and queues are empty, then we're done -> EOF
					if(readall)
					{
						bool videoEmpty = false, audioEmpty = false;

						ctx->video_mutex->lock();
						videoEmpty = ctx->video_queue->size() == 0;
						ctx->video_mutex->unlock();

						ctx->audio_mutex->lock();
						audioEmpty = ctx->audio_queue->size() == 0;
						ctx->audio_mutex->unlock();

						if(videoEmpty && audioEmpty)
						{
							eof = true; // Quit playing with eof reason
							break;
						}
						else
						{
							usleep(DEMUXER_WAIT_TIME); // Nothing to do
						}
					}
					// Else demux as long as queues aren't full
					else
					{
						bool decodedVideo, decodedAudio, videoFull, audioFull;

						demux(readall, decodedVideo, videoFull, decodedAudio, audioFull);
						D(kprintf("[MediaPlayer Thread] [V: %d A: %d EOF: %d VideoFull: %d AudioFull: %d]\n", decodedVideo, decodedAudio, eof, videoFull, audioFull));

						if(videoFull || audioFull)
						{
							D(kprintf("[MediaPlayer Thread] No more space in decoding queue, waiting\n"));
							usleep(DEMUXER_WAIT_TIME);
						}
					}

					// If there's a new command, quit the loop to process it
					if(commandInQueue())
					{
						newcommand = true;
						break;
					}

				} while(ctx->running && !eof && !newcommand);

				if(!ctx->running)
				{
					D(kprintf("[MediaPlayer Thread] Quit request\n"));
				}
				else if(newcommand)
				{
					D(kprintf("[MediaPlayer Thread] New command to process\n"));
				}
				else if(eof)
				{
					D(kprintf("[MediaPlayer Thread] End of stream reached\n"));

					if(ctx->has_audio && ctx->audio_stream)
					{
						audioClose();
					}

					didEnd();
				}

				break;
			}

/*******************************************************************************************/

			case IPC_CMD_STOP:
			{
				D(kprintf("[MediaPlayer Thread] Stop\n"));

				ctx->running = false;
				break;
			}

			default:
			{
				D(kprintf("[MediaPlayer Thread] Unsupported command %d, quitting\n", cmd.id()));
				ctx->running = false;
				break;
			}
		}

		if(!ctx->running)
		{
			D(kprintf("[MediaPlayer Thread] Quitting loop\n"));
		}
	}

	/* Wait for audio/video threads */
	if(ctx->video_thread_running)
	{
		D(kprintf("[MediaPlayer Thread] Wait for video thread end\n"));
		ctx->video_mutex->lock();
		ctx->video_condition->signal();
		ctx->video_mutex->unlock();
		waitForThreadCompletion(ctx->video_thread);
	}

	if(ctx->audio_thread_running)
	{
		D(kprintf("[MediaPlayer Thread] Wait for audio thread end\n"));
		ctx->audio_mutex->lock();
		ctx->audio_condition->signal();
		ctx->audio_mutex->unlock();
		waitForThreadCompletion(ctx->audio_thread);
	}

#if DUMPINFO
	// dump media at cleanup
	dumpMedia(ctx);
#endif

	/* Shutdown ffmpeg objects */
	ctx->close();

	m_threadRunning = false;

	D(kprintf("[MediaPlayer Thread] Bye\n"));
}

void MediaPlayerPrivate::fetchRequest(void *c)
{
	FFMpegContext *ctx = (FFMpegContext *) c;

	if(ctx && ctx->mediaplayer)
	{
		ctx->mediaplayer->cancelFetch();
		ctx->mediaplayer->fetchData(ctx->fetch_offset);
	}
}

void MediaPlayerPrivate::playerPaint(void *c)
{
	FFMpegContext *ctx = (FFMpegContext *) c;

	if(ctx && ctx->mediaplayer)
	{
		Object *browser = NULL;
		FrameView* frameView = ctx->mediaplayer->player()->frameView();

		if(frameView)
		{
			BalWidget *widget = frameView->hostWindow()->platformPageClient();

			if(widget)
			{
				browser = widget->browser;
			}
		}

		if(browser)
		{
			// Accelerated mode (overlay)

#if 0 // FIXME
			// Only render as overlay for the matching overlay element. Relying on this cast may break in further webkit, beware.
			MediaPlayerClient& client = ctx->mediaplayer->player()->client();
			if(client && client == (HTMLMediaElement *) getv(browser, MA_OWBBrowser_VideoElement))
#else
			if (0)
#endif
			{
				BENCHMARK_INIT;
				BENCHMARK_RESET;

				AVFrame *frame = ac_get_frame(ctx->video);
				if(frame)
				{
					DoMethod(browser, MM_OWBBrowser_VideoBlit, frame->data, frame->linesize, ctx->width, ctx->height);
				}

				BENCHMARK_EVALUATE;
				BENCHMARK_EXPRESSION(kprintf("[MediaPlayer] Video blit %f ms\n", diffBenchmark*1000));
			}
			// Normal window mode, going through cairo (slow)
			else
			{
				ctx->mediaplayer->repaint();
			}
		}

		ctx->mediaplayer->m_lock.lock();
		if(ctx->mediaplayer->m_pendingPaintQueue.size())
		{
			ctx->mediaplayer->m_pendingPaintQueue.remove(0);
		}
		ctx->mediaplayer->m_lock.unlock();
	}
}

void MediaPlayerPrivate::playerAdvance(void *c)
{
	FFMpegContext *ctx = (FFMpegContext *) c;

	if(ctx && ctx->mediaplayer)
	{
		//ctx->mediaplayer->play();
		ctx->mediaplayer->sendCommand(IPCCommand(IPC_CMD_PLAY));
	}
}

MediaPlayerPrivate::MediaPlayerPrivate(MediaPlayer* player)
    : m_player(player)
	, m_isEndReached(false)
	, m_errorOccured(false)
    , m_volume(0.5f)
    , m_networkState(MediaPlayer::Empty)
    , m_readyState(MediaPlayer::HaveNothing)
	, m_startedPlaying(false)
    , m_isStreaming(false)
	, m_isSeeking(false)
    , m_visible(true)
	, m_ctx(0)
	, m_threadRunning(false)
{
	D(kprintf("[MediaPlayer] MediaPlayerPrivate()\n"));

	/* Media Settings */

	m_ctx = new FFMpegContext(this);

	/* Start player thread */
	m_lock.lock();
	m_thread = createThread(MediaPlayerPrivate::playerLoopStart, this, "[OWB] MediaPlayer");
	m_threadRunning = m_thread;
	m_lock.unlock();

	if(!m_thread)
	{
		kprintf("[MediaPlayer] Couldn't create a MediaPlayer thread (no more free signals)\n");
	}

	D(kprintf("[MediaPlayer] Add media instance %p\n", this));
	mediaObjects.append(this);

	D(kprintf("[MediaPlayer] MediaPlayerPrivate() OK\n"));
}

MediaPlayerPrivate::~MediaPlayerPrivate()
{
	D(kprintf("[MediaPlayer] ~MediaPlayerPrivate()\n"));

	WTF::cancelCallOnMainThread(MediaPlayerPrivate::callNetworkStateChanged, m_player);
	WTF::cancelCallOnMainThread(MediaPlayerPrivate::callReadyStateChanged, m_player);
	WTF::cancelCallOnMainThread(MediaPlayerPrivate::callTimeChanged, m_player);

	m_player = 0; // Any further reference to m_player might result in a crash since a few webkit versions

	// Wait for player thread completion
	if(m_threadRunning)
	{
		m_startedPlaying = false;
		m_ctx->running = false; // some inner loops might only interrupt with that
		sendCommand(IPCCommand(IPC_CMD_STOP));

		D(kprintf("[MediaPlayer] Wait for thread end\n"));
		waitForThreadCompletion(m_thread);
	}

	m_ctx->mediaplayer = 0;

	D(kprintf("[MediaPlayer] Flushing pending paint jobs queue\n"));
	Vector<long long>::const_iterator end = m_pendingPaintQueue.end();
	for (Vector<long long>::const_iterator it = m_pendingPaintQueue.begin(); it != end; ++it)
	{
		 D(kprintf("."));
		 WTF::removeFromMainThreadFab(*it);
	}
	D(kprintf("\n"));

	if(m_ctx->resource_handle)
	{
		m_ctx->resource_handle->clearClient();
		m_ctx->resource_handle->cancel();
		m_ctx->resource_handle.release();
		m_ctx->resource_handle = nullptr;
	}

	if(m_ctx->stream_client)
	{
		delete m_ctx->stream_client;
		m_ctx->stream_client = 0;
	}

	delete m_ctx;

	D(kprintf("[MediaPlayer] ~MediaPlayerPrivate() this %p media instance count %d\n", this, mediaObjects.size()));

	for(size_t i = 0; i < mediaObjects.size(); i++)
	{
		D(kprintf("[MediaPlayer] ~MediaPlayerPrivate() checking %p\n", mediaObjects[i]));
		if(mediaObjects[i] == this)
		{
			D(kprintf("[MediaPlayer] ~MediaPlayerPrivate() that's us, let's remove\n"));
			mediaObjects.remove(i);
			break;
		}
	}

	D(kprintf("[MediaPlayer] ~MediaPlayerPrivate() OK\n"));
}

void MediaPlayerPrivate::load(const String& url)
{
	D(kprintf("[MediaPlayer] Load %s\n", url.utf8().data()));

	String statusString = String::format("[MediaPlayer] Loading %s", url.utf8().data());
	DoMethod(app, MM_OWBApp_AddConsoleMessage, statusString.utf8().data());

	// Abort currently loading streams first
	cancelLoad();

	m_isEndReached   = false;
	m_startedPlaying = false;

	updateStates(MediaPlayer::Loading, MediaPlayer::HaveNothing);

	m_ctx->url = url;

	fetchData(0);
}

#if ENABLE(MEDIA_SOURCE)
void MediaPlayerPrivate::load(const String& url, PassRefPtr<MediaSource>)
{
    String statusString = String::format("[MediaPlayer] Loading media source %s (Not implemented yet)", url.utf8().data());
    #warning "check what to do with mediasource"
    load(url);
}
#endif

void MediaPlayerPrivate::prepareToPlay()
{
	D(kprintf("[MediaPlayer] PreparetoPlay\n"));
	play();
}

void MediaPlayerPrivate::play()
{
	D(kprintf("[MediaPlayer] Play\n"));

	if(m_isEndReached)
	{
		m_isEndReached = false;
	}

	sendCommand(IPCCommand(IPC_CMD_PLAY));
}

void MediaPlayerPrivate::pause()
{
	D(kprintf("[MediaPlayer] Pause\n"));

	sendCommand(IPCCommand(IPC_CMD_PAUSE));
}

float MediaPlayerPrivate::duration() const
{
	D(kprintf("[MediaPlayer] Duration %f\n", (float) m_ctx->duration));
	return (float) m_ctx->duration;
}

float MediaPlayerPrivate::currentTime() const
{
	float ret = 0.0;

	D(kprintf("[MediaPlayer] timecodes [A: %f V: %f]\n", m_ctx->audio ? m_ctx->audio->timecode : 0.0, m_ctx->video ? m_ctx->video->timecode : 0.0 ));

	if(m_isEndReached)
	{
		ret = duration();
	}
	else if(m_ctx->audio)
	{
		ret = min((float) m_ctx->audio_timecode, duration() > 0 ? duration() : numeric_limits<float>::infinity()); // Use compensated timecode
	}
	else if(m_ctx->video)
	{
		ret = min((float) m_ctx->video->timecode, duration() > 0 ? duration() : numeric_limits<float>::infinity());
	}

	D(kprintf("[MediaPlayer] currentTime %f / %f\n", ret, duration()));

	return ret;
}

void MediaPlayerPrivate::seek(float time)
{
	D(kprintf("[MediaPlayer] Seek to %f\n", time));

	if(m_isEndReached)
	{
		m_isEndReached = false;
	}

	if(duration() && time > duration())
	{
		time = duration();
	}

	if(m_ctx->allow_seek)
	{
		m_isSeeking = true;
		sendCommand(IPCCommand(IPC_CMD_SEEK, time));
	}
}

bool MediaPlayerPrivate::paused() const
{
	D(kprintf("[MediaPlayer] Is Paused ? %d\n", !m_startedPlaying));
	return m_startedPlaying == false;
}

bool MediaPlayerPrivate::seeking() const
{
	return m_isSeeking;
}

// Returns the size of the video
FloatSize MediaPlayerPrivate::naturalSize() const
{
    if (!hasVideo())
        return FloatSize();

	int width = m_ctx->width, height = m_ctx->height;
	/*
	width  *= m_ctx->aspect;
	height /= m_ctx->aspect;
	*/
	D(kprintf("[MediaPlayer] Natural size (%dx%d)\n", width, height));

	return FloatSize(width, height);
}

bool MediaPlayerPrivate::hasVideo() const
{
	return m_ctx->has_video;
}

bool MediaPlayerPrivate::hasAudio() const
{
	return m_ctx->has_audio;
}

void MediaPlayerPrivate::setVolume(float volume)
{
    m_volume = volume;

	if(m_ctx->has_audio)
	{
		audioSetVolume(volume);
	}
}

void MediaPlayerPrivate::setRate(float rate)
{
    (void)rate;
	D(kprintf("[MediaPlayer] Set rate to %f\n", rate));
}

MediaPlayer::NetworkState MediaPlayerPrivate::networkState() const
{
	D(kprintf("[MediaPlayer] networkState %d\n", m_networkState));
    return m_networkState;
}

MediaPlayer::ReadyState MediaPlayerPrivate::readyState() const
{
	D(kprintf("[MediaPlayer] readyState %d\n", m_readyState));
    return m_readyState;
}

std::unique_ptr<PlatformTimeRanges> MediaPlayerPrivate::buffered() const
{
	// XXX: We could really use timeranges more precisely here

	D(kprintf("[MediaPlayer] buffered\n"));
    auto timeRanges = make_unique<PlatformTimeRanges>();
    float loaded = maxTimeLoaded();
    if (!m_errorOccured && !m_isStreaming && loaded > 0)
        timeRanges->add(MediaTime::zeroTime(), MediaTime::createWithDouble(loaded));
    return timeRanges;
}

float MediaPlayerPrivate::maxTimeSeekable() const
{
	if(m_errorOccured)
	{
        return 0.0;
	}

	D(kprintf("[MediaPlayer] maxTimeSeekable\n"));

	if(m_isStreaming)
	{
        return numeric_limits<float>::infinity();
	}

	if(m_ctx->use_partial_content && m_ctx->supportsPartialContent())
	{
		return duration(); // We can seek everywhere in the file
	}
	else
	{
        return maxTimeLoaded();
	}
}

float MediaPlayerPrivate::maxTimeLoaded() const
{
	float res;

	if(m_errorOccured)
	{
		res = 0.0;
	}
	else if(totalBytesKnown())
	{
		res = m_ctx->media_buffer.maxTimeLoaded(m_ctx, currentTime(), duration());
	}
	else
	{
		res = duration();
	}

	D(kprintf("[MediaPlayer] maxTimeLoaded %f\n", res));

	return res;
}

bool MediaPlayerPrivate::totalBytesKnown() const
{
	D(kprintf("[MediaPlayer] totalBytesKnown %d", totalBytes() > 0));
    return totalBytes() > 0;
}

unsigned long long MediaPlayerPrivate::totalBytes() const
{
	D(kprintf("[MediaPlayer] totalBytes %ld\n", (long) m_ctx->totalsize));
	return (unsigned long long)  m_ctx->totalsize;
}

void MediaPlayerPrivate::cancelLoad()
{
	D(kprintf("[MediaPlayer] cancelLoad()\n"));

    if (m_networkState < MediaPlayer::Loading || m_networkState == MediaPlayer::Loaded)
        return;

	/* Cancel ResourceHandle */
	cancelFetch();
	updateStates(MediaPlayer::Empty, MediaPlayer::HaveNothing);
}


void MediaPlayerPrivate::callNetworkStateChanged(void *c)
{
	MediaPlayer* player = (MediaPlayer *) c;
	if(player) player->networkStateChanged();
}

void MediaPlayerPrivate::callReadyStateChanged(void *c)
{
	MediaPlayer* player = (MediaPlayer *) c;
	if(player) player->readyStateChanged();
}

void MediaPlayerPrivate::callTimeChanged(void *c)
{
	MediaPlayer* player = (MediaPlayer *) c;
	if(player) player->timeChanged();
}


void MediaPlayerPrivate::updateStates(MediaPlayer::NetworkState networkState, MediaPlayer::ReadyState readyState)
{
    if (m_errorOccured || !m_ctx->running)
        return;

	if(m_player)
	{  
		if(networkState != m_networkState)
		{
			m_networkState = networkState;
			WTF::callOnMainThread(MediaPlayerPrivate::callNetworkStateChanged, m_player);
		}

		if(readyState != m_readyState)
		{
			m_readyState = readyState;
			WTF::callOnMainThread(MediaPlayerPrivate::callReadyStateChanged, m_player);
		}
	}
}

void MediaPlayerPrivate::didEnd()
{
	m_startedPlaying = false;
    m_isEndReached = true;

	updateStates(m_networkState, MediaPlayer::HaveEnoughData);

	WTF::callOnMainThread(MediaPlayerPrivate::callTimeChanged, m_player);
}

void MediaPlayerPrivate::setSize(const IntSize& size)
{
    (void)size;
}

void MediaPlayerPrivate::setVisible(bool visible)
{
    m_visible = visible;
}

bool MediaPlayerPrivate::didLoadingProgress() const
{
    return true;
}

void MediaPlayerPrivate::repaint()
{
	if(m_player)
	{
		m_player->repaint();
	}
}

void MediaPlayerPrivate::paint(GraphicsContext* context, const FloatRect& rect)
{
    if (context->paintingDisabled())
        return;

    if (!m_visible)
        return;

	Object *browser = NULL;
	FrameView* frameView = player()->frameView();

	if(frameView)
	{
		BalWidget *widget = frameView->hostWindow()->platformPageClient();

		if(widget)
		{
			browser = widget->browser;
		}
	}

	if(browser)
	{
		if(getv(browser, MA_OWBBrowser_VideoElement))
		{
			return;
		}
	}

	BENCHMARK_INIT;
	BENCHMARK_RESET;

	bool canDisplayFrame = m_ctx->video && m_ctx->video->pBuffer && m_readyState >= MediaPlayer::HaveCurrentData;

	if(canDisplayFrame)
	{
		cairo_t* cr = context->platformContext()->cr();
	    int width = m_ctx->width, height = m_ctx->height;
		int stride = 0;
	    double doublePixelAspectRatioNumerator = 0;
	    double doublePixelAspectRatioDenominator = 0;
	    double displayWidth;
	    double displayHeight;
		double scale, gapHeight, gapWidth;
		struct image srcImage, *targetImage = NULL, *scaledImage = NULL;
		cairo_surface_t* src = NULL;

		AVFrame *frame = ac_get_frame(m_ctx->video);
		srcImage.data.p = frame->data[0];/*m_ctx->video->pBuffer;*/
		srcImage.stride = frame->linesize[0];
		srcImage.width = width;
		srcImage.height = height;
		srcImage.pixfmt = IMG_PIXFMT_ARGB8888;
		srcImage.depth = 24;

		displayWidth = width;
		displayHeight = height;

		/*
		if(m_ctx->codec_ctx->sample_aspect_ratio.num == 0)
		{
			doublePixelAspectRatioNumerator = 4.0;
			doublePixelAspectRatioDenominator = 3.0;
		}
		else
		{
			doublePixelAspectRatioNumerator = m_ctx->codec_ctx->sample_aspect_ratio.num;
			doublePixelAspectRatioDenominator = m_ctx->codec_ctx->sample_aspect_ratio.den;
		}
		*/

		doublePixelAspectRatioNumerator = 1.0;
		doublePixelAspectRatioDenominator = 1.0;

		displayWidth *= doublePixelAspectRatioNumerator / doublePixelAspectRatioDenominator;
		displayHeight *= doublePixelAspectRatioDenominator / doublePixelAspectRatioNumerator;

		D(kprintf("[MediaPlayer] Rect (%dx%d) Video (%fx%f)\n", rect.width(), rect.height(), displayWidth, displayHeight));

		double a = rect.width () / displayWidth;
		double b = rect.height () / displayHeight;

		if(a < b) scale = a; else scale = b;

	    displayWidth *= scale;
	    displayHeight *= scale;

		if(width != (int) displayWidth || height != (int) displayHeight)
		{
			scaledImage = imgResize(&srcImage, (int) displayWidth, (int) displayHeight, TRUE);
		}

		if(scaledImage)
		{
			D(kprintf("[MediaPlayer] Using scaled image\n"));
			targetImage = scaledImage;
			stride = (int) displayWidth * 4;
		}
		else
		{
			D(kprintf("[MediaPlayer] Using original image (stride %d)\n", frame->linesize[0]));
			targetImage = &srcImage;
			stride = frame->linesize[0];
		}

		BENCHMARK_EVALUATE;
		BENCHMARK_EXPRESSION(kprintf("[MediaPlayer] Video Scale [%dx%d] -> [%dx%d] %f ms\n", width, height, (int) displayWidth, (int) displayHeight, diffBenchmark*1000));
		BENCHMARK_RESET;

	    // Calculate gap between border and picture
	    gapWidth = (rect.width() - displayWidth) / 2.0;
	    gapHeight = (rect.height() - displayHeight) / 2.0;

		src = cairo_image_surface_create_for_data(targetImage->data.b,
												  CAIRO_FORMAT_ARGB32,
												  (int) displayWidth,
												  (int) displayHeight,
												  stride /*(int) displayWidth *4*/);

		if(src)
		{
		    cairo_save(cr);
			cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);

		    cairo_translate(cr, rect.x() + gapWidth, rect.y() + gapHeight);
			cairo_rectangle(cr, 0, 0, displayWidth, displayHeight);
			
			/*
			cairo_scale(cr, doublePixelAspectRatioNumerator / doublePixelAspectRatioDenominator,
							doublePixelAspectRatioDenominator / doublePixelAspectRatioNumerator);
			cairo_scale(cr, scale, scale);
			*/

		    cairo_set_source_surface(cr, src, 0, 0);
		    cairo_fill(cr);
		    cairo_restore(cr);
		    cairo_surface_destroy(src);
		}

		if(scaledImage)
		{
			imgFree(scaledImage);
		}

		BENCHMARK_EVALUATE;
		BENCHMARK_EXPRESSION(kprintf("[MediaPlayer] Video Cairo blit %f ms\n", diffBenchmark*1000));
	}
	else
	{
		cairo_t* cr = context->platformContext()->cr();

		cairo_save(cr);
		cairo_set_source_rgb(cr, 0, 0, 0);
		cairo_rectangle(cr, rect.x(), rect.y(), rect.width(), rect.height());
		cairo_fill(cr);
		cairo_restore(cr);

		BENCHMARK_EVALUATE;
		BENCHMARK_EXPRESSION(kprintf("[MediaPlayer] Idle Cairo blit %f ms\n", diffBenchmark*1000));
	}

	D(kprintf("[MediaPlayer] Paint %f ms\n", (WTF::currentTime() - startBenchmark)*1000));
}

void MediaPlayerPrivate::getSupportedTypes(HashSet<String>& types)
{
	types.add(String("text/html"));

	if(getv(app, MA_OWBApp_EnableOgg))
	{
		types.add(String("video/x-theora+ogg"));
		types.add(String("video/ogg"));
	}

	types.add(String("video/vnd.objectvideo"));
	if(getv(app, MA_OWBApp_EnableMP4))
	{
		types.add(String("video/mp4"));
	}
	types.add(String("video/avi"));
	if(getv(app, MA_OWBApp_EnableFLV))
	{
		types.add(String("video/flv"));
		types.add(String("video/x-flv")); // It isn't in the specs, and seeking doesn't work well
	}
#if USE_WEBM
	if(getv(app, MA_OWBApp_EnableVP8))
	{
		types.add(String("video/webm"));
	}
#endif

	types.add(String("audio/aac"));
	if(getv(app, MA_OWBApp_EnableMP4))
	{
		types.add(String("audio/mp4"));
		types.add(String("audio/x-m4a"));
	}
	if(getv(app, MA_OWBApp_EnableOgg))
	{
		types.add(String("audio/ogg"));
	}
	types.add(String("audio/mpeg"));
	types.add(String("audio/x-wav"));
	types.add(String("audio/wav"));
	types.add(String("audio/basic"));
	types.add(String("audio/x-aiff"));
#if USE_WEBM
	if(getv(app, MA_OWBApp_EnableVP8))
	{
		types.add(String("audio/webm"));
	}
#endif
}

MediaPlayer::SupportsType MediaPlayerPrivate::supportsType(const MediaEngineSupportParameters& parameters)
{
    HashSet<String> types;

    if (parameters.type.isNull() || parameters.type.isEmpty())
        return MediaPlayer::IsNotSupported;

    getSupportedTypes(types);
    D(kprintf("supportsType(%s, %s) %d\n", parameters.type.utf8().data(), parameters.codecs.utf8().data(), types.contains(parameters.type)));

    // spec says we should not return "probably" if the codecs string is empty
    if(types.contains(parameters.type))
	return parameters.codecs.isEmpty() ? MediaPlayer::MayBeSupported : MediaPlayer::IsSupported;	

    return MediaPlayer::IsNotSupported;
}

bool MediaPlayerPrivate::hasSingleSecurityOrigin() const
{
    return true;
}

bool MediaPlayerPrivate::supportsFullscreen() const
{
    return true;
}

void MediaPlayerPrivate::setOutputPixelFormat(int pixfmt)
{
	ac_set_output_format(m_ctx->video, (ac_output_format) pixfmt);
}

}

#endif


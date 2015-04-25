/*
    This file is part of Acinerella.

    Acinerella is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Acinerella is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Acinerella.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "config.h"

#if ENABLE(VIDEO)

#include <stdlib.h>
#include <stdbool.h>
#include "libavformat/avformat.h"
#include "libavformat/avio.h"
#include "libavformat/url.h"
#include "libavcodec/avcodec.h"
#include "libavutil/samplefmt.h"
#include "libavutil/avutil.h"
#include "libavutil/opt.h"
#include "libswscale/swscale.h"
#include "libswresample/swresample.h"
#include <string.h>
#include <proto/exec.h>
#include <clib/debug_protos.h>


#include "gui.h"
#include "acinerella.h"

#define D(x)

#define AUDIO_BUFFER_BASE_SIZE ((192000 * 3) / 2)

long loopfilter_mode = AVDISCARD_DEFAULT;

static struct SignalSemaphore semAcinerella;

//This struct represents one Acinerella video object.
//It contains data needed by FFMpeg.
struct _ac_data
{
	ac_instance instance;
	  
	AVFormatContext *pFormatCtx;
	  
	void *sender;
	ac_openclose_callback open_proc;
	ac_read_callback read_proc; 
	ac_seek_callback seek_proc; 
	ac_openclose_callback close_proc; 
	  
	URLProtocol protocol;
	char protocol_name[32];
};

typedef struct _ac_data ac_data;
typedef ac_data* lp_ac_data;

struct _ac_decoder_data
{
	ac_decoder decoder;
	int sought;
	double last_timecode;
};

typedef struct _ac_decoder_data ac_decoder_data;
typedef ac_decoder_data* lp_ac_decoder_data;

struct _ac_video_decoder
{
	ac_decoder decoder;
	int sought;
	double last_timecode;
	AVCodec *pCodec;
	AVCodecContext *pCodecCtx;
	int codec_opened;
	AVFrame *pFrame;
	AVFrame *pFrameRGB; 
	struct SwsContext *pSwsCtx;  
	struct SignalSemaphore sem;
};

typedef struct _ac_video_decoder ac_video_decoder;
typedef ac_video_decoder* lp_ac_video_decoder;

struct _ac_audio_decoder
{
	ac_decoder decoder;
	int sought;
	double last_timecode;
	AVCodec *pCodec;
	AVCodecContext *pCodecCtx;
	int codec_opened;
	AVFrame *pFrame;
	struct SwrContext *pSwrCtx;
	int max_buffer_size;
	char *pBuffer1;
	char *pBuffer2;	
};

typedef struct _ac_audio_decoder ac_audio_decoder;
typedef ac_audio_decoder* lp_ac_audio_decoder;

struct _ac_package_data
{
	ac_package package;
	AVPacket *ffpackage;
	int64_t pts;
};

typedef struct _ac_package_data ac_package_data;
typedef ac_package_data* lp_ac_package_data;

//
//--- Memory manager ---
//

ac_malloc_callback mgr_malloc = &av_malloc;
ac_realloc_callback mgr_realloc =  &av_realloc;
ac_free_callback mgr_free = &av_free;

void CALL_CONVT ac_mem_mgr(ac_malloc_callback mc, ac_realloc_callback rc, ac_free_callback fc)
{
	mgr_malloc = mc;
	mgr_realloc = rc;
	mgr_free = fc;
}

//
//--- Initialization and Stream opening---
//

void init_info(lp_ac_file_info info)
{
	info->title[0] = 0;
	info->author[0] = 0;
	info->copyright[0] = 0;
	info->comment[0] = 0;
	info->album[0] = 0;
	info->year = -1;
	info->track = -1;
	info->genre[0] = 0;
	info->duration = -1;
	info->bitrate = -1;
}

// Function called by FFMpeg when opening an ac stream.
static int file_open(URLContext *h, const char *filename, int flags)
{
    (void)flags;
	char * ptr = filename ? strstr(filename, "owb://") : NULL;

	if(ptr)
	{
		lp_ac_data instance = NULL;
		ptr += sizeof("owb://") - 1;

		/* Convert hex string to ULONG address */
		instance = (lp_ac_data) strtoul(ptr, 0, 16);
		D(kprintf("Received instance %p\n", instance));

		if(instance)
		{
			h->priv_data = instance;
			h->is_streamed = instance->seek_proc == NULL;

			if (instance->open_proc != NULL)
			{
				instance->open_proc(instance->sender);
		    }

			return 0;
		}
	}

	return -1;
}

// Function called by FFMpeg when reading from the stream
static int file_read(URLContext *h, unsigned char *buf, int size)
{
	if (((lp_ac_data)(h->priv_data))->read_proc != NULL)
	{
		return ((lp_ac_data)(h->priv_data))->read_proc(((lp_ac_data)(h->priv_data))->sender, (char *) buf, size);
	}

	return -1;
}

static int file_write(URLContext *h, const unsigned char *buf, int size)
{
    (void)h;
    (void)buf;
    (void)size;
	return -1;
}

// Function called by FFMpeg when seeking the stream
int64_t file_seek(URLContext *h, int64_t pos, int whence)
{
	if((whence >= 0) && (whence <= 2))
	{
		if(((lp_ac_data)(h->priv_data))->seek_proc != NULL)
		{
			return ((lp_ac_data)(h->priv_data))->seek_proc(((lp_ac_data)(h->priv_data))->sender, pos, whence);
	    }
	}

	return -1;
}

// Function called by FFMpeg when the stream should be closed
static int file_close(URLContext *h)
{
	if(((lp_ac_data)(h->priv_data))->close_proc != NULL)
	{
		return ((lp_ac_data)(h->priv_data))->close_proc(((lp_ac_data)(h->priv_data))->sender);
	}

	return 0;
}

static void av_log_callback(void* ptr, int level, const char* fmt, va_list vl)
{
    (void)ptr;
    (void)level;
    (void)fmt;
    (void)vl;
	/*
	char buffer[512];
	vsnprintf(buffer, sizeof(buffer), fmt, vl);
	kprintf(buffer);
	*/
}

static int initialized = 0;
ac_package flush_pkt;

static struct URLProtocol OWBProtocol =
{
	"owb",
	&file_open,
	NULL,
	&file_read,
	&file_write,
	&file_seek,
	&file_close,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	0,
	NULL,
	0,
	NULL
};

lp_ac_package ac_flush_packet(void)
{
	return &flush_pkt;
}

lp_ac_instance CALL_CONVT ac_init(void)
{
	if(!initialized)
	{
		InitSemaphore(&semAcinerella);
		
		D(kprintf("Initializing ffmpeg\n"));

		ObtainSemaphore(&semAcinerella);
		
		av_log_set_callback(av_log_callback/*AV_LOG_QUIET*/);
		avcodec_register_all();
		ffurl_register_protocol(&OWBProtocol);
		av_register_all();
		flush_pkt.size = 0;
		flush_pkt.data = NULL;
		
		initialized = 1;
		
		ReleaseSemaphore(&semAcinerella);		
	}

	// Allocate a new instance of the videoplayer data and return it
	lp_ac_data ptmp = (lp_ac_data)mgr_malloc(sizeof(ac_data));

	if(ptmp)
	{
	    ptmp->instance.opened = 0;
	    ptmp->instance.stream_count = 0;
	    ptmp->instance.output_format = AC_OUTPUT_BGR24;
	    init_info(&(ptmp->instance.info));
	}
	return (lp_ac_instance)ptmp;
}

void CALL_CONVT ac_free(lp_ac_instance pacInstance)
{
	// Close the decoder. If it is already closed, this won't be a problem as ac_close checks the streams state
	ac_close(pacInstance);

	if(pacInstance != NULL)
	{
		mgr_free((lp_ac_data)pacInstance);
	}
}

int CALL_CONVT ac_open(
	lp_ac_instance pacInstance,
	void *sender, 
	ac_openclose_callback open_proc,
	ac_read_callback read_proc, 
	ac_seek_callback seek_proc,
	ac_openclose_callback close_proc)
{
	char filename[64];
	AVFormatContext *ctx;

	pacInstance->opened = 0;
	  
	//Store the given parameters in the ac Instance
	((lp_ac_data)pacInstance)->sender = sender;
	((lp_ac_data)pacInstance)->open_proc = open_proc;  
	((lp_ac_data)pacInstance)->read_proc = read_proc;
	((lp_ac_data)pacInstance)->seek_proc = seek_proc;
	((lp_ac_data)pacInstance)->close_proc = close_proc;      

	//Generate a unique filename
	snprintf(filename, sizeof(filename), "%s://%p", "owb", pacInstance);
	D(kprintf("filename: <%s> instance <%p>\n", filename, pacInstance));

	((lp_ac_data)pacInstance)->pFormatCtx =	avformat_alloc_context();
	if(avformat_open_input(&(((lp_ac_data)pacInstance)->pFormatCtx), filename, NULL, NULL) != 0)
	{
		return -1;
	}

	ctx = ((lp_ac_data)pacInstance)->pFormatCtx;

	// Don't lock global semaphore on this, else deadlocks are easy to get
	if(avformat_find_stream_info(ctx, NULL) < 0)
	{
		return -1;
	}

	av_dump_format(ctx, 0, filename, 0);

	// Set some information in the instance variable 
	pacInstance->stream_count = ctx->nb_streams;
	pacInstance->opened = pacInstance->stream_count > 0;  

	pacInstance->info.bitrate  = ctx->bit_rate;
	pacInstance->info.duration = ctx->duration / AV_TIME_BASE;

	return 0;
}

void CALL_CONVT ac_close(lp_ac_instance pacInstance)
{
	if(pacInstance && pacInstance->opened)
	{
	    avformat_close_input(&((lp_ac_data)(pacInstance))->pFormatCtx);
	    pacInstance->opened = 0;    
	}
}

void CALL_CONVT ac_get_stream_info(lp_ac_instance pacInstance, int nb, lp_ac_stream_info info)
{
	if(!(pacInstance->opened))
	{
		return;
	}
  
	switch(((lp_ac_data)pacInstance)->pFormatCtx->streams[nb]->codec->codec_type)
	{
		case AVMEDIA_TYPE_VIDEO:
		{
			// Set stream type to "VIDEO"
			info->stream_type = AC_STREAM_TYPE_VIDEO;
					      
			// Store more information about the video stream
			info->additional_info.video_info.frame_width = ((lp_ac_data)pacInstance)->pFormatCtx->streams[nb]->codec->width;
			info->additional_info.video_info.frame_height = ((lp_ac_data)pacInstance)->pFormatCtx->streams[nb]->codec->height;
			info->additional_info.video_info.pixel_aspect = av_q2d(((lp_ac_data)pacInstance)->pFormatCtx->streams[nb]->codec->sample_aspect_ratio);
			// Sometimes "pixel aspect" may be zero. Correct this.
			if (info->additional_info.video_info.pixel_aspect == 0.0)
			{
				info->additional_info.video_info.pixel_aspect = 1.0;
			}
					      
			info->additional_info.video_info.frames_per_second =
			  (double)((lp_ac_data)pacInstance)->pFormatCtx->streams[nb]->r_frame_rate.den /
			  (double)((lp_ac_data)pacInstance)->pFormatCtx->streams[nb]->r_frame_rate.num;

			int64_t duration = ((lp_ac_data)pacInstance)->pFormatCtx->streams[nb]->duration;

			if(duration != AV_NOPTS_VALUE)
			{
				AVRational tb = ((lp_ac_data)pacInstance)->pFormatCtx->streams[nb]->time_base;
				info->additional_info.video_info.duration = (((double) tb.num) * duration) / tb.den;
			}
			else
			{
				info->additional_info.video_info.duration = 0.0;
			}

			break;
		}

		case AVMEDIA_TYPE_AUDIO:
		{
			// Set stream type to "AUDIO"
			info->stream_type = AC_STREAM_TYPE_AUDIO;

			// Store more information about the video stream
			info->additional_info.audio_info.samples_per_second = ((lp_ac_data)pacInstance)->pFormatCtx->streams[nb]->codec->sample_rate;        
			info->additional_info.audio_info.channel_count = ((lp_ac_data)pacInstance)->pFormatCtx->streams[nb]->codec->channels;

			int64_t duration = ((lp_ac_data)pacInstance)->pFormatCtx->streams[nb]->duration;

			if(duration != AV_NOPTS_VALUE)
			{
				AVRational tb = ((lp_ac_data)pacInstance)->pFormatCtx->streams[nb]->time_base;
				info->additional_info.audio_info.duration = (((double) tb.num) * duration) / tb.den;
			}
			else
			{
				info->additional_info.audio_info.duration = 0.0;
			}
					    
			// Set bit depth      
			switch (((lp_ac_data)pacInstance)->pFormatCtx->streams[nb]->codec->sample_fmt)
			{
			    // 8-Bit
			    case AV_SAMPLE_FMT_U8:
			    case AV_SAMPLE_FMT_U8P:
					info->additional_info.audio_info.bit_depth = 8;
					break;
						        
			    // 16-Bit
			    case AV_SAMPLE_FMT_S16:
			    case AV_SAMPLE_FMT_S16P:
					info->additional_info.audio_info.bit_depth = 16;
					break;

			    // 32-Bit
			    case AV_SAMPLE_FMT_S32:
			    case AV_SAMPLE_FMT_S32P:
			    case AV_SAMPLE_FMT_FLT:
			    case AV_SAMPLE_FMT_FLTP:
					info->additional_info.audio_info.bit_depth = 32;
					break;

			    case AV_SAMPLE_FMT_DBL:
			    case AV_SAMPLE_FMT_DBLP:
					 info->additional_info.audio_info.bit_depth = 64;
					 break;

			    // Unknown format, return zero
			    default:
					info->additional_info.audio_info.bit_depth = 0;
			}
					        
			break;
		}

		default:
		{
			info->stream_type = AC_STREAM_TYPE_UNKNOWN;
		}
	}
}

//
//---Package management---
//

AVPacket* ac_clone_packet(AVPacket* packet)
{
	AVPacket *clone = mgr_malloc(sizeof(AVPacket));

	if(!clone)
	{
		return NULL;
	}

	if(av_new_packet(clone, packet->size) < 0)
	{
		mgr_free(clone);
		return NULL;
	}

    clone->dts = packet->dts;
    clone->pts = packet->pts;
	clone->stream_index = packet->stream_index;
    clone->duration = packet->duration;
    memcpy(clone->data, packet->data, clone->size);

	return clone;
}

lp_ac_package CALL_CONVT ac_read_package(lp_ac_instance pacInstance)
{
	AVPacket Package;

	if (av_read_frame(((lp_ac_data)(pacInstance))->pFormatCtx, &Package) >= 0)
	{
		AVPacket *clone = ac_clone_packet(&Package);
		
		av_free_packet(&Package);

		if(clone)
		{
			lp_ac_package_data pTmp = (lp_ac_package_data)(mgr_malloc(sizeof(ac_package_data)));
							    
			if(pTmp)
			{
				memset(pTmp, 0, sizeof(ac_package_data));

				//Set package data
				pTmp->package.data = (char *) clone->data;
				pTmp->package.size = clone->size;
				pTmp->package.stream_index = clone->stream_index;
				pTmp->ffpackage = clone;
				if (clone->dts != AV_NOPTS_VALUE)
				{
					pTmp->pts = clone->dts;
				}
				D(kprintf("Allocate packet %p (data %p size %d stream %d destruct %p priv %p)\n", pTmp, pTmp->package.data, pTmp->package.size, pTmp->package.stream_index, pTmp->ffpackage->destruct, pTmp->ffpackage->priv));
			}
			return (lp_ac_package)(pTmp);
		}
	}

	return NULL;
}

// Frees the currently loaded package
void CALL_CONVT ac_free_package(lp_ac_package pPackage)
{
	//Free the packet
	if (pPackage != NULL && pPackage != ac_flush_packet())
	{
		D(kprintf("Free packet %p (data %p size %d stream %d destruct %p priv %p)\n", pPackage, ((lp_ac_package_data)pPackage)->package.data, ((lp_ac_package_data)pPackage)->package.size, ((lp_ac_package_data)pPackage)->package.stream_index, ((lp_ac_package_data)pPackage)->ffpackage->destruct, ((lp_ac_package_data)pPackage)->ffpackage->priv));
		av_free_packet(((lp_ac_package_data)pPackage)->ffpackage);
		mgr_free(((lp_ac_package_data)pPackage)->ffpackage);
		mgr_free((lp_ac_package_data)pPackage);
	}
}

//
//--- Decoder management ---
//

enum PixelFormat convert_pix_format(ac_output_format fmt)
{
	switch (fmt)
	{
		case AC_OUTPUT_RGB24:   return PIX_FMT_RGB24;
		case AC_OUTPUT_BGR24:   return PIX_FMT_BGR24;
		case AC_OUTPUT_RGBA32:  return PIX_FMT_RGB32;
		case AC_OUTPUT_BGRA32:  return PIX_FMT_BGR32;
		case AC_OUTPUT_YUV420P: return PIX_FMT_YUV420P;
		case AC_OUTPUT_YUV422:  return PIX_FMT_YUYV422;
	}
	return PIX_FMT_RGB24;
}

AVFrame* ac_get_frame(lp_ac_decoder decoder)
{
	if(decoder == NULL) return 0;

	if(decoder->type == AC_DECODER_TYPE_VIDEO)
	{
	    lp_ac_video_decoder pDecoder = (lp_ac_video_decoder) decoder;
		return pDecoder->pFrameRGB;
	}
	return NULL;
}

int	ac_set_output_format(lp_ac_decoder decoder, ac_output_format fmt)
{
	if(decoder == NULL) return 0;

	if(decoder->type == AC_DECODER_TYPE_VIDEO && fmt != decoder->pacInstance->output_format)
	{
		lp_ac_video_decoder pDecoder = (lp_ac_video_decoder) decoder;

		if(pDecoder)
		{
			ObtainSemaphore(&pDecoder->sem);

			pDecoder->decoder.pacInstance->output_format = fmt;

			if(pDecoder->pFrameRGB)
			{
				av_free(pDecoder->pFrameRGB);
			}
			if(pDecoder->pSwsCtx)
			{
				sws_freeContext(pDecoder->pSwsCtx);
		    }
			av_free(pDecoder->decoder.pBuffer);

			pDecoder->pFrameRGB = av_frame_alloc();
		    pDecoder->pSwsCtx = NULL;

		    //Reserve buffer memory
		    if(!pDecoder->pCodecCtx->width || !pDecoder->pCodecCtx->height)
		    {
				D(kprintf("Invalid video size\n"));
				ReleaseSemaphore(&pDecoder->sem);
				return -1;
		    }

			pDecoder->decoder.buffer_size = 2 * avpicture_get_size(convert_pix_format(pDecoder->decoder.pacInstance->output_format),
																   pDecoder->pCodecCtx->width, pDecoder->pCodecCtx->height);
			pDecoder->decoder.pBuffer = (char *)mgr_malloc(pDecoder->decoder.buffer_size);

		    if(pDecoder->decoder.pBuffer)
		    {
				memset(pDecoder->decoder.pBuffer, 0, pDecoder->decoder.buffer_size);
			    //Link decoder to buffer
			    avpicture_fill(
			      (AVPicture*)(pDecoder->pFrameRGB),
				  (const uint8_t *) pDecoder->decoder.pBuffer, convert_pix_format(pDecoder->decoder.pacInstance->output_format),
			      pDecoder->pCodecCtx->width, pDecoder->pCodecCtx->height);

			    pDecoder->decoder.linesize = pDecoder->pFrameRGB->linesize[0];
		    }
		    else
		    {
				ReleaseSemaphore(&pDecoder->sem);
				return -1;
		    }

			ReleaseSemaphore(&pDecoder->sem);
		}
	}

	return 0;
}

// Init a video decoder
void* ac_create_video_decoder(lp_ac_instance pacInstance, lp_ac_stream_info info, int nb)
{
	long skipvalue;
	lp_ac_video_decoder pDecoder = (lp_ac_video_decoder)(mgr_malloc(sizeof(ac_video_decoder)));

	if(!pDecoder)
	{
		return NULL;
	}

	memset(pDecoder, 0, sizeof(ac_video_decoder));
	  
	//Set a few properties
	pDecoder->decoder.pacInstance = pacInstance;
	pDecoder->decoder.type = AC_DECODER_TYPE_VIDEO;
	pDecoder->decoder.stream_index = nb;
	pDecoder->pCodecCtx = ((lp_ac_data)(pacInstance))->pFormatCtx->streams[nb]->codec;
	pDecoder->decoder.stream_info = *info;  

	// pDecoder->pCodecCtx->flags2 |= CODEC_FLAG2_FAST;  // Enable faster H264 decode.
	// Enable motion vector search (potentially slow), strong deblocking filter
	// for damaged macroblocks, and set our error detection sensitivity.
	pDecoder->pCodecCtx->error_concealment = FF_EC_GUESS_MVS | FF_EC_DEBLOCK;
	pDecoder->pCodecCtx->err_recognition = AV_EF_CAREFUL;

	switch(loopfilter_mode)
	{
		case MV_OWBApp_LoopFilterMode_SkipNonRef:
			skipvalue = AVDISCARD_NONREF;
			break;

		case MV_OWBApp_LoopFilterMode_SkipNonKey:
			skipvalue = AVDISCARD_NONKEY;
			break;

		case MV_OWBApp_LoopFilterMode_SkipAll:
			skipvalue = AVDISCARD_ALL;
			break;

		default:
		case MV_OWBApp_LoopFilterMode_SkipNone:
			skipvalue = AVDISCARD_DEFAULT;
			break;
	}

	pDecoder->pCodecCtx->skip_loop_filter = skipvalue;

	if (!(pDecoder->pCodec = avcodec_find_decoder(pDecoder->pCodecCtx->codec_id)))
	{
		ac_free_decoder((lp_ac_decoder)pDecoder);
		return NULL;
	}

	ObtainSemaphore(&semAcinerella);
	
	if (avcodec_open2(pDecoder->pCodecCtx, pDecoder->pCodec, NULL) < 0)
	{
		ReleaseSemaphore(&semAcinerella);
		
		ac_free_decoder((lp_ac_decoder)pDecoder);
	    return NULL;
	}
	else
	{
		pDecoder->codec_opened = TRUE;
	}
	
	ReleaseSemaphore(&semAcinerella);

	InitSemaphore(&pDecoder->sem);

	pDecoder->pFrame = av_frame_alloc();
	if(!pDecoder->pFrame)
	{
		ac_free_decoder((lp_ac_decoder)pDecoder);
		return NULL;
	}
	
	pDecoder->pFrameRGB = av_frame_alloc();	 
	if(!pDecoder->pFrameRGB)
	{
		ac_free_decoder((lp_ac_decoder)pDecoder);
		return NULL;
	}	
	
	pDecoder->pSwsCtx = NULL;
	  
	if(!pDecoder->pCodecCtx->width || !pDecoder->pCodecCtx->height)
	{
		D(kprintf("Invalid video size\n"));
		ac_free_decoder((lp_ac_decoder)pDecoder);
		return NULL;
	}

	pDecoder->decoder.buffer_size = 2 * avpicture_get_size(convert_pix_format(pacInstance->output_format),
	                                                       pDecoder->pCodecCtx->width, pDecoder->pCodecCtx->height);
	pDecoder->decoder.pBuffer = (char *)mgr_malloc(pDecoder->decoder.buffer_size);

	if(pDecoder->decoder.pBuffer)
	{
		memset(pDecoder->decoder.pBuffer, 0, pDecoder->decoder.buffer_size);

	    avpicture_fill(
	      (AVPicture*)(pDecoder->pFrameRGB), 
		  (const uint8_t *) pDecoder->decoder.pBuffer, convert_pix_format(pacInstance->output_format),
	      pDecoder->pCodecCtx->width, pDecoder->pCodecCtx->height);

	    pDecoder->decoder.linesize = pDecoder->pFrameRGB->linesize[0];
	}
	else
	{
		ac_free_decoder((lp_ac_decoder)pDecoder);
		return NULL;
	}

	return (void*)pDecoder;
}

// Init an audio decoder
void* ac_create_audio_decoder(lp_ac_instance pacInstance, lp_ac_stream_info info, int nb)
{
	lp_ac_audio_decoder pDecoder = (lp_ac_audio_decoder)(mgr_malloc(sizeof(ac_audio_decoder)));

	if(!pDecoder)
	{
		return NULL;
	}

	memset(pDecoder, 0, sizeof(ac_audio_decoder));
	  
	pDecoder->decoder.pacInstance = pacInstance;
	pDecoder->decoder.type = AC_DECODER_TYPE_AUDIO;
	pDecoder->decoder.stream_index = nb;
	pDecoder->decoder.stream_info = *info;
	  
	AVCodecContext *pCodecCtx = ((lp_ac_data)(pacInstance))->pFormatCtx->streams[nb]->codec;
	pDecoder->pCodecCtx = pCodecCtx;  
	  
	if (!(pDecoder->pCodec = avcodec_find_decoder(pCodecCtx->codec_id)))
	{
		ac_free_decoder((lp_ac_decoder)pDecoder);
		return NULL;
	}
	
	ObtainSemaphore(&semAcinerella);

	if (avcodec_open2(pCodecCtx, pDecoder->pCodec, NULL) < 0)
	{
		ReleaseSemaphore(&semAcinerella);
		ac_free_decoder((lp_ac_decoder)pDecoder);
	    return NULL;
	}
	else
	{
		pDecoder->codec_opened = TRUE;
	}
	
	ReleaseSemaphore(&semAcinerella);

	pDecoder->max_buffer_size = AUDIO_BUFFER_BASE_SIZE;
	pDecoder->pBuffer1 = (char *)(mgr_malloc(pDecoder->max_buffer_size));
	pDecoder->pBuffer2 = (char *)(mgr_malloc(pDecoder->max_buffer_size));

	if(pDecoder->pBuffer1 && pDecoder->pBuffer2)
	{
		memset(pDecoder->pBuffer1, 0, pDecoder->max_buffer_size);
		memset(pDecoder->pBuffer2, 0, pDecoder->max_buffer_size);
	}
	else
	{
		ac_free_decoder((lp_ac_decoder)pDecoder);
		return NULL;
	}

	pDecoder->decoder.pBuffer = pDecoder->pBuffer1;
	pDecoder->decoder.buffer_size = 0;

	pDecoder->pFrame = av_frame_alloc();

	if(!pDecoder->pFrame)
	{
		ac_free_decoder((lp_ac_decoder)pDecoder);
		return NULL;		
	}

	pDecoder->pSwrCtx = swr_alloc();
	
	if(pDecoder->pSwrCtx)
	{
		av_opt_set_int(pDecoder->pSwrCtx, "in_channel_layout",    pCodecCtx->channel_layout, 0);
		av_opt_set_int(pDecoder->pSwrCtx, "out_channel_layout",   AV_CH_LAYOUT_STEREO, 0);
		av_opt_set_int(pDecoder->pSwrCtx, "in_sample_rate",       pCodecCtx->sample_rate, 0);
		av_opt_set_int(pDecoder->pSwrCtx, "out_sample_rate",      pCodecCtx->sample_rate, 0);
		av_opt_set_sample_fmt(pDecoder->pSwrCtx, "in_sample_fmt", pCodecCtx->sample_fmt, 0);
		av_opt_set_sample_fmt(pDecoder->pSwrCtx, "out_sample_fmt", AV_SAMPLE_FMT_S16, 0);

		if(swr_init(pDecoder->pSwrCtx) < 0)
		{
			ac_free_decoder((lp_ac_decoder)pDecoder);
			return NULL;
		}
	}
	else
	{
		ac_free_decoder((lp_ac_decoder)pDecoder);
		return NULL;
	}

	return (void*)pDecoder;
}

lp_ac_decoder CALL_CONVT ac_create_decoder(lp_ac_instance pacInstance, int nb)
{
	ac_stream_info info;
	ac_get_stream_info(pacInstance, nb, &info);
	  
	lp_ac_decoder result = NULL;
	  
	if (info.stream_type == AC_STREAM_TYPE_VIDEO)
	{
		result = ac_create_video_decoder(pacInstance, &info, nb);
	} 
	else if (info.stream_type == AC_STREAM_TYPE_AUDIO)
	{
		result = ac_create_audio_decoder(pacInstance, &info, nb);
	}

	if(result)
	{
	    ((lp_ac_decoder_data)result)->last_timecode = 0;
	    ((lp_ac_decoder_data)result)->sought = 1;
	}
	  
	return result;
}

int ac_decode_video_package(lp_ac_package pPackage, lp_ac_video_decoder pDecoder)
{
	int finished;

	avcodec_decode_video2(pDecoder->pCodecCtx,
						  pDecoder->pFrame,
						  &finished,
						  ((lp_ac_package_data)pPackage)->ffpackage);
	  
	if (finished)
	{
		ObtainSemaphore(&pDecoder->sem);

	    pDecoder->pSwsCtx = sws_getCachedContext(pDecoder->pSwsCtx,
	      pDecoder->pCodecCtx->width, pDecoder->pCodecCtx->height, pDecoder->pCodecCtx->pix_fmt,
		  pDecoder->pCodecCtx->width, pDecoder->pCodecCtx->height, convert_pix_format(pDecoder->decoder.pacInstance->output_format),
		  /*SWS_BICUBIC*/SWS_FAST_BILINEAR, NULL, NULL, NULL);
	                                  
	    sws_scale(
	      pDecoder->pSwsCtx,
		  (const uint8_t * const*) pDecoder->pFrame->data,
	      pDecoder->pFrame->linesize,
	      0,
	      pDecoder->pCodecCtx->height, 
		  pDecoder->pFrameRGB->data,
		  (const int *) pDecoder->pFrameRGB->linesize);

		ReleaseSemaphore(&pDecoder->sem);

		return 1;
	}
	  
	return 0;
}

int ac_decode_audio_package(lp_ac_package pPackage, lp_ac_audio_decoder pDecoder)
{
	int finished, len;

	len = avcodec_decode_audio4(pDecoder->pCodecCtx, pDecoder->pFrame, &finished, ((lp_ac_package_data)pPackage)->ffpackage);
	
	if(finished && len > 0)
	{		
		int max_nb_samples = pDecoder->max_buffer_size / av_get_bytes_per_sample(AV_SAMPLE_FMT_S16);
		int nb_requested_samples = pDecoder->pFrame->nb_samples * 2; // Ok, why "2", i don't get it.
		int nb_converted_samples = swr_convert(pDecoder->pSwrCtx, (uint8_t **) &pDecoder->pBuffer1, max_nb_samples,
						  (const uint8_t **) pDecoder->pFrame->extended_data, nb_requested_samples);
																		  
		if(nb_converted_samples > 0)
		{
			int output_size = nb_converted_samples * av_get_bytes_per_sample(AV_SAMPLE_FMT_S16);
					
			pDecoder->decoder.pBuffer = pDecoder->pBuffer1;
			pDecoder->decoder.buffer_size = output_size;
					
			return 1;
		}	
	}
	return 0;
}

int CALL_CONVT ac_decode_package(lp_ac_package pPackage, lp_ac_decoder pDecoder)
{
	double timebase = 
	  av_q2d(((lp_ac_data)pDecoder->pacInstance)->pFormatCtx->streams[pPackage->stream_index]->time_base);

	//Create a valid timecode
	if(((lp_ac_package_data)pPackage)->pts > 0)
	{
	    lp_ac_decoder_data dec_dat = (lp_ac_decoder_data)pDecoder;    

	    dec_dat->last_timecode = pDecoder->timecode;
	    pDecoder->timecode = ((lp_ac_package_data)pPackage)->pts * timebase;
		    
	    double delta = pDecoder->timecode - dec_dat->last_timecode;
	    double max_delta, min_delta;
		    

	    if(dec_dat->sought > 0)
	    {
			max_delta = 120.0;
			min_delta = -120.0;
			--dec_dat->sought;
	    }
	    else
	    {
			max_delta = 4.0;
			min_delta = 0.0;
	    }
		      
	    if((delta < min_delta) || (delta > max_delta))
	    {
			pDecoder->timecode = dec_dat->last_timecode;
			if (dec_dat->sought > 0)
			{
			    ++dec_dat->sought;
			}
	    }
	}
	  
	D(kprintf("Decode packet %p (data %p size %d stream %d destruct %p priv %p)\n", pPackage, ((lp_ac_package_data)pPackage)->package.data, ((lp_ac_package_data)pPackage)->package.size, ((lp_ac_package_data)pPackage)->package.stream_index, ((lp_ac_package_data)pPackage)->ffpackage->destruct, ((lp_ac_package_data)pPackage)->ffpackage->priv));

	if(pDecoder->type == AC_DECODER_TYPE_VIDEO)
	{
		return ac_decode_video_package(pPackage, (lp_ac_video_decoder)pDecoder);
	} 
	else if(pDecoder->type == AC_DECODER_TYPE_AUDIO)
	{
		return ac_decode_audio_package(pPackage, (lp_ac_audio_decoder)pDecoder);
	}
	return 0;
}

// Seek function

int CALL_CONVT ac_seek(lp_ac_decoder pDecoder, int dir, int64_t target_pos)
{
	AVRational timebase = 
	  ((lp_ac_data)pDecoder->pacInstance)->pFormatCtx->streams[pDecoder->stream_index]->time_base;
	  
	int flags = dir < 0 ? AVSEEK_FLAG_BACKWARD : 0;    
	int64_t pos = av_rescale(target_pos, AV_TIME_BASE, 1000);
	  
	((lp_ac_decoder_data)pDecoder)->sought = 100;
	pDecoder->timecode = target_pos / 1000;
	  
	if(av_seek_frame(((lp_ac_data)pDecoder->pacInstance)->pFormatCtx, pDecoder->stream_index,
		av_rescale_q(pos, AV_TIME_BASE_Q, timebase), flags) >= 0)
	{
		return 1;
	}
	return 0;  
}

void CALL_CONVT ac_flush_buffers(lp_ac_decoder pDecoder)
{
	avcodec_flush_buffers(((lp_ac_data)pDecoder->pacInstance)->pFormatCtx->streams[pDecoder->stream_index]->codec);
}

// Free video decoder
void ac_free_video_decoder(lp_ac_video_decoder pDecoder)
{ 
	if(pDecoder)
	{
		if(pDecoder->pFrame)
			av_free(pDecoder->pFrame);
			
		if(pDecoder->pFrameRGB)
			av_free(pDecoder->pFrameRGB);    

		ObtainSemaphore(&semAcinerella);
			
		if(pDecoder->pSwsCtx)
			sws_freeContext(pDecoder->pSwsCtx);
		
		if(pDecoder->pCodecCtx && pDecoder->codec_opened)
			avcodec_close(pDecoder->pCodecCtx);
		
		ReleaseSemaphore(&semAcinerella);
		
		if(pDecoder->decoder.pBuffer)
			mgr_free(pDecoder->decoder.pBuffer);  
		
		mgr_free(pDecoder);
	}	
}

// Free video decoder
void ac_free_audio_decoder(lp_ac_audio_decoder pDecoder)
{
	if(pDecoder)
	{
		ObtainSemaphore(&semAcinerella);
		
		if(pDecoder->pSwrCtx)
			swr_free(&pDecoder->pSwrCtx);

		if(pDecoder->pCodecCtx && pDecoder->codec_opened)
			avcodec_close(pDecoder->pCodecCtx);
		
		ReleaseSemaphore(&semAcinerella);
		
		mgr_free(pDecoder->pBuffer1);
		mgr_free(pDecoder->pBuffer2);
		mgr_free(pDecoder);
	}	
}

void CALL_CONVT ac_free_decoder(lp_ac_decoder pDecoder)
{
	if(pDecoder)
	{
		if(pDecoder->type == AC_DECODER_TYPE_VIDEO)
		{
			ac_free_video_decoder((lp_ac_video_decoder)pDecoder);
		}
		else if(pDecoder->type == AC_DECODER_TYPE_AUDIO)
		{
			ac_free_audio_decoder((lp_ac_audio_decoder)pDecoder);
		} 
	}	 
}

#endif

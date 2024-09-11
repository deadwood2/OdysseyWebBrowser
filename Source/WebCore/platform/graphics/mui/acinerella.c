/*
 * Acinerella -- ffmpeg Wrapper Library
 * Copyright (C) 2008-2018  Andreas Stöckel
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with Acinerella.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavformat/avio.h>
#include <libavutil/avutil.h>
#include <libavutil/imgutils.h>
#include <libswresample/swresample.h>
#include <libswscale/swscale.h>

#include "acinerella.h"
#if LIBAVCODEC_VERSION_MAJOR < 57
#define codecpar codec
#endif

#define MIN(X, Y) (((X) < (Y)) ? (X) : (Y))
#define MAX(X, Y) (((X) > (Y)) ? (X) : (Y))

#define PROBE_BUF_MIN 1024
#define PROBE_BUF_MAX (1 << 15)

#define ERR(E)          \
	do {                   \
		if (!(E)) {     \
			goto error; \
		}               \
	} while (0)
#define AV_ERR(E)       \
	do {                   \
		if ((E) < 0) {  \
			goto error; \
		}               \
	} while (0)

struct _ac_data {
	ac_instance instance;

	AVFormatContext *pFormatCtx;
	AVIOContext *pIo;

	void *sender;
	ac_openclose_callback open_proc;
	ac_read_callback read_proc;
	ac_seek_callback seek_proc;
	ac_openclose_callback close_proc;

	uint8_t *buffer;
	uint8_t *probe_buffer;
	size_t probe_buffer_size;
	size_t probe_buffer_offs;
};

typedef struct _ac_data ac_data;
typedef ac_data *lp_ac_data;

struct _ac_decoder_data {
	ac_decoder decoder;
	int sought;
	double last_timecode;
	int doseek;
};

typedef struct _ac_decoder_data ac_decoder_data;
typedef ac_decoder_data *lp_ac_decoder_data;

struct _ac_video_decoder {
	ac_decoder decoder;
	int sought;
	double last_timecode;
	int doseek;
	AVCodec *pCodec;
	AVCodecContext *pCodecCtx;
	AVFrame *pFrame;
	AVFrame *pFrameRGB;
	struct SwsContext *pSwsCtx;
};

typedef struct _ac_video_decoder ac_video_decoder;
typedef ac_video_decoder *lp_ac_video_decoder;

struct _ac_audio_decoder {
	ac_decoder decoder;
	int sought;
	double last_timecode;
	int doseek;
	AVCodec *pCodec;
	AVCodecContext *pCodecCtx;
	AVFrame *pFrame;
	SwrContext *pSwrCtx;
	size_t own_buffer_size;
};

struct _ac_decoder_frame_internal {
	ac_decoder_frame frame;
	AVFrame *pFrame;
	size_t own_buffer_size;
	int needs_unref;
};

typedef struct _ac_audio_decoder ac_audio_decoder;
typedef ac_audio_decoder *lp_ac_audio_decoder;

struct _ac_package_data {
	ac_package package;
	AVPacket *pPack;
	int64_t pts;
};

typedef struct _ac_package_data ac_package_data;
typedef ac_package_data *lp_ac_package_data;

//
//--- Forward declarations ---
//

static void ac_free_video_decoder(lp_ac_video_decoder pDecoder);
static void ac_free_audio_decoder(lp_ac_audio_decoder pDecoder);

//
//--- Initialization and Stream opening---
//

static void init_info(lp_ac_file_info info) {
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

/* avcodec_register_all(), av_register_all() deprecated since lavc 58.9.100 */
#if (LIBAVCODEC_VERSION_MAJOR < 58) || \
    ((LIBAVCODEC_VERSION_MAJOR == 58) && (LIBAVCODEC_VERSION_MINOR < 9))
#define AC_NEED_REGISTER_ALL
#endif

#ifdef AC_NEED_REGISTER_ALL
static bool av_initialized = false;
static void ac_init_ffmpeg(void) {
	if (!av_initialized) {
		avcodec_register_all();
		av_register_all();

		av_initialized = true;
	}
}
#endif /* AC_NEED_REGISTER_ALL */

lp_ac_instance CALL_CONVT ac_init(void) {
#ifdef AC_NEED_REGISTER_ALL
	ac_init_ffmpeg();
#endif /* AC_NEED_REGISTER_ALL */

	// Allocate a new instance of the videoplayer data and return it
	lp_ac_data ptmp;
	ERR(ptmp = (lp_ac_data)av_malloc(sizeof(ac_data)));

	// Initialize the created structure
	memset(ptmp, 0, sizeof(ac_data));

	ptmp->instance.opened = 0;
	ptmp->instance.stream_count = 0;
	ptmp->instance.output_format = AC_OUTPUT_BGR24;
	init_info(&(ptmp->instance.info));
	return (lp_ac_instance)ptmp;

error:
	return NULL;
}

void CALL_CONVT ac_free(lp_ac_instance pacInstance) {
	if (pacInstance) {
		// Close the decoder. If it is already closed, this won't be a problem
		// as
		// ac_close checks the streams state
		ac_close(pacInstance);
		av_free((lp_ac_data)pacInstance);
	}
}

static int io_read(void *opaque, uint8_t *buf, int buf_size) {
	lp_ac_data self = ((lp_ac_data)opaque);

	// If there is still memory in the probe buffer, consume this memory first
	if (self->probe_buffer &&
	    self->probe_buffer_offs < self->probe_buffer_size) {
		// Copy as many bytes as possible from the probe buffer
		size_t cnt =
		    MIN(buf_size, self->probe_buffer_size - self->probe_buffer_offs);
		memcpy(buf, self->probe_buffer + self->probe_buffer_offs, cnt);

		// Advance the read/write pointers
		self->probe_buffer_offs += cnt;
		buf += cnt;
		buf_size -= cnt;

		// Free the probe buffer once all bytes have been read
		if (self->probe_buffer_offs == self->probe_buffer_size) {
			av_free(self->probe_buffer);
			self->probe_buffer = NULL;
			self->probe_buffer_size = 0;
			self->probe_buffer_offs = 0;
		}

		// If the caller requester more bytes than in the probe
		// buffer, read them using read_proc, if available.
		if (buf_size && self->read_proc != NULL) {
			int rest = self->read_proc(self->sender, buf, buf_size);
			if (rest != -1) {
				cnt += rest;
			}
		}

		return (int) cnt;
	}

	// Read more data by using the read_proc
	if (self->read_proc != NULL) {
		return self->read_proc(self->sender, buf, buf_size);
	}

	return -1;
}

static int64_t io_seek(void *opaque, int64_t pos, int whence) {
	lp_ac_data self = ((lp_ac_data)opaque);
	if (self->seek_proc != NULL) {
		if ((whence >= 0) && (whence <= 2)) {
			// Throw away the probe buffer if seek is performed.
			// NOTE: This could be improved to check if the seek
			// is inside the probe_buffer, but it would get really
			// complicated as real seek would need to be omitted
			// then.
			if (self->probe_buffer) {
				av_free(self->probe_buffer);
				self->probe_buffer = NULL;
				self->probe_buffer_size = 0;
				self->probe_buffer_offs = 0;
			}
			return self->seek_proc(self->sender, pos, whence);
		}
	}
	return -1;
}

lp_ac_proberesult CALL_CONVT ac_probe_input_buffer(uint8_t *buf, int bufsize,
                                                   char *filename,
                                                   int *score_max) {
	AVProbeData pd;
	AVInputFormat *fmt;

#ifdef AC_NEED_REGISTER_ALL
	// Initialize FFMpeg libraries
	ac_init_ffmpeg();
#endif /* AC_NEED_REGISTER_ALL */

	// Set the filename and mime_type
	pd.mime_type = "";
	pd.filename = "";
	if (filename) {
		pd.filename = filename;
	}

	// The given buffer has to be copied to a new one, which is aligned and
	// padded
	uint8_t *aligned_buf;
	ERR(aligned_buf = av_malloc(bufsize + AVPROBE_PADDING_SIZE));
	memcpy(aligned_buf, buf, bufsize);
	memset(aligned_buf + bufsize, 0, AVPROBE_PADDING_SIZE);

	// Set the probe data buffer
	pd.buf = aligned_buf;
	pd.buf_size = bufsize;

	// Test it
	int score_ret;
	fmt = av_probe_input_format3(&pd, 1, &score_ret);
	if (score_ret > *score_max) {
		*score_max = score_ret;
	}

	// Free the temporary buffer
	av_free(aligned_buf);

	return (lp_ac_proberesult)fmt;

error:
	return NULL;
}

static AVInputFormat *ac_probe_input_stream(void *sender, ac_read_callback read_proc,
                                            char *filename, uint8_t **buf,
                                            size_t *buf_read) {
	// Initialize the result variables
	AVInputFormat *fmt = NULL;
	*buf_read = 0;
	*buf = NULL;
	int last_iteration = 0;
	int probe_size = 0;

	for (probe_size = PROBE_BUF_MIN;
	     (probe_size <= PROBE_BUF_MAX) && !fmt && !last_iteration;
	     probe_size <<= 1) {
		int score = AVPROBE_SCORE_MAX / 4;

		// Allocate some memory for the current probe buffer
		uint8_t *tmp_buf = av_malloc(probe_size);
		if (!tmp_buf) {
			return fmt;  // An error occurred, abort
		}

		// Copy the old data to the new buffer
		if (*buf) {
			memcpy(tmp_buf, *buf, *buf_read);
			// Free the old data memory
			av_free(*buf);
			// Zero the pointer to avoid double free on read_proc error
			*buf = NULL;
		}

		// Read the new data
		uint8_t *write_ptr = tmp_buf + *buf_read;
		int read_size = probe_size - *buf_read;
		int size = read_proc(sender, write_ptr, read_size);
		if (size < 0) {
			av_free(tmp_buf);
			return fmt;  // An error occurred, abort
		}
		if (size < read_size) {
			last_iteration = 1;
			probe_size = *buf_read + size;
		}

		// Probe it
		fmt = (AVInputFormat *)ac_probe_input_buffer(tmp_buf, probe_size,
		                                             filename, &score);

		// Set the new buffer
		*buf = tmp_buf;
		*buf_read = probe_size;
	}

	// Return the result
	return fmt;
}

static void cpymetadata(const AVFormatContext *ctx, const char *key, char *tar,
                        size_t len) {
	const AVDictionaryEntry *entry = av_dict_get(ctx->metadata, key, NULL, 0);
	if (entry) {
		strncpy(tar, entry->value, len - 1);
		tar[len - 1] = '\0';
	} else {
		strncpy(tar, "", len);
	}
}

static void cpymetadatai(const AVFormatContext *ctx, const char *key,
                         int *tar) {
	char buf[16];
	cpymetadata(ctx, key, buf, 16);
	*tar = atoi(buf);
}

static int finalize_open(lp_ac_instance pacInstance) {
	lp_ac_data self = ((lp_ac_data)pacInstance);

    if (!self || !self->pFormatCtx)
        return -1;

	// Assume the stream is opened
	pacInstance->opened = 1;

	// Retrieve stream information
	AVFormatContext *ctx = self->pFormatCtx;
	AV_ERR(avformat_find_stream_info(ctx, NULL));
	cpymetadata(ctx, "title", pacInstance->info.title, 512);
	cpymetadata(ctx, "artist", pacInstance->info.author, 512);
	cpymetadata(ctx, "copyright", pacInstance->info.copyright, 512);
	cpymetadata(ctx, "comment", pacInstance->info.comment, 512);
	cpymetadata(ctx, "album", pacInstance->info.album, 512);
	cpymetadata(ctx, "genre", pacInstance->info.genre, 32);
	cpymetadatai(ctx, "year", &(pacInstance->info.year));
	cpymetadatai(ctx, "track", &(pacInstance->info.track));
	pacInstance->info.bitrate = ctx->bit_rate;
	pacInstance->info.duration = ctx->duration * 1000 / AV_TIME_BASE;

	// Set some information in the instance variable
	pacInstance->stream_count =
	    ((lp_ac_data)pacInstance)->pFormatCtx->nb_streams;
	if (pacInstance->stream_count <= 0) {
		goto error;
	}
	return 0;

error:
	ac_close(pacInstance);
	return -1;
}

void __av_log_default_callback(void *ig, int no, const char *re, va_list me)
{
}

int CALL_CONVT ac_open(lp_ac_instance pacInstance, void *sender,
                       ac_openclose_callback open_proc,
                       ac_read_callback read_proc, ac_seek_callback seek_proc,
                       ac_openclose_callback close_proc,
                       lp_ac_proberesult proberesult) {
	// Instance cannot be opened twice!
	if (pacInstance->opened) {
		return -1;
	}

	av_log_set_level(AV_LOG_QUIET);
	av_log_set_callback(&__av_log_default_callback);

	// Reference at the underlying lp_ac_data instance
	lp_ac_data self = ((lp_ac_data)pacInstance);

	// Store the given parameters in the ac Instance
	self->sender = sender;
	self->open_proc = open_proc;
	self->read_proc = read_proc;
	self->seek_proc = seek_proc;
	self->close_proc = close_proc;

	// Call the file open proc
	if (self->open_proc != NULL) {
		if (self->open_proc(sender) < 0) {
			goto error;
		}
	}

	AVInputFormat *fmt = NULL;

	// Probe the input format, if no probe result is specified
	if (proberesult == NULL) {
		fmt =
		    ac_probe_input_stream(sender, read_proc, "", &(self->probe_buffer),
		                          &(self->probe_buffer_size));
	} else {
		fmt = (AVInputFormat *)proberesult;
	}

	ERR(fmt);

	// Reserve the buffer memory and initialise the context
	ERR(self->buffer = av_malloc(AC_BUFSIZE));
	ERR(self->pIo = avio_alloc_context(self->buffer, AC_BUFSIZE, 0, self, io_read,
	                                   0, seek_proc ? io_seek : NULL));
	self->pIo->seekable = seek_proc != NULL;

	// Open the given input stream (the io structure) with the given format of
	// the stream (fmt) and write the pointer to the new format context to the
	// pFormatCtx variable
	ERR(self->pFormatCtx = avformat_alloc_context());
	self->pFormatCtx->pb = self->pIo;
	AV_ERR(avformat_open_input(&(self->pFormatCtx), "", fmt, NULL));

	return finalize_open(pacInstance);

error:
	ac_close(pacInstance);
	return -1;
}

int CALL_CONVT ac_open_file(lp_ac_instance pacInstance, const char *filename) {
	// Instance cannot be opened twice!
	if (pacInstance->opened) {
		return -1;
	}

	// Reference at the underlying lp_ac_data instance
	lp_ac_data self = ((lp_ac_data)pacInstance);

	self->pFormatCtx = NULL;
	AV_ERR(avformat_open_input(&(self->pFormatCtx), filename, NULL, NULL));

	return finalize_open(pacInstance);

error:
	ac_close(pacInstance);
	return -1;
}

void CALL_CONVT ac_close(lp_ac_instance pacInstance) {
	lp_ac_data self = ((lp_ac_data)pacInstance);
	if (pacInstance->opened) {
		// Close the opened file
		if (self->close_proc != NULL) {
			self->close_proc(((lp_ac_data)(pacInstance))->sender);
		}

		avformat_close_input(&(self->pFormatCtx));

		pacInstance->opened = 0;
	}

	// Make sure all buffers are freed
	av_free(self->probe_buffer);
	if (self->pIo)
		av_free(self->pIo->buffer); // this is either self->buffer or a re-allocated buffer!
	av_free(self->pIo);
	self->buffer = NULL;
	self->probe_buffer = NULL;
	self->pIo = NULL;
	self->pFormatCtx = NULL;
	self->sender = NULL;
	self->open_proc = NULL;
	self->read_proc = NULL;
	self->seek_proc = NULL;
	self->close_proc = NULL;
}

void CALL_CONVT ac_get_stream_info(lp_ac_instance pacInstance, int nb,
                                   lp_ac_stream_info info) {
	// Rese the stream info structure
	memset(info, 0, sizeof(ac_stream_info));
	info->stream_type = AC_STREAM_TYPE_UNKNOWN;

	// Abort if the instance is not opened
	if (!(pacInstance->opened)) {
		return;
	}

	// Read the information
	lp_ac_data self = ((lp_ac_data)pacInstance);
	switch (self->pFormatCtx->streams[nb]->codecpar->codec_type) {
		case AVMEDIA_TYPE_VIDEO:
			// Set stream type to "VIDEO"
			info->stream_type = AC_STREAM_TYPE_VIDEO;

			// Store more information about the video stream
			info->additional_info.video_info.frame_width =
			    self->pFormatCtx->streams[nb]->codecpar->width;
			info->additional_info.video_info.frame_height =
			    self->pFormatCtx->streams[nb]->codecpar->height;

			double pixel_aspect_num = self->pFormatCtx->streams[nb]
			                              ->codecpar->sample_aspect_ratio.num;
			double pixel_aspect_den = self->pFormatCtx->streams[nb]
			                              ->codecpar->sample_aspect_ratio.den;

			// Sometime "pixel aspect" may be zero or have other invalid values.
			// Correct this.
			if (pixel_aspect_num <= 0.0 || pixel_aspect_den <= 0.0)
				info->additional_info.video_info.pixel_aspect = 1.0;
			else
				info->additional_info.video_info.pixel_aspect =
				    pixel_aspect_num / pixel_aspect_den;

			info->additional_info.video_info.frames_per_second =
			    (double)self->pFormatCtx->streams[nb]->r_frame_rate.num /
			    (double)self->pFormatCtx->streams[nb]->r_frame_rate.den;
			break;
		case AVMEDIA_TYPE_AUDIO:
			// Set stream type to "AUDIO"
			info->stream_type = AC_STREAM_TYPE_AUDIO;

			// Store more information about the video stream
			info->additional_info.audio_info.samples_per_second =
			    self->pFormatCtx->streams[nb]->codecpar->sample_rate;
			info->additional_info.audio_info.channel_count =
			    self->pFormatCtx->streams[nb]->codecpar->channels;

			// Set bit depth
#if LIBAVCODEC_VERSION_MAJOR >= 57
			switch (self->pFormatCtx->streams[nb]->codecpar->format) {
#else
			switch (self->pFormatCtx->streams[nb]->codec->sample_fmt) {
#endif /* LIBAVCODEC_VERSION_MAJOR >= 57 */
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

				/*        //24-Bit (removed in the newest ffmpeg version)
					    case SAMPLE_FMT_S24:
					      info->additional_info.audio_info.bit_depth =
					          24;
					    break; */

				// 32-Bit
				case AV_SAMPLE_FMT_S32:
				case AV_SAMPLE_FMT_FLT:
				case AV_SAMPLE_FMT_DBL:
				case AV_SAMPLE_FMT_S32P:
				case AV_SAMPLE_FMT_FLTP:
				case AV_SAMPLE_FMT_DBLP:
					info->additional_info.audio_info.bit_depth = 32;
					break;

				// Unknown format, return zero
				default:
					info->additional_info.audio_info.bit_depth = 0;
			}

			break;
		default: {
			// Do nothing
		}
	}
}

//
//---Package management---
//

lp_ac_package CALL_CONVT ac_read_package(lp_ac_instance pacInstance) {
	if (NULL == pacInstance)
		return NULL;
	if (NULL == ((lp_ac_data)(pacInstance))->pFormatCtx->internal)
		return NULL; // why?

	// Allocate the result packet
	lp_ac_package_data pkt;
	ERR(pkt = av_malloc(sizeof(ac_package_data)));
	memset(pkt, 0, sizeof(ac_package_data));
	ERR(pkt->pPack = av_malloc(sizeof(AVPacket)));

	av_init_packet(pkt->pPack);
	pkt->pPack->data = NULL;
	pkt->pPack->size = 0;

	// Try to read package
	AV_ERR(av_read_frame(((lp_ac_data)(pacInstance))->pFormatCtx, pkt->pPack));

	if (pkt->pPack->dts != AV_NOPTS_VALUE) {
		pkt->pts = pkt->pPack->dts;
	}
	pkt->package.stream_index = pkt->pPack->stream_index;
	return (lp_ac_package)(pkt);

error:
	ac_free_package((lp_ac_package)pkt);
	return NULL;
}

// Frees the currently loaded package
void CALL_CONVT ac_free_package(lp_ac_package pPackage) {
	if (pPackage && pPackage != ac_flush_packet()) {
		lp_ac_package_data self = (lp_ac_package_data)pPackage;
		av_packet_unref(self->pPack);
		av_free_packet(self->pPack);
		av_free(self->pPack);
		av_free(self);
	}
}

//
//--- Decoder management ---
//

static enum AVPixelFormat convert_pix_format(ac_output_format fmt) {
	switch (fmt) {
		case AC_OUTPUT_RGB24:
			return AV_PIX_FMT_RGB24;
		case AC_OUTPUT_BGR24:
			return AV_PIX_FMT_BGR24;
		case AC_OUTPUT_RGBA32:
			return AV_PIX_FMT_RGB32;
		case AC_OUTPUT_BGRA32:
			return AV_PIX_FMT_BGR32;
		case AC_OUTPUT_YUV420P:
			return AV_PIX_FMT_YUV420P;
		case AC_OUTPUT_YUV422:
			return AV_PIX_FMT_YUYV422;
		case AC_OUTPUT_ARGB32:
			return AV_PIX_FMT_ARGB;
	}
	return AV_PIX_FMT_RGB24;
}

// Init a video decoder
static void *ac_create_video_decoder(lp_ac_instance pacInstance,
                                     lp_ac_stream_info info, int nb,
                                     ac_codecctx_callback codec_proc) {
	// Allocate memory for a new decoder instance
	lp_ac_data self = ((lp_ac_data)(pacInstance));
	lp_ac_video_decoder pDecoder;
	ERR(pDecoder = (lp_ac_video_decoder)(av_malloc(sizeof(ac_video_decoder))));
	memset(pDecoder, 0, sizeof(ac_video_decoder));

#if LIBAVCODEC_VERSION_MAJOR >= 57
	// Manually create a codec context
	AVFormatContext *pFormatCtx = self->pFormatCtx;
	AVCodec *pCodec =
	    avcodec_find_decoder(pFormatCtx->streams[nb]->codecpar->codec_id);
	AVCodecContext *pCodecCtx;
	ERR(pCodecCtx = avcodec_alloc_context3(pCodec));
	AV_ERR(avcodec_parameters_to_context(pCodecCtx, pFormatCtx->streams[nb]->codecpar));
#else
	AVCodecContext *pCodecCtx = self->pFormatCtx->streams[nb]->codec;
#endif /* LIBAVCODEC_VERSION_MAJOR >= 57 */

	// Call codec_proc if provided
	if (codec_proc != NULL) {
		ERR(codec_proc(pCodecCtx));
	}
	else {
		pCodecCtx->skip_loop_filter = AVDISCARD_ALL;
	}

	// Set a few properties
	pDecoder->decoder.pacInstance = pacInstance;
	pDecoder->decoder.type = AC_DECODER_TYPE_VIDEO;
	pDecoder->decoder.stream_index = nb;
	pDecoder->pCodecCtx = pCodecCtx;
	pDecoder->decoder.stream_info = *info;

	// Find correspondenting codec
	ERR(pDecoder->pCodec =
	          avcodec_find_decoder(pDecoder->pCodecCtx->codec_id));

	// Open codec
	AV_ERR(avcodec_open2(pDecoder->pCodecCtx, pDecoder->pCodec, NULL));

	// Reserve frame variables
	ERR(pDecoder->pFrame = av_frame_alloc());
	ERR(pDecoder->pFrameRGB = av_frame_alloc());

	pDecoder->pSwsCtx = NULL;

	// Reserve buffer memory
	enum AVPixelFormat pix_fmt_out =
	    convert_pix_format(pacInstance->output_format);
	pDecoder->decoder.buffer_size =
	    av_image_get_buffer_size(pix_fmt_out, pDecoder->pCodecCtx->width,
	                             pDecoder->pCodecCtx->height, 1);
	ERR(pDecoder->decoder.pBuffer =
	    (uint8_t *)av_malloc(pDecoder->decoder.buffer_size));

	memset(pDecoder->decoder.pBuffer, 0, pDecoder->decoder.buffer_size);

#if 0
	// Link decoder to buffer
	AVFrame *picture = (AVFrame *)(pDecoder->pFrameRGB);
	AV_ERR(av_image_fill_arrays(picture->data, picture->linesize,
	                            pDecoder->decoder.pBuffer, pix_fmt_out,
	                            pDecoder->pCodecCtx->width,
	                            pDecoder->pCodecCtx->height, 1));
#endif
	return (void *)pDecoder;

error:
	ac_free_video_decoder(pDecoder);
	return NULL;
}

void ac_decoder_set_loopfilter(lp_ac_decoder pDecoder, int lflevel)
{
    ((lp_ac_video_decoder)pDecoder)->pCodecCtx->skip_loop_filter = lflevel;
}

int ac_get_audio_rate(lp_ac_decoder pDecoder)
{
	lp_ac_audio_decoder audioDecoder = (lp_ac_audio_decoder)pDecoder;
	if (audioDecoder)
	{
		if (audioDecoder->pSwrCtx)
			return 44100;
		return audioDecoder->decoder.stream_info.additional_info.audio_info.samples_per_second;
	}
	return 44100; // wat? :)
}

// Init a audio decoder
static void *ac_create_audio_decoder(lp_ac_instance pacInstance,
                                     lp_ac_stream_info info, int nb,
                                     ac_codecctx_callback codec_proc) {
	// Allocate memory for a new decoder instance
	lp_ac_data self = ((lp_ac_data)(pacInstance));
	lp_ac_audio_decoder pDecoder;
	ERR(pDecoder = (lp_ac_audio_decoder)(av_malloc(sizeof(ac_audio_decoder))));
	memset(pDecoder, 0, sizeof(ac_audio_decoder));

#if LIBAVCODEC_VERSION_MAJOR >= 57
	// Manually create a codec context
	AVFormatContext *pFormatCtx = self->pFormatCtx;
	AVCodec *pCodec =
	    avcodec_find_decoder(pFormatCtx->streams[nb]->codecpar->codec_id);
	AVCodecContext *pCodecCtx;
	ERR(pCodecCtx = avcodec_alloc_context3(pCodec));
	AV_ERR(avcodec_parameters_to_context(pCodecCtx, pFormatCtx->streams[nb]->codecpar));
#else
	AVCodecContext *pCodecCtx = self->pFormatCtx->streams[nb]->codec;
#endif /* LIBAVCODEC_VERSION_MAJOR >= 57 */

	// Call codec_proc if provided
	if (codec_proc != NULL) {
		ERR(codec_proc(pCodecCtx));
	}

	// Set a few properties
	pDecoder->decoder.pacInstance = pacInstance;
	pDecoder->decoder.type = AC_DECODER_TYPE_AUDIO;
	pDecoder->decoder.stream_index = nb;
	pDecoder->decoder.stream_info = *info;
	pDecoder->pCodecCtx = pCodecCtx;

	// Find correspondenting codec
	ERR(pDecoder->pCodec = avcodec_find_decoder(pCodecCtx->codec_id));

	// Open codec
	AV_ERR(avcodec_open2(pCodecCtx, pDecoder->pCodec, NULL));

	// Initialize the buffers
	pDecoder->decoder.pBuffer = NULL;  // av_malloc(AUDIO_BUFFER_BASE_SIZE);
	pDecoder->decoder.buffer_size = 0;
	ERR(pDecoder->pFrame = av_frame_alloc());

	// Fetch audio format, rate and channel layout -- under some circumstances,
	// the layout is not known to the decoder, then a channel layout is guessed
	// from the channel count.
	const enum AVSampleFormat fmt = pCodecCtx->sample_fmt;
	const int rate = pCodecCtx->sample_rate;
	const int64_t layout =
	    pCodecCtx->channel_layout
	        ? pCodecCtx->channel_layout
	        : av_get_default_channel_layout(pCodecCtx->channels);

#if 1
	// Always output AV_CH_LAYOUT_STEREO, AV_SAMPLE_FMT_S16, maximum 48kHz
	if (layout != AV_CH_LAYOUT_STEREO || fmt != AV_SAMPLE_FMT_S16 || rate > 44100) {
		ERR(pDecoder->pSwrCtx = swr_alloc_set_opts(NULL, AV_CH_LAYOUT_STEREO, AV_SAMPLE_FMT_S16,
		                                           44100, // ahi sucks so may as well convert ourselves to a common format
		                                           layout, fmt, rate, 0, NULL));
		AV_ERR(swr_init(pDecoder->pSwrCtx));
	}
#else
	// Initialize libswresample if needed
	if (av_sample_fmt_is_planar(fmt) || (fmt == AV_SAMPLE_FMT_S32) ||
	    (fmt == AV_SAMPLE_FMT_DBL)) {
		enum AVSampleFormat out_fmt = av_get_packed_sample_fmt(fmt);
		if (out_fmt == AV_SAMPLE_FMT_DBL || out_fmt == AV_SAMPLE_FMT_S32) {
			out_fmt = AV_SAMPLE_FMT_FLT;
		}
		ERR(pDecoder->pSwrCtx = swr_alloc_set_opts(NULL, layout, out_fmt, rate,
		                                           layout, fmt, rate, 0, NULL));
		AV_ERR(swr_init(pDecoder->pSwrCtx));
	}
#endif
	return (void *)pDecoder;

error:
	ac_free_audio_decoder(pDecoder);
	return NULL;
}

lp_ac_decoder CALL_CONVT ac_create_decoder(lp_ac_instance pacInstance, int nb) {
	return ac_create_decoder_ex(pacInstance, nb, NULL);
}

lp_ac_decoder CALL_CONVT ac_create_decoder_ex(lp_ac_instance pacInstance, int nb,
                                              ac_codecctx_callback codec_proc) {
	// Get information about the chosen data stream and create an decoder that
	// can handle this kind of stream.
	ac_stream_info info;
	ac_get_stream_info(pacInstance, nb, &info);

	lp_ac_decoder_data result = NULL;

	if (info.stream_type == AC_STREAM_TYPE_VIDEO) {
		result =
		    (lp_ac_decoder_data)ac_create_video_decoder(pacInstance, &info, nb,
		                                                codec_proc);
	} else if (info.stream_type == AC_STREAM_TYPE_AUDIO) {
		result =
		    (lp_ac_decoder_data)ac_create_audio_decoder(pacInstance, &info, nb,
		                                                codec_proc);
	}

	if (result) {
		result->decoder.timecode = 0;
		result->last_timecode = 0;
		result->sought = 1;
		result->doseek = 1;
	}

	return (lp_ac_decoder)result;
}

const char *ac_codec_name(lp_ac_instance pacInstance, int nb) {
	lp_ac_data self = ((lp_ac_data)(pacInstance));
	AVCodecContext *pCodecCtx = self->pFormatCtx->streams[nb]->codec;
	return avcodec_get_name(pCodecCtx->codec_id);
}

static int ac_decode_video_package(lp_ac_package pPackage,
                                   lp_ac_video_decoder pDecoder) {
	lp_ac_package_data pkt = ((lp_ac_package_data)pPackage);

#if LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(57,64,0)
	AV_ERR(avcodec_send_packet(pDecoder->pCodecCtx, pkt->pPack));

	AV_ERR(avcodec_receive_frame(pDecoder->pCodecCtx, pDecoder->pFrame));
#else
	int finished;

	AV_ERR(avcodec_decode_video2(pDecoder->pCodecCtx, pDecoder->pFrame, &finished,
	                             pkt->pPack));
	ERR(finished);
#endif /* LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(57,64,0) */

	ERR(pDecoder->pSwsCtx = sws_getCachedContext(
	    pDecoder->pSwsCtx, pDecoder->pCodecCtx->width,
	    pDecoder->pCodecCtx->height, pDecoder->pCodecCtx->pix_fmt,
	    pDecoder->pCodecCtx->width, pDecoder->pCodecCtx->height,
	    convert_pix_format(pDecoder->decoder.pacInstance->output_format),
	    /*SWS_BICUBIC*/SWS_FAST_BILINEAR, NULL, NULL, NULL));

	AV_ERR(sws_scale(pDecoder->pSwsCtx,
	                 (const uint8_t *const *)(pDecoder->pFrame->data),
	                 pDecoder->pFrame->linesize,
	                 0,  //?
	                 pDecoder->pCodecCtx->height, pDecoder->pFrameRGB->data,
	                 pDecoder->pFrameRGB->linesize));
	return 1;

error:
	return 0;
}

static int ac_decode_audio_package(lp_ac_package pPackage,
                                   lp_ac_audio_decoder pDecoder) {
	lp_ac_package_data pkt = ((lp_ac_package_data)pPackage);

#if LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(57,64,0)
	AV_ERR(avcodec_send_packet(pDecoder->pCodecCtx, pkt->pPack));

	AV_ERR(avcodec_receive_frame(pDecoder->pCodecCtx, pDecoder->pFrame));
#else
	int got_frame = 0;
	int len = 0;

	AV_ERR(len = avcodec_decode_audio4(pDecoder->pCodecCtx, pDecoder->pFrame,
	                                   &got_frame, pkt->pPack));
#endif /* LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(57,64,0) */


	// Calculate the output buffer size
#if 1
	// Always output AV_CH_LAYOUT_STEREO, AV_SAMPLE_FMT_S16
	const int sample_size = 2; // AV_SAMPLE_FMT_S16 => 2 bytes per sample
	const int sample_count = pDecoder->pFrame->nb_samples;
	const int channel_count = 2; // AV_CH_LAYOUT_STEREO => 2 channels
	size_t buffer_size = sample_size * sample_count * channel_count;

	if (pDecoder->pSwrCtx) {
		// Only realloc if the buffer is too small
		int src_rate = pDecoder->decoder.stream_info.additional_info.audio_info.samples_per_second;
		int dst_nb_samples = av_rescale_rnd(swr_get_delay(pDecoder->pSwrCtx, src_rate) +
			sample_count, 44100, src_rate, AV_ROUND_UP);

		buffer_size = sample_size * dst_nb_samples * channel_count;

		if (pDecoder->own_buffer_size < buffer_size) {
			void *newbuffer;
			ERR(newbuffer = av_realloc(pDecoder->decoder.pBuffer, buffer_size));
			pDecoder->decoder.pBuffer = newbuffer;
			pDecoder->own_buffer_size = buffer_size;
		}
		int rc = swr_convert(pDecoder->pSwrCtx, &(pDecoder->decoder.pBuffer),
		                     dst_nb_samples, (const uint8_t **)(pDecoder->pFrame->data),
		                     sample_count);
		AV_ERR(rc);
		pDecoder->decoder.buffer_size = av_samples_get_buffer_size(&buffer_size, 2, rc, AV_SAMPLE_FMT_S16, 1);
	} else {
		// No conversion needs to be done, simply set the buffer pointer
		pDecoder->decoder.pBuffer = pDecoder->pFrame->data[0];
		pDecoder->decoder.buffer_size = buffer_size;
	}
#else
	const int sample_size =
	    MIN(4, av_get_bytes_per_sample(pDecoder->pCodecCtx->sample_fmt));
	const int sample_count = pDecoder->pFrame->nb_samples;
	const int channel_count = pDecoder->pFrame->channels;
	const int buffer_size = sample_size * sample_count * channel_count;
	pDecoder->decoder.buffer_size = buffer_size;

	if (pDecoder->pSwrCtx) {
		if (pDecoder->own_buffer_size != buffer_size) {
			void *newbuffer;
			ERR(newbuffer = av_realloc(pDecoder->decoder.pBuffer, buffer_size));
			pDecoder->decoder.pBuffer = newbuffer;
			pDecoder->own_buffer_size = buffer_size;
		}
		AV_ERR(swr_convert(pDecoder->pSwrCtx, &(pDecoder->decoder.pBuffer),
		                   sample_count, (const uint8_t **)(pDecoder->pFrame->data),
		                   sample_count));
	} else {
		// No conversion needs to be done, simply set the buffer pointer
		pDecoder->decoder.pBuffer = pDecoder->pFrame->data[0];
	}
#endif

#if LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(57,64,0)
	return 1;
#else
	return got_frame;
#endif /* LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(57,64,0) */

error:
	return 0;
}

static int ac_decode_audio_package_ex(lp_ac_package pPackage,
                                   lp_ac_audio_decoder pDecoder, lp_ac_decoder_frame pFrame) {
	lp_ac_package_data pkt = ((lp_ac_package_data)pPackage);

	if (!pFrame)
		return 0;

	struct _ac_decoder_frame_internal *frame = (struct _ac_decoder_frame_internal *)pFrame;

#if LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(57,64,0)
	AV_ERR(avcodec_send_packet(pDecoder->pCodecCtx, pkt->pPack));

	AV_ERR(avcodec_receive_frame(pDecoder->pCodecCtx, frame->pFrame));
	frame->needs_unref = 1;
#else
	int got_frame = 0;
	int len = 0;

	AV_ERR(len = avcodec_decode_audio4(pDecoder->pCodecCtx, frame->pFrame,
	                                   &got_frame, pkt->pPack));
	
	if (got_frame == 0)
		return 0;
#endif /* LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(57,64,0) */


	// Calculate the output buffer size
#if 1
	// Always output AV_CH_LAYOUT_STEREO, AV_SAMPLE_FMT_S16
	const int sample_size = 2; // AV_SAMPLE_FMT_S16 => 2 bytes per sample
	const int sample_count = frame->pFrame->nb_samples;
	const int channel_count = 2; // AV_CH_LAYOUT_STEREO => 2 channels
	size_t buffer_size = sample_size * sample_count * channel_count;

	if (pDecoder->pSwrCtx) {
		int src_rate = pDecoder->decoder.stream_info.additional_info.audio_info.samples_per_second;
		int dst_nb_samples = av_rescale_rnd(swr_get_delay(pDecoder->pSwrCtx, src_rate) +
			sample_count, 44100, src_rate, AV_ROUND_UP);

		buffer_size = sample_size * dst_nb_samples * channel_count;
		// Only realloc if the buffer is too small
		if (frame->own_buffer_size < buffer_size) {
			ERR(frame->frame.pBuffer = av_realloc(frame->frame.pBuffer, buffer_size));
			frame->own_buffer_size = buffer_size;
		}
		int rc = swr_convert(pDecoder->pSwrCtx, &(frame->frame.pBuffer),
		                     dst_nb_samples, (const uint8_t **)(frame->pFrame->data),
		                     sample_count);
		AV_ERR(rc);
		frame->frame.buffer_size = av_samples_get_buffer_size(&buffer_size, 2, rc, AV_SAMPLE_FMT_S16, 1);
	} else {
		// No conversion needs to be done, simply set the buffer pointer
		frame->frame.pBuffer = frame->pFrame->data[0];
		frame->frame.buffer_size = buffer_size;
	}
#else
	const int sample_size =
	    MIN(4, av_get_bytes_per_sample(pDecoder->pCodecCtx->sample_fmt));
	const int sample_count = frame->pFrame->nb_samples;
	const int channel_count = frame->pFrame->channels;
	const int buffer_size = sample_size * sample_count * channel_count;
	frame->frame.buffer_size = buffer_size;

	if (pDecoder->pSwrCtx) {
		if (frame->own_buffer_size != buffer_size) {
			ERR(frame->frame.pBuffer = av_realloc(frame->frame.pBuffer, buffer_size));
			frame->own_buffer_size = buffer_size;
		}
		AV_ERR(swr_convert(pDecoder->pSwrCtx, &(frame->frame.pBuffer),
		                   sample_count, (const uint8_t **)(frame->pFrame->data),
		                   sample_count));
	} else {
		// No conversion needs to be done, simply set the buffer pointer
		frame->frame.pBuffer = frame->pFrame->data[0];
	}
#endif

	return 1;

error:
	return 0;
}

lp_ac_decoder_frame ac_alloc_decoder_frame(lp_ac_decoder decoder)
{
	struct _ac_decoder_frame_internal *frame = NULL;

	ERR(frame = av_malloc(sizeof(struct _ac_decoder_frame_internal)));
	ERR(frame->pFrame = av_frame_alloc());
	frame->own_buffer_size = 0;
	frame->frame.pBuffer = NULL;
	frame->needs_unref = 0;

	return (lp_ac_decoder_frame)frame;

error:
	if (frame)
		av_free(frame);

	return NULL;
}

void ac_free_decoder_frame(lp_ac_decoder_frame pFrame)
{
	if (!pFrame)
		return;

	struct _ac_decoder_frame_internal *frame = (struct _ac_decoder_frame_internal *)pFrame;
	if (frame->needs_unref)
		av_frame_unref(frame->pFrame);
	av_frame_free(&frame->pFrame);
	if (frame->own_buffer_size > 0)
		av_free(frame->frame.pBuffer);
}

int CALL_CONVT ac_decode_package_ex(lp_ac_package pPackage, lp_ac_decoder pDecoder, lp_ac_decoder_frame pFrame)
{
	double timebase = av_q2d(((lp_ac_data)pDecoder->pacInstance)
	                             ->pFormatCtx->streams[pPackage->stream_index]
	                             ->time_base);

	// Create a valid timecode
	if (((lp_ac_package_data)pPackage)->pts > 0) {
		lp_ac_decoder_data dec_dat = (lp_ac_decoder_data)pDecoder;

		dec_dat->last_timecode = pDecoder->timecode;
		pDecoder->timecode = ((lp_ac_package_data)pPackage)->pts * timebase;

		if (dec_dat->doseek)
		{
			double delta = pDecoder->timecode - dec_dat->last_timecode;
			double max_delta, min_delta;

			if (dec_dat->sought > 0) {
				max_delta = 120.0;
				min_delta = -120.0;
				--dec_dat->sought;
			} else {
				max_delta = 4.0;
				min_delta = 0.0;
			}

			if ((delta < min_delta) || (delta > max_delta)) {
				pDecoder->timecode = dec_dat->last_timecode;
				if (dec_dat->sought > 0) {
					++dec_dat->sought;
				}
			}
		}
	}
	
	if (pFrame)
		pFrame->timecode = pDecoder->timecode;

	if (pDecoder->type == AC_DECODER_TYPE_VIDEO) {
		return ac_decode_video_package(pPackage, (lp_ac_video_decoder)pDecoder);
	} else if (pDecoder->type == AC_DECODER_TYPE_AUDIO) {
		return pFrame ? ac_decode_audio_package_ex(pPackage, (lp_ac_audio_decoder)pDecoder, pFrame) : ac_decode_audio_package(pPackage, (lp_ac_audio_decoder)pDecoder);
	}
	return 0;
}

int CALL_CONVT ac_decode_package(lp_ac_package pPackage,
                                 lp_ac_decoder pDecoder) {
	return ac_decode_package_ex(pPackage, pDecoder, NULL);
}

ac_receive_frame_rc ac_receive_frame(lp_ac_decoder pDecoder, lp_ac_decoder_frame pFrame)
{
	if (!pDecoder || !pFrame)
		return RECEIVE_FRAME_ERROR;

	AVCodecContext *pCodecCtx = NULL;

	if (pDecoder->type == AC_DECODER_TYPE_VIDEO) {
		pCodecCtx = ((lp_ac_video_decoder)pDecoder)->pCodecCtx;
	} else if (pDecoder->type == AC_DECODER_TYPE_AUDIO) {
		pCodecCtx = ((lp_ac_audio_decoder)pDecoder)->pCodecCtx;
	}
	
	if (!pCodecCtx)
		return RECEIVE_FRAME_ERROR;
	
	struct _ac_decoder_frame_internal *frame = (struct _ac_decoder_frame_internal *)pFrame;
	int rc = avcodec_receive_frame(pCodecCtx, frame->pFrame);
	
	switch (rc)
	{
	case 0:
		pFrame->timecode = pDecoder->timecode; // is this correct?
		frame->needs_unref = 1;
		
		if (pDecoder->type == AC_DECODER_TYPE_AUDIO) {
			lp_ac_audio_decoder aDecoder = (lp_ac_audio_decoder)pDecoder;
			// Always output AV_CH_LAYOUT_STEREO, AV_SAMPLE_FMT_S16
			const int sample_size = 2; // AV_SAMPLE_FMT_S16 => 2 bytes per sample
			const int sample_count = frame->pFrame->nb_samples;
			const int channel_count = 2; // AV_CH_LAYOUT_STEREO => 2 channels
			size_t buffer_size = sample_size * sample_count * channel_count;

			if (aDecoder->pSwrCtx) {
				int src_rate = aDecoder->decoder.stream_info.additional_info.audio_info.samples_per_second;
				int dst_nb_samples = av_rescale_rnd(swr_get_delay(aDecoder->pSwrCtx, src_rate) +
					sample_count, 44100, src_rate, AV_ROUND_UP);

				buffer_size = sample_size * dst_nb_samples * channel_count;
				// Only realloc if the buffer is too small
				if (frame->own_buffer_size < buffer_size) {
					if (NULL == (frame->frame.pBuffer = av_realloc(frame->frame.pBuffer, buffer_size)))
						return RECEIVE_FRAME_ERROR;
					frame->own_buffer_size = buffer_size;
				}
				int rc = swr_convert(aDecoder->pSwrCtx, &(frame->frame.pBuffer),
									 dst_nb_samples, (const uint8_t **)(frame->pFrame->data),
									 sample_count);
				if (rc < 0)
					return RECEIVE_FRAME_ERROR;
				frame->frame.buffer_size = av_samples_get_buffer_size(&buffer_size, 2, rc, AV_SAMPLE_FMT_S16, 1);
			} else {
				// No conversion needs to be done, simply set the buffer pointer
				frame->frame.pBuffer = frame->pFrame->data[0];
				frame->frame.buffer_size = buffer_size;
			}
		}
		else if (pDecoder->type == AC_DECODER_TYPE_VIDEO) {
#if 0
			lp_ac_video_decoder vDecoder = (lp_ac_video_decoder)pDecoder;

			if (NULL == (vDecoder->pSwsCtx = sws_getCachedContext(
				vDecoder->pSwsCtx, vDecoder->pCodecCtx->width,
				vDecoder->pCodecCtx->height, vDecoder->pCodecCtx->pix_fmt,
				vDecoder->pCodecCtx->width, vDecoder->pCodecCtx->height,
				convert_pix_format(vDecoder->decoder.pacInstance->output_format),
				/*SWS_BICUBIC*/SWS_FAST_BILINEAR, NULL, NULL, NULL)))
			{
				return RECEIVE_FRAME_ERROR;
			}

			if (sws_scale(vDecoder->pSwsCtx,
				 (const uint8_t *const *)(vDecoder->pFrame->data),
				 vDecoder->pFrame->linesize,
				 0,  //?
				 vDecoder->pCodecCtx->height, vDecoder->pFrameRGB->data,
				 vDecoder->pFrameRGB->linesize) < 0)
			{
				return RECEIVE_FRAME_ERROR;
			}
#endif
		}
		
		return RECEIVE_FRAME_SUCCESS;

	case AVERROR(EAGAIN): return RECEIVE_FRAME_NEED_PACKET;
	case AVERROR_EOF: return RECEIVE_FRAME_EOF;
	default: return RECEIVE_FRAME_ERROR;
	}
}

ac_push_package_rc ac_push_package(lp_ac_decoder pDecoder, lp_ac_package pPackage)
{
	lp_ac_package_data pkt = ((lp_ac_package_data)pPackage);
	
	if (!pDecoder || !pPackage)
		return PUSH_PACKAGE_ERROR;

	AVCodecContext *pCodecCtx = NULL;

	if (pDecoder->type == AC_DECODER_TYPE_VIDEO) {
		pCodecCtx = ((lp_ac_video_decoder)pDecoder)->pCodecCtx;
	} else if (pDecoder->type == AC_DECODER_TYPE_AUDIO) {
		pCodecCtx = ((lp_ac_audio_decoder)pDecoder)->pCodecCtx;
	}
	
	if (!pCodecCtx)
		return PUSH_PACKAGE_ERROR;

	double timebase = av_q2d(((lp_ac_data)pDecoder->pacInstance)
	                             ->pFormatCtx->streams[pPackage->stream_index]
	                             ->time_base);
	int rc = avcodec_send_packet(pCodecCtx, pkt->pPack);
	switch (rc)
	{
	case 0:
		// Create a valid timecode
		if (((lp_ac_package_data)pPackage)->pts > 0) {
			lp_ac_decoder_data dec_dat = (lp_ac_decoder_data)pDecoder;

			dec_dat->last_timecode = pDecoder->timecode;
			pDecoder->timecode = ((lp_ac_package_data)pPackage)->pts * timebase;

			if (dec_dat->doseek)
			{
				double delta = pDecoder->timecode - dec_dat->last_timecode;
				double max_delta, min_delta;

				if (dec_dat->sought > 0) {
					max_delta = 120.0;
					min_delta = -120.0;
					--dec_dat->sought;
				} else {
					max_delta = 4.0;
					min_delta = 0.0;
				}

				if ((delta < min_delta) || (delta > max_delta)) {
					pDecoder->timecode = dec_dat->last_timecode;
					if (dec_dat->sought > 0) {
						++dec_dat->sought;
					}
				}
			}
		}
		return PUSH_PACKAGE_SUCCESS;
	case AVERROR(EAGAIN): return PUSH_PACKAGE_NEED_RECEIVE;
	case AVERROR_EOF: return PUSH_PACKAGE_SUCCESS; // happens after flush, double-flush so let's ignore this
	default: return PUSH_PACKAGE_ERROR;
	}
}

void ac_decoder_fake_seek(lp_ac_decoder pDecoder)
{
	lp_ac_decoder_data dec_dat = (lp_ac_decoder_data)pDecoder;
	dec_dat->doseek = 0;
}


// Seek function
int CALL_CONVT ac_seek(lp_ac_decoder pDecoder, int dir, int64_t target_pos) {
	AVRational timebase = ((lp_ac_data)pDecoder->pacInstance)
	                          ->pFormatCtx->streams[pDecoder->stream_index]
	                          ->time_base;

	int flags = dir < 0 ? AVSEEK_FLAG_BACKWARD : 0;

	int64_t pos = av_rescale(target_pos, AV_TIME_BASE, 1000);

	((lp_ac_decoder_data)pDecoder)->sought = 100;
	pDecoder->timecode = target_pos / 1000;

	AV_ERR(av_seek_frame(((lp_ac_data)pDecoder->pacInstance)->pFormatCtx,
	                     pDecoder->stream_index,
	                     av_rescale_q(pos, AV_TIME_BASE_Q, timebase),
	                     flags));
	return 1;

error:
	return 0;
}

// Free video decoder
static void ac_free_video_decoder(lp_ac_video_decoder pDecoder) {
	if (pDecoder) {
		av_frame_free(&(pDecoder->pFrame));
		av_frame_free(&(pDecoder->pFrameRGB));
		sws_freeContext(pDecoder->pSwsCtx);
#if LIBAVCODEC_VERSION_MAJOR >= 57
		avcodec_close(pDecoder->pCodecCtx);
		av_free(pDecoder->pCodecCtx);
#endif /* LIBAVCODEC_VERSION_MAJOR >= 57 */
		av_free(pDecoder->decoder.pBuffer);
		av_free(pDecoder);
	}
}

// Free video decoder
static void ac_free_audio_decoder(lp_ac_audio_decoder pDecoder) {
	if (pDecoder) {
#if LIBAVCODEC_VERSION_MAJOR >= 57
		avcodec_close(pDecoder->pCodecCtx);
		av_free(pDecoder->pCodecCtx);
#endif /* LIBAVCODEC_VERSION_MAJOR >= 57 */
		av_frame_free(&(pDecoder->pFrame));
		if (pDecoder->pSwrCtx) {
			swr_free(&(pDecoder->pSwrCtx));
		}
		if (pDecoder->own_buffer_size > 0) {
			av_free(pDecoder->decoder.pBuffer);
		}
		av_free(pDecoder);
	}
}

void CALL_CONVT ac_free_decoder(lp_ac_decoder pDecoder) {
	if (pDecoder) {
		if (pDecoder->type == AC_DECODER_TYPE_VIDEO) {
			ac_free_video_decoder((lp_ac_video_decoder)pDecoder);
		} else if (pDecoder->type == AC_DECODER_TYPE_AUDIO) {
			ac_free_audio_decoder((lp_ac_audio_decoder)pDecoder);
		}
	}
}

// Additional support functions

double ac_get_stream_duration(lp_ac_instance pacInstance, int nb) {

	if (pacInstance->opened) {
		int64_t duration = ((lp_ac_data)pacInstance)->pFormatCtx->streams[nb]->duration;
		if(duration != AV_NOPTS_VALUE) {
			AVRational tb = ((lp_ac_data)pacInstance)->pFormatCtx->streams[nb]->time_base;
			double d = (double)duration * av_q2d(tb);
			return d;
		}
	}

	return 0.0;
}

int CALL_CONVT ac_get_package_size(lp_ac_package pPackage) {
	lp_ac_package_data self = (lp_ac_package_data)pPackage;
	if (pPackage == ac_flush_packet())
		return 0;
	return self->pPack->size;
}

EXTERN char CALL_CONVT ac_get_package_keyframe(lp_ac_package pPackage)
{
	lp_ac_package_data self = (lp_ac_package_data)pPackage;
	if (pPackage == ac_flush_packet())
		return 0;
	return (self->pPack->flags & AV_PKT_FLAG_KEY) ? 1 : 0;
}

double CALL_CONVT ac_get_package_pts(lp_ac_instance pacInstance, lp_ac_package pPackage) {
	lp_ac_package_data self = (lp_ac_package_data)pPackage;
	if (pPackage == ac_flush_packet())
		return 0.0;
	AVRational tb = ((lp_ac_data)pacInstance)->pFormatCtx->streams[self->pPack->stream_index]->time_base;
	return ((double)self->pPack->pts) / tb.den;
}

double CALL_CONVT ac_get_package_dts(lp_ac_instance pacInstance, lp_ac_package pPackage) {
	lp_ac_package_data self = (lp_ac_package_data)pPackage;
	if (pPackage == ac_flush_packet())
		return 0.0;

	AVRational tb = ((lp_ac_data)pacInstance)->pFormatCtx->streams[self->pPack->stream_index]->time_base;
	return ((double)self->pPack->dts) / tb.den;
}

double CALL_CONVT ac_get_package_duration(lp_ac_instance pacInstance, lp_ac_package pPackage) {
	lp_ac_package_data self = (lp_ac_package_data)pPackage;
	if (pPackage == ac_flush_packet())
		return 0.0;

	AVRational tb = ((lp_ac_data)pacInstance)->pFormatCtx->streams[self->pPack->stream_index]->time_base;
	return ((double)self->pPack->duration) / tb.den;
}

static const ac_package_data flush_pkt = {{0}, NULL, 0};

lp_ac_package CALL_CONVT ac_flush_packet(void) {
	return (lp_ac_package) &flush_pkt;
}

void CALL_CONVT ac_flush_buffers(lp_ac_decoder pDecoder) {

	if (NULL == pDecoder)
		return;

	AVCodecContext *pCodecCtx = NULL;

	if (pDecoder->type == AC_DECODER_TYPE_VIDEO) {
		pCodecCtx = ((lp_ac_video_decoder)pDecoder)->pCodecCtx;
	} else if (pDecoder->type == AC_DECODER_TYPE_AUDIO) {
		pCodecCtx = ((lp_ac_audio_decoder)pDecoder)->pCodecCtx;
	}
	
	if (pCodecCtx)
		avcodec_flush_buffers(pCodecCtx);
}


AVFrame * CALL_CONVT ac_get_frame(lp_ac_decoder decoder) {
	ERR(decoder);

	ERR(decoder->type == AC_DECODER_TYPE_VIDEO);

	lp_ac_video_decoder pDecoder = (lp_ac_video_decoder) decoder;
	return pDecoder->pFrameRGB;

error:
	return NULL;
}

AVFrame * CALL_CONVT ac_get_frame_real(lp_ac_decoder_frame pFrame)
{
	if (pFrame)
	{
		struct _ac_decoder_frame_internal *frame = (struct _ac_decoder_frame_internal *)pFrame;
		return frame->pFrame;
	}
	
	return NULL;
}

int CALL_CONVT ac_set_output_format(lp_ac_decoder decoder, ac_output_format fmt) {
        AVFrame *pFrameRGB = NULL;
        uint8_t *pBuffer = NULL;
        int buffer_size;

	ERR(decoder);

	ERR(decoder->type == AC_DECODER_TYPE_VIDEO);

	lp_ac_video_decoder pDecoder = (lp_ac_video_decoder) decoder;

	if (!pDecoder->pCodecCtx->width || !pDecoder->pCodecCtx->height) {
		// Invalid video size
		goto error;
	}

	if (fmt == pDecoder->decoder.pacInstance->output_format) {
		// Nothing to do, already using that format
		return 1;
	}

	// allocate new resources

	ERR(pFrameRGB = av_frame_alloc());

	enum AVPixelFormat pix_fmt_out = convert_pix_format(fmt);
	buffer_size =
	    av_image_get_buffer_size(pix_fmt_out, pDecoder->pCodecCtx->width,
	                             pDecoder->pCodecCtx->height, 1);
	ERR(pBuffer = (uint8_t *)av_malloc(buffer_size));

	memset(pBuffer, 0, buffer_size);

	AV_ERR(av_image_fill_arrays(pFrameRGB->data, pFrameRGB->linesize,
	                            pBuffer, pix_fmt_out,
	                            pDecoder->pCodecCtx->width,
	                            pDecoder->pCodecCtx->height, 1));

	// free old resources

	if (pDecoder->pFrameRGB) {
		av_frame_free(&(pDecoder->pFrameRGB));
	}
	if (pDecoder->pSwsCtx) {
		sws_freeContext(pDecoder->pSwsCtx);
		pDecoder->pSwsCtx = NULL;
	}
	if (pDecoder->decoder.pBuffer) {
		av_free(pDecoder->decoder.pBuffer);
	}

	// swap in new resources

	pDecoder->pFrameRGB = pFrameRGB;
	pDecoder->decoder.buffer_size = buffer_size;
	pDecoder->decoder.pBuffer = pBuffer;
	pDecoder->decoder.pacInstance->output_format = fmt;

	return 1;

error:
	av_frame_free(&pFrameRGB);
	av_free(pBuffer);

	return 0;
}

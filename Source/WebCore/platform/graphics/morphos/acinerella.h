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

#ifndef _ACINERELLA_H_
#define _ACINERELLA_H_

#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>

#define CALL_CONVT
#define EXTERN

#ifdef __cplusplus
extern "C" {
#endif

#define AC_BUFSIZE (1024 * 4)

#ifndef INT64_MAX
#define INT64_MAX 9223372036854775807LL
#endif
#ifndef INT64_MIN
#define INT64_MIN (-INT64_MAX-1)
#endif
#ifndef UINT64_MAX
#define UINT64_MAX 18446744073709551615ULL
#endif

/**
 * Defines the type of an Acinerella media stream. Currently only video and
 * audio streams are supported, subtitle and data streams will be marked as
 * "unknown".
 */
typedef enum _ac_stream_type {
	/**
	 * The type of the media stream is not known. This kind of stream can not be
	 * decoded.
	 */
	AC_STREAM_TYPE_VIDEO = 0,

	/**
	 * This media stream is a video stream.
	 */
	AC_STREAM_TYPE_AUDIO = 1,

	/**
	 * This media stream is an audio stream.
	 */
	AC_STREAM_TYPE_UNKNOWN = -1
} ac_stream_type;

/**
 * Defines the type of an Acinerella media decoder.
 */
typedef enum _ac_decoder_type {
	/**
	 * This decoder is used to decode a video stream.
	 */
	AC_DECODER_TYPE_VIDEO = 0,

	/**
	 * This decoder is used to decode an audio stram.
	 */
	AC_DECODER_TYPE_AUDIO = 1
} ac_decoder_type;

/**
 * Defines the format video/image data is returned in.
 */
typedef enum _ac_output_format {
	AC_OUTPUT_RGB24 = 0,
	AC_OUTPUT_BGR24 = 1,
	AC_OUTPUT_RGBA32 = 2,
	AC_OUTPUT_BGRA32 = 3,
	AC_OUTPUT_YUV422 = 4,
	AC_OUTPUT_YUV420P = 5,
	AC_OUTPUT_ARGB32 = 6,
} ac_output_format;

typedef enum _ac_audio_output_format {
	AC_AUDIO_OUTPUT_16_2,
	AC_AUDIO_OUTPUT_FLOAT_1,
	AC_AUDIO_OUTPUT_FLOAT_2,
} ac_audio_output_format;

/*Contains information about the whole file/stream that has been opened. Default
values are ""
for strings and -1 for integer values.*/
typedef struct _ac_file_info {
	/**
	 * Title of the media file
	 */
	char title[512];

	/**
	 * Author or artist of the media file.
	 */
	char author[512];

	/**
	 * Copyright information.
	 */
	char copyright[512];

	/**
	 * Comment stored in the file.
	 */
	char comment[512];

	/**
	 * Album the media file is from.
	 */
	char album[512];

	/**
	 * Year in which the media file was created.
	 */
	int year;

	/**
	 * Track number.
	 */
	int track;

	/**
	 * Genre of the media file.
	 */
	char genre[32];

	/**
	 * Length of the file in milliseconds.
	 */
	int64_t duration;

	/**
	 * Bitrate of the file.
	 */
	int bitrate;
} ac_file_info;

typedef ac_file_info *lp_ac_file_info;

/**
 * ac_instance represents an Acinerella instance. Each instance can open and
 * decode one file at once.
 */
typedef struct _ac_instance {
	/**
	 * If true, the instance currently opened a media file.
	 */
	bool opened;

	/**
	 * Contains the count of streams the media file has. This value is available
	 * after calling the ac_open function.
	 */
	int stream_count;

	/**
	 * Set this value to change the image output format
	 */
	ac_output_format output_format;

	ac_audio_output_format audio_output_format;
	int audio_rate;

	/**
	 * Contains information about the opened stream/file
	 */
	ac_file_info info;
} ac_instance;

/**
 * Pointer on the Acinerella instance record.
 */
typedef ac_instance *lp_ac_instance;

/**
 * Contains information about an Acinerella video stream.
 */
struct _ac_video_stream_info {
	/**
	 * The width of one frame.
	 */
	int frame_width;

	/**
	 * The height of one frame.
	 */
	int frame_height;

	/**
	 * The width of one pixel. 1.07 for 4:3 format, 1,42 for the 16:9 format
	 */
	float pixel_aspect;

	/**
	 * Frames per second that should be played.
	 */
	double frames_per_second;
};

/**
 * Contains information about an Acinerella audio stream.
 */
struct _ac_audio_stream_info {
	/**
	 * Samples per second. Default values are 44100 or 48000.
	 */
	int samples_per_second;

	/**
	 * Bits per sample. Can be 8, 16 or 32 Bit. In the latter case the data is
	 * in floating point format.
	 */
	int bit_depth;

	/**
	 * Count of channels in the audio stream.
	 */
	int channel_count;
};

/**
 * Additional info about the stream - use "video_info" when the stream
 * is an video stream, "audio_info" when the stream is an audio stream.
 */
union _ac_additional_stream_info {
	struct _ac_audio_stream_info audio_info;
	struct _ac_video_stream_info video_info;
};

/*Contains information about an Acinerella stream.*/
typedef struct _ac_stream_info {
	/**
	 * Contains the type of the stream.
	 */
	ac_stream_type stream_type;

	/**
	 * Additional data depending on the stream type. Either access the
	 * "audio_info" or the "video_info" field, depending on the value stored in
	 * the "stream_type" member.
	 */
	union _ac_additional_stream_info additional_info;
} ac_stream_info;

/**
 * Pointer on ac_stream_info
 */
typedef ac_stream_info *lp_ac_stream_info;

/**
 * Contains information about an Acinerella video/audio decoder.
 */
typedef struct _ac_decoder {
	/**
	 * Pointer on the Acinerella instance.
	 */
	lp_ac_instance pacInstance;

	/**
	 * Contains the type of the decoder.
	 */
	ac_decoder_type type;

	/**
	 * The timecode of the currently decoded picture in seconds.
	 */
	double timecode;

	/**
	 * Contains information about the stream the decoder is attached to.
	 */
	ac_stream_info stream_info;

	/**
	 * The index of the stream the decoder is attached to.
	 */
	int stream_index;

	/**
	 * Pointer to the buffer which contains the data.
	 */
	uint8_t *pBuffer;

	/**
	 * Size of the data in the buffer.
	 */
	int buffer_size;
} ac_decoder;

typedef ac_decoder *lp_ac_decoder;

/**
 * Contains information about an Acinerella package.
 */
typedef struct _ac_package {
	/*The stream the package belongs to.*/
	int stream_index;
} ac_package;

typedef ac_package *lp_ac_package;

typedef void *lp_ac_proberesult;

/**
 * Contains information about an Acinerella video/audio decoder.
 */
typedef struct _ac_decoder_frame {
	/**
	 * The timecode of the currently decoded picture in seconds.
	 */
	double timecode;
	
	/**
	 * Pointer to the buffer which contains the data.
	 */
	uint8_t *pBuffer;

	/**
	 * Size of the data in the buffer.
	 */
	int buffer_size;
} ac_decoder_frame;

typedef ac_decoder_frame *lp_ac_decoder_frame;

/**
 * Callback function used to ask the application to read data. Should return
 * the number of bytes read or an value smaller than zero if an error occured.
 */
typedef int (CALL_CONVT *ac_read_callback)(void *sender, uint8_t *buf,
                                           int size);

/**
 * Callback function used to ask the application to seek.
 *
 * @param sender is the pointer specified in ac_open.
 * @param pos is the position to which the stream should be seeked.
 * @param whence specifies whether the seeking should be relative to the
 * beginning of the file (0, SEEK_SET), relative to the current location (1,
 * SEEK_CUR), or relative to the end of the file (2, SEEK_END)
 * @return 0 if the function succeeds, -1 on failure.
 */
typedef int64_t (CALL_CONVT *ac_seek_callback)(void *sender, int64_t pos,
                                               int whence);

/**
 * Callback function that is used to notify the application when the data stream
 * is opened or closed. For example the file pointer should be resetted to zero
 * when the "open" function is called.
 */
typedef int (CALL_CONVT *ac_openclose_callback)(void *sender);

/**
 * Initializes an Acinerella instance.
 *
 * @return a pointer at an acinerella instance. The instance must be freed using
 * ac_free.
 */
EXTERN lp_ac_instance CALL_CONVT ac_init(void);

/**
 * Frees a previously created Acinerella instance.
 *
 * @param pacInstance is the Acinerella instance that should be freed.
 */
EXTERN void CALL_CONVT ac_free(lp_ac_instance pacInstance);

/**
 * Opens a media stream.
 *
 * @param inst specifies the Acinerella Instance the stream should be opened
 * for.
 * @param sender specifies a pointer that is sent to all callback functions to
 * allow you to do object orientated programming. May be NULL.
 * @param open_proc specifies the callback function that is called, when the
 * media file is opened. May be NULL.
 * @param seek_proc specifies the callback function that is called, when the
 * ffmpeg decoder wants to seek in the file. May be NULL.
 * @param close_proc specifies the callback function that is called when the
 * media file is closed. May be NULL.
 * @param proberesult is a pointer at a structure previously returned by
 * ac_probe_input_buffer. May be NULL.
 */
EXTERN int CALL_CONVT
    ac_open(lp_ac_instance pacInstance, void *sender,
            ac_openclose_callback open_proc, ac_read_callback read_proc,
            ac_seek_callback seek_proc, ac_openclose_callback close_proc,
            lp_ac_proberesult proberesult);

/**
 * Opens a media file.
 *
 * @param inst specifies the Acinerella Instance the stream should be opened
 * for.
 * @param file is the name of the file that should be opened.
 */
EXTERN int CALL_CONVT
    ac_open_file(lp_ac_instance pacInstance, const char *filename);

/**
 * Closes an opened media file.
 */
EXTERN void CALL_CONVT ac_close(lp_ac_instance pacInstance);

/**
 * Stores information in "pInfo" about stream number "nb".
 */
EXTERN void CALL_CONVT ac_get_stream_info(lp_ac_instance pacInstance, int nb,
                                          lp_ac_stream_info info);

/**
 * Reads a package from an opened media file.
 *
 * @param pacInstance is the Acinerella instance from which the data should be
 * read.
 * @return a pointer at a ac_package structure or NULL on failure. The returned
 * pointer must be freed with ac_free_package after use.
 */
EXTERN lp_ac_package CALL_CONVT ac_read_package(lp_ac_instance pacInstance);

/**
 * Frees a package that has been read.
 *
 * @param pPackage is a pointer at the ac_package that should be freed. May be
 * NULL in which case no operation is performed.
 */
EXTERN void CALL_CONVT ac_free_package(lp_ac_package pPackage);

/**
 * Creates an decoder for the specified stream number. Returns NULL if no
 * decoder could be found.
 */
EXTERN lp_ac_decoder CALL_CONVT
    ac_create_decoder(lp_ac_instance pacInstance, int nb);

EXTERN const char *ac_codec_name(lp_ac_instance pacInstance, int nb);

/**
 * Callback function used to configure the codec context (AVCodecContext *).
 * The return value 0 indicates failure, 1 indicates success.
 */
typedef int (CALL_CONVT *ac_codecctx_callback)(void *codecctx);

/**
 * Creates an decoder for the specified stream number. Returns NULL if no
 * decoder could be found or some other error occurred.
 *
 * @param codec_proc is a function that will get called with the
 * decoder's codec context (AVCodecContext *). May be NULL.
 */
EXTERN lp_ac_decoder CALL_CONVT
    ac_create_decoder_ex(lp_ac_instance pacInstance, int nb,
                         ac_codecctx_callback codec_proc);

/**
 * Frees an created decoder.
 */
EXTERN void CALL_CONVT ac_free_decoder(lp_ac_decoder pDecoder);

#if 0
/**
 * Decodes a package using the specified decoder. The decoded data is stored in
 * the "buffer" property of the decoder.
 *
 * @param pPackage is the package that should be decoded, previously read by the
 * ac_read_package method.
 * @param pDecoder is the decoder instance that should be used to decode the
 * package -- use the stream index stored in the package to determine the
 * correct decoder.
 */
EXTERN int CALL_CONVT
    ac_decode_package(lp_ac_package pPackage, lp_ac_decoder pDecoder);

EXTERN int CALL_CONVT
    ac_decode_package_ex(lp_ac_package pPackage, lp_ac_decoder pDecoder, lp_ac_decoder_frame pFrame);
#endif

typedef enum _ac_receive_frame_rc
{
	RECEIVE_FRAME_SUCCESS = 0,
	RECEIVE_FRAME_NEED_PACKET,
	RECEIVE_FRAME_EOF,
	RECEIVE_FRAME_ERROR,
} ac_receive_frame_rc;

ac_receive_frame_rc ac_receive_frame(lp_ac_decoder pDecoder, lp_ac_decoder_frame pFrame);

typedef enum _ac_push_package_rc
{
	PUSH_PACKAGE_SUCCESS,
	PUSH_PACKAGE_NEED_RECEIVE,
	PUSH_PACKAGE_ERROR
} ac_push_package_rc;

ac_push_package_rc ac_push_package(lp_ac_decoder pDecoder, lp_ac_package pPackage);

/**
 * Seeks to the given target position in the file. The seek funtion is not able
 * to seek a single audio/video stream but seeks the whole file forward. The
 * stream number paremter (nb) is only used for the timecode reference.
 * The parameter "dir" specifies the seek direction: 0 for forward, -1 for
 * backward. The target_pos paremeter is in milliseconds. Returns 1 if the
 * functions succeded.
 */
EXTERN int CALL_CONVT
    ac_seek(lp_ac_decoder pDecoder, int dir, int64_t target_pos);

/**
 * Checks whether the given input buffer contains data that can potentially be
 * decoded by Acinerella. If yes, returns a pointer that may be passed to
 * ac_open in order to speed up the open process. Otherwise returns NULL.
 *
 * @param buf is a pointer to the buffer in which the data is stored.
 * @param bufsize is the size of the buffer.
 * @param filename is the name of the file the buffer originated from. May be
 * empty or NULL.
 * @param score_max is set to the maximum of the previous value of score_max
 * and an internal score value.
 * @return either a pointer at an internal "probe result" or NULL if the buffer
 * contains no decodeable data.
 */
EXTERN lp_ac_proberesult CALL_CONVT
    ac_probe_input_buffer(uint8_t *buf, int bufsize, char *filename,
                          int *score_max);

/*
 * Additional support functions
 */
EXTERN double ac_get_stream_duration(lp_ac_instance pacInstance, int nb);
EXTERN int CALL_CONVT ac_get_package_size(lp_ac_package pPackage);
EXTERN char CALL_CONVT ac_get_package_keyframe(lp_ac_package pPackage);
EXTERN void CALL_CONVT ac_flush_buffers(lp_ac_decoder pDecoder);
EXTERN lp_ac_package CALL_CONVT ac_flush_packet(void);
EXTERN int CALL_CONVT ac_set_output_format(lp_ac_decoder pDecoder, ac_output_format fmt);
void ac_set_audio_output_format(lp_ac_instance pacInstance, ac_audio_output_format fmt, int rate);
struct AVFrame;
EXTERN AVFrame * CALL_CONVT ac_get_frame(lp_ac_decoder decoder);
EXTERN AVFrame * CALL_CONVT ac_get_frame_scaled(lp_ac_decoder decoder);
EXTERN AVFrame * CALL_CONVT ac_get_frame_real(lp_ac_decoder_frame pFrame);

EXTERN lp_ac_decoder_frame ac_alloc_decoder_frame(lp_ac_decoder decoder);
EXTERN void ac_free_decoder_frame(lp_ac_decoder_frame pFrame);

EXTERN void ac_scale_to_scaled_rgb_decoder_frame(lp_ac_decoder_frame pFrame, lp_ac_decoder pDecoder, int dst_width, int dst_height);

EXTERN int CALL_CONVT ac_get_audio_rate(lp_ac_decoder pDecoder);

EXTERN double ac_get_package_pts(lp_ac_instance pacInstance, lp_ac_package pPackage);
EXTERN double ac_get_package_dts(lp_ac_instance pacInstance, lp_ac_package pPackage);
EXTERN double ac_get_package_duration(lp_ac_instance pacInstance, lp_ac_package pPackage);

EXTERN void ac_decoder_fake_seek(lp_ac_decoder pDecoder);
EXTERN void ac_decoder_set_loopfilter(lp_ac_decoder pDecoder, int lflevel);
#ifdef __cplusplus
} //end extern "C"
#endif

#endif /* _ACINERELLA_H_ */


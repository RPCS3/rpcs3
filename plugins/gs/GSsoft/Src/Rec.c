/*  GSsoft
 *  Copyright (C) 2002-2005  GSsoft Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <stdlib.h>
#include <ffmpeg/avformat.h>

#include "GS.h"
#include "Draw.h"
#include "Rec.h"

AVFormatContext *output = NULL;
AVStream *stream;
AVFrame *picture;
AVFrame *yuv420p;
u8 *picture_buf;
int picture_size;
u8 *outbuf;
int outbuf_size;

#define av_register_all			_av_register_all
#define av_new_stream			_av_new_stream
#define av_set_parameters		_av_set_parameters
#define av_write_header			_av_write_header
#define av_write_frame			_av_write_frame
#define av_write_trailer		_av_write_trailer
#define avpicture_fill			_avpicture_fill
#define avpicture_get_size		_avpicture_get_size
#define img_convert				_img_convert
#define avcodec_alloc_frame		_avcodec_alloc_frame
#define avcodec_open			_avcodec_open
#define avcodec_find_encoder	_avcodec_find_encoder
#define avcodec_encode_video	_avcodec_encode_video
#define avcodec_close			_avcodec_close
#define av_mallocz				_av_mallocz
#define av_free					_av_free
#define __av_freep				___av_freep
#define url_fopen				_url_fopen
#define url_fclose				_url_fclose
#define first_oformat			_first_oformat

void *avcodeclib;
void *avformatlib;

#ifdef __WIN32__
#define AVCODECLIB		"avcodec.dll"
#define AVFORMATLIB		"avformat.dll"
#else
#define AVCODECLIB		"libavcodec.so"
#define AVFORMATLIB		"libavformat.so"
#endif

void (*av_register_all)(void);
AVStream *(*av_new_stream)(AVFormatContext *s, int id);
int (*av_set_parameters)(AVFormatContext *s, AVFormatParameters *ap);
int (*av_write_header)(AVFormatContext *s);
int (*av_write_frame)(AVFormatContext *s, int stream_index, const uint8_t *buf, 
                   int size);
int (*av_write_trailer)(AVFormatContext *s);
int (*avpicture_fill)(AVPicture *picture, uint8_t *ptr,
                   int pix_fmt, int width, int height);
int (*avpicture_get_size)(int pix_fmt, int width, int height);
int (*img_convert)(AVPicture *dst, int dst_pix_fmt,
                AVPicture *src, int pix_fmt, 
                int width, int height);
AVFrame *(*avcodec_alloc_frame)(void);
int (*avcodec_open)(AVCodecContext *avctx, AVCodec *codec);

AVCodec *(*avcodec_find_encoder)(enum CodecID id);
int (*avcodec_encode_video)(AVCodecContext *avctx, uint8_t *buf, int buf_size, 
                         const AVFrame *pict);
int (*avcodec_close)(AVCodecContext *avctx);
void *(*av_mallocz)(unsigned int size);
void (*av_free)(void *ptr);
void (*__av_freep)(void **ptr);
int (*url_fopen)(URLContext **h, const char *filename, int flags);
int (*url_fclose)(URLContext *h);
AVOutputFormat **first_oformat;

int recLoad() {
	avcodeclib  = SysLoadLibrary(AVCODECLIB);
	if (avcodeclib == NULL) {
		printf("Couldn't load %s: %s\n", AVCODECLIB, SysLibError()); 
		return -1;
	}
	avformatlib = SysLoadLibrary(AVFORMATLIB);
	if (avformatlib == NULL) {
		printf("Couldn't load %s: %s\n", AVFORMATLIB, SysLibError());
		return -1;
	}
	av_register_all			= SysLoadSym(avformatlib, "av_register_all");
	av_new_stream			= SysLoadSym(avformatlib, "av_new_stream");
	av_set_parameters		= SysLoadSym(avformatlib, "av_set_parameters");
	av_write_header			= SysLoadSym(avformatlib, "av_write_header");
	av_write_frame			= SysLoadSym(avformatlib, "av_write_frame");
	av_write_trailer		= SysLoadSym(avformatlib, "av_write_trailer");
	avpicture_fill			= SysLoadSym(avcodeclib, "avpicture_fill");
	avpicture_get_size  	= SysLoadSym(avcodeclib, "avpicture_get_size");
	img_convert				= SysLoadSym(avcodeclib, "img_convert");
	avcodec_alloc_frame		= SysLoadSym(avcodeclib, "avcodec_alloc_frame");
	avcodec_open			= SysLoadSym(avcodeclib, "avcodec_open");
	avcodec_find_encoder	= SysLoadSym(avcodeclib, "avcodec_find_encoder");
	avcodec_encode_video	= SysLoadSym(avcodeclib, "avcodec_encode_video");
	avcodec_close			= SysLoadSym(avcodeclib, "avcodec_close");
	av_mallocz				= SysLoadSym(avcodeclib, "av_mallocz");
	av_free					= SysLoadSym(avcodeclib, "av_free");
	__av_freep				= SysLoadSym(avcodeclib, "__av_freep");
	url_fopen				= SysLoadSym(avformatlib, "url_fopen");
	url_fclose				= SysLoadSym(avformatlib, "url_fclose");
	first_oformat			= SysLoadSym(avformatlib, "first_oformat");

	return 0;
}

void recUnload() {
	SysLoadLibrary(avcodeclib);
	SysLoadLibrary(avformatlib);
}

void recOpen() {
	AVOutputFormat *fmt;
	AVCodecContext *codec_ctx;
	AVCodec *codec;
	char filename[256];
	int video_codec;

	if (output != NULL) recClose();

	if (recLoad() == -1) {
		conf.record = 0; return;
	}

	av_register_all();

	switch (conf.codec) {
		case 1: // divx
			video_codec = CODEC_ID_MPEG4;
			sprintf(filename, "gssoft.avi");
			break;
		case 2: // mpeg2
			video_codec = CODEC_ID_MPEG2VIDEO;
			sprintf(filename, "gssoft.mpg");
			break;
		default: // mpeg1
			video_codec = CODEC_ID_MPEG1VIDEO;
			sprintf(filename, "gssoft.mpg");
			break;
	}

	fmt = *first_oformat;
	while (fmt != NULL) {
		if (fmt->video_codec == video_codec) break;
		fmt = fmt->next;
	}
	if (fmt == NULL) {
		SysMessage("codec not found");
		conf.record = 0; return;
	}

	output = av_mallocz(sizeof(AVFormatContext));
	if (output == NULL) {
		SysMessage("Out of Memory");
		conf.record = 0; return;
	}

	output->oformat = fmt;
	snprintf(output->filename, sizeof(output->filename), "%s", filename);

	stream = av_new_stream(output, 0);
	if (stream == NULL) {
		SysMessage("Out of Memory");
		conf.record = 0; return;
	}
	codec_ctx = &stream->codec;
	codec_ctx->codec_id = video_codec;
	codec_ctx->codec_type = CODEC_TYPE_VIDEO;
	codec_ctx->bit_rate = 400000;
	codec_ctx->width = cmode->width;
	codec_ctx->height = cmode->height;
	codec_ctx->frame_rate = gs.SMODE1.cmod & 1 ? 50 : 60;
	codec_ctx->frame_rate_base = 1;
	if (conf.codec == 0) {
		codec_ctx->gop_size = 10;
		codec_ctx->max_b_frames = 1;
	}

	if (av_set_parameters(output, NULL) < 0) {
		SysMessage("set parameters failed");
		conf.record = 0; return;
	}

	codec = avcodec_find_encoder(codec_ctx->codec_id);
	if (codec == NULL) {
		SysMessage("codec not found");
		conf.record = 0; return;
	}
	if (avcodec_open(codec_ctx, codec) < 0) {
		SysMessage("Unable to open codec");
		conf.record = 0; return;
	}

	if (url_fopen(&output->pb, filename, URL_WRONLY) < 0) {
		SysMessage("Unable to open %s for writing", filename);
		conf.record = 0; return;
	}

	av_write_header(output);

	picture = avcodec_alloc_frame();
	yuv420p = avcodec_alloc_frame();

	outbuf_size = 100000;
	outbuf = malloc(outbuf_size);
	if (outbuf == NULL) {
		SysMessage("Out of Memory");
		conf.record = 0; return;
	}

	picture_size = avpicture_get_size(PIX_FMT_YUV420P, codec_ctx->width, codec_ctx->height);
	picture_buf = malloc(picture_size);
	if (picture_buf == NULL) {
		SysMessage("Out of Memory");
		conf.record = 0; return;
	}

	avpicture_fill((AVPicture*)yuv420p, picture_buf, PIX_FMT_YUV420P, codec_ctx->width, codec_ctx->height);
}

void recFrame(char *sbuff, int lPitch, int bpp) {
	int size;
	int pix_fmt;

	picture->data[0] = sbuff;
	picture->linesize[0] = lPitch;
	switch (bpp) {
		case 15: pix_fmt = PIX_FMT_RGB555; break;
		case 16: pix_fmt = PIX_FMT_RGB565; break;
		case 24: pix_fmt = PIX_FMT_RGB24; break;
		case 32: pix_fmt = PIX_FMT_RGBA32; break;
		default: return;
	}
    img_convert((AVPicture*)yuv420p, PIX_FMT_YUV420P, (AVPicture*)picture, pix_fmt, stream->codec.width, stream->codec.height);
    size = avcodec_encode_video(&stream->codec, outbuf, outbuf_size, yuv420p);
	if (size == -1) {
		printf("error encoding frame\n"); return;
	}

	av_write_frame(output, stream->index, outbuf, size);
}

int recExist() {
	if (recLoad() == -1) return -1;
	recUnload();
	return 0;
}

void recClose() {
	avcodec_close(&stream->codec);
	free(outbuf);
	free(picture_buf);
	free(picture);
	free(yuv420p);

	av_write_trailer(output);
	url_fclose(&output->pb);
	av_freep(&output->streams[0]);
	av_free(output); output = NULL;

	recUnload();
}


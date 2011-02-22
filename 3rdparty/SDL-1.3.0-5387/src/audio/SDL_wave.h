/*
    SDL - Simple DirectMedia Layer
    Copyright (C) 1997-2011 Sam Lantinga

    This library is SDL_free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

    Sam Lantinga
    slouken@libsdl.org
*/
#include "SDL_config.h"

/* WAVE files are little-endian */

/*******************************************/
/* Define values for Microsoft WAVE format */
/*******************************************/
#define RIFF		0x46464952      /* "RIFF" */
#define WAVE		0x45564157      /* "WAVE" */
#define FACT		0x74636166      /* "fact" */
#define LIST		0x5453494c      /* "LIST" */
#define FMT		0x20746D66      /* "fmt " */
#define DATA		0x61746164      /* "data" */
#define PCM_CODE	0x0001
#define MS_ADPCM_CODE	0x0002
#define IEEE_FLOAT_CODE	0x0003
#define IMA_ADPCM_CODE	0x0011
#define MP3_CODE	0x0055
#define WAVE_MONO	1
#define WAVE_STEREO	2

/* Normally, these three chunks come consecutively in a WAVE file */
typedef struct WaveFMT
{
/* Not saved in the chunk we read:
	Uint32	FMTchunk;
	Uint32	fmtlen;
*/
    Uint16 encoding;
    Uint16 channels;            /* 1 = mono, 2 = stereo */
    Uint32 frequency;           /* One of 11025, 22050, or 44100 Hz */
    Uint32 byterate;            /* Average bytes per second */
    Uint16 blockalign;          /* Bytes per sample block */
    Uint16 bitspersample;       /* One of 8, 12, 16, or 4 for ADPCM */
} WaveFMT;

/* The general chunk found in the WAVE file */
typedef struct Chunk
{
    Uint32 magic;
    Uint32 length;
    Uint8 *data;
} Chunk;
/* vi: set ts=4 sw=4 expandtab: */

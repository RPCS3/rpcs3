/*
    SDL - Simple DirectMedia Layer
    Copyright (C) 1997-2011 Sam Lantinga

    This library is free software; you can redistribute it and/or
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

/* Functions for audio drivers to perform runtime conversion of audio format */

#include "SDL_audio.h"
#include "SDL_audio_c.h"

/* #define DEBUG_CONVERT */

/* !!! FIXME */
#ifndef assert
#define assert(x)
#endif

/* Effectively mix right and left channels into a single channel */
static void SDLCALL
SDL_ConvertMono(SDL_AudioCVT * cvt, SDL_AudioFormat format)
{
    int i;
    Sint32 sample;

#ifdef DEBUG_CONVERT
    fprintf(stderr, "Converting to mono\n");
#endif
    switch (format & (SDL_AUDIO_MASK_SIGNED | SDL_AUDIO_MASK_BITSIZE)) {
    case AUDIO_U8:
        {
            Uint8 *src, *dst;

            src = cvt->buf;
            dst = cvt->buf;
            for (i = cvt->len_cvt / 2; i; --i) {
                sample = src[0] + src[1];
                *dst = (Uint8) (sample / 2);
                src += 2;
                dst += 1;
            }
        }
        break;

    case AUDIO_S8:
        {
            Sint8 *src, *dst;

            src = (Sint8 *) cvt->buf;
            dst = (Sint8 *) cvt->buf;
            for (i = cvt->len_cvt / 2; i; --i) {
                sample = src[0] + src[1];
                *dst = (Sint8) (sample / 2);
                src += 2;
                dst += 1;
            }
        }
        break;

    case AUDIO_U16:
        {
            Uint8 *src, *dst;

            src = cvt->buf;
            dst = cvt->buf;
            if (SDL_AUDIO_ISBIGENDIAN(format)) {
                for (i = cvt->len_cvt / 4; i; --i) {
                    sample = (Uint16) ((src[0] << 8) | src[1]) +
                        (Uint16) ((src[2] << 8) | src[3]);
                    sample /= 2;
                    dst[1] = (sample & 0xFF);
                    sample >>= 8;
                    dst[0] = (sample & 0xFF);
                    src += 4;
                    dst += 2;
                }
            } else {
                for (i = cvt->len_cvt / 4; i; --i) {
                    sample = (Uint16) ((src[1] << 8) | src[0]) +
                        (Uint16) ((src[3] << 8) | src[2]);
                    sample /= 2;
                    dst[0] = (sample & 0xFF);
                    sample >>= 8;
                    dst[1] = (sample & 0xFF);
                    src += 4;
                    dst += 2;
                }
            }
        }
        break;

    case AUDIO_S16:
        {
            Uint8 *src, *dst;

            src = cvt->buf;
            dst = cvt->buf;
            if (SDL_AUDIO_ISBIGENDIAN(format)) {
                for (i = cvt->len_cvt / 4; i; --i) {
                    sample = (Sint16) ((src[0] << 8) | src[1]) +
                        (Sint16) ((src[2] << 8) | src[3]);
                    sample /= 2;
                    dst[1] = (sample & 0xFF);
                    sample >>= 8;
                    dst[0] = (sample & 0xFF);
                    src += 4;
                    dst += 2;
                }
            } else {
                for (i = cvt->len_cvt / 4; i; --i) {
                    sample = (Sint16) ((src[1] << 8) | src[0]) +
                        (Sint16) ((src[3] << 8) | src[2]);
                    sample /= 2;
                    dst[0] = (sample & 0xFF);
                    sample >>= 8;
                    dst[1] = (sample & 0xFF);
                    src += 4;
                    dst += 2;
                }
            }
        }
        break;

    case AUDIO_S32:
        {
            const Uint32 *src = (const Uint32 *) cvt->buf;
            Uint32 *dst = (Uint32 *) cvt->buf;
            if (SDL_AUDIO_ISBIGENDIAN(format)) {
                for (i = cvt->len_cvt / 8; i; --i, src += 2) {
                    const Sint64 added =
                        (((Sint64) (Sint32) SDL_SwapBE32(src[0])) +
                         ((Sint64) (Sint32) SDL_SwapBE32(src[1])));
                    *(dst++) = SDL_SwapBE32((Uint32) ((Sint32) (added / 2)));
                }
            } else {
                for (i = cvt->len_cvt / 8; i; --i, src += 2) {
                    const Sint64 added =
                        (((Sint64) (Sint32) SDL_SwapLE32(src[0])) +
                         ((Sint64) (Sint32) SDL_SwapLE32(src[1])));
                    *(dst++) = SDL_SwapLE32((Uint32) ((Sint32) (added / 2)));
                }
            }
        }
        break;

    case AUDIO_F32:
        {
            const float *src = (const float *) cvt->buf;
            float *dst = (float *) cvt->buf;
            if (SDL_AUDIO_ISBIGENDIAN(format)) {
                for (i = cvt->len_cvt / 8; i; --i, src += 2) {
                    const float src1 = SDL_SwapFloatBE(src[0]);
                    const float src2 = SDL_SwapFloatBE(src[1]);
                    const double added = ((double) src1) + ((double) src2);
                    const float halved = (float) (added * 0.5);
                    *(dst++) = SDL_SwapFloatBE(halved);
                }
            } else {
                for (i = cvt->len_cvt / 8; i; --i, src += 2) {
                    const float src1 = SDL_SwapFloatLE(src[0]);
                    const float src2 = SDL_SwapFloatLE(src[1]);
                    const double added = ((double) src1) + ((double) src2);
                    const float halved = (float) (added * 0.5);
                    *(dst++) = SDL_SwapFloatLE(halved);
                }
            }
        }
        break;
    }

    cvt->len_cvt /= 2;
    if (cvt->filters[++cvt->filter_index]) {
        cvt->filters[cvt->filter_index] (cvt, format);
    }
}


/* Discard top 4 channels */
static void SDLCALL
SDL_ConvertStrip(SDL_AudioCVT * cvt, SDL_AudioFormat format)
{
    int i;

#ifdef DEBUG_CONVERT
    fprintf(stderr, "Converting down from 6 channels to stereo\n");
#endif

#define strip_chans_6_to_2(type) \
    { \
        const type *src = (const type *) cvt->buf; \
        type *dst = (type *) cvt->buf; \
        for (i = cvt->len_cvt / (sizeof (type) * 6); i; --i) { \
            dst[0] = src[0]; \
            dst[1] = src[1]; \
            src += 6; \
            dst += 2; \
        } \
    }

    /* this function only cares about typesize, and data as a block of bits. */
    switch (SDL_AUDIO_BITSIZE(format)) {
    case 8:
        strip_chans_6_to_2(Uint8);
        break;
    case 16:
        strip_chans_6_to_2(Uint16);
        break;
    case 32:
        strip_chans_6_to_2(Uint32);
        break;
    }

#undef strip_chans_6_to_2

    cvt->len_cvt /= 3;
    if (cvt->filters[++cvt->filter_index]) {
        cvt->filters[cvt->filter_index] (cvt, format);
    }
}


/* Discard top 2 channels of 6 */
static void SDLCALL
SDL_ConvertStrip_2(SDL_AudioCVT * cvt, SDL_AudioFormat format)
{
    int i;

#ifdef DEBUG_CONVERT
    fprintf(stderr, "Converting 6 down to quad\n");
#endif

#define strip_chans_6_to_4(type) \
    { \
        const type *src = (const type *) cvt->buf; \
        type *dst = (type *) cvt->buf; \
        for (i = cvt->len_cvt / (sizeof (type) * 6); i; --i) { \
            dst[0] = src[0]; \
            dst[1] = src[1]; \
            dst[2] = src[2]; \
            dst[3] = src[3]; \
            src += 6; \
            dst += 4; \
        } \
    }

    /* this function only cares about typesize, and data as a block of bits. */
    switch (SDL_AUDIO_BITSIZE(format)) {
    case 8:
        strip_chans_6_to_4(Uint8);
        break;
    case 16:
        strip_chans_6_to_4(Uint16);
        break;
    case 32:
        strip_chans_6_to_4(Uint32);
        break;
    }

#undef strip_chans_6_to_4

    cvt->len_cvt /= 6;
    cvt->len_cvt *= 4;
    if (cvt->filters[++cvt->filter_index]) {
        cvt->filters[cvt->filter_index] (cvt, format);
    }
}

/* Duplicate a mono channel to both stereo channels */
static void SDLCALL
SDL_ConvertStereo(SDL_AudioCVT * cvt, SDL_AudioFormat format)
{
    int i;

#ifdef DEBUG_CONVERT
    fprintf(stderr, "Converting to stereo\n");
#endif

#define dup_chans_1_to_2(type) \
    { \
        const type *src = (const type *) (cvt->buf + cvt->len_cvt); \
        type *dst = (type *) (cvt->buf + cvt->len_cvt * 2); \
        for (i = cvt->len_cvt / 2; i; --i, --src) { \
            const type val = *src; \
            dst -= 2; \
            dst[0] = dst[1] = val; \
        } \
    }

    /* this function only cares about typesize, and data as a block of bits. */
    switch (SDL_AUDIO_BITSIZE(format)) {
    case 8:
        dup_chans_1_to_2(Uint8);
        break;
    case 16:
        dup_chans_1_to_2(Uint16);
        break;
    case 32:
        dup_chans_1_to_2(Uint32);
        break;
    }

#undef dup_chans_1_to_2

    cvt->len_cvt *= 2;
    if (cvt->filters[++cvt->filter_index]) {
        cvt->filters[cvt->filter_index] (cvt, format);
    }
}


/* Duplicate a stereo channel to a pseudo-5.1 stream */
static void SDLCALL
SDL_ConvertSurround(SDL_AudioCVT * cvt, SDL_AudioFormat format)
{
    int i;

#ifdef DEBUG_CONVERT
    fprintf(stderr, "Converting stereo to surround\n");
#endif

    switch (format & (SDL_AUDIO_MASK_SIGNED | SDL_AUDIO_MASK_BITSIZE)) {
    case AUDIO_U8:
        {
            Uint8 *src, *dst, lf, rf, ce;

            src = (Uint8 *) (cvt->buf + cvt->len_cvt);
            dst = (Uint8 *) (cvt->buf + cvt->len_cvt * 3);
            for (i = cvt->len_cvt; i; --i) {
                dst -= 6;
                src -= 2;
                lf = src[0];
                rf = src[1];
                ce = (lf / 2) + (rf / 2);
                dst[0] = lf;
                dst[1] = rf;
                dst[2] = lf - ce;
                dst[3] = rf - ce;
                dst[4] = ce;
                dst[5] = ce;
            }
        }
        break;

    case AUDIO_S8:
        {
            Sint8 *src, *dst, lf, rf, ce;

            src = (Sint8 *) cvt->buf + cvt->len_cvt;
            dst = (Sint8 *) cvt->buf + cvt->len_cvt * 3;
            for (i = cvt->len_cvt; i; --i) {
                dst -= 6;
                src -= 2;
                lf = src[0];
                rf = src[1];
                ce = (lf / 2) + (rf / 2);
                dst[0] = lf;
                dst[1] = rf;
                dst[2] = lf - ce;
                dst[3] = rf - ce;
                dst[4] = ce;
                dst[5] = ce;
            }
        }
        break;

    case AUDIO_U16:
        {
            Uint8 *src, *dst;
            Uint16 lf, rf, ce, lr, rr;

            src = cvt->buf + cvt->len_cvt;
            dst = cvt->buf + cvt->len_cvt * 3;

            if (SDL_AUDIO_ISBIGENDIAN(format)) {
                for (i = cvt->len_cvt / 4; i; --i) {
                    dst -= 12;
                    src -= 4;
                    lf = (Uint16) ((src[0] << 8) | src[1]);
                    rf = (Uint16) ((src[2] << 8) | src[3]);
                    ce = (lf / 2) + (rf / 2);
                    rr = lf - ce;
                    lr = rf - ce;
                    dst[1] = (lf & 0xFF);
                    dst[0] = ((lf >> 8) & 0xFF);
                    dst[3] = (rf & 0xFF);
                    dst[2] = ((rf >> 8) & 0xFF);

                    dst[1 + 4] = (lr & 0xFF);
                    dst[0 + 4] = ((lr >> 8) & 0xFF);
                    dst[3 + 4] = (rr & 0xFF);
                    dst[2 + 4] = ((rr >> 8) & 0xFF);

                    dst[1 + 8] = (ce & 0xFF);
                    dst[0 + 8] = ((ce >> 8) & 0xFF);
                    dst[3 + 8] = (ce & 0xFF);
                    dst[2 + 8] = ((ce >> 8) & 0xFF);
                }
            } else {
                for (i = cvt->len_cvt / 4; i; --i) {
                    dst -= 12;
                    src -= 4;
                    lf = (Uint16) ((src[1] << 8) | src[0]);
                    rf = (Uint16) ((src[3] << 8) | src[2]);
                    ce = (lf / 2) + (rf / 2);
                    rr = lf - ce;
                    lr = rf - ce;
                    dst[0] = (lf & 0xFF);
                    dst[1] = ((lf >> 8) & 0xFF);
                    dst[2] = (rf & 0xFF);
                    dst[3] = ((rf >> 8) & 0xFF);

                    dst[0 + 4] = (lr & 0xFF);
                    dst[1 + 4] = ((lr >> 8) & 0xFF);
                    dst[2 + 4] = (rr & 0xFF);
                    dst[3 + 4] = ((rr >> 8) & 0xFF);

                    dst[0 + 8] = (ce & 0xFF);
                    dst[1 + 8] = ((ce >> 8) & 0xFF);
                    dst[2 + 8] = (ce & 0xFF);
                    dst[3 + 8] = ((ce >> 8) & 0xFF);
                }
            }
        }
        break;

    case AUDIO_S16:
        {
            Uint8 *src, *dst;
            Sint16 lf, rf, ce, lr, rr;

            src = cvt->buf + cvt->len_cvt;
            dst = cvt->buf + cvt->len_cvt * 3;

            if (SDL_AUDIO_ISBIGENDIAN(format)) {
                for (i = cvt->len_cvt / 4; i; --i) {
                    dst -= 12;
                    src -= 4;
                    lf = (Sint16) ((src[0] << 8) | src[1]);
                    rf = (Sint16) ((src[2] << 8) | src[3]);
                    ce = (lf / 2) + (rf / 2);
                    rr = lf - ce;
                    lr = rf - ce;
                    dst[1] = (lf & 0xFF);
                    dst[0] = ((lf >> 8) & 0xFF);
                    dst[3] = (rf & 0xFF);
                    dst[2] = ((rf >> 8) & 0xFF);

                    dst[1 + 4] = (lr & 0xFF);
                    dst[0 + 4] = ((lr >> 8) & 0xFF);
                    dst[3 + 4] = (rr & 0xFF);
                    dst[2 + 4] = ((rr >> 8) & 0xFF);

                    dst[1 + 8] = (ce & 0xFF);
                    dst[0 + 8] = ((ce >> 8) & 0xFF);
                    dst[3 + 8] = (ce & 0xFF);
                    dst[2 + 8] = ((ce >> 8) & 0xFF);
                }
            } else {
                for (i = cvt->len_cvt / 4; i; --i) {
                    dst -= 12;
                    src -= 4;
                    lf = (Sint16) ((src[1] << 8) | src[0]);
                    rf = (Sint16) ((src[3] << 8) | src[2]);
                    ce = (lf / 2) + (rf / 2);
                    rr = lf - ce;
                    lr = rf - ce;
                    dst[0] = (lf & 0xFF);
                    dst[1] = ((lf >> 8) & 0xFF);
                    dst[2] = (rf & 0xFF);
                    dst[3] = ((rf >> 8) & 0xFF);

                    dst[0 + 4] = (lr & 0xFF);
                    dst[1 + 4] = ((lr >> 8) & 0xFF);
                    dst[2 + 4] = (rr & 0xFF);
                    dst[3 + 4] = ((rr >> 8) & 0xFF);

                    dst[0 + 8] = (ce & 0xFF);
                    dst[1 + 8] = ((ce >> 8) & 0xFF);
                    dst[2 + 8] = (ce & 0xFF);
                    dst[3 + 8] = ((ce >> 8) & 0xFF);
                }
            }
        }
        break;

    case AUDIO_S32:
        {
            Sint32 lf, rf, ce;
            const Uint32 *src = (const Uint32 *) cvt->buf + cvt->len_cvt;
            Uint32 *dst = (Uint32 *) cvt->buf + cvt->len_cvt * 3;

            if (SDL_AUDIO_ISBIGENDIAN(format)) {
                for (i = cvt->len_cvt / 8; i; --i) {
                    dst -= 6;
                    src -= 2;
                    lf = (Sint32) SDL_SwapBE32(src[0]);
                    rf = (Sint32) SDL_SwapBE32(src[1]);
                    ce = (lf / 2) + (rf / 2);
                    dst[0] = SDL_SwapBE32((Uint32) lf);
                    dst[1] = SDL_SwapBE32((Uint32) rf);
                    dst[2] = SDL_SwapBE32((Uint32) (lf - ce));
                    dst[3] = SDL_SwapBE32((Uint32) (rf - ce));
                    dst[4] = SDL_SwapBE32((Uint32) ce);
                    dst[5] = SDL_SwapBE32((Uint32) ce);
                }
            } else {
                for (i = cvt->len_cvt / 8; i; --i) {
                    dst -= 6;
                    src -= 2;
                    lf = (Sint32) SDL_SwapLE32(src[0]);
                    rf = (Sint32) SDL_SwapLE32(src[1]);
                    ce = (lf / 2) + (rf / 2);
                    dst[0] = src[0];
                    dst[1] = src[1];
                    dst[2] = SDL_SwapLE32((Uint32) (lf - ce));
                    dst[3] = SDL_SwapLE32((Uint32) (rf - ce));
                    dst[4] = SDL_SwapLE32((Uint32) ce);
                    dst[5] = SDL_SwapLE32((Uint32) ce);
                }
            }
        }
        break;

    case AUDIO_F32:
        {
            float lf, rf, ce;
            const float *src = (const float *) cvt->buf + cvt->len_cvt;
            float *dst = (float *) cvt->buf + cvt->len_cvt * 3;

            if (SDL_AUDIO_ISBIGENDIAN(format)) {
                for (i = cvt->len_cvt / 8; i; --i) {
                    dst -= 6;
                    src -= 2;
                    lf = SDL_SwapFloatBE(src[0]);
                    rf = SDL_SwapFloatBE(src[1]);
                    ce = (lf * 0.5f) + (rf * 0.5f);
                    dst[0] = src[0];
                    dst[1] = src[1];
                    dst[2] = SDL_SwapFloatBE(lf - ce);
                    dst[3] = SDL_SwapFloatBE(rf - ce);
                    dst[4] = dst[5] = SDL_SwapFloatBE(ce);
                }
            } else {
                for (i = cvt->len_cvt / 8; i; --i) {
                    dst -= 6;
                    src -= 2;
                    lf = SDL_SwapFloatLE(src[0]);
                    rf = SDL_SwapFloatLE(src[1]);
                    ce = (lf * 0.5f) + (rf * 0.5f);
                    dst[0] = src[0];
                    dst[1] = src[1];
                    dst[2] = SDL_SwapFloatLE(lf - ce);
                    dst[3] = SDL_SwapFloatLE(rf - ce);
                    dst[4] = dst[5] = SDL_SwapFloatLE(ce);
                }
            }
        }
        break;

    }
    cvt->len_cvt *= 3;
    if (cvt->filters[++cvt->filter_index]) {
        cvt->filters[cvt->filter_index] (cvt, format);
    }
}


/* Duplicate a stereo channel to a pseudo-4.0 stream */
static void SDLCALL
SDL_ConvertSurround_4(SDL_AudioCVT * cvt, SDL_AudioFormat format)
{
    int i;

#ifdef DEBUG_CONVERT
    fprintf(stderr, "Converting stereo to quad\n");
#endif

    switch (format & (SDL_AUDIO_MASK_SIGNED | SDL_AUDIO_MASK_BITSIZE)) {
    case AUDIO_U8:
        {
            Uint8 *src, *dst, lf, rf, ce;

            src = (Uint8 *) (cvt->buf + cvt->len_cvt);
            dst = (Uint8 *) (cvt->buf + cvt->len_cvt * 2);
            for (i = cvt->len_cvt; i; --i) {
                dst -= 4;
                src -= 2;
                lf = src[0];
                rf = src[1];
                ce = (lf / 2) + (rf / 2);
                dst[0] = lf;
                dst[1] = rf;
                dst[2] = lf - ce;
                dst[3] = rf - ce;
            }
        }
        break;

    case AUDIO_S8:
        {
            Sint8 *src, *dst, lf, rf, ce;

            src = (Sint8 *) cvt->buf + cvt->len_cvt;
            dst = (Sint8 *) cvt->buf + cvt->len_cvt * 2;
            for (i = cvt->len_cvt; i; --i) {
                dst -= 4;
                src -= 2;
                lf = src[0];
                rf = src[1];
                ce = (lf / 2) + (rf / 2);
                dst[0] = lf;
                dst[1] = rf;
                dst[2] = lf - ce;
                dst[3] = rf - ce;
            }
        }
        break;

    case AUDIO_U16:
        {
            Uint8 *src, *dst;
            Uint16 lf, rf, ce, lr, rr;

            src = cvt->buf + cvt->len_cvt;
            dst = cvt->buf + cvt->len_cvt * 2;

            if (SDL_AUDIO_ISBIGENDIAN(format)) {
                for (i = cvt->len_cvt / 4; i; --i) {
                    dst -= 8;
                    src -= 4;
                    lf = (Uint16) ((src[0] << 8) | src[1]);
                    rf = (Uint16) ((src[2] << 8) | src[3]);
                    ce = (lf / 2) + (rf / 2);
                    rr = lf - ce;
                    lr = rf - ce;
                    dst[1] = (lf & 0xFF);
                    dst[0] = ((lf >> 8) & 0xFF);
                    dst[3] = (rf & 0xFF);
                    dst[2] = ((rf >> 8) & 0xFF);

                    dst[1 + 4] = (lr & 0xFF);
                    dst[0 + 4] = ((lr >> 8) & 0xFF);
                    dst[3 + 4] = (rr & 0xFF);
                    dst[2 + 4] = ((rr >> 8) & 0xFF);
                }
            } else {
                for (i = cvt->len_cvt / 4; i; --i) {
                    dst -= 8;
                    src -= 4;
                    lf = (Uint16) ((src[1] << 8) | src[0]);
                    rf = (Uint16) ((src[3] << 8) | src[2]);
                    ce = (lf / 2) + (rf / 2);
                    rr = lf - ce;
                    lr = rf - ce;
                    dst[0] = (lf & 0xFF);
                    dst[1] = ((lf >> 8) & 0xFF);
                    dst[2] = (rf & 0xFF);
                    dst[3] = ((rf >> 8) & 0xFF);

                    dst[0 + 4] = (lr & 0xFF);
                    dst[1 + 4] = ((lr >> 8) & 0xFF);
                    dst[2 + 4] = (rr & 0xFF);
                    dst[3 + 4] = ((rr >> 8) & 0xFF);
                }
            }
        }
        break;

    case AUDIO_S16:
        {
            Uint8 *src, *dst;
            Sint16 lf, rf, ce, lr, rr;

            src = cvt->buf + cvt->len_cvt;
            dst = cvt->buf + cvt->len_cvt * 2;

            if (SDL_AUDIO_ISBIGENDIAN(format)) {
                for (i = cvt->len_cvt / 4; i; --i) {
                    dst -= 8;
                    src -= 4;
                    lf = (Sint16) ((src[0] << 8) | src[1]);
                    rf = (Sint16) ((src[2] << 8) | src[3]);
                    ce = (lf / 2) + (rf / 2);
                    rr = lf - ce;
                    lr = rf - ce;
                    dst[1] = (lf & 0xFF);
                    dst[0] = ((lf >> 8) & 0xFF);
                    dst[3] = (rf & 0xFF);
                    dst[2] = ((rf >> 8) & 0xFF);

                    dst[1 + 4] = (lr & 0xFF);
                    dst[0 + 4] = ((lr >> 8) & 0xFF);
                    dst[3 + 4] = (rr & 0xFF);
                    dst[2 + 4] = ((rr >> 8) & 0xFF);
                }
            } else {
                for (i = cvt->len_cvt / 4; i; --i) {
                    dst -= 8;
                    src -= 4;
                    lf = (Sint16) ((src[1] << 8) | src[0]);
                    rf = (Sint16) ((src[3] << 8) | src[2]);
                    ce = (lf / 2) + (rf / 2);
                    rr = lf - ce;
                    lr = rf - ce;
                    dst[0] = (lf & 0xFF);
                    dst[1] = ((lf >> 8) & 0xFF);
                    dst[2] = (rf & 0xFF);
                    dst[3] = ((rf >> 8) & 0xFF);

                    dst[0 + 4] = (lr & 0xFF);
                    dst[1 + 4] = ((lr >> 8) & 0xFF);
                    dst[2 + 4] = (rr & 0xFF);
                    dst[3 + 4] = ((rr >> 8) & 0xFF);
                }
            }
        }
        break;

    case AUDIO_S32:
        {
            const Uint32 *src = (const Uint32 *) (cvt->buf + cvt->len_cvt);
            Uint32 *dst = (Uint32 *) (cvt->buf + cvt->len_cvt * 2);
            Sint32 lf, rf, ce;

            if (SDL_AUDIO_ISBIGENDIAN(format)) {
                for (i = cvt->len_cvt / 8; i; --i) {
                    dst -= 4;
                    src -= 2;
                    lf = (Sint32) SDL_SwapBE32(src[0]);
                    rf = (Sint32) SDL_SwapBE32(src[1]);
                    ce = (lf / 2) + (rf / 2);
                    dst[0] = src[0];
                    dst[1] = src[1];
                    dst[2] = SDL_SwapBE32((Uint32) (lf - ce));
                    dst[3] = SDL_SwapBE32((Uint32) (rf - ce));
                }
            } else {
                for (i = cvt->len_cvt / 8; i; --i) {
                    dst -= 4;
                    src -= 2;
                    lf = (Sint32) SDL_SwapLE32(src[0]);
                    rf = (Sint32) SDL_SwapLE32(src[1]);
                    ce = (lf / 2) + (rf / 2);
                    dst[0] = src[0];
                    dst[1] = src[1];
                    dst[2] = SDL_SwapLE32((Uint32) (lf - ce));
                    dst[3] = SDL_SwapLE32((Uint32) (rf - ce));
                }
            }
        }
        break;
    }
    cvt->len_cvt *= 2;
    if (cvt->filters[++cvt->filter_index]) {
        cvt->filters[cvt->filter_index] (cvt, format);
    }
}


int
SDL_ConvertAudio(SDL_AudioCVT * cvt)
{
    /* !!! FIXME: (cvt) should be const; stack-copy it here. */
    /* !!! FIXME: (actually, we can't...len_cvt needs to be updated. Grr.) */

    /* Make sure there's data to convert */
    if (cvt->buf == NULL) {
        SDL_SetError("No buffer allocated for conversion");
        return (-1);
    }
    /* Return okay if no conversion is necessary */
    cvt->len_cvt = cvt->len;
    if (cvt->filters[0] == NULL) {
        return (0);
    }

    /* Set up the conversion and go! */
    cvt->filter_index = 0;
    cvt->filters[0] (cvt, cvt->src_format);
    return (0);
}


static SDL_AudioFilter
SDL_HandTunedTypeCVT(SDL_AudioFormat src_fmt, SDL_AudioFormat dst_fmt)
{
    /*
     * Fill in any future conversions that are specialized to a
     *  processor, platform, compiler, or library here.
     */

    return NULL;                /* no specialized converter code available. */
}


/*
 * Find a converter between two data types. We try to select a hand-tuned
 *  asm/vectorized/optimized function first, and then fallback to an
 *  autogenerated function that is customized to convert between two
 *  specific data types.
 */
static int
SDL_BuildAudioTypeCVT(SDL_AudioCVT * cvt,
                      SDL_AudioFormat src_fmt, SDL_AudioFormat dst_fmt)
{
    if (src_fmt != dst_fmt) {
        const Uint16 src_bitsize = SDL_AUDIO_BITSIZE(src_fmt);
        const Uint16 dst_bitsize = SDL_AUDIO_BITSIZE(dst_fmt);
        SDL_AudioFilter filter = SDL_HandTunedTypeCVT(src_fmt, dst_fmt);

        /* No hand-tuned converter? Try the autogenerated ones. */
        if (filter == NULL) {
            int i;
            for (i = 0; sdl_audio_type_filters[i].filter != NULL; i++) {
                const SDL_AudioTypeFilters *filt = &sdl_audio_type_filters[i];
                if ((filt->src_fmt == src_fmt) && (filt->dst_fmt == dst_fmt)) {
                    filter = filt->filter;
                    break;
                }
            }

            if (filter == NULL) {
                SDL_SetError("No conversion available for these formats");
                return -1;
            }
        }

        /* Update (cvt) with filter details... */
        cvt->filters[cvt->filter_index++] = filter;
        if (src_bitsize < dst_bitsize) {
            const int mult = (dst_bitsize / src_bitsize);
            cvt->len_mult *= mult;
            cvt->len_ratio *= mult;
        } else if (src_bitsize > dst_bitsize) {
            cvt->len_ratio /= (src_bitsize / dst_bitsize);
        }

        return 1;               /* added a converter. */
    }

    return 0;                   /* no conversion necessary. */
}


static SDL_AudioFilter
SDL_HandTunedResampleCVT(SDL_AudioCVT * cvt, int dst_channels,
                         int src_rate, int dst_rate)
{
    /*
     * Fill in any future conversions that are specialized to a
     *  processor, platform, compiler, or library here.
     */

    return NULL;                /* no specialized converter code available. */
}

static int
SDL_FindFrequencyMultiple(const int src_rate, const int dst_rate)
{
    int retval = 0;

    /* If we only built with the arbitrary resamplers, ignore multiples. */
#if !LESS_RESAMPLERS
    int lo, hi;
    int div;

    assert(src_rate != 0);
    assert(dst_rate != 0);
    assert(src_rate != dst_rate);

    if (src_rate < dst_rate) {
        lo = src_rate;
        hi = dst_rate;
    } else {
        lo = dst_rate;
        hi = src_rate;
    }

    /* zero means "not a supported multiple" ... we only do 2x and 4x. */
    if ((hi % lo) != 0)
        return 0;               /* not a multiple. */

    div = hi / lo;
    retval = ((div == 2) || (div == 4)) ? div : 0;
#endif

    return retval;
}

static int
SDL_BuildAudioResampleCVT(SDL_AudioCVT * cvt, int dst_channels,
                          int src_rate, int dst_rate)
{
    if (src_rate != dst_rate) {
        SDL_AudioFilter filter = SDL_HandTunedResampleCVT(cvt, dst_channels,
                                                          src_rate, dst_rate);

        /* No hand-tuned converter? Try the autogenerated ones. */
        if (filter == NULL) {
            int i;
            const int upsample = (src_rate < dst_rate) ? 1 : 0;
            const int multiple =
                SDL_FindFrequencyMultiple(src_rate, dst_rate);

            for (i = 0; sdl_audio_rate_filters[i].filter != NULL; i++) {
                const SDL_AudioRateFilters *filt = &sdl_audio_rate_filters[i];
                if ((filt->fmt == cvt->dst_format) &&
                    (filt->channels == dst_channels) &&
                    (filt->upsample == upsample) &&
                    (filt->multiple == multiple)) {
                    filter = filt->filter;
                    break;
                }
            }

            if (filter == NULL) {
                SDL_SetError("No conversion available for these rates");
                return -1;
            }
        }

        /* Update (cvt) with filter details... */
        cvt->filters[cvt->filter_index++] = filter;
        if (src_rate < dst_rate) {
            const double mult = ((double) dst_rate) / ((double) src_rate);
            cvt->len_mult *= (int) SDL_ceil(mult);
            cvt->len_ratio *= mult;
        } else {
            cvt->len_ratio /= ((double) src_rate) / ((double) dst_rate);
        }

        return 1;               /* added a converter. */
    }

    return 0;                   /* no conversion necessary. */
}


/* Creates a set of audio filters to convert from one format to another.
   Returns -1 if the format conversion is not supported, 0 if there's
   no conversion needed, or 1 if the audio filter is set up.
*/

int
SDL_BuildAudioCVT(SDL_AudioCVT * cvt,
                  SDL_AudioFormat src_fmt, Uint8 src_channels, int src_rate,
                  SDL_AudioFormat dst_fmt, Uint8 dst_channels, int dst_rate)
{
    /*
     * !!! FIXME: reorder filters based on which grow/shrink the buffer.
     * !!! FIXME: ideally, we should do everything that shrinks the buffer
     * !!! FIXME: first, so we don't have to process as many bytes in a given
     * !!! FIXME: filter and abuse the CPU cache less. This might not be as
     * !!! FIXME: good in practice as it sounds in theory, though.
     */

    /* there are no unsigned types over 16 bits, so catch this up front. */
    if ((SDL_AUDIO_BITSIZE(src_fmt) > 16) && (!SDL_AUDIO_ISSIGNED(src_fmt))) {
        SDL_SetError("Invalid source format");
        return -1;
    }
    if ((SDL_AUDIO_BITSIZE(dst_fmt) > 16) && (!SDL_AUDIO_ISSIGNED(dst_fmt))) {
        SDL_SetError("Invalid destination format");
        return -1;
    }

    /* prevent possible divisions by zero, etc. */
    if ((src_rate == 0) || (dst_rate == 0)) {
        SDL_SetError("Source or destination rate is zero");
        return -1;
    }
#ifdef DEBUG_CONVERT
    printf("Build format %04x->%04x, channels %u->%u, rate %d->%d\n",
           src_fmt, dst_fmt, src_channels, dst_channels, src_rate, dst_rate);
#endif

    /* Start off with no conversion necessary */
    SDL_zerop(cvt);
    cvt->src_format = src_fmt;
    cvt->dst_format = dst_fmt;
    cvt->needed = 0;
    cvt->filter_index = 0;
    cvt->filters[0] = NULL;
    cvt->len_mult = 1;
    cvt->len_ratio = 1.0;
    cvt->rate_incr = ((double) dst_rate) / ((double) src_rate);

    /* Convert data types, if necessary. Updates (cvt). */
    if (SDL_BuildAudioTypeCVT(cvt, src_fmt, dst_fmt) == -1) {
        return -1;              /* shouldn't happen, but just in case... */
    }

    /* Channel conversion */
    if (src_channels != dst_channels) {
        if ((src_channels == 1) && (dst_channels > 1)) {
            cvt->filters[cvt->filter_index++] = SDL_ConvertStereo;
            cvt->len_mult *= 2;
            src_channels = 2;
            cvt->len_ratio *= 2;
        }
        if ((src_channels == 2) && (dst_channels == 6)) {
            cvt->filters[cvt->filter_index++] = SDL_ConvertSurround;
            src_channels = 6;
            cvt->len_mult *= 3;
            cvt->len_ratio *= 3;
        }
        if ((src_channels == 2) && (dst_channels == 4)) {
            cvt->filters[cvt->filter_index++] = SDL_ConvertSurround_4;
            src_channels = 4;
            cvt->len_mult *= 2;
            cvt->len_ratio *= 2;
        }
        while ((src_channels * 2) <= dst_channels) {
            cvt->filters[cvt->filter_index++] = SDL_ConvertStereo;
            cvt->len_mult *= 2;
            src_channels *= 2;
            cvt->len_ratio *= 2;
        }
        if ((src_channels == 6) && (dst_channels <= 2)) {
            cvt->filters[cvt->filter_index++] = SDL_ConvertStrip;
            src_channels = 2;
            cvt->len_ratio /= 3;
        }
        if ((src_channels == 6) && (dst_channels == 4)) {
            cvt->filters[cvt->filter_index++] = SDL_ConvertStrip_2;
            src_channels = 4;
            cvt->len_ratio /= 2;
        }
        /* This assumes that 4 channel audio is in the format:
           Left {front/back} + Right {front/back}
           so converting to L/R stereo works properly.
         */
        while (((src_channels % 2) == 0) &&
               ((src_channels / 2) >= dst_channels)) {
            cvt->filters[cvt->filter_index++] = SDL_ConvertMono;
            src_channels /= 2;
            cvt->len_ratio /= 2;
        }
        if (src_channels != dst_channels) {
            /* Uh oh.. */ ;
        }
    }

    /* Do rate conversion, if necessary. Updates (cvt). */
    if (SDL_BuildAudioResampleCVT(cvt, dst_channels, src_rate, dst_rate) ==
        -1) {
        return -1;              /* shouldn't happen, but just in case... */
    }

    /* Set up the filter information */
    if (cvt->filter_index != 0) {
        cvt->needed = 1;
        cvt->src_format = src_fmt;
        cvt->dst_format = dst_fmt;
        cvt->len = 0;
        cvt->buf = NULL;
        cvt->filters[cvt->filter_index] = NULL;
    }
    return (cvt->needed);
}


/* vi: set ts=4 sw=4 expandtab: */

/* SDL Audio sink for SPU2-X.
 * Copyright (c) 2013, Matt Scheirer <matt.scheirer@gmail.com>
 *
 * SPU2-X is free software: you can redistribute it and/or modify it under the terms
 * of the GNU Lesser General Public License as published by the Free Software Found-
 * ation, either version 3 of the License, or (at your option) any later version.
 *
 * SPU2-X is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with SPU2-X.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <cassert>
#include <iostream>

#include "Global.h"
#include "SndOut.h"

#if __cplusplus >= 201103L
#include <memory>
#else
#include <cstddef>
#endif

/* Using SDL2 requires other SDL dependencies in pcsx2 get upgraded as well, or the
 * symbol tables would conflict. Other dependencies in Linux are wxwidgets (you can
 * build wx without sdl support, though) and onepad at the time of writing this. */
#ifdef SPU2X_SDL2
#include <SDL2/SDL.h>
#include <SDL2/SDL_audio.h>
typedef StereoOut32 StereoOut_SDL;
#else
#include <SDL/SDL.h>
#include <SDL/SDL_audio.h>
typedef StereoOut16 StereoOut_SDL;
#endif

namespace {
	/* Since spu2 only ever outputs stereo, we don't worry about emitting surround sound
	 * even though SDL2 supports it */
	const Uint8 channels = 2;
	/* SDL2 supports s32 audio */
	/* Samples should vary from [512,8192] according to SDL spec. Take note this is the desired
	 * sample count and SDL may provide otherwise. Pulseaudio will cut this value in half if
	 * PA_STREAM_ADJUST_LATENCY is set in the backened, for example. */
	const Uint16 desiredSamples = 1024;
	const Uint16 format =
#if SDL_MAJOR_VERSION >= 2
		AUDIO_S32SYS;
#else
		AUDIO_S16SYS;
#endif

	Uint16 samples = desiredSamples;

#if __cplusplus >= 201103L
	std::unique_ptr<StereoOut_SDL[]> buffer;
#else
	StereoOut_SDL *buffer = NULL;
#endif

	void callback_fillBuffer(void *userdata, Uint8 *stream, int len) {
		// Length should always be samples in bytes.
		assert(len / sizeof(StereoOut_SDL) == samples);

		for(Uint16 i = 0; i < samples; i += SndOutPacketSize)
			SndBuffer::ReadSamples(&buffer[i]);
#if __cplusplus >= 201103L
		SDL_MixAudio(stream, (Uint8*) buffer.get() , len, SDL_MIX_MAXVOLUME);
#else
		SDL_MixAudio(stream, (Uint8*) buffer , len, SDL_MIX_MAXVOLUME);
#endif
	}
}

struct SDLAudioMod : public SndOutModule {
	static SDLAudioMod mod;

	s32 Init() {
		/* SDL backends will mangle the AudioSpec and change the sample count. If we reopen
		 * the audio backend, we need to make sure we keep our desired samples in the spec */
		spec.samples = desiredSamples;

		if(SDL_Init(SDL_INIT_AUDIO) < 0 || SDL_OpenAudio(&spec, NULL) < 0) {
			std::cerr << "SPU2-X: SDL audio error: " << SDL_GetError() << std::endl;
			return -1;
		}
		/* This is so ugly. It is hilariously ugly. I didn't use a vector to save reallocs. */
		if(samples != spec.samples || buffer == NULL)
#if __cplusplus >= 201103L
			buffer = std::unique_ptr<StereoOut_SDL[]>(new StereoOut_SDL[spec.samples]);
#else
			buffer = new StereoOut_SDL[spec.samples];
#endif
		if(samples != spec.samples) {
			// Samples must always be a multiple of packet size.
			assert(spec.samples % SndOutPacketSize == 0);
			samples = spec.samples;
		}
		SDL_PauseAudio(0);
		return 0;
	}

	const wchar_t* GetIdent() const { return L"SDLAudio"; }
	const wchar_t* GetLongName() const { return L"SDL Audio"; }

	void Close() {
		SDL_CloseAudio();
#if __cplusplus < 201103L
		delete[] buffer;
		buffer = NULL;
#endif
	}

	s32 Test() const { return 0; }
	void Configure(uptr parent) {}
	void ReadSettings() {}
	void SetApiSettings(wxString api) {}
	void WriteSettings() const {};
	int GetEmptySampleCount() { return 0; }

	~SDLAudioMod() {  Close(); }

	private:
	SDL_AudioSpec spec;

	/* Only C++11 supports the aggregate initializer list syntax used here. */
	SDLAudioMod()
#if __cplusplus >= 201103L
		: spec({SampleRate, format, channels, 0,
				desiredSamples, 0, 0, &callback_fillBuffer, nullptr}) {
#else
			{
				spec.freq = SampleRate;
				spec.format = format;
				spec.channels = channels;
				spec.samples = desiredSamples;
				spec.callback = callback_fillBuffer;
				spec.userdata = NULL;
#endif
				// Number of samples must be a multiple of packet size.
				assert(samples % SndOutPacketSize == 0);
			}
		};

	SDLAudioMod SDLAudioMod::mod;

	SndOutModule * const SDLOut = &SDLAudioMod::mod;

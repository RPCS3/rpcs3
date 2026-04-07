#pragma once

#include "util/video_source.h"

namespace rsx
{
	namespace overlays
	{
		class audio_player
		{
		public:
			audio_player(const std::string& audio_path);
			~audio_player() = default;

			void set_active(bool active);

		private:
			void init_audio(const std::string& audio_path);

			std::unique_ptr<video_source> m_video_source;
		};
	}
}

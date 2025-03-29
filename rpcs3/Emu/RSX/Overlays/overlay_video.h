#pragma once

#include "overlay_controls.h"
#include "util/video_source.h"

namespace rsx
{
	namespace overlays
	{
		struct video_info : public image_info_base
		{
			std::vector<u8> data;
			const u8* get_data() const override { return data.empty() ? nullptr : data.data(); }
		};

		class video_view final : public image_view
		{
		public:
			video_view(const std::string& video_path);
			virtual ~video_view();

			void update();
			compiled_resource& get_compiled() override;

			const video_info* get_info() { return m_video_info.at(m_buffer_index).get(); }

		private:
			usz m_buffer_index = 0;
			std::array<std::unique_ptr<video_info>, 2> m_video_info; // double buffer
			std::unique_ptr<video_source> m_video_source;
		};
	}
}

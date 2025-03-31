#pragma once

#include "overlay_controls.h"
#include "util/video_source.h"

namespace rsx
{
	namespace overlays
	{
		struct video_info : public image_info_base
		{
			using image_info_base::image_info_base;
			virtual ~video_info() {}
			const u8* get_data() const override { return data.empty() ? nullptr : data.data(); }

			std::vector<u8> data;
		};

		class video_view final : public image_view
		{
		public:
			video_view(const std::string& video_path, const std::string& thumbnail_path);
			video_view(const std::string& video_path, const std::vector<u8>& thumbnail_buf);
			video_view(const std::string& video_path, u8 thumbnail_id);
			virtual ~video_view();

			void set_active(bool active);

			void update();
			compiled_resource& get_compiled() override;

		private:
			void init_video(const std::string& video_path);

			usz m_buffer_index = 0;
			std::array<std::unique_ptr<video_info>, 2> m_video_info; // double buffer
			std::unique_ptr<video_source> m_video_source;
			std::unique_ptr<image_info> m_thumbnail_info;
			u8 m_thumbnail_id = image_resource_id::none;
			bool m_video_active = false; // This is the expected state. The actual state is found in the video source.
		};
	}
}

#pragma once

namespace gui
{
	namespace utils
	{
		void check_microphone_permission();
		bool check_camera_permission(void* obj, std::function<void()> repeat_callback, std::function<void()> denied_callback);
	}
}

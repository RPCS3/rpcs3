#pragma once

#include "shader_loading_dialog.h"

class GSRender;

namespace rsx
{
	namespace overlays
	{
		class message_dialog;
	}

	class thread;

	struct shader_loading_dialog_native : rsx::shader_loading_dialog
	{
		rsx::thread* owner = nullptr;
		std::shared_ptr<rsx::overlays::message_dialog> dlg{};

		shader_loading_dialog_native(GSRender* ptr);

		void create(const std::string& msg, const std::string&/* title*/) override;
		void update_msg(u32 index, std::string msg) override;
		void inc_value(u32 index, u32 value) override;
		void set_value(u32 index, u32 value) override;
		void set_limit(u32 index, u32 limit) override;
		void refresh() override;
		void close() override;
	};
}

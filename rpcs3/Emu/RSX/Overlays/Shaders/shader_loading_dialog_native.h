#pragma once

namespace rsx
{
	namespace overlays
	{
		class message_dialog;
	}

	class thread;

	struct shader_loading_dialog_native
	{
	public:
		rsx::thread* owner = nullptr;
		std::shared_ptr<rsx::overlays::message_dialog> dlg{};

		shader_loading_dialog_native(rsx::thread* ptr);

		void create(const std::string& msg, const std::string&/* title*/);
		void update_msg(u32 index, std::string msg);
		void inc_value(u32 index, u32 value);
		void set_value(u32 index, u32 value);
		void set_limit(u32 index, u32 limit);
		void refresh();
		void close();
		void enable_hold_to_skip();

		bool skip_requested() const { return *m_skip_requested; }

	private:
		// Shared with the overlay skip callback so a late callback cannot outlive the dialog object
		std::shared_ptr<atomic_t<bool>> m_skip_requested = std::make_shared<atomic_t<bool>>(false);
	};
}

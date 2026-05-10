#pragma once

#include "../overlays.h"
#include "../overlay_list_view.hpp"
#include "Emu/Cell/ErrorCodes.h"
#include "Emu/Cell/Modules/sceNp.h"

namespace rsx
{
	namespace overlays
	{
		struct sendmessage_dialog : public user_interface, public SendMessageDialogBase
		{
		private:
			struct list_entry : horizontal_layout
			{
			public:
				list_entry(const std::string& msg);
			};

			shared_mutex m_mutex;
			std::vector<std::string> m_entry_names;
			std::unique_ptr<overlay_element> m_dim_background;
			std::unique_ptr<list_view> m_list;
			std::unique_ptr<label> m_description;
			atomic_t<bool> m_open_confirmation_dialog = false;
			atomic_t<bool> m_confirmation_dialog_open = false;

			animation_color_interpolate fade_animation;

			std::string get_current_selection() const;
			void reload(const std::string& previous_selection);

		public:
			sendmessage_dialog();

			void update(u64 timestamp_us) override;
			void on_button_pressed(pad_button button_press, bool is_auto_repeat) override;

			compiled_resource get_compiled() override;

			error_code Exec(message_data& msg_data, std::set<std::string>& npids) override;
			void callback_handler(rpcn::NotificationType ntype, const std::string& username, bool status) override;
		};
	}
}

#pragma once

#include "../overlays.h"
#include "../overlay_list_view.hpp"
#include "../HomeMenu/overlay_home_menu_message_box.h"
#include "Emu/Cell/ErrorCodes.h"
#include "Emu/NP/rpcn_client.h"

namespace rsx
{
	namespace overlays
	{
		enum class friends_list_dialog_page
		{
			friends,
			invites,
			blocked
		};

		struct friends_list_dialog : public user_interface
		{
		private:
			struct friends_list_entry : horizontal_layout
			{
			private:
				std::unique_ptr<image_info> icon_data;

			public:
				friends_list_entry(friends_list_dialog_page page, const std::string& username, const rpcn::friend_online_data& data);
			};

			std::mutex m_list_mutex;
			std::vector<u32> m_entry_ids;
			std::unique_ptr<overlay_element> m_dim_background;
			std::unique_ptr<list_view> m_list;
			std::unique_ptr<label> m_description;
			image_button m_page_btn;
			image_button m_extra_btn;
			std::shared_ptr<home_menu_message_box> m_message_box;

			animation_color_interpolate fade_animation;

			std::shared_ptr<rpcn::rpcn_client> m_rpcn;
			rpcn::friend_data m_friend_data;
			atomic_t<bool> m_list_dirty { true };
			atomic_t<friends_list_dialog_page> m_current_page { friends_list_dialog_page::friends };
			atomic_t<friends_list_dialog_page> m_last_page { friends_list_dialog_page::friends };

			void reload();

		public:
			friends_list_dialog();

			void update(u64 timestamp_us) override;
			void on_button_pressed(pad_button button_press, bool is_auto_repeat) override;

			compiled_resource get_compiled() override;

			error_code show(bool enable_overlay, std::function<void(s32 status)> on_close);

			void callback_handler(rpcn::NotificationType ntype, const std::string& username, bool status);

			static bool rpcn_configured();
		};
	}
}

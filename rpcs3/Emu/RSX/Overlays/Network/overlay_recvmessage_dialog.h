#pragma once

#include "../overlays.h"
#include "../overlay_list_view.hpp"
#include "Emu/Cell/ErrorCodes.h"
#include "Emu/Cell/Modules/sceNp.h"

namespace rsx
{
	namespace overlays
	{
		struct recvmessage_dialog : public user_interface, public RecvMessageDialogBase
		{
		private:
			struct list_entry : horizontal_layout
			{
			public:
				list_entry(const std::string& name, const std::string& subj, const std::string& body);
			};

			shared_mutex m_mutex;
			std::vector<std::unique_ptr<overlay_element>> m_entries;
			std::vector<u64> m_entry_ids;
			std::unique_ptr<overlay_element> m_dim_background;
			std::unique_ptr<list_view> m_list;
			std::unique_ptr<label> m_description;

			animation_color_interpolate fade_animation;

		public:
			recvmessage_dialog();

			void update(u64 timestamp_us) override;
			void on_button_pressed(pad_button button_press, bool is_auto_repeat) override;

			compiled_resource get_compiled() override;

			error_code Exec(SceNpBasicMessageMainType type, SceNpBasicMessageRecvOptions options, SceNpBasicMessageRecvAction& recv_result, u64& chosen_msg_id) override;
			void callback_handler(const shared_ptr<std::pair<std::string, message_data>> new_msg, u64 msg_id) override;
		};
	}
}

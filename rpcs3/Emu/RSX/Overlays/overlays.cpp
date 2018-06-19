#include "stdafx.h"
#include "overlays.h"
#include "../GSRender.h"

namespace rsx
{
	namespace overlays
	{
		//Singleton instance declaration
		fontmgr* fontmgr::m_instance = nullptr;

		void user_interface::close()
		{
			//Force unload
			exit = true;
			if (auto manager = fxm::get<display_manager>())
			{
				if (auto dlg = manager->get<rsx::overlays::message_dialog>())
				{
					if (dlg->progress_bar_count())
						Emu.GetCallbacks().handle_taskbar_progress(0, 1);
				}

				manager->remove(uid);
			}

			if (on_close)
				on_close(return_code);
		}

		void overlay::refresh()
		{
			if (auto rsxthr = rsx::get_current_renderer())
			{
				rsxthr->native_ui_flip_request.store(true);
			}
		}
	} // namespace overlays
} // namespace rsx

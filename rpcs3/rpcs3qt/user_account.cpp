#include "user_account.h"

#include "Emu/system_utils.hpp"
#include "Utilities/File.h"
#include "util/logs.hpp"

LOG_CHANNEL(gui_log, "GUI");

user_account::user_account(const std::string& user_id)
{
	// Setting userId.
	m_user_id = user_id;

	// Setting userDir.
	m_user_dir = rpcs3::utils::get_hdd0_dir() + "home/" + m_user_id + "/";

	// Setting userName.
	if (fs::file file; file.open(m_user_dir + "localusername", fs::read))
	{
		m_username = file.to_string();
		file.close();

		if (m_username.length() > 16) // max of 16 chars on real PS3
		{
			m_username = m_username.substr(0, 16);
			gui_log.warning("user_account: localusername of userId=%s was too long, cropped to: %s", m_user_id, m_username);
		}
	}
	else
	{
		gui_log.error("user_account: localusername file read error (userId=%s, userDir=%s).", m_user_id, m_user_dir);
	}
}

std::map<u32, user_account> user_account::GetUserAccounts(const std::string& base_dir)
{
	std::map<u32, user_account> user_list;

	// I believe this gets the folder list sorted alphabetically by default,
	// but I can't find proof of this always being true.
	for (const auto& user_folder : fs::dir(base_dir))
	{
		if (!user_folder.is_directory)
		{
			continue;
		}

		// Is the folder name exactly 8 all-numerical characters long?
		const u32 key = rpcs3::utils::check_user(user_folder.name);

		if (key == 0)
		{
			continue;
		}

		// Does the localusername file exist?
		if (!fs::is_file(base_dir + "/" + user_folder.name + "/localusername"))
		{
			continue;
		}

		user_list.emplace(key, user_account(user_folder.name));
	}

	return user_list;
}

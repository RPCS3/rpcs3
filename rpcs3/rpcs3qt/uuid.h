#pragma once

namespace gui
{
	namespace utils
	{
		std::string get_uuid_path();
		std::string make_uuid();
		std::string load_uuid();

		bool validate_uuid(const std::string& uuid);
		bool save_uuid(const std::string& uuid);
		bool create_new_uuid(std::string& uuid);

		void log_uuid();
	}
}

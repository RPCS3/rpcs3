#include "stdafx.h"
#include "uuid.h"
#include "Utilities/StrUtil.h"

#include <QUuid>
#include <QRegularExpressionValidator>

LOG_CHANNEL(uuid_log, "UUID");

namespace gui
{
	namespace utils
	{
		std::string get_uuid_path()
		{
			const std::string config_dir = fs::get_config_dir(true);
			const std::string uuid_path = config_dir + "uuid";
#ifdef _WIN32
			if (!fs::create_path(config_dir))
			{
				uuid_log.error("Could not create path: %s (%s)", uuid_path, fs::g_tls_error);
			}
#endif
			return uuid_path;
		}

		std::string make_uuid()
		{
			return QUuid::createUuid().toString().toStdString();
		}

		std::string load_uuid()
		{
			const std::string uuid_path = get_uuid_path();

			if (!fs::is_file(uuid_path))
			{
				uuid_log.notice("File does not exist: %s (%s)", uuid_path, fs::g_tls_error);
				return {};
			}

			if (fs::file uuid_file = fs::file(uuid_path); uuid_file)
			{
				const std::string uuid = fmt::trim(uuid_file.to_string());

				if (!validate_uuid(uuid))
				{
					uuid_log.error("Invalid uuid '%s' found in file: %s", uuid, uuid_path);
					return {};
				}

				return uuid;
			}

			uuid_log.error("Could not open file: %s (%s)", uuid_path, fs::g_tls_error);
			return {};
		}

		bool validate_uuid(const std::string& uuid)
		{
			const QRegularExpressionValidator validator(QRegularExpression("^[a-fA-F0-9{}-]*$"));
			QString test_string = QString::fromStdString(uuid);
			int pos = 0;

			if (uuid.empty() || !uuid.starts_with("{") || !uuid.ends_with("}") || validator.validate(test_string, pos) == QValidator::State::Invalid)
			{
				return false;
			}

			return true;
		}

		bool save_uuid(const std::string& uuid)
		{
			if (!validate_uuid(uuid))
			{
				uuid_log.error("Can not save invalid uuid '%s'", uuid);
				return false;
			}

			const std::string uuid_path = get_uuid_path();

			if (fs::file uuid_file(uuid_path, fs::rewrite); !uuid_file || !uuid_file.write(uuid))
			{
				uuid_log.error("Could not write file: %s (%s)", uuid_path, fs::g_tls_error);
				return false;
			}

			uuid_log.notice("Wrote to file: %s", uuid_path);
			return true;
		}

		bool create_new_uuid(std::string& uuid)
		{
			uuid = make_uuid();

			if (uuid.empty())
			{
				uuid_log.error("Empty uuid");
				return false;
			}

			if (!save_uuid(uuid))
			{
				uuid_log.error("Failed to save uuid");
				return false;
			}

			return true;
		}

		void log_uuid()
		{
			std::string uuid = load_uuid();

			if (uuid.empty())
			{
				if (!create_new_uuid(uuid))
				{
					return;
				}
			}

			uuid_log.notice("Installation ID: %s", uuid);
		}
	}
}

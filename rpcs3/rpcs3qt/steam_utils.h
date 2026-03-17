#pragma once

#include "util/types.hpp"
#include "Utilities/StrFmt.h"
#include <variant>
#include <expected>
#include <QPixmap>

class iso_archive;

namespace gui::utils
{
	class steam_shortcut
	{
	public:
		steam_shortcut() {}
		~steam_shortcut() {}

		void add_shortcut(
			const std::string& app_name,
			const std::string& exe,
			const std::string& start_dir,
			const std::string& launch_options,
			const std::string& icon_path,
			const std::string& banner_small_path,
			const std::string& banner_large_path,
			std::shared_ptr<iso_archive> archive);

		void remove_shortcut(
			const std::string& app_name,
			const std::string& exe,
			const std::string& start_dir);

		bool write_file();

		static bool steam_installed();
		static bool is_steam_running();

	private:
		enum type_id
		{
			Null = 0x00,
			Start = Null,
			String = 0x01,
			Integer = 0x02,
			Float = 0x03,
			Pointer = 0x04,
			Nested = 0x05,
			Array = 0x06,
			Bool = 0x07,
			End = 0x08,
		};

		enum class steam_banner
		{
			cover,
			wide_cover,
			background,
			logo,
			icon
		};

		struct shortcut_entry
		{
			std::string app_name;
			std::string exe;
			std::string start_dir;
			std::string launch_options;
			std::string icon;
			std::string banner_small;
			std::string banner_large;
			u32 appid = 0;

			std::shared_ptr<iso_archive> iso;

			std::string build_binary_blob(usz index) const;
		};

		struct vdf_shortcut_entry
		{
			std::vector<std::pair<std::string, std::variant<u32, std::string>>> values;
			std::vector<std::pair<std::string, std::string>> tags;

			template <typename T>
			std::expected<T, std::string> value(const std::string& key) const
			{
				const auto it = std::find_if(values.cbegin(), values.cend(), [&key](const auto& v){ return v.first == key; });
				if (it == values.cend())
				{
					return std::unexpected(fmt::format("key '%s' not found", key));
				}

				if (const auto* p = std::get_if<T>(&it->second))
				{
					return *p;
				}

				return std::unexpected(fmt::format("value for key '%s' has wrong type", key));
			}

			std::expected<std::string, std::string> build_binary_blob(usz index) const;
		};

		bool parse_file(const std::string& path);

		static u32 crc32(const std::string& data);
		static u32 steam_appid(const std::string& exe, const std::string& name);

		static void append(std::string& s, const std::string& val);

		static std::string quote(const std::string& s, bool force);
		static std::string fix_slashes(const std::string& s);
		static std::string kv_string(const std::string& key, const std::string& value);
		static std::string kv_int(const std::string& key, u32 value);
		static std::string steamid64_to_32(const std::string& steam_id);
		static std::string get_steam_path();
		static std::string get_last_active_steam_user(const std::string& steam_path);
		
		static std::string get_steam_banner_path(steam_banner banner, const std::string& grid_dir, u32 appid);
		static void create_steam_banner(steam_banner banner, const std::string& src_path, const QPixmap& src_icon, const std::string& grid_dir, const shortcut_entry& entry);

		std::vector<shortcut_entry> m_entries_to_add;
		std::vector<shortcut_entry> m_entries_to_remove;
		std::vector<vdf_shortcut_entry> m_vdf_entries;
	};
}

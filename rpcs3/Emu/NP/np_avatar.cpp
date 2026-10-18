#include "stdafx.h"
#include "np_avatar.h"

#include "Crypto/key_vault.h"
#include "Crypto/unedat.h"
#include "Emu/system_utils.hpp"
#include "Emu/vfs_config.h"
#include "Emu/Cell/Modules/sceNp.h"
#include "Utilities/File.h"

#include "png.h"

#include <algorithm>
#include <cmath>
#include <map>
#include <mutex>

LOG_CHANNEL(avatar_log, "NpAvatar");

namespace np::avatar
{
	static constexpr std::string_view selection_file_name = "rpcs3_avatar";

	static constexpr u8 png_signature[8] = {0x89, 'P', 'N', 'G', 0x0D, 0x0A, 0x1A, 0x0A};

	struct rgba_image
	{
		u32 width = 0;
		u32 height = 0;
		std::vector<u8> pixels;
	};

	static std::mutex s_cache_mutex;
	static std::map<std::pair<std::string, s32>, std::vector<u8>> s_cache;

	u32 get_dimension(s32 size_type)
	{
		switch (size_type)
		{
		case SCE_NP_AVATAR_SIZE_LARGE: return 240;
		case SCE_NP_AVATAR_SIZE_MIDDLE: return 160;
		case SCE_NP_AVATAR_SIZE_SMALL: return 50;
		default: return 0;
		}
	}

	u32 get_max_size(s32 size_type)
	{
		switch (size_type)
		{
		case SCE_NP_AVATAR_SIZE_LARGE: return SCE_NET_NP_AVATAR_IMAGE_MAX_SIZE_LARGE;
		case SCE_NP_AVATAR_SIZE_MIDDLE: return SCE_NET_NP_AVATAR_IMAGE_MAX_SIZE_MIDDLE;
		case SCE_NP_AVATAR_SIZE_SMALL: return SCE_NET_NP_AVATAR_IMAGE_MAX_SIZE_SMALL;
		default: return 0;
		}
	}

	static std::string get_user_dir(const std::string& user_id)
	{
		return rpcs3::utils::get_hdd0_dir() + "home/" + user_id + "/";
	}

	static bool is_npd_file(const fs::file& file)
	{
		char magic[4]{};
		return file.read_at(0, magic, sizeof(magic)) == sizeof(magic) && std::memcmp(magic, "NPD\0", sizeof(magic)) == 0;
	}

	std::vector<u8> load_file(const std::string& path)
	{
		const fs::file file(path);

		if (!file)
		{
			avatar_log.error("Failed to open avatar file '%s' (%s)", path, fs::g_tls_error);
			return {};
		}

		std::vector<u8> data;

		if (is_npd_file(file))
		{
			u128 klicensee{};
			std::memcpy(&klicensee, NP_KLIC_FREE, sizeof(klicensee));

			EDATADecrypter decrypter(file, klicensee, path, false);

			if (decrypter.ReadHeader(false))
			{
				data.resize(decrypter.size());
			}

			if (data.empty() || decrypter.ReadData(0, data.data(), data.size(), false) != data.size())
			{
				avatar_log.error("Failed to decrypt avatar file '%s' (missing or wrong RAP license?)", path);
				return {};
			}
		}
		else
		{
			data = file.to_vector<u8>();
		}

		const auto search_end = data.begin() + std::min<usz>(data.size(), 0x400);
		const auto png_start = std::search(data.begin(), search_end, std::begin(png_signature), std::end(png_signature));

		if (png_start == search_end)
		{
			avatar_log.error("Avatar file '%s' does not contain a PNG image", path);
			return {};
		}

		data.erase(data.begin(), png_start);
		return data;
	}

	bool is_file_usable(const std::string& path)
	{
		const fs::file file(path);

		if (!file)
		{
			return false;
		}

		if (!is_npd_file(file))
		{
			u8 signature[sizeof(png_signature)]{};
			return file.read_at(0, signature, sizeof(signature)) == sizeof(signature) && std::memcmp(signature, png_signature, sizeof(signature)) == 0;
		}

		NPD_HEADER npd{};
		EDAT_HEADER edat{};
		read_npd_edat_header(&file, npd, edat);

		if ((edat.flags & SDAT_FLAG) || (npd.license & 0x3) == 0x3)
		{
			return true;
		}

		return fs::is_file(rpcs3::utils::get_rap_file_path(npd.get_content_id()));
	}

	std::vector<std::string> get_installed_avatars(const std::string& user_id)
	{
		std::vector<std::string> avatars;

		const std::string avatar_dir = get_user_dir(user_id) + "psn_avatar/";

		for (const auto& title_entry : fs::dir(avatar_dir))
		{
			if (!title_entry.is_directory || title_entry.name == "." || title_entry.name == "..")
			{
				continue;
			}

			const std::string title_dir = avatar_dir + title_entry.name + "/";

			for (const auto& entry : fs::dir(title_dir))
			{
				if (entry.is_directory || !entry.name.starts_with("PSNA_") || !fmt::to_lower(entry.name).ends_with(".edat"))
				{
					continue;
				}

				avatars.push_back(title_dir + entry.name);
			}
		}

		std::sort(avatars.begin(), avatars.end());
		return avatars;
	}

	std::string get_selected_avatar(const std::string& user_id)
	{
		const std::string user_dir = get_user_dir(user_id);

		const fs::file selection_file(user_dir + std::string(selection_file_name));

		// No selection file means the default avatar is used
		if (!selection_file)
		{
			return {};
		}

		std::string path = selection_file.to_string();

		while (!path.empty() && (path.back() == '\n' || path.back() == '\r' || path.back() == ' '))
		{
			path.pop_back();
		}

		if (path.empty())
		{
			return {};
		}

		if (fs::is_file(user_dir + path))
		{
			return user_dir + path;
		}

		return path;
	}

	bool set_selected_avatar(const std::string& user_id, const std::string& path)
	{
		const std::string user_dir = get_user_dir(user_id);
		const std::string selection_path = user_dir + std::string(selection_file_name);

		if (path.empty())
		{
			if (fs::is_file(selection_path) && !fs::remove_file(selection_path))
			{
				avatar_log.error("Failed to remove avatar selection of user %s (%s)", user_id, fs::g_tls_error);
				return false;
			}

			avatar_log.notice("User %s now uses the default avatar", user_id);
			return true;
		}

		std::string stored_path = path;

		if (stored_path.starts_with(user_dir))
		{
			stored_path = stored_path.substr(user_dir.size());
		}

		if (!fs::write_file(selection_path, fs::rewrite, stored_path))
		{
			avatar_log.error("Failed to store avatar selection of user %s (%s)", user_id, fs::g_tls_error);
			return false;
		}

		avatar_log.notice("User %s now uses the avatar '%s'", user_id, path);
		return true;
	}

	static bool decode_png(const std::vector<u8>& data, rgba_image& out)
	{
		png_image image{};
		image.version = PNG_IMAGE_VERSION;

		if (!png_image_begin_read_from_memory(&image, data.data(), data.size()))
		{
			avatar_log.error("Failed to read avatar PNG header: %s", image.message);
			return false;
		}

		image.format = PNG_FORMAT_RGBA;
		out.width = image.width;
		out.height = image.height;
		out.pixels.resize(PNG_IMAGE_SIZE(image));

		if (!png_image_finish_read(&image, nullptr, out.pixels.data(), 0, nullptr))
		{
			avatar_log.error("Failed to decode avatar PNG: %s", image.message);
			png_image_free(&image);
			return false;
		}

		return out.width && out.height;
	}

	static std::vector<u8> encode_png(const rgba_image& in)
	{
		png_image image{};
		image.version = PNG_IMAGE_VERSION;
		image.width = in.width;
		image.height = in.height;
		image.format = PNG_FORMAT_RGBA;

		png_alloc_size_t size = 0;

		if (!png_image_write_get_memory_size(image, size, 0, in.pixels.data(), 0, nullptr))
		{
			avatar_log.error("Failed to encode avatar PNG: %s", image.message);
			return {};
		}

		std::vector<u8> out(size);

		if (!png_image_write_to_memory(&image, out.data(), &size, 0, in.pixels.data(), 0, nullptr))
		{
			avatar_log.error("Failed to encode avatar PNG: %s", image.message);
			return {};
		}

		out.resize(size);
		return out;
	}

	static rgba_image resize_square(const rgba_image& src, u32 dim)
	{
		const u32 side = std::min(src.width, src.height);
		const double src_x = (src.width - side) / 2.0;
		const double src_y = (src.height - side) / 2.0;
		const double scale = static_cast<double>(side) / dim;

		rgba_image dst;
		dst.width = dim;
		dst.height = dim;
		dst.pixels.resize(dim * dim * 4);

		for (u32 dy = 0; dy < dim; dy++)
		{
			const double y0 = src_y + dy * scale;
			const double y1 = y0 + scale;

			for (u32 dx = 0; dx < dim; dx++)
			{
				const double x0 = src_x + dx * scale;
				const double x1 = x0 + scale;

				double acc[4]{};
				double area = 0.;

				for (u32 sy = static_cast<u32>(y0); sy < std::min<u32>(static_cast<u32>(std::ceil(y1)), src.height); sy++)
				{
					const double wy = std::min<double>(y1, sy + 1) - std::max<double>(y0, sy);

					for (u32 sx = static_cast<u32>(x0); sx < std::min<u32>(static_cast<u32>(std::ceil(x1)), src.width); sx++)
					{
						const double w = wy * (std::min<double>(x1, sx + 1) - std::max<double>(x0, sx));
						const u8* p = &src.pixels[(sy * src.width + sx) * 4];
						const double alpha_w = w * p[3];

						acc[0] += p[0] * alpha_w;
						acc[1] += p[1] * alpha_w;
						acc[2] += p[2] * alpha_w;
						acc[3] += alpha_w;
						area += w;
					}
				}

				u8* out = &dst.pixels[(dy * dim + dx) * 4];

				if (area <= 0. || acc[3] <= 0.)
				{
					std::memset(out, 0, 4);
					continue;
				}

				for (int c = 0; c < 3; c++)
				{
					out[c] = static_cast<u8>(std::clamp(std::lround(acc[c] / acc[3]), 0l, 255l));
				}

				out[3] = static_cast<u8>(std::clamp(std::lround(acc[3] / area), 0l, 255l));
			}
		}

		return dst;
	}

	// Converts a PNG to the format of a size type
	static std::vector<u8> convert_image(const std::vector<u8>& png, s32 size_type)
	{
		const u32 dim = get_dimension(size_type);
		const u32 max_size = get_max_size(size_type);

		rgba_image image;

		if (!dim || png.empty() || !decode_png(png, image))
		{
			return {};
		}

		if (image.width == dim && image.height == dim && png.size() <= max_size)
		{
			return png;
		}

		if (image.width != dim || image.height != dim)
		{
			image = resize_square(image, dim);
		}

		std::vector<u8> result = encode_png(image);

		for (u8 mask : {0xfe, 0xfc, 0xf8, 0xf0, 0xe0})
		{
			if (result.empty() || result.size() <= max_size)
			{
				break;
			}

			rgba_image reduced = image;

			for (u8& value : reduced.pixels)
			{
				value &= mask;
			}

			result = encode_png(reduced);
		}

		if (result.size() > max_size)
		{
			avatar_log.error("Avatar image of size type %d is too large (%d > %d)", size_type, result.size(), max_size);
			return {};
		}

		return result;
	}

	std::string get_default_avatar_path(const std::string& user_id)
	{
		const u32 index = user_id.empty() ? 0 : rpcs3::utils::check_user(user_id) % 9;
		return g_cfg_vfs.get_dev_flash() + fmt::format("vsh/resource/explore/user/%03d.png", index);
	}

	std::vector<u8> get_default_image(const std::string& user_id, s32 size_type)
	{
		if (!get_dimension(size_type))
		{
			return {};
		}

		const std::string path = get_default_avatar_path(user_id);

		{
			std::lock_guard lock(s_cache_mutex);

			if (const auto found = s_cache.find({path, size_type}); found != s_cache.end())
			{
				return found->second;
			}
		}

		std::vector<u8> image;

		if (fs::file file{path})
		{
			image = convert_image(file.to_vector<u8>(), size_type);
		}
		else
		{
			avatar_log.error("Default avatar '%s' not found, is the firmware installed?", path);
		}

		std::lock_guard lock(s_cache_mutex);
		s_cache[{path, size_type}] = image;
		return image;
	}

	std::vector<u8> get_user_image(const std::string& user_id, s32 size_type)
	{
		if (!get_dimension(size_type))
		{
			return {};
		}

		if (const std::string path = get_selected_avatar(user_id); !path.empty())
		{
			{
				std::lock_guard lock(s_cache_mutex);

				if (const auto found = s_cache.find({path, size_type}); found != s_cache.end())
				{
					return found->second;
				}
			}

			std::vector<u8> image = convert_image(load_file(path), size_type);

			if (image.empty())
			{
				avatar_log.error("Failed to load the avatar '%s' of user %s, using the default avatar", path, user_id);
				image = get_default_image(user_id, size_type);
			}

			std::lock_guard lock(s_cache_mutex);
			s_cache[{path, size_type}] = image;
			return image;
		}

		return get_default_image(user_id, size_type);
	}
}

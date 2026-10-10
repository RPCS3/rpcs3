// PS5: screenshots (ps5_screenshots.h).

#include "stdafx.h"
#include "ps5_screenshots.h"

#include "Utilities/File.h"

#include <algorithm>
#include <ctime>
#include <mutex>
#include <thread>
#include <sys/stat.h>

// PNG: stb_image_write, its own copy here (VKFrameGen.cpp has another)
#define STB_IMAGE_WRITE_IMPLEMENTATION
#define STB_IMAGE_WRITE_STATIC
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wall"
#pragma GCC diagnostic ignored "-Wextra"
#pragma GCC diagnostic ignored "-Wold-style-cast"
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
#pragma GCC diagnostic ignored "-Wunused-function"
#pragma GCC diagnostic ignored "-Wimplicit-fallthrough"
#include <stb_image_write.h>
#pragma GCC diagnostic pop

LOG_CHANNEL(shot_log, "PauseMenu");

extern atomic_t<bool> g_user_asked_for_screenshot; // RSXThread.cpp: the next game frame read back (VKPresent.cpp)

namespace ps5_screenshots
{
	namespace
	{
		std::mutex g_mutex;
		bool g_waiting = false; // asked for, to hold
		std::vector<u8> g_pixels;
		u32 g_width = 0;
		u32 g_height = 0;
		bool g_bgra = false;

		std::string time_text(std::time_t when)
		{
			std::tm parts{};
			if (!localtime_r(&when, &parts) && !gmtime_r(&when, &parts))
			{
				return "unknown";
			}
			return fmt::format("%04d-%02d-%02d_%02d-%02d-%02d", parts.tm_year + 1900, parts.tm_mon + 1, parts.tm_mday, parts.tm_hour, parts.tm_min, parts.tm_sec);
		}
	}

	void hold_next()
	{
		{
			std::lock_guard lock(g_mutex);
			g_waiting = true;
			g_pixels.clear();
		}
		g_user_asked_for_screenshot = true;
	}

	void take(std::vector<u8>&& pixels, u32 width, u32 height, bool bgra)
	{
		std::lock_guard lock(g_mutex);
		if (!g_waiting || pixels.size() < usz{width} * height * 4)
		{
			return;
		}
		g_waiting = false;
		g_pixels = std::move(pixels);
		g_width = width;
		g_height = height;
		g_bgra = bgra;
	}

	bool held()
	{
		std::lock_guard lock(g_mutex);
		return !g_pixels.empty();
	}

	void drop_held()
	{
		std::lock_guard lock(g_mutex);
		g_waiting = false;
		g_pixels.clear();
		g_pixels.shrink_to_fit();
	}

	std::string folder()
	{
		return fs::get_config_dir() + "screenshots/";
	}

	std::string save_held(const std::string& serial)
	{
		std::vector<u8> pixels;
		u32 width = 0, height = 0;
		bool bgra = false;
		{
			std::lock_guard lock(g_mutex);
			if (g_pixels.empty())
			{
				return {};
			}
			pixels = g_pixels; // kept, for another save of the same moment to find it there
			width = g_width;
			height = g_height;
			bgra = g_bgra;
		}

		const std::string name = serial.empty() ? std::string("unknown") : serial;
		const std::string dir = folder() + name + "/";
		const std::string path = dir + name + "_" + time_text(std::time(nullptr)) + ".png";

		// PNG takes a while: on a thread of its own, as RGB (the alpha the frame
		// carries is not meant to be seen)
		std::thread([dir, path, pixels = std::move(pixels), width, height, bgra]()
		{
			std::vector<u8> rgb(usz{width} * height * 3);
			for (usz i = 0; i < usz{width} * height; i++)
			{
				rgb[i * 3 + 0] = pixels[i * 4 + (bgra ? 2 : 0)];
				rgb[i * 3 + 1] = pixels[i * 4 + 1];
				rgb[i * 3 + 2] = pixels[i * 4 + (bgra ? 0 : 2)];
			}
			std::vector<u8> out;
			stbi_write_png_to_func([](void* context, void* bytes, int size)
			{
				auto& sink = *static_cast<std::vector<u8>*>(context);
				sink.insert(sink.end(), static_cast<u8*>(bytes), static_cast<u8*>(bytes) + size);
			}, &out, static_cast<int>(width), static_cast<int>(height), 3, rgb.data(), static_cast<int>(width * 3));

			// Readable and writable over FTP
			for (const std::string& each : {folder(), dir})
			{
				fs::create_path(each);
				::chmod(each.c_str(), 0777);
			}
			if (!fs::write_file(path, fs::rewrite, out.data(), out.size()))
			{
				shot_log.error("Screenshot: could not write %s: %s", path, fs::g_tls_error);
				return;
			}
			::chmod(path.c_str(), 0666);
			shot_log.notice("Screenshot: %ux%u, %u KiB, written to %s", width, height, out.size() / 1024, path);
		}).detach();
		return path;
	}

	std::vector<shot> list()
	{
		std::vector<shot> result;
		for (const auto& game : fs::dir(folder()))
		{
			if (!game.is_directory || game.name == "." || game.name == "..")
			{
				continue;
			}
			for (const auto& file : fs::dir(folder() + game.name))
			{
				if (file.is_directory || !file.name.ends_with(".png"))
				{
					continue;
				}
				result.push_back({folder() + game.name + "/" + file.name, game.name, file.mtime});
			}
		}
		std::sort(result.begin(), result.end(), [](const shot& a, const shot& b) { return a.time != b.time ? a.time > b.time : a.path > b.path; });
		return result;
	}
}

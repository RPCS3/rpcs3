#include "stdafx.h"
#include "main_application.h"
#include "display_sleep_control.h"
#include "gamemode_control.h"

#include "util/types.hpp"
#include "util/logs.hpp"
#include "util/sysinfo.hpp"

#include "Utilities/Thread.h"
#include "Utilities/File.h"
#include "Input/pad_thread.h"
#include "Emu/System.h"
#include "Emu/system_config.h"
#include "Emu/system_utils.hpp"
#include "Emu/IdManager.h"
#include "Emu/Io/Null/NullKeyboardHandler.h"
#include "Emu/Io/Null/NullMouseHandler.h"
#include "Emu/Io/KeyboardHandler.h"
#include "Emu/Io/MouseHandler.h"
#include "Input/basic_keyboard_handler.h"
#include "Input/basic_mouse_handler.h"
#include "Input/raw_mouse_handler.h"

#include "Emu/Audio/AudioBackend.h"
#include "Emu/Audio/Null/NullAudioBackend.h"
#include "Emu/Audio/Null/null_enumerator.h"
#include "Emu/Audio/Cubeb/CubebBackend.h"
#include "Emu/Audio/Cubeb/cubeb_enumerator.h"
#ifdef _WIN32
#include "Emu/Audio/XAudio2/XAudio2Backend.h"
#include "Emu/Audio/XAudio2/xaudio2_enumerator.h"
#endif
#ifdef HAVE_FAUDIO
#include "Emu/Audio/FAudio/FAudioBackend.h"
#include "Emu/Audio/FAudio/faudio_enumerator.h"
#endif

#include <QFileInfo> // This shouldn't be outside rpcs3qt...
#include <QImageReader> // This shouldn't be outside rpcs3qt...
#include <QStandardPaths> // This shouldn't be outside rpcs3qt...
#include <thread>

LOG_CHANNEL(sys_log, "SYS");

namespace audio
{
	extern void configure_audio(bool force_reset = false);
	extern void configure_rsxaudio();
}

namespace rsx::overlays
{
	extern void reset_performance_overlay();
	extern void reset_debug_overlay();
}

extern void qt_events_aware_op(int repeat_duration_ms, std::function<bool()> wrapped_op);

/** Emu.Init() wrapper for user management */
void main_application::InitializeEmulator(const std::string& user, bool show_gui)
{
	Emu.SetHasGui(show_gui);
	Emu.SetUsr(user);
	Emu.Init();

	// Log Firmware Version after Emu was initialized
	const std::string firmware_version = utils::get_firmware_version();
	const std::string firmware_string  = firmware_version.empty() ? "Missing Firmware" : ("Firmware version: " + firmware_version);
	sys_log.always()("%s", firmware_string);
}

void main_application::OnEmuSettingsChange()
{
	if (Emu.IsRunning())
	{
		enable_display_sleep(!g_cfg.misc.prevent_display_sleep);
	}

	if (!Emu.IsStopped())
	{
		// Change logging (only allowed during gameplay)
		rpcs3::utils::configure_logs();

		// Force audio provider
		g_cfg.audio.provider.set(Emu.IsVsh() ? audio_provider::rsxaudio : audio_provider::cell_audio);
	}

	audio::configure_audio();
	audio::configure_rsxaudio();
	rsx::overlays::reset_performance_overlay();
	rsx::overlays::reset_debug_overlay();
}

/** RPCS3 emulator has functions it desires to call from the GUI at times. Initialize them in here. */
EmuCallbacks main_application::CreateCallbacks()
{
	EmuCallbacks callbacks{};

	callbacks.update_emu_settings = [this]()
	{
		Emu.CallFromMainThread([&]()
		{
			OnEmuSettingsChange();
		});
	};

	callbacks.save_emu_settings = [this]()
	{
		Emu.BlockingCallFromMainThread([&]()
		{
			Emulator::SaveSettings(g_cfg.to_string(), Emu.GetTitleID());
		});
	};

	callbacks.init_kb_handler = [this]()
	{
		switch (g_cfg.io.keyboard.get())
		{
		case keyboard_handler::null:
		{
			ensure(g_fxo->init<KeyboardHandlerBase, NullKeyboardHandler>(Emu.DeserialManager()));
			break;
		}
		case keyboard_handler::basic:
		{
			basic_keyboard_handler* ret = g_fxo->init<KeyboardHandlerBase, basic_keyboard_handler>(Emu.DeserialManager());
			ensure(ret);
			ret->moveToThread(get_thread());
			ret->SetTargetWindow(reinterpret_cast<QWindow*>(m_game_window));
			break;
		}
		}
	};

	callbacks.init_mouse_handler = [this]()
	{
		mouse_handler handler = g_cfg.io.mouse;

		if (handler == mouse_handler::null)
		{
			switch (g_cfg.io.move)
			{
			case move_handler::mouse:
				handler = mouse_handler::basic;
				break;
			case move_handler::raw_mouse:
				handler = mouse_handler::raw;
				break;
			default:
				break;
			}
		}

		switch (handler)
		{
		case mouse_handler::null:
		{
			ensure(g_fxo->init<MouseHandlerBase, NullMouseHandler>(Emu.DeserialManager()));
			break;
		}
		case mouse_handler::basic:
		{
			basic_mouse_handler* ret = g_fxo->init<MouseHandlerBase, basic_mouse_handler>(Emu.DeserialManager());
			ensure(ret);
			ret->moveToThread(get_thread());
			ret->SetTargetWindow(reinterpret_cast<QWindow*>(m_game_window));
			break;
		}
		case mouse_handler::raw:
		{
			ensure(g_fxo->init<MouseHandlerBase, raw_mouse_handler>(Emu.DeserialManager()));
			break;
		}
		}
	};

	callbacks.init_pad_handler = [this](std::string_view title_id)
	{
		ensure(g_fxo->init<named_thread<pad_thread>>(get_thread(), m_game_window, title_id));

		qt_events_aware_op(0, [](){ return !!pad::g_started; });
	};

	callbacks.get_audio = []() -> std::shared_ptr<AudioBackend>
	{
		std::shared_ptr<AudioBackend> result;
		switch (g_cfg.audio.renderer.get())
		{
		case audio_renderer::null: result = std::make_shared<NullAudioBackend>(); break;
#ifdef _WIN32
		case audio_renderer::xaudio: result = std::make_shared<XAudio2Backend>(); break;
#endif
		case audio_renderer::cubeb: result = std::make_shared<CubebBackend>(); break;
#ifdef HAVE_FAUDIO
		case audio_renderer::faudio: result = std::make_shared<FAudioBackend>(); break;
#endif
		}

		if (!result->Initialized())
		{
			// Fall back to a null backend if something went wrong
			sys_log.error("Audio renderer %s could not be initialized, using a Null renderer instead. Make sure that no other application is running that might block audio access (e.g. Netflix).", result->GetName());
			result = std::make_shared<NullAudioBackend>();
		}
		return result;
	};

	callbacks.get_audio_enumerator = [](u64 renderer) -> std::shared_ptr<audio_device_enumerator>
	{
		switch (static_cast<audio_renderer>(renderer))
		{
		case audio_renderer::null: return std::make_shared<null_enumerator>();
#ifdef _WIN32
		case audio_renderer::xaudio: return std::make_shared<xaudio2_enumerator>();
#endif
		case audio_renderer::cubeb: return std::make_shared<cubeb_enumerator>();
#ifdef HAVE_FAUDIO
		case audio_renderer::faudio: return std::make_shared<faudio_enumerator>();
#endif
		default: fmt::throw_exception("Invalid renderer index %u", renderer);
		}
	};

	callbacks.get_image_info = [](const std::string& filename, std::string& sub_type, s32& width, s32& height, s32& orientation) -> bool
	{
		sub_type.clear();
		width = 0;
		height = 0;
		orientation = 0; // CELL_SEARCH_ORIENTATION_UNKNOWN

		bool success = false;
		Emu.BlockingCallFromMainThread([&]()
		{
			const QImageReader reader(QString::fromStdString(filename));
			if (reader.canRead())
			{
				const QSize size = reader.size();
				width = size.width();
				height = size.height();
				sub_type = reader.subType().toStdString();

				switch (reader.transformation())
				{
				case QImageIOHandler::Transformation::TransformationNone:
					orientation = 1; // CELL_SEARCH_ORIENTATION_TOP_LEFT = 0°
					break;
				case QImageIOHandler::Transformation::TransformationRotate90:
					orientation = 2; // CELL_SEARCH_ORIENTATION_TOP_RIGHT = 90°
					break;
				case QImageIOHandler::Transformation::TransformationRotate180:
					orientation = 3; // CELL_SEARCH_ORIENTATION_BOTTOM_RIGHT = 180°
					break;
				case QImageIOHandler::Transformation::TransformationRotate270:
					orientation = 4; // CELL_SEARCH_ORIENTATION_BOTTOM_LEFT = 270°
					break;
				default:
					// Ignore other transformations for now
					break;
				}

				success = true;
				sys_log.notice("get_image_info found image: filename='%s', sub_type='%s', width=%d, height=%d, orientation=%d", filename, sub_type, width, height, orientation);
			}
			else
			{
				sys_log.error("get_image_info failed to read '%s'. Error='%s'", filename, reader.errorString());
			}
		});
		return success;
	};

	callbacks.get_scaled_image = [](const std::string& path, s32 target_width, s32 target_height, s32& width, s32& height, u8* dst, bool force_fit) -> bool
	{
		width = 0;
		height = 0;

		if (target_width <= 0 || target_height <= 0 || !dst || !fs::is_file(path))
		{
			return false;
		}

		bool success = false;
		Emu.BlockingCallFromMainThread([&]()
		{
			// We use QImageReader instead of QImage. This way we can load and scale image in one step.
			QImageReader reader(QString::fromStdString(path));

			if (reader.canRead())
			{
				QSize size = reader.size();
				width = size.width();
				height = size.height();

				if (width <= 0 || height <= 0)
				{
					return;
				}

				if (force_fit || width > target_width || height > target_height)
				{
					const f32 target_ratio = target_width / static_cast<f32>(target_height);
					const f32 image_ratio = width / static_cast<f32>(height);
					const f32 convert_ratio = image_ratio / target_ratio;

					if (convert_ratio > 1.0f)
					{
						size = QSize(target_width, target_height / convert_ratio);
					}
					else if (convert_ratio < 1.0f)
					{
						size = QSize(target_width * convert_ratio, target_height);
					}
					else
					{
						size = QSize(target_width, target_height);
					}

					reader.setScaledSize(size);
					width = size.width();
					height = size.height();
				}

				QImage image = reader.read();

				if (image.format() != QImage::Format::Format_RGBA8888)
				{
					image = image.convertToFormat(QImage::Format::Format_RGBA8888);
				}

				std::memcpy(dst, image.constBits(), std::min(target_width * target_height * 4LL, image.height() * image.bytesPerLine()));
				success = true;
				sys_log.notice("get_scaled_image scaled image: path='%s', width=%d, height=%d", path, width, height);
			}
			else
			{
				sys_log.error("get_scaled_image failed to read '%s'. Error='%s'", path, reader.errorString());
			}
		});
		return success;
	};

	callbacks.resolve_path = [](std::string_view sv)
	{
		// May result in an empty string if path does not exist
		return QFileInfo(QString::fromUtf8(sv.data(), static_cast<int>(sv.size()))).canonicalFilePath().toStdString();
	};

	callbacks.get_font_dirs = []()
	{
		const QStringList locations = QStandardPaths::standardLocations(QStandardPaths::FontsLocation);
		std::vector<std::string> font_dirs;
		for (const QString& location : locations)
		{
			std::string font_dir = location.toStdString();
			if (!font_dir.ends_with('/'))
			{
				font_dir += '/';
			}
			font_dirs.push_back(font_dir);
		}
		return font_dirs;
	};

	callbacks.on_install_pkgs = [](const std::vector<std::string>& pkgs)
	{
		for (const std::string& pkg : pkgs)
		{
			if (!rpcs3::utils::install_pkg(pkg))
			{
				sys_log.error("Failed to install %s", pkg);
				return false;
			}
		}
		return true;
	};

	callbacks.enable_gamemode = [](bool enabled){ enable_gamemode(enabled); };

	return callbacks;
}

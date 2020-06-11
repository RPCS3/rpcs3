#include "emu_settings.h"
#include "config_adapter.h"

#include <QMessageBox>
#include <QLineEdit>

#include "Emu/System.h"
#include "Emu/system_config.h"

LOG_CHANNEL(cfg_log, "CFG");

extern std::string g_cfg_defaults; //! Default settings grabbed from Utilities/Config.h

inline std::string sstr(const QString& _in) { return _in.toStdString(); }
inline std::string sstr(const QVariant& _in) { return sstr(_in.toString()); }

// Emit sorted YAML
namespace
{
	static NEVER_INLINE void emitData(YAML::Emitter& out, const YAML::Node& node)
	{
		// TODO
		out << node;
	}

	// Incrementally load YAML
	static NEVER_INLINE void operator +=(YAML::Node& left, const YAML::Node& node)
	{
		if (node && !node.IsNull())
		{
			if (node.IsMap())
			{
				for (const auto& pair : node)
				{
					if (pair.first.IsScalar())
					{
						auto&& lhs = left[pair.first.Scalar()];
						lhs += pair.second;
					}
					else
					{
						// Exotic case (TODO: probably doesn't work)
						auto&& lhs = left[YAML::Clone(pair.first)];
						lhs += pair.second;
					}
				}
			}
			else if (node.IsScalar() || node.IsSequence())
			{
				// Scalars and sequences are replaced completely, but this may change in future.
				// This logic may be overwritten by custom demands of every specific cfg:: node.
				left = node;
			}
		}
	}
}

emu_settings::emu_settings()
	: QObject()
	, m_render_creator(new render_creator(this))
{
}

emu_settings::~emu_settings()
{
}

void emu_settings::LoadSettings(const std::string& title_id)
{
	m_title_id = title_id;

	// Create config path if necessary
	fs::create_path(title_id.empty() ? fs::get_config_dir() : Emulator::GetCustomConfigDir());

	// Load default config
	m_defaultSettings = YAML::Load(g_cfg_defaults);
	m_currentSettings = YAML::Load(g_cfg_defaults);

	// Add global config
	fs::file config(fs::get_config_dir() + "/config.yml", fs::read + fs::write + fs::create);
	m_currentSettings += YAML::Load(config.to_string());
	config.close();

	// Add game config
	if (!title_id.empty())
	{
		const std::string config_path_new = Emulator::GetCustomConfigPath(m_title_id);
		const std::string config_path_old = Emulator::GetCustomConfigPath(m_title_id, true);

		if (fs::is_file(config_path_new))
		{
			config = fs::file(config_path_new, fs::read + fs::write);
			m_currentSettings += YAML::Load(config.to_string());
			config.close();
		}
		else if (fs::is_file(config_path_old))
		{
			config = fs::file(config_path_old, fs::read + fs::write);
			m_currentSettings += YAML::Load(config.to_string());
			config.close();
		}
	}
}

void emu_settings::SaveSettings()
{
	fs::file config;
	YAML::Emitter out;
	emitData(out, m_currentSettings);

	std::string config_name;

	if (m_title_id.empty())
	{
		config_name = fs::get_config_dir() + "/config.yml";
	}
	else
	{
		config_name = Emulator::GetCustomConfigPath(m_title_id);
	}

	config = fs::file(config_name, fs::read + fs::write + fs::create);

	// Save config
	config.seek(0);
	config.trunc(0);
	config.write(out.c_str(), out.size());

	// Check if the running config/title is the same as the edited config/title.
	if (config_name == g_cfg.name || m_title_id == Emu.GetTitleID())
	{
		// Update current config
		g_cfg.from_string(config.to_string(), !Emu.IsStopped());

		if (!Emu.IsStopped()) // Don't spam the log while emulation is stopped. The config will be logged on boot anyway.
		{
			cfg_log.notice("Updated configuration:\n%s\n", g_cfg.to_string());
		}
	}

	config.close();
}

void emu_settings::EnhanceComboBox(QComboBox* combobox, emu_settings_type type, bool is_ranged, bool use_max, int max, bool sorted)
{
	if (!combobox)
	{
		cfg_log.fatal("EnhanceComboBox '%s' was used with an invalid object", cfg_adapter::get_setting_name(type));
		return;
	}

	if (is_ranged)
	{
		if (sorted)
		{
			cfg_log.warning("EnhanceCombobox '%s': ignoring sorting request on ranged combo box", cfg_adapter::get_setting_name(type));
		}

		const QStringList range = GetSettingOptions(type);

		const int max_item = use_max ? max : range.last().toInt();

		for (int i = range.first().toInt(); i <= max_item; i++)
		{
			combobox->addItem(QString::number(i), i);
		}
	}
	else
	{
		const QStringList settings = GetSettingOptions(type);

		for (int i = 0; i < settings.count(); i++)
		{
			const QString localized_setting = GetLocalizedSetting(settings[i], type, combobox->count());
			combobox->addItem(localized_setting, QVariant({settings[i], i}));
		}

		if (sorted)
		{
			combobox->model()->sort(0, Qt::AscendingOrder);
		}
	}

	const std::string selected = GetSetting(type);
	const QString selected_q = qstr(selected);
	int index = -1;

	if (is_ranged)
	{
		index = combobox->findData(selected_q);
	}
	else
	{
		for (int i = 0; i < combobox->count(); i++)
		{
			const QVariantList var_list = combobox->itemData(i).toList();
			ASSERT(var_list.size() == 2 && var_list[0].canConvert<QString>());

			if (selected_q == var_list[0].toString())
			{
				index = i;
				break;
			}
		}
	}

	if (index == -1)
	{
		const std::string def = GetSettingDefault(type);
		cfg_log.fatal("EnhanceComboBox '%s' tried to set an invalid value: %s. Setting to default: %s", cfg_adapter::get_setting_name(type), selected, def);
		combobox->setCurrentIndex(combobox->findData(qstr(def)));
		m_broken_types.insert(type);
	}
	else
	{
		combobox->setCurrentIndex(index);
	}

	connect(combobox, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged), [=, this](int index)
	{
		if (is_ranged)
		{
			SetSetting(type, sstr(combobox->itemData(index)));
		}
		else
		{
			const QVariantList var_list = combobox->itemData(index).toList();
			ASSERT(var_list.size() == 2 && var_list[0].canConvert<QString>());
			SetSetting(type, sstr(var_list[0]));
		}
	});
}

void emu_settings::EnhanceCheckBox(QCheckBox* checkbox, emu_settings_type type)
{
	if (!checkbox)
	{
		cfg_log.fatal("EnhanceCheckBox '%s' was used with an invalid object", cfg_adapter::get_setting_name(type));
		return;
	}

	std::string def = GetSettingDefault(type);
	std::transform(def.begin(), def.end(), def.begin(), ::tolower);

	if (def != "true" && def != "false")
	{
		cfg_log.fatal("EnhanceCheckBox '%s' was used with an invalid emu_settings_type", cfg_adapter::get_setting_name(type));
		return;
	}

	std::string selected = GetSetting(type);
	std::transform(selected.begin(), selected.end(), selected.begin(), ::tolower);

	if (selected == "true")
	{
		checkbox->setChecked(true);
	}
	else if (selected != "false")
	{
		cfg_log.fatal("EnhanceCheckBox '%s' tried to set an invalid value: %s. Setting to default: %s", cfg_adapter::get_setting_name(type), selected, def);
		checkbox->setChecked(def == "true");
		m_broken_types.insert(type);
	}

	connect(checkbox, &QCheckBox::stateChanged, [type, this](int val)
	{
		const std::string str = val != 0 ? "true" : "false";
		SetSetting(type, str);
	});
}

void emu_settings::EnhanceSlider(QSlider* slider, emu_settings_type type)
{
	if (!slider)
	{
		cfg_log.fatal("EnhanceSlider '%s' was used with an invalid object", cfg_adapter::get_setting_name(type));
		return;
	}

	const QStringList range = GetSettingOptions(type);
	bool ok_def, ok_sel, ok_min, ok_max;

	const int def = qstr(GetSettingDefault(type)).toInt(&ok_def);
	const int min = range.first().toInt(&ok_min);
	const int max = range.last().toInt(&ok_max);

	if (!ok_def || !ok_min || !ok_max)
	{
		cfg_log.fatal("EnhanceSlider '%s' was used with an invalid emu_settings_type", cfg_adapter::get_setting_name(type));
		return;
	}

	const QString selected = qstr(GetSetting(type));
	int val = selected.toInt(&ok_sel);

	if (!ok_sel || val < min || val > max)
	{
		cfg_log.fatal("EnhanceSlider '%s' tried to set an invalid value: %d. Setting to default: %d. Allowed range: [%d, %d]", cfg_adapter::get_setting_name(type), val, def, min, max);
		val = def;
		m_broken_types.insert(type);
	}

	slider->setRange(min, max);
	slider->setValue(val);

	connect(slider, &QSlider::valueChanged, [type, this](int value)
	{
		SetSetting(type, sstr(value));
	});
}

void emu_settings::EnhanceSpinBox(QSpinBox* spinbox, emu_settings_type type, const QString& prefix, const QString& suffix)
{
	if (!spinbox)
	{
		cfg_log.fatal("EnhanceSpinBox '%s' was used with an invalid object", cfg_adapter::get_setting_name(type));
		return;
	}

	const QStringList range = GetSettingOptions(type);
	bool ok_def, ok_sel, ok_min, ok_max;

	const int def = qstr(GetSettingDefault(type)).toInt(&ok_def);
	const int min = range.first().toInt(&ok_min);
	const int max = range.last().toInt(&ok_max);

	if (!ok_def || !ok_min || !ok_max)
	{
		cfg_log.fatal("EnhanceSpinBox '%s' was used with an invalid type", cfg_adapter::get_setting_name(type));
		return;
	}

	const std::string selected = GetSetting(type);
	int val = qstr(selected).toInt(&ok_sel);

	if (!ok_sel || val < min || val > max)
	{
		cfg_log.fatal("EnhanceSpinBox '%s' tried to set an invalid value: %d. Setting to default: %d. Allowed range: [%d, %d]", cfg_adapter::get_setting_name(type), selected, def, min, max);
		val = def;
		m_broken_types.insert(type);
	}

	spinbox->setPrefix(prefix);
	spinbox->setSuffix(suffix);
	spinbox->setRange(min, max);
	spinbox->setValue(val);

	connect(spinbox, &QSpinBox::textChanged, [=, this](const QString&/* text*/)
	{
		SetSetting(type, sstr(spinbox->cleanText()));
	});
}

void emu_settings::EnhanceDoubleSpinBox(QDoubleSpinBox* spinbox, emu_settings_type type, const QString& prefix, const QString& suffix)
{
	if (!spinbox)
	{
		cfg_log.fatal("EnhanceDoubleSpinBox '%s' was used with an invalid object", cfg_adapter::get_setting_name(type));
		return;
	}

	const QStringList range = GetSettingOptions(type);
	bool ok_def, ok_sel, ok_min, ok_max;

	const double def = qstr(GetSettingDefault(type)).toDouble(&ok_def);
	const double min = range.first().toDouble(&ok_min);
	const double max = range.last().toDouble(&ok_max);

	if (!ok_def || !ok_min || !ok_max)
	{
		cfg_log.fatal("EnhanceDoubleSpinBox '%s' was used with an invalid type", cfg_adapter::get_setting_name(type));
		return;
	}

	const std::string selected = GetSetting(type);
	double val = qstr(selected).toDouble(&ok_sel);

	if (!ok_sel || val < min || val > max)
	{
		cfg_log.fatal("EnhanceDoubleSpinBox '%s' tried to set an invalid value: %f. Setting to default: %f. Allowed range: [%f, %f]", cfg_adapter::get_setting_name(type), val, def, min, max);
		val = def;
		m_broken_types.insert(type);
	}

	spinbox->setPrefix(prefix);
	spinbox->setSuffix(suffix);
	spinbox->setRange(min, max);
	spinbox->setValue(val);

	connect(spinbox, &QDoubleSpinBox::textChanged, [=, this](const QString&/* text*/)
	{
		SetSetting(type, sstr(spinbox->cleanText()));
	});
}

void emu_settings::EnhanceLineEdit(QLineEdit* edit, emu_settings_type type)
{
	if (!edit)
	{
		cfg_log.fatal("EnhanceEdit '%s' was used with an invalid object", cfg_adapter::get_setting_name(type));
		return;
	}

	const std::string set_text = GetSetting(type);
	edit->setText(qstr(set_text));

	connect(edit, &QLineEdit::textChanged, [type, this](const QString &text)
	{
		SetSetting(type, sstr(text));
	});
}

void emu_settings::EnhanceRadioButton(QButtonGroup* button_group, emu_settings_type type)
{
	if (!button_group)
	{
		cfg_log.fatal("EnhanceRadioButton '%s' was used with an invalid object", cfg_adapter::get_setting_name(type));
		return;
	}

	const QString selected    = qstr(GetSetting(type));
	const QStringList options = GetSettingOptions(type);

	if (button_group->buttons().count() < options.size())
	{
		cfg_log.fatal("EnhanceRadioButton '%s': wrong button count", cfg_adapter::get_setting_name(type));
		return;
	}

	for (int i = 0; i < options.count(); i++)
	{
		const QString localized_setting = GetLocalizedSetting(options[i], type, i);

		button_group->button(i)->setText(localized_setting);

		if (options[i] == selected)
		{
			button_group->button(i)->setChecked(true);
		}

		connect(button_group->button(i), &QAbstractButton::clicked, [=, this]()
		{
			SetSetting(type, sstr(options[i]));
		});
	}
}

std::vector<std::string> emu_settings::GetLoadedLibraries()
{
	return m_currentSettings["Core"]["Load libraries"].as<std::vector<std::string>, std::initializer_list<std::string>>({});
}

void emu_settings::SaveSelectedLibraries(const std::vector<std::string>& libs)
{
	m_currentSettings["Core"]["Load libraries"] = libs;
}

QStringList emu_settings::GetSettingOptions(emu_settings_type type) const
{
	return cfg_adapter::get_options(const_cast<cfg_location&&>(settings_location[type]));
}

std::string emu_settings::GetSettingDefault(emu_settings_type type) const
{
	return cfg_adapter::get_node(m_defaultSettings, settings_location[type]).Scalar();
}

std::string emu_settings::GetSetting(emu_settings_type type) const
{
	return cfg_adapter::get_node(m_currentSettings, settings_location[type]).Scalar();
}

void emu_settings::SetSetting(emu_settings_type type, const std::string& val)
{
	cfg_adapter::get_node(m_currentSettings, settings_location[type]) = val;
}

void emu_settings::OpenCorrectionDialog(QWidget* parent)
{
	if (!m_broken_types.empty() && QMessageBox::question(parent, tr("Fix invalid settings?"),
		tr(
			"Your config file contained one or more unrecognized values for settings.\n"
			"Their default value will be used until they are corrected.\n"
			"Consider that a correction might render them invalid for other versions of RPCS3.\n"
			"\n"
			"Do you wish to let the program correct them for you?\n"
			"This change will only be final when you save the config."
		),
		QMessageBox::Yes | QMessageBox::No, QMessageBox::No) == QMessageBox::Yes)
	{
		for (const auto& type : m_broken_types)
		{
			const std::string def = GetSettingDefault(type);
			const std::string old = GetSetting(type);
			SetSetting(type, def);
			cfg_log.success("The config entry '%s' was corrected from '%s' to '%s'", cfg_adapter::get_setting_name(type), old, def);
		}

		m_broken_types.clear();
		cfg_log.success("You need to save the settings in order to make these changes permanent!");
	}
}

QString emu_settings::GetLocalizedSetting(const QString& original, emu_settings_type type, int index) const
{
	switch (type)
	{
	case emu_settings_type::SPUBlockSize:
		switch (static_cast<spu_block_size_type>(index))
		{
		case spu_block_size_type::safe: return tr("Safe", "SPU block size");
		case spu_block_size_type::mega: return tr("Mega", "SPU block size");
		case spu_block_size_type::giga: return tr("Giga", "SPU block size");
		}
		break;
	case emu_settings_type::EnableTSX:
		switch (static_cast<tsx_usage>(index))
		{
		case tsx_usage::disabled: return tr("Disabled", "Enable TSX");
		case tsx_usage::enabled: return tr("Enabled", "Enable TSX");
		case tsx_usage::forced: return tr("Forced", "Enable TSX");
		}
		break;
	case emu_settings_type::Renderer:
		switch (static_cast<video_renderer>(index))
		{
		case video_renderer::null: return tr("Disable Video Output", "Video renderer");
		case video_renderer::opengl: return tr("OpenGL", "Video renderer");
		case video_renderer::vulkan: return tr("Vulkan", "Video renderer");
		}
		break;
	case emu_settings_type::ShaderMode:
		switch (static_cast<shader_mode>(index))
		{
		case shader_mode::recompiler: return tr("Legacy (single threaded)", "Shader Mode");
		case shader_mode::async_recompiler: return tr("Async (multi threaded)", "Shader Mode");
		case shader_mode::async_with_interpreter: return tr("Async with Shader Interpreter", "Shader Mode");
		case shader_mode::interpreter_only: return tr("Shader Interpreter only", "Shader Mode");
		}
		break;
	case emu_settings_type::FrameLimit:
		switch (static_cast<frame_limit_type>(index))
		{
		case frame_limit_type::none: return tr("Off", "Frame limit");
		case frame_limit_type::_59_94: return tr("59.94", "Frame limit");
		case frame_limit_type::_50: return tr("50", "Frame limit");
		case frame_limit_type::_60: return tr("60", "Frame limit");
		case frame_limit_type::_30: return tr("30", "Frame limit");
		case frame_limit_type::_auto: return tr("Auto", "Frame limit");
		}
		break;
	case emu_settings_type::MSAA:
		switch (static_cast<msaa_level>(index))
		{
		case msaa_level::none: return tr("Disabled", "MSAA");
		case msaa_level::_auto: return tr("Auto", "MSAA");
		}
		break;
	case emu_settings_type::AudioRenderer:
		switch (static_cast<audio_renderer>(index))
		{
		case audio_renderer::null: return tr("Disable Audio Output", "Audio renderer");
#ifdef _WIN32
		case audio_renderer::xaudio: return tr("XAudio2", "Audio renderer");
#endif
#ifdef HAVE_ALSA
		case audio_renderer::alsa: return tr("ALSA", "Audio renderer");
#endif
#ifdef HAVE_PULSE
		case audio_renderer::pulse: return tr("PulseAudio", "Audio renderer");
#endif
		case audio_renderer::openal: return tr("OpenAL", "Audio renderer");
#ifdef HAVE_FAUDIO
		case audio_renderer::faudio: return tr("FAudio", "Audio renderer");
#endif
		}
		break;
	case emu_settings_type::MicrophoneType:
		switch (static_cast<microphone_handler>(index))
		{
		case microphone_handler::null: return tr("Disabled", "Microphone handler");
		case microphone_handler::standard: return tr("Standard", "Microphone handler");
		case microphone_handler::singstar: return tr("SingStar", "Microphone handler");
		case microphone_handler::real_singstar: return tr("Real SingStar", "Microphone handler");
		case microphone_handler::rocksmith: return tr("Rocksmith", "Microphone handler");
		}
		break;
	case emu_settings_type::KeyboardHandler:
		switch (static_cast<keyboard_handler>(index))
		{
		case keyboard_handler::null: return tr("Null", "Keyboard handler");
		case keyboard_handler::basic: return tr("Basic", "Keyboard handler");
		}
		break;
	case emu_settings_type::MouseHandler:
		switch (static_cast<mouse_handler>(index))
		{
		case mouse_handler::null: return tr("Null", "Mouse handler");
		case mouse_handler::basic: return tr("Basic", "Mouse handler");
		}
		break;
	case emu_settings_type::CameraType:
		switch (static_cast<fake_camera_type>(index))
		{
		case fake_camera_type::unknown: return tr("Unknown", "Camera type");
		case fake_camera_type::eyetoy: return tr("EyeToy", "Camera type");
		case fake_camera_type::eyetoy2: return tr("PS Eye", "Camera type");
		case fake_camera_type::uvc1_1: return tr("UVC 1.1", "Camera type");
		}
		break;
	case emu_settings_type::Camera:
		switch (static_cast<camera_handler>(index))
		{
		case camera_handler::null: return tr("Null", "Camera handler");
		case camera_handler::fake: return tr("Fake", "Camera handler");
		}
		break;
	case emu_settings_type::Move:
		switch (static_cast<move_handler>(index))
		{
		case move_handler::null: return tr("Null", "Move handler");
		case move_handler::fake: return tr("Fake", "Move handler");
		case move_handler::mouse: return tr("Mouse", "Move handler");
		}
		break;
	case emu_settings_type::InternetStatus:
		switch (static_cast<np_internet_status>(index))
		{
		case np_internet_status::disabled: return tr("Disconnected", "Internet Status");
		case np_internet_status::enabled: return tr("Connected", "Internet Status");
		}
		break;
	case emu_settings_type::PSNStatus:
		switch (static_cast<np_psn_status>(index))
		{
		case np_psn_status::disabled: return tr("Disconnected", "PSN Status");
		case np_psn_status::fake: return tr("Simulated", "PSN Status");
		}
		break;
	case emu_settings_type::SleepTimersAccuracy:
		switch (static_cast<sleep_timers_accuracy_level>(index))
		{
		case sleep_timers_accuracy_level::_as_host: return tr("As Host", "Sleep timers accuracy");
		case sleep_timers_accuracy_level::_usleep: return tr("Usleep Only", "Sleep timers accuracy");
		case sleep_timers_accuracy_level::_all_timers: return tr("All Timers", "Sleep timers accuracy");
		}
		break;
	case emu_settings_type::PerfOverlayDetailLevel:
		switch (static_cast<detail_level>(index))
		{
		case detail_level::minimal: return tr("Minimal", "Detail Level");
		case detail_level::low: return tr("Low", "Detail Level");
		case detail_level::medium: return tr("Medium", "Detail Level");
		case detail_level::high: return tr("High", "Detail Level");
		}
		break;
	case emu_settings_type::PerfOverlayPosition:
		switch (static_cast<screen_quadrant>(index))
		{
		case screen_quadrant::top_left: return tr("Top Left", "Performance overlay position");
		case screen_quadrant::top_right: return tr("Top Right", "Performance overlay position");
		case screen_quadrant::bottom_left: return tr("Bottom Left", "Performance overlay position");
		case screen_quadrant::bottom_right: return tr("Bottom Right", "Performance overlay position");
		}
		break;
	case emu_settings_type::LibLoadOptions:
		switch (static_cast<lib_loading_type>(index))
		{
		case lib_loading_type::manual: return tr("Manually load selected libraries", "Libraries");
		case lib_loading_type::hybrid: return tr("Load automatic and manual selection", "Libraries");
		case lib_loading_type::liblv2only: return tr("Load liblv2.sprx only", "Libraries");
		case lib_loading_type::liblv2both: return tr("Load liblv2.sprx and manual selection", "Libraries");
		case lib_loading_type::liblv2list: return tr("Load liblv2.sprx and strict selection", "Libraries");
		}
		break;
	case emu_settings_type::PPUDecoder:
		switch (static_cast<ppu_decoder_type>(index))
		{
		case ppu_decoder_type::precise: return tr("Interpreter (precise)", "PPU decoder");
		case ppu_decoder_type::fast: return tr("Interpreter (fast)", "PPU decoder");
		case ppu_decoder_type::llvm: return tr("Recompiler (LLVM)", "PPU decoder");
		}
		break;
	case emu_settings_type::SPUDecoder:
		switch (static_cast<spu_decoder_type>(index))
		{
		case spu_decoder_type::precise: return tr("Interpreter (precise)", "SPU decoder");
		case spu_decoder_type::fast: return tr("Interpreter (fast)", "SPU decoder");
		case spu_decoder_type::asmjit: return tr("Recompiler (ASMJIT)", "SPU decoder");
		case spu_decoder_type::llvm: return tr("Recompiler (LLVM)", "SPU decoder");
		}
		break;
	case emu_settings_type::EnterButtonAssignment:
		switch (static_cast<enter_button_assign>(index))
		{
		case enter_button_assign::circle: return tr("Enter with circle", "Enter button assignment");
		case enter_button_assign::cross: return tr("Enter with cross", "Enter button assignment");
		}
		break;
	default:
		break;
	}

	return original;
}

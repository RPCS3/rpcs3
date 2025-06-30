#include <QButtonGroup>
#include <QCameraDevice>
#include <QMediaDevices>
#include <QDialogButtonBox>
#include <QFontMetrics>
#include <QPushButton>
#include <QMessageBox>
#include <QInputDialog>
#include <QDesktopServices>
#include <QColorDialog>
#include <QSpinBox>
#include <QTimer>
#include <QScreen>
#include <QStyleFactory>

#include "gui_settings.h"
#include "qt_utils.h"
#include "uuid.h"
#include "settings_dialog.h"
#include "ui_settings_dialog.h"
#include "tooltips.h"
#include "input_dialog.h"
#include "emu_settings_type.h"
#include "render_creator.h"
#include "microphone_creator.h"
#include "Emu/NP/rpcn_countries.h"

#include "Emu/GameInfo.h"
#include "Emu/System.h"
#include "Emu/system_config.h"
#include "Emu/title.h"

#include "Emu/Audio/audio_device_enumerator.h"

#include "Loader/PSF.h"

#include <set>

#include "util/sysinfo.hpp"
#include "util/asm.hpp"

#ifdef WITH_DISCORD_RPC
#include "_discord_utils.h"
#endif

LOG_CHANNEL(cfg_log, "CFG");

std::pair<QString, int> get_data(const QComboBox* box, int index)
{
	if (!box) return {};

	const QVariantList var_list = box->itemData(index).toList();
	ensure(var_list.size() == 2);
	ensure(var_list[0].canConvert<QString>());
	ensure(var_list[1].canConvert<int>());

	return { var_list[0].toString(), var_list[1].toInt() };
}

int find_item(const QComboBox* box, int value)
{
	for (int i = 0; box && i < box->count(); i++)
	{
		if (get_data(box, i).second == value)
		{
			return i;
		}
	}

	return -1;
}

void remove_item(QComboBox* box, int data_value, int def_value)
{
	if (!box) return;

	const int index = find_item(box, data_value);
	const bool was_selected = index == box->currentIndex();

	box->removeItem(index);

	if (was_selected)
	{
		box->setCurrentIndex(find_item(box, def_value));
	}
}

extern const std::map<std::string_view, int> g_prx_list;

settings_dialog::settings_dialog(std::shared_ptr<gui_settings> gui_settings, std::shared_ptr<emu_settings> emu_settings, int tab_index, QWidget* parent, const GameInfo* game, bool create_cfg_from_global_cfg)
	: QDialog(parent)
	, m_tab_index(tab_index)
	, ui(new Ui::settings_dialog)
	, m_gui_settings(std::move(gui_settings))
	, m_emu_settings(std::move(emu_settings))
{
	ui->setupUi(this);
	ui->buttonBox->button(QDialogButtonBox::StandardButton::Close)->setFocus();
	ui->tab_widget_settings->setUsesScrollButtons(false);
	ui->tab_widget_settings->tabBar()->setObjectName("tab_bar_settings");

	if (!m_gui_settings->GetValue(gui::m_showDebugTab).toBool())
	{
		ui->tab_widget_settings->removeTab(9);
		ui->audioDump->setVisible(false);
		ui->audioDump->setChecked(false);
		m_gui_settings->SetValue(gui::m_showDebugTab, false);
	}
	if (game)
	{
		ui->tab_widget_settings->removeTab(8);
		ui->buttonBox->button(QDialogButtonBox::StandardButton::Save)->setText(tr("Save custom configuration", "Settings dialog"));
	}

	// Localized tooltips
	const Tooltips tooltips;

	// Add description labels
	SubscribeDescription(ui->description_cpu);
	SubscribeDescription(ui->description_gpu);
	SubscribeDescription(ui->description_audio);
	SubscribeDescription(ui->description_io);
	SubscribeDescription(ui->description_system);
	SubscribeDescription(ui->description_network);
	SubscribeDescription(ui->description_advanced);
	SubscribeDescription(ui->description_emulator);
	if (!game)
	{
		SubscribeDescription(ui->description_gui);
	}
	SubscribeDescription(ui->description_debug);

	if (game)
	{
		m_emu_settings->LoadSettings(game->serial, create_cfg_from_global_cfg);
		setWindowTitle(tr("Settings: [%0] %1", "Settings dialog").arg(QString::fromStdString(game->serial)).arg(QString::fromStdString(game->name)));
	}
	else
	{
		m_emu_settings->LoadSettings();
		setWindowTitle(tr("Settings", "Settings dialog"));
	}

	// Discord variables
	m_use_discord = m_gui_settings->GetValue(gui::m_richPresence).toBool();
	m_discord_state = m_gui_settings->GetValue(gui::m_discordState).toString();

	// Various connects

	const auto apply_configs = [this, use_discord_old = m_use_discord, discord_state_old = m_discord_state, game](bool do_exit)
	{
		u32 selected_audio_formats = 0;
		for (int i = 0; i < ui->list_audio_formats->count(); ++i)
		{
			const auto& item = ui->list_audio_formats->item(i);
			if (item->checkState() != Qt::CheckState::Unchecked)
			{
				selected_audio_formats |= item->data(Qt::UserRole).toUInt();
			}
		}
		m_emu_settings->SetSetting(emu_settings_type::AudioFormats, std::to_string(selected_audio_formats));

		std::set<std::string> selected;
		for (int i = 0; i < ui->lleList->count(); ++i)
		{
			const auto& item = ui->lleList->item(i);
			if (item->checkState() != Qt::CheckState::Unchecked)
			{
				// suffix indicates forced HLE mode
				selected.emplace(item->text().toStdString() + ":hle");
			}
		}

		for (int i = 0; i < ui->hleList->count(); ++i)
		{
			const auto& item = ui->hleList->item(i);
			if (item->checkState() != Qt::CheckState::Unchecked)
			{
				// suffix indicates forced LLE mode
				selected.emplace(item->text().toStdString() + ":lle");
			}
		}

		const std::vector<std::string> selected_ls(selected.begin(), selected.end());
		m_emu_settings->SaveSelectedLibraries(selected_ls);
		m_emu_settings->SaveSettings();

		if (do_exit)
		{
			accept();
		}

		Q_EMIT EmuSettingsApplied();

		// Discord Settings can be saved regardless of WITH_DISCORD_RPC
		m_gui_settings->SetValue(gui::m_richPresence, m_use_discord, false);
		m_gui_settings->SetValue(gui::m_discordState, m_discord_state, true);

#ifdef WITH_DISCORD_RPC
		if (m_use_discord != use_discord_old)
		{
			if (m_use_discord)
			{
				discord::initialize();
				discord::update_presence(m_discord_state.toStdString());
			}
			else
			{
				discord::shutdown();
			}
		}
		else if (m_discord_state != discord_state_old && Emu.IsStopped())
		{
			discord::update_presence(m_discord_state.toStdString(), "Idle", false);
		}
#endif

		if (!game)
		{
			ApplyStylesheet(false);
		}
	};

	connect(ui->buttonBox, &QDialogButtonBox::clicked, this, [apply_configs, this](QAbstractButton* button)
	{
		if (button == ui->buttonBox->button(QDialogButtonBox::Save))
		{
			apply_configs(true);
		}
		else if (button == ui->buttonBox->button(QDialogButtonBox::Apply))
		{
			apply_configs(false);
		}
		else if (button == ui->buttonBox->button(QDialogButtonBox::RestoreDefaults))
		{
			m_emu_settings->RestoreDefaults();

			// Handle special restrictions after the settings were restored
			Q_EMIT signal_restore_dependant_defaults();
		}
	});

	connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &QWidget::close);

	connect(ui->tab_widget_settings, &QTabWidget::currentChanged, this, [this]()
	{
		ui->buttonBox->button(QDialogButtonBox::StandardButton::Close)->setFocus();
	});

	//     _____ _____  _    _   _______    _
	//    / ____|  __ \| |  | | |__   __|  | |
	//   | |    | |__) | |  | |    | | __ _| |__
	//   | |    |  ___/| |  | |    | |/ _` | '_ ＼
	//   | |____| |    | |__| |    | | (_| | |_) |
	//    \_____|_|     \____/     |_|\__,_|_.__/

	// Checkboxes

	m_emu_settings->EnhanceCheckBox(ui->spuLoopDetection, emu_settings_type::SPULoopDetection);
	SubscribeTooltip(ui->spuLoopDetection, tooltips.settings.spu_loop_detection);

	// Comboboxes
	m_emu_settings->EnhanceComboBox(ui->xfloatAccuracy, emu_settings_type::XFloatAccuracy);
	SubscribeTooltip(ui->gb_xfloat_accuracy, tooltips.settings.xfloat);
	remove_item(ui->xfloatAccuracy, static_cast<int>(xfloat_accuracy::inaccurate), static_cast<int>(g_cfg.core.spu_xfloat_accuracy.def));

	m_emu_settings->EnhanceComboBox(ui->spuBlockSize, emu_settings_type::SPUBlockSize);
	SubscribeTooltip(ui->gb_spuBlockSize, tooltips.settings.spu_block_size);

	m_emu_settings->EnhanceComboBox(ui->threadsched, emu_settings_type::ThreadSchedulerMode);
	if (constexpr u32 min_thread_count = 12; utils::get_thread_count() < min_thread_count)
	{
		ui->gb_threadsched->setDisabled(true);
		SubscribeTooltip(ui->gb_threadsched,
			tr(
				"Changing the thread scheduler is not supported on CPUs with less than %0 threads.\n"
				"\n"
				"Control how RPCS3 utilizes the threads of your system.\n"
				"Each option heavily depends on the game and on your CPU, it's recommended to try each option to find out which performs the best."
			).arg(min_thread_count));
	}
	else
	{
		SubscribeTooltip(ui->gb_threadsched, tooltips.settings.enable_thread_scheduler);
	}

	m_emu_settings->EnhanceComboBox(ui->preferredSPUThreads, emu_settings_type::PreferredSPUThreads, true);
	SubscribeTooltip(ui->gb_spu_threads, tooltips.settings.preferred_spu_threads);
	ui->preferredSPUThreads->setItemText(ui->preferredSPUThreads->findData(0), tr("Auto", "Preferred SPU threads"));

	if (utils::has_rtm())
	{
		m_emu_settings->EnhanceComboBox(ui->enableTSX, emu_settings_type::EnableTSX);
		SubscribeTooltip(ui->gb_tsx, tooltips.settings.enable_tsx);

		if (!utils::has_mpx() || utils::has_tsx_force_abort())
		{
			remove_item(ui->enableTSX, static_cast<int>(tsx_usage::enabled), static_cast<int>(g_cfg.core.enable_TSX.def));
		}

		connect(ui->enableTSX, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int index)
		{
			if (index < 0) return;
			if (const auto [text, value] = get_data(ui->enableTSX, index); value == static_cast<int>(tsx_usage::forced) &&
				(!utils::has_mpx() || utils::has_tsx_force_abort()))
			{
				QString title;
				QString message;
				if (!utils::has_mpx())
				{
					title = tr("Haswell/Broadwell TSX Warning");
					message = gui::utils::make_paragraph(tr(
						"RPCS3 has detected that you are using TSX functions on a Haswell or Broadwell CPU.\n"
						"Intel has deactivated these functions in newer Microcode revisions, since they can lead to unpredicted behaviour.\n"
						"That means using TSX may break games or even <font color=\"red\"><b>damage</b></font> your data.\n"
						"We recommend to disable this feature and update your computer BIOS.\n"
						"\n"
						"Do you wish to use TSX anyway?"
					));
				}
				else
				{
					title = tr("TSX-FA Warning");
					message = gui::utils::make_paragraph(tr(
						"RPCS3 has detected your CPU only supports TSX-FA.\n"
						"That means using TSX may break games or even <font color=\"red\"><b>damage</b></font> your data.\n"
						"We recommend to disable this feature.\n"
						"\n"
						"Do you wish to use TSX anyway?"
					));
				}

				if (QMessageBox::No == QMessageBox::critical(this, title, message, QMessageBox::Yes, QMessageBox::No))
				{
					// Reset if the messagebox was answered with no. This prevents the currentIndexChanged signal in EnhanceComboBox
					ui->enableTSX->setCurrentIndex(find_item(ui->enableTSX, static_cast<int>(g_cfg.core.enable_TSX.def)));
				}
			}
		});
	}
	else
	{
		ui->enableTSX->setEnabled(false);
		ui->enableTSX->setPlaceholderText(tr("Not supported", "Enable TSX"));
		SubscribeTooltip(ui->enableTSX, tr("Unfortunately, your CPU model does not support this instruction set.", "Enable TSX"));

		m_emu_settings->SetSetting(emu_settings_type::EnableTSX, fmt::format("%s", tsx_usage::disabled));
		connect(this, &settings_dialog::signal_restore_dependant_defaults, [this]()
		{
			m_emu_settings->SetSetting(emu_settings_type::EnableTSX, fmt::format("%s", tsx_usage::disabled));
		});
	}

	// PPU tool tips
	SubscribeTooltip(ui->ppu__static, tooltips.settings.ppu__static);
	SubscribeTooltip(ui->ppu_llvm,    tooltips.settings.ppu_llvm);

	QButtonGroup *ppu_bg = new QButtonGroup(this);
	ppu_bg->addButton(ui->ppu__static, static_cast<int>(ppu_decoder_type::_static));
	ppu_bg->addButton(ui->ppu_llvm,    static_cast<int>(ppu_decoder_type::llvm));

	connect(ppu_bg, &QButtonGroup::idToggled, [this](int id, bool checked)
	{
		if (!checked) return;

		switch (id)
		{
		case static_cast<int>(ppu_decoder_type::_static):
			ui->accuratePPUFPCC->setEnabled(true);
			ui->accuratePPUNJ->setEnabled(true);
			ui->accuratePPUVNAN->setEnabled(true);
			break;
		case static_cast<int>(ppu_decoder_type::llvm):
			ui->accuratePPUFPCC->setEnabled(false);
			ui->accuratePPUNJ->setEnabled(false);
			ui->accuratePPUVNAN->setEnabled(false);
			break;
		default:
			break;
		}
	});

	m_emu_settings->EnhanceRadioButton(ppu_bg, emu_settings_type::PPUDecoder);

	// SPU tool tips
	SubscribeTooltip(ui->spu__static, tooltips.settings.spu__static);
	SubscribeTooltip(ui->spu_dynamic, tooltips.settings.spu_dynamic);
	SubscribeTooltip(ui->spu_asmjit,  tooltips.settings.spu_asmjit);
	SubscribeTooltip(ui->spu_llvm,    tooltips.settings.spu_llvm);

	QButtonGroup* spu_bg = new QButtonGroup(this);
	spu_bg->addButton(ui->spu__static, static_cast<int>(spu_decoder_type::_static));
	spu_bg->addButton(ui->spu_dynamic, static_cast<int>(spu_decoder_type::dynamic));
	spu_bg->addButton(ui->spu_asmjit,  static_cast<int>(spu_decoder_type::asmjit));
	spu_bg->addButton(ui->spu_llvm,    static_cast<int>(spu_decoder_type::llvm));

#ifndef ARCH_X64
	ui->spu_asmjit->setEnabled(false);
#endif

	connect(spu_bg, &QButtonGroup::idToggled, [this](int id, bool checked)
	{
		if (!checked) return;

		switch (id)
		{
		case static_cast<int>(spu_decoder_type::_static):
		case static_cast<int>(spu_decoder_type::dynamic):
		case static_cast<int>(spu_decoder_type::llvm):
			ui->xfloatAccuracy->setEnabled(true);
			break;
		case static_cast<int>(spu_decoder_type::asmjit):
			ui->xfloatAccuracy->setEnabled(false);
			break;
		default:
			break;
		}
	});

	m_emu_settings->EnhanceRadioButton(spu_bg, emu_settings_type::SPUDecoder);

#ifndef LLVM_AVAILABLE
	ui->ppu_llvm->setEnabled(false);
	ui->spu_llvm->setEnabled(false);
	ui->spu_dynamic->setEnabled(false);
#endif

	//     _____ _____  _    _   _______    _
	//    / ____|  __ \| |  | | |__   __|  | |
	//   | |  __| |__) | |  | |    | | __ _| |__
	//   | | |_ |  ___/| |  | |    | |/ _` | '_ ＼
	//   | |__| | |    | |__| |    | | (_| | |_) |
	//    \_____|_|     \____/     |_|\__,_|_.__/

	render_creator* r_creator = m_emu_settings->m_render_creator;

	if (!r_creator)
	{
		fmt::throw_exception("settings_dialog::settings_dialog() render_creator is null");
	}

	r_creator->update_names(
	{
		m_emu_settings->GetLocalizedSetting(QString("Vulkan"), emu_settings_type::Renderer, static_cast<int>(video_renderer::vulkan), true),
		m_emu_settings->GetLocalizedSetting(QString("OpenGl"), emu_settings_type::Renderer, static_cast<int>(video_renderer::opengl), true),
		m_emu_settings->GetLocalizedSetting(QString("Null"), emu_settings_type::Renderer, static_cast<int>(video_renderer::null), true)
	});

	// Comboboxes
	m_emu_settings->EnhanceComboBox(ui->renderBox, emu_settings_type::Renderer);
	SubscribeTooltip(ui->gb_renderer, tooltips.settings.renderer);
	SubscribeTooltip(ui->gb_graphicsAdapter, tooltips.settings.graphics_adapter);

	m_emu_settings->EnhanceComboBox(ui->resBox, emu_settings_type::Resolution, false, false, 0, false, false);
	SubscribeTooltip(ui->gb_default_resolution, tooltips.settings.resolution);
	// remove unsupported resolutions from the dropdown
	bool saved_index_removed = false;
	//if (game && game->resolution > 0) // Add this line when interlaced resolutions are implemented
	{
		const std::map<video_resolution, u32> resolutions
		{
			{ video_resolution::_480p,       psf::resolution_flag::_480 | psf::resolution_flag::_480_16_9 },
			{ video_resolution::_480i,       psf::resolution_flag::_480 | psf::resolution_flag::_480_16_9 },
			{ video_resolution::_576p,       psf::resolution_flag::_576 | psf::resolution_flag::_576_16_9 },
			{ video_resolution::_576i,       psf::resolution_flag::_576 | psf::resolution_flag::_576_16_9 },
			{ video_resolution::_720p,       psf::resolution_flag::_720  },
			{ video_resolution::_1080p,      psf::resolution_flag::_1080 },
			{ video_resolution::_1080i,      psf::resolution_flag::_1080 },
			{ video_resolution::_1600x1080p, psf::resolution_flag::_1080 },
			{ video_resolution::_1440x1080p, psf::resolution_flag::_1080 },
			{ video_resolution::_1280x1080p, psf::resolution_flag::_1080 },
			{ video_resolution::_960x1080p,  psf::resolution_flag::_1080 },
		};

		const int saved_index = ui->resBox->currentIndex();
		bool remove_720p = false;

		for (int i = ui->resBox->count() - 1; i >= 0; i--)
		{
			const auto [text, value] = get_data(ui->resBox, i);
			const video_resolution resolution = static_cast<video_resolution>(value);

			// Remove interlaced resolutions until they are properly implemented
			const bool is_interlaced = (resolution == video_resolution::_1080i ||
			                            resolution == video_resolution::_480i ||
			                            resolution == video_resolution::_576i);
			const bool supported_by_game = !game || !game->resolution || (resolutions.contains(resolution) && (game->resolution & resolutions.at(resolution)));

			if (!supported_by_game || is_interlaced)
			{
				// Don't remove 720p yet. We may need it as fallback if no other resolution is supported.
				if (resolution == video_resolution::_720p)
				{
					remove_720p = true;
					continue;
				}

				ui->resBox->removeItem(i);

				if (i == saved_index)
				{
					saved_index_removed = true;
				}
			}
		}

		// Remove 720p if unsupported unless it's the only option
		if (remove_720p && ui->resBox->count() > 1)
		{
			if (const int index = find_item(ui->resBox, static_cast<int>(video_resolution::_720p)); index >= 0)
			{
				ui->resBox->removeItem(index);
			}
		}
	}

	for (int i = 0; i < ui->resBox->count(); i++)
	{
		const auto [text, value] = get_data(ui->resBox, i);

		if (static_cast<video_resolution>(value) == video_resolution::_720p)
		{
			// Rename the default resolution for users
			ui->resBox->setItemText(i, tr("720p (Recommended)", "Resolution"));

			// Set the current selection to the default if the original setting wasn't valid
			if (saved_index_removed)
			{
				ui->resBox->setCurrentIndex(i);
			}
			break;
		}
	}

	m_emu_settings->EnhanceComboBox(ui->aspectBox, emu_settings_type::AspectRatio, false, false, 0, false, false);
	SubscribeTooltip(ui->gb_aspectRatio, tooltips.settings.aspect_ratio);

	m_emu_settings->EnhanceComboBox(ui->frameLimitBox, emu_settings_type::FrameLimit);
	SubscribeTooltip(ui->gb_frameLimit, tooltips.settings.frame_limit);

	{
		const QList<QScreen*> screens = QGuiApplication::screens();

		f64 rate = 20.; // Minimum rate

		for (int i = 0; i < screens.count(); i++)
		{
			rate = std::fmax(rate, ::at32(screens, i)->refreshRate());
		}

		for (int i = 0; i < ui->frameLimitBox->count(); i++)
		{
			const QVariantList var_list = ui->frameLimitBox->itemData(i).toList();

			if (var_list.size() != 2 || !var_list[1].canConvert<int>())
			{
				fmt::throw_exception("Invalid data found in combobox entry %d (text='%s', listsize=%d, itemcount=%d)", i, ui->frameLimitBox->itemText(i), var_list.size(), ui->frameLimitBox->count());
			}

			if (static_cast<int>(frame_limit_type::display_rate) == var_list[1].toInt())
			{
				ui->frameLimitBox->setItemText(i, tr("Display (%1)", "Frame Limit").arg(std::round(rate)));
				break;
			}
		}
	}

	m_emu_settings->EnhanceComboBox(ui->antiAliasing, emu_settings_type::MSAA);
	SubscribeTooltip(ui->gb_antiAliasing, tooltips.settings.anti_aliasing);

	m_emu_settings->EnhanceComboBox(ui->anisotropicFilterOverride, emu_settings_type::AnisotropicFilterOverride, true);
	SubscribeTooltip(ui->gb_anisotropicFilter, tooltips.settings.anisotropic_filter);
	// only allow values 0,2,4,8,16
	for (int i = ui->anisotropicFilterOverride->count() - 1; i >= 0; i--)
	{
		switch (int val = ui->anisotropicFilterOverride->itemData(i).toInt())
		{
		case 0:
			ui->anisotropicFilterOverride->setItemText(i, tr("Auto", "Anisotropic filter override"));
			break;
		case 2:
		case 4:
		case 8:
		case 16:
			ui->anisotropicFilterOverride->setItemText(i, tr("%1x", "Anisotropic filter override").arg(val));
			break;
		default:
			ui->anisotropicFilterOverride->removeItem(i);
			break;
		}
	}

	m_emu_settings->EnhanceComboBox(ui->shaderPrecision, emu_settings_type::ShaderPrecisionQuality);
	SubscribeTooltip(ui->gbShaderPrecision, tooltips.settings.shader_precision);

	// Custom control that simplifies operation of two independent variables. Can probably be done better but this works.
	ui->zcullPrecisionMode->addItem(tr("Precise (Slowest)"), static_cast<int>(zcull_precision_level::precise));
	ui->zcullPrecisionMode->addItem(tr("Approximate (Fast)"), static_cast<int>(zcull_precision_level::approximate));
	ui->zcullPrecisionMode->addItem(tr("Relaxed (Fastest)"), static_cast<int>(zcull_precision_level::relaxed));

	const auto reset_zcull_options = [this]()
	{
		if (m_emu_settings->GetSetting(emu_settings_type::RelaxedZCULL) == "true")
		{
			ui->zcullPrecisionMode->setCurrentIndex(ui->zcullPrecisionMode->findData(static_cast<int>(zcull_precision_level::relaxed)));
		}
		else if (m_emu_settings->GetSetting(emu_settings_type::PreciseZCULL) == "true")
		{
			ui->zcullPrecisionMode->setCurrentIndex(ui->zcullPrecisionMode->findData(static_cast<int>(zcull_precision_level::precise)));
		}
		else
		{
			ui->zcullPrecisionMode->setCurrentIndex(ui->zcullPrecisionMode->findData(static_cast<int>(zcull_precision_level::approximate)));
		}
	};
	reset_zcull_options();
	connect(this, &settings_dialog::signal_restore_dependant_defaults, this, reset_zcull_options);

	connect(ui->zcullPrecisionMode, QOverload<int>::of(&QComboBox::currentIndexChanged), [this](int index)
	{
		if (index < 0) return;

		bool relaxed = false, precise = false;

		switch (static_cast<zcull_precision_level>(ui->zcullPrecisionMode->itemData(index).toInt()))
		{
		case zcull_precision_level::precise: precise = true; break;
		case zcull_precision_level::approximate: break;
		case zcull_precision_level::relaxed: relaxed = true; break;
		default: fmt::throw_exception("Unexpected selection");
		}

		m_emu_settings->SetSetting(emu_settings_type::RelaxedZCULL, relaxed ? "true" : "false");
		m_emu_settings->SetSetting(emu_settings_type::PreciseZCULL, precise ? "true" : "false");
	});
	SubscribeTooltip(ui->gbZCULL, tooltips.settings.zcull_operation_mode);

	m_emu_settings->EnhanceComboBox(ui->outputScalingMode, emu_settings_type::OutputScalingMode);
	SubscribeTooltip(ui->outputScalingMode, tooltips.settings.output_scaling_mode);

	// 3D
	m_emu_settings->EnhanceComboBox(ui->stereoRenderMode, emu_settings_type::StereoRenderMode);
	SubscribeTooltip(ui->gb_stereo, tooltips.settings.stereo_render_mode);
	if (game)
	{
		const auto on_resolution = [this](int index)
		{
			const auto [text, value] = get_data(ui->resBox, index);
			ui->stereoRenderMode->setEnabled(value == static_cast<int>(video_resolution::_720p));
		};
		connect(ui->resBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, on_resolution);
		on_resolution(ui->resBox->currentIndex());
	}
	else
	{
		ui->stereoRenderMode->setCurrentIndex(find_item(ui->stereoRenderMode, static_cast<int>(g_cfg.video.stereo_render_mode.def)));
		ui->stereoRenderMode->setEnabled(false);
	}

	// Checkboxes: main options
	m_emu_settings->EnhanceCheckBox(ui->dumpColor, emu_settings_type::WriteColorBuffers);
	SubscribeTooltip(ui->dumpColor, tooltips.settings.dump_color);

	m_emu_settings->EnhanceCheckBox(ui->vsync, emu_settings_type::VSync);
	SubscribeTooltip(ui->vsync, tooltips.settings.vsync);

	m_emu_settings->EnhanceCheckBox(ui->stretchToDisplayArea, emu_settings_type::StretchToDisplayArea);
	SubscribeTooltip(ui->stretchToDisplayArea, tooltips.settings.stretch_to_display_area);

	m_emu_settings->EnhanceCheckBox(ui->multithreadedRSX, emu_settings_type::MultithreadedRSX);
	SubscribeTooltip(ui->multithreadedRSX, tooltips.settings.multithreaded_rsx);

	m_emu_settings->EnhanceCheckBox(ui->strictModeRendering, emu_settings_type::StrictRenderingMode);
	SubscribeTooltip(ui->strictModeRendering, tooltips.settings.strict_rendering_mode);
	const auto on_strict_rendering_mode = [this](bool checked)
	{
		ui->gb_resolutionScale->setEnabled(!checked);
		ui->gb_minimumScalableDimension->setEnabled(!checked);
		ui->gb_anisotropicFilter->setEnabled(!checked);
		ui->vulkansched->setEnabled(!checked);
	};
	connect(ui->strictModeRendering, &QCheckBox::toggled, this, on_strict_rendering_mode);

	m_emu_settings->EnhanceCheckBox(ui->asyncTextureStreaming, emu_settings_type::VulkanAsyncTextureUploads);
	SubscribeTooltip(ui->asyncTextureStreaming, tooltips.settings.async_texture_streaming);

	// Radio buttons

	SubscribeTooltip(ui->rb_legacy_recompiler, tooltips.settings.legacy_shader_recompiler);
	SubscribeTooltip(ui->rb_async_recompiler, tooltips.settings.async_shader_recompiler);
	SubscribeTooltip(ui->rb_async_with_shader_interpreter, tooltips.settings.async_with_shader_interpreter);
	SubscribeTooltip(ui->rb_shader_interpreter_only, tooltips.settings.shader_interpreter_only);

	QButtonGroup *shader_mode_bg = new QButtonGroup(this);
	shader_mode_bg->addButton(ui->rb_legacy_recompiler, static_cast<int>(shader_mode::recompiler));
	shader_mode_bg->addButton(ui->rb_async_recompiler, static_cast<int>(shader_mode::async_recompiler));
	shader_mode_bg->addButton(ui->rb_async_with_shader_interpreter, static_cast<int>(shader_mode::async_with_interpreter));
	shader_mode_bg->addButton(ui->rb_shader_interpreter_only, static_cast<int>(shader_mode::interpreter_only));

	m_emu_settings->EnhanceRadioButton(shader_mode_bg, emu_settings_type::ShaderMode);

	// Hide interpreter_only unless it is selected or the debug mode is active
	if (!ui->rb_shader_interpreter_only->isChecked() && !m_gui_settings->GetValue(gui::m_showDebugTab).toBool())
	{
		ui->rb_shader_interpreter_only->setVisible(false);
	}

	// Sliders

	m_emu_settings->EnhanceSlider(ui->resolutionScale, emu_settings_type::ResolutionScale);
	SubscribeTooltip(ui->gb_resolutionScale, tooltips.settings.resolution_scale);
	// rename label texts to fit current state of Resolution Scale
	const int resolution_scale_def = stoi(m_emu_settings->GetSettingDefault(emu_settings_type::ResolutionScale));
	const auto scaled_resolution = [resolution_scale_def](int percentage)
	{
		if (percentage == resolution_scale_def)
		{
			return tr("100% (1280x720) (Default)", "Resolution scale");
		}
		return tr("%1% (%2x%3)", "Resolution scale").arg(percentage).arg(1280 * percentage / 100).arg(720 * percentage / 100);
	};
	ui->resolutionScale->setPageStep(50);
	ui->resolutionScaleMin->setText(QString::number(ui->resolutionScale->minimum()));
	ui->resolutionScaleMin->setFixedWidth(gui::utils::get_label_width(QStringLiteral("00")));
	ui->resolutionScaleMax->setText(QString::number(ui->resolutionScale->maximum()));
	ui->resolutionScaleMax->setFixedWidth(gui::utils::get_label_width(QStringLiteral("0000")));
	ui->resolutionScaleVal->setText(scaled_resolution(ui->resolutionScale->value()));
	connect(ui->resolutionScale, &QSlider::valueChanged, [scaled_resolution, this](int value)
	{
		ui->resolutionScaleVal->setText(scaled_resolution(value));
	});
	connect(ui->resolutionScaleReset, &QAbstractButton::clicked, [resolution_scale_def, this]()
	{
		ui->resolutionScale->setValue(resolution_scale_def);
	});
	SnapSlider(ui->resolutionScale, 25);

	m_emu_settings->EnhanceSlider(ui->minimumScalableDimension, emu_settings_type::MinimumScalableDimension);
	SubscribeTooltip(ui->gb_minimumScalableDimension, tooltips.settings.minimum_scalable_dimension);
	// rename label texts to fit current state of Minimum Scalable Dimension
	const int minimum_scalable_dimension_def = stoi(m_emu_settings->GetSettingDefault(emu_settings_type::MinimumScalableDimension));
	const auto min_scalable_dimension = [minimum_scalable_dimension_def](int dim)
	{
		if (dim == minimum_scalable_dimension_def)
		{
			return tr("%1x%1 (Default)", "Minimum scalable dimension").arg(dim);
		}
		return tr("%1x%1", "Minimum scalable dimension").arg(dim);
	};
	ui->minimumScalableDimension->setPageStep(64);
	ui->minimumScalableDimensionMin->setText(QString::number(ui->minimumScalableDimension->minimum()));
	ui->minimumScalableDimensionMin->setFixedWidth(gui::utils::get_label_width(QStringLiteral("00")));
	ui->minimumScalableDimensionMax->setText(QString::number(ui->minimumScalableDimension->maximum()));
	ui->minimumScalableDimensionMax->setFixedWidth(gui::utils::get_label_width(QStringLiteral("0000")));
	ui->minimumScalableDimensionVal->setText(min_scalable_dimension(ui->minimumScalableDimension->value()));
	connect(ui->minimumScalableDimension, &QSlider::valueChanged, [min_scalable_dimension, this](int value)
	{
		ui->minimumScalableDimensionVal->setText(min_scalable_dimension(value));
	});
	connect(ui->minimumScalableDimensionReset, &QAbstractButton::clicked, [minimum_scalable_dimension_def, this]()
	{
		ui->minimumScalableDimension->setValue(minimum_scalable_dimension_def);
	});

	const int fsr_sharpening_strength_def = stoi(m_emu_settings->GetSettingDefault(emu_settings_type::FsrSharpeningStrength));
	const auto fmt_fsr_sharpening_strength = [fsr_sharpening_strength_def](int value)
	{
		if (value == fsr_sharpening_strength_def)
		{
			return tr("%1% (Default)").arg(value);
		}
		return tr("%1%").arg(value);
	};
	m_emu_settings->EnhanceSlider(ui->fsrSharpeningStrength, emu_settings_type::FsrSharpeningStrength);
	SubscribeTooltip(ui->fsrSharpeningStrengthWidget, tooltips.settings.fsr_rcas_strength);
	ui->fsrSharpeningStrengthVal->setText(fmt_fsr_sharpening_strength(ui->fsrSharpeningStrength->value()));
	connect(ui->fsrSharpeningStrength, &QSlider::valueChanged, [fmt_fsr_sharpening_strength, this](int value)
	{
		ui->fsrSharpeningStrengthVal->setText(fmt_fsr_sharpening_strength(value));
	});
	connect(ui->fsrSharpeningStrengthReset, &QAbstractButton::clicked, [fsr_sharpening_strength_def, this]()
	{
		ui->fsrSharpeningStrength->setValue(fsr_sharpening_strength_def);
	});

	// DoubleSpinBoxes
	m_emu_settings->EnhanceDoubleSpinBox(ui->TextureLODBias, emu_settings_type::TextureLodBias);
	SubscribeTooltip(ui->gb_Level_of_Detail, tooltips.settings.texture_lod_bias);
	connect(ui->TextureLODBiasReset, &QAbstractButton::clicked, [this]()
	{
		ui->TextureLODBias->setValue(stod(m_emu_settings->GetSettingDefault(emu_settings_type::TextureLodBias)));
	});

	// Remove renderers from the renderer Combobox if not supported
	for (const auto& renderer : r_creator->renderers)
	{
		if (renderer->supported)
		{
			if (renderer->has_adapters)
			{
				renderer->old_adapter = QString::fromStdString(m_emu_settings->GetSetting(renderer->type));
			}
			continue;
		}

		for (int i = 0; i < ui->renderBox->count(); i++)
		{
			if (ui->renderBox->itemText(i) == renderer->name)
			{
				ui->renderBox->removeItem(i);
				break;
			}
		}
	}

	m_old_renderer = ui->renderBox->currentText();

	const auto set_renderer = [r_creator, this](const QString& text)
	{
		if (text.isEmpty())
		{
			return;
		}

		const auto switchTo = [r_creator, text, this](const render_creator::render_info& renderer)
		{
			// Reset other adapters to old config
			for (const auto& render : r_creator->renderers)
			{
				if (renderer.name != render->name && render->has_adapters && render->supported)
				{
					m_emu_settings->SetSetting(render->type, render->old_adapter.toStdString());
				}
			}

			ui->graphicsAdapterBox->clear();

			// Fill combobox with placeholder if no adapters needed
			if (!renderer.has_adapters)
			{
				ui->graphicsAdapterBox->clear();
				ui->graphicsAdapterBox->setPlaceholderText(tr("Not needed for %0 renderer", "Graphics adapter").arg(text));
				return;
			}

			// Fill combobox
			ui->graphicsAdapterBox->clear();
			ui->graphicsAdapterBox->addItems(renderer.adapters);

			// Reset Adapter to old config
			int idx = ui->graphicsAdapterBox->findText(renderer.old_adapter);
			if (idx < 0)
			{
				idx = 0;
				if (renderer.old_adapter.isEmpty())
				{
					rsx_log.warning("%s adapter config empty: setting to default!", renderer.name);
				}
				else
				{
					rsx_log.warning("Last used %s adapter not found: setting to default!", renderer.name);
				}
			}
			ui->graphicsAdapterBox->setCurrentIndex(idx);
			m_emu_settings->SetSetting(renderer.type, ui->graphicsAdapterBox->currentText().toStdString());
		};

		for (const auto& renderer : r_creator->renderers)
		{
			if (renderer->name == text)
			{
				switchTo(*renderer);
				ui->graphicsAdapterBox->setEnabled(renderer->has_adapters);
			}
		}
	};

	const auto set_adapter = [r_creator, this](const QString& text)
	{
		if (text.isEmpty())
		{
			return;
		}

		// don't set adapter if signal was created by switching render
		const QString new_renderer = ui->renderBox->currentText();
		if (m_old_renderer != new_renderer)
		{
			m_old_renderer = new_renderer;
			return;
		}
		for (const auto& render : r_creator->renderers)
		{
			if (render->name == new_renderer && render->has_adapters && render->adapters.contains(text))
			{
				m_emu_settings->SetSetting(render->type, text.toStdString());
				break;
			}
		}
	};

	// Init
	set_renderer(ui->renderBox->currentText());
	set_adapter(ui->graphicsAdapterBox->currentText());

	// Events
	connect(ui->graphicsAdapterBox, &QComboBox::currentTextChanged, set_adapter);
	connect(ui->renderBox, &QComboBox::currentTextChanged, set_renderer);

	const auto apply_renderer_specific_options = [r_creator, this](const QString& text)
	{
		// Vulkan-only
		const bool is_vulkan = (text == r_creator->Vulkan.name);
		ui->asyncTextureStreaming->setEnabled(is_vulkan);
		ui->vulkansched->setEnabled(is_vulkan);
	};

	const auto apply_fsr_specific_options = [r_creator, this]()
	{
		const auto [text, value] = get_data(ui->outputScalingMode, ui->outputScalingMode->currentIndex());
		const bool fsr_selected = static_cast<output_scaling_mode>(value) == output_scaling_mode::fsr;
		ui->fsrSharpeningStrength->setEnabled(fsr_selected);
		ui->fsrSharpeningStrengthReset->setEnabled(fsr_selected);
	};

	// Handle connects to disable specific checkboxes that depend on GUI state.
	on_strict_rendering_mode(ui->strictModeRendering->isChecked());
	apply_renderer_specific_options(ui->renderBox->currentText()); // Init
	apply_fsr_specific_options();
	connect(ui->renderBox, &QComboBox::currentTextChanged, apply_renderer_specific_options);
	connect(ui->renderBox, &QComboBox::currentTextChanged, this, apply_fsr_specific_options);
	connect(ui->outputScalingMode, QOverload<int>::of(&QComboBox::currentIndexChanged), this, apply_fsr_specific_options);

	//                      _ _         _______    _
	//       /\            | (_)       |__   __|  | |
	//      /  \  _   _  __| |_  ___      | | __ _| |__
	//     / /\ \| | | |/ _` | |/ _ \     | |/ _` | '_ ＼
	//    / ____ \ |_| | (_| | | (_) |    | | (_| | |_) |
	//   /_/    \_\__,_|\__,_|_|\___/     |_|\__,_|_.__/

	const QString mic_none = m_emu_settings->m_microphone_creator.get_none();

	const auto change_microphone_type = [mic_none, this](int index)
	{
		if (index < 0)
		{
			return;
		}

		const auto [text, handler_id] = get_data(ui->microphoneBox, index);
		int max = 0;

		switch (static_cast<microphone_handler>(handler_id))
		{
		case microphone_handler::standard:
			max = 4;
			break;
		case microphone_handler::singstar:
			max = 2;
			break;
		case microphone_handler::real_singstar:
		case microphone_handler::rocksmith:
			max = 1;
			break;
		case microphone_handler::null:
			break;
		}

		ui->microphone1Box->setEnabled(max > 0);
		ui->microphone2Box->setEnabled(max > 1 && ui->microphone1Box->currentText() != mic_none);
		ui->microphone3Box->setEnabled(max > 2 && ui->microphone2Box->currentText() != mic_none);
		ui->microphone4Box->setEnabled(ui->microphone3Box->isEnabled() && ui->microphone3Box->currentText() != mic_none);
	};

	const auto propagate_used_devices = [mic_none, change_microphone_type, this]()
	{
		for (u32 index = 0; index < m_mics_combo.size(); index++)
		{
			const QString cur_item = m_mics_combo[index]->currentText();
			QStringList cur_list = m_emu_settings->m_microphone_creator.get_microphone_list();
			for (u32 subindex = 0; subindex < m_mics_combo.size(); subindex++)
			{
				if (subindex != index && m_mics_combo[subindex]->currentText() != mic_none)
					cur_list.removeOne(m_mics_combo[subindex]->currentText());
			}
			m_mics_combo[index]->blockSignals(true);
			m_mics_combo[index]->clear();
			m_mics_combo[index]->addItems(cur_list);
			m_mics_combo[index]->setCurrentText(cur_item);
			m_mics_combo[index]->blockSignals(false);
		}
		change_microphone_type(ui->microphoneBox->currentIndex());
	};

	const auto change_microphone_device = [mic_none, propagate_used_devices, this](u32 index, const QString& text)
	{
		m_emu_settings->SetSetting(emu_settings_type::MicrophoneDevices, m_emu_settings->m_microphone_creator.set_device(index, text));
		if (const u32 next_index = index + 1; next_index < m_mics_combo.size() && text == mic_none)
			m_mics_combo[next_index]->setCurrentText(mic_none);
		propagate_used_devices();
	};

	const auto get_audio_output_devices = [this](bool keep_old = true)
	{
		const auto [text, value] = get_data(ui->audioOutBox, ui->audioOutBox->currentIndex());
		auto dev_enum = Emu.GetCallbacks().get_audio_enumerator(value);
		std::vector<audio_device_enumerator::audio_device> dev_array = dev_enum->get_output_devices();

		ui->audioDeviceBox->clear();
		ui->audioDeviceBox->blockSignals(true);
		ui->audioDeviceBox->addItem(tr("Default"), QString(audio_device_enumerator::DEFAULT_DEV_ID.data()));

		const std::string selected_device = m_emu_settings->GetSetting(emu_settings_type::AudioDevice);
		int device_index = 0;

		for (const audio_device_enumerator::audio_device& dev : dev_array)
		{
			const QString cur_item = QString::fromStdString(dev.id);
			ui->audioDeviceBox->addItem(QString::fromStdString(dev.name), cur_item);
			if (selected_device == dev.id)
			{
				device_index = ui->audioDeviceBox->findData(cur_item);
			}
		}

		if (device_index == 0 && keep_old && selected_device != audio_device_enumerator::DEFAULT_DEV_ID)
		{
			cfg_log.error("The selected audio device (%s) was not found", selected_device);
			ui->audioDeviceBox->addItem(tr("Unknown device"), QString::fromStdString(selected_device));
			device_index = ui->audioDeviceBox->count() - 1;
		}

		ui->audioDeviceBox->blockSignals(false);
		ui->audioDeviceBox->setCurrentIndex(std::max(device_index, 0));
	};

	const auto change_audio_output_device = [this](int index)
	{
		if (index < 0)
		{
			return;
		}

		const QVariant item_data = ui->audioDeviceBox->itemData(index);
		m_emu_settings->SetSetting(emu_settings_type::AudioDevice, item_data.toString().toStdString());
		ui->audioDeviceBox->setCurrentIndex(index);
	};

	// Comboboxes

	m_emu_settings->EnhanceComboBox(ui->audioOutBox, emu_settings_type::AudioRenderer);
#ifdef WIN32
	SubscribeTooltip(ui->gb_audio_out, tooltips.settings.audio_out);
#else
	SubscribeTooltip(ui->gb_audio_out, tooltips.settings.audio_out_linux);
#endif
	connect(ui->audioOutBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [change_audio_output_device, get_audio_output_devices](int)
	{
		get_audio_output_devices(false);
		change_audio_output_device(0); // Set device to 'Default'
	});

	m_emu_settings->EnhanceComboBox(ui->combo_audio_channel_layout, emu_settings_type::AudioChannelLayout);
	SubscribeTooltip(ui->gb_audio_channel_layout, tooltips.settings.audio_channel_layout);

	connect(ui->combo_audio_format, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int index)
	{
		const auto [text, value] = get_data(ui->combo_audio_format, index);
		ui->list_audio_formats->setEnabled(static_cast<audio_format>(value) == audio_format::manual);
	});
	m_emu_settings->EnhanceComboBox(ui->combo_audio_format, emu_settings_type::AudioFormat);
	SubscribeTooltip(ui->gb_audio_format, tooltips.settings.audio_format);

	// Manual audio format selection
	const std::string audio_formats_str = m_emu_settings->GetSetting(emu_settings_type::AudioFormats);
	u64 selected_audio_formats = 0;
	if (!try_to_uint64(&selected_audio_formats, audio_formats_str, 0, 0xFFFFFFFF))
	{
		cfg_log.error("Can not interpret AudioFormats value '%s' as number", audio_formats_str);
	}
	const std::array<audio_format_flag, 5> audio_formats = {
		audio_format_flag::lpcm_2_48khz,
		audio_format_flag::lpcm_5_1_48khz,
		audio_format_flag::lpcm_7_1_48khz,
		audio_format_flag::ac3,
		audio_format_flag::dts,
	};
	for (const audio_format_flag& audio_fmt : audio_formats)
	{
		const QString audio_format_name = m_emu_settings->GetLocalizedSetting(QString(), emu_settings_type::AudioFormats, static_cast<int>(audio_fmt), true);
		QListWidgetItem* item = new QListWidgetItem(audio_format_name, ui->list_audio_formats);
		item->setData(Qt::UserRole, static_cast<u32>(audio_fmt));
		if (audio_fmt == audio_format_flag::lpcm_2_48khz)
		{
			item->setFlags(item->flags() & ~Qt::ItemIsEnabled);
			item->setCheckState(Qt::Checked);
		}
		else
		{
			item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
			item->setCheckState(!!(selected_audio_formats & static_cast<u32>(audio_fmt)) ? Qt::Checked : Qt::Unchecked);
		}
		ui->list_audio_formats->addItem(item);
	}
	connect(this, &settings_dialog::signal_restore_dependant_defaults, this, [this]()
	{
		const u32 default_audio_formats = std::stoi(m_emu_settings->GetSettingDefault(emu_settings_type::AudioFormats));
		for (int i = 0; i < ui->list_audio_formats->count(); ++i)
		{
			const auto& item = ui->list_audio_formats->item(i);
			const u32 audio_fmt = item->data(Qt::UserRole).toUInt();
			if (audio_fmt == static_cast<u32>(audio_format_flag::lpcm_2_48khz))
			{
				item->setCheckState(Qt::Checked);
			}
			else
			{
				item->setCheckState(!!(default_audio_formats & audio_fmt) ? Qt::Checked : Qt::Unchecked);
			}
		}
	});

	m_emu_settings->EnhanceComboBox(ui->audioProviderBox, emu_settings_type::AudioProvider);
	SubscribeTooltip(ui->gb_audio_provider, tooltips.settings.audio_provider);
	ui->gb_audio_provider->setVisible(false); // Hidden for now. This option is forced on boot.

	m_emu_settings->EnhanceComboBox(ui->audioAvportBox, emu_settings_type::AudioAvport);
	SubscribeTooltip(ui->gb_audio_avport, tooltips.settings.audio_avport);

	SubscribeTooltip(ui->gb_audio_device, tooltips.settings.audio_device);
	connect(ui->audioDeviceBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, change_audio_output_device);
	connect(this, &settings_dialog::signal_restore_dependant_defaults, this, [change_audio_output_device]() { change_audio_output_device(0); }); // Set device to 'Default'
	get_audio_output_devices();

	// Microphone Comboboxes
	m_mics_combo[0] = ui->microphone1Box;
	m_mics_combo[1] = ui->microphone2Box;
	m_mics_combo[2] = ui->microphone3Box;
	m_mics_combo[3] = ui->microphone4Box;

	for (u32 i = 0; i < m_mics_combo.size(); i++)
	{
		connect(m_mics_combo[i], &QComboBox::currentTextChanged, this, [change_microphone_device, i](const QString& text) { change_microphone_device(i, text); });
		connect(this, &settings_dialog::signal_restore_dependant_defaults, this, [change_microphone_device, i, mic_none]() { change_microphone_device(i, mic_none); });
	}

	m_emu_settings->m_microphone_creator.refresh_list();
	propagate_used_devices(); // Fills comboboxes list

	m_emu_settings->m_microphone_creator.parse_devices(m_emu_settings->GetSetting(emu_settings_type::MicrophoneDevices));

	const std::array<std::string, 4> mic_sel_list = m_emu_settings->m_microphone_creator.get_selection_list();

	for (s32 index = static_cast<int>(mic_sel_list.size()) - 1; index >= 0; index--)
	{
		const QString qmic = QString::fromStdString(mic_sel_list[index]);

		if (qmic.isEmpty() || m_mics_combo[index]->findText(qmic) == -1)
		{
			m_mics_combo[index]->setCurrentText(mic_none);
			change_microphone_device(index, mic_none); // Ensures the value is set in config
		}
		else
		{
			m_mics_combo[index]->setCurrentText(qmic);
		}
	}

	m_emu_settings->EnhanceComboBox(ui->microphoneBox, emu_settings_type::MicrophoneType);
	SubscribeTooltip(ui->microphoneBox, tooltips.settings.microphone);
	connect(ui->microphoneBox, QOverload<int>::of(&QComboBox::currentIndexChanged), change_microphone_type);
	propagate_used_devices(); // Enables/Disables comboboxes and checks values from config for sanity

	// Checkboxes

	m_emu_settings->EnhanceCheckBox(ui->audioDump, emu_settings_type::DumpToFile);
	SubscribeTooltip(ui->audioDump, tooltips.settings.audio_dump);

	m_emu_settings->EnhanceCheckBox(ui->convert, emu_settings_type::ConvertTo16Bit);
	SubscribeTooltip(ui->convert, tooltips.settings.convert);

	m_emu_settings->EnhanceCheckBox(ui->enableBuffering, emu_settings_type::EnableBuffering);
	SubscribeTooltip(ui->enableBuffering, tooltips.settings.enable_buffering);

	m_emu_settings->EnhanceCheckBox(ui->enableTimeStretching, emu_settings_type::EnableTimeStretching);
	SubscribeTooltip(ui->enableTimeStretching, tooltips.settings.enable_time_stretching);

	// Sliders

	EnhanceSlider(emu_settings_type::MasterVolume, ui->masterVolume, ui->masterVolumeLabel, tr("Master: %0 %", "Master volume"));
	SubscribeTooltip(ui->master_volume, tooltips.settings.master_volume);

	EnhanceSlider(emu_settings_type::AudioBufferDuration, ui->audioBufferDuration, ui->audioBufferDurationLabel, tr("Audio Buffer Duration: %0 ms", "Audio buffer duration"));
	SubscribeTooltip(ui->audio_buffer_duration, tooltips.settings.audio_buffer_duration);

	EnhanceSlider(emu_settings_type::TimeStretchingThreshold, ui->timeStretchingThreshold, ui->timeStretchingThresholdLabel, tr("Time Stretching Threshold: %0 %", "Time stretching threshold"));
	SubscribeTooltip(ui->time_stretching_threshold, tooltips.settings.time_stretching_threshold);

	//    _____       __   ____    _______    _
	//   |_   _|     / /  / __ \  |__   __|  | |
	//     | |      / /  | |  | |    | | __ _| |__
	//     | |     / /   | |  | |    | |/ _` | '_ ＼
	//    _| |_   / /    | |__| |    | | (_| | |_) |
	//   |_____| /_/      \____/     |_|\__,_|_.__/

	// Comboboxes

	m_emu_settings->EnhanceComboBox(ui->keyboardHandlerBox, emu_settings_type::KeyboardHandler);
	SubscribeTooltip(ui->gb_keyboard_handler, tooltips.settings.keyboard_handler);

	m_emu_settings->EnhanceComboBox(ui->mouseHandlerBox, emu_settings_type::MouseHandler);
	SubscribeTooltip(ui->gb_mouse_handler, tooltips.settings.mouse_handler);

	m_emu_settings->EnhanceComboBox(ui->cameraTypeBox, emu_settings_type::CameraType);
	SubscribeTooltip(ui->gb_camera_type, tooltips.settings.camera_type);

	m_emu_settings->EnhanceComboBox(ui->cameraBox, emu_settings_type::Camera);
	SubscribeTooltip(ui->gb_camera_setting, tooltips.settings.camera);

	m_emu_settings->EnhanceComboBox(ui->cameraFlipBox, emu_settings_type::CameraFlip);
	SubscribeTooltip(ui->gb_camera_flip, tooltips.settings.camera_flip);

	{
		const std::string default_camera = m_emu_settings->GetSettingDefault(emu_settings_type::CameraID);
		const std::string selected_camera = m_emu_settings->GetSetting(emu_settings_type::CameraID);
		ui->cameraIdBox->addItem(tr("None", "Camera Device"), "");
		ui->cameraIdBox->addItem(tr("Default", "Camera Device"), QString::fromStdString(default_camera));
		for (const QCameraDevice& camera_info : QMediaDevices::videoInputs())
		{
			if (!camera_info.isNull())
			{
				ui->cameraIdBox->addItem(camera_info.description(), QString(camera_info.id()));
			}
		}
		if (const int index = ui->cameraIdBox->findData(QString::fromStdString(selected_camera)); index >= 0)
		{
			ui->cameraIdBox->setCurrentIndex(index);
		}
		else
		{
			cfg_log.error("The selected camera was not found. Selecting default camera as fallback.");
			ui->cameraIdBox->setCurrentIndex(ui->cameraIdBox->findData(QString::fromStdString(default_camera)));
		}
		connect(ui->cameraIdBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int index)
		{
			if (index >= 0) m_emu_settings->SetSetting(emu_settings_type::CameraID, ui->cameraIdBox->itemData(index).toString().toStdString());
		});
		connect(this, &settings_dialog::signal_restore_dependant_defaults, this, [this, default_camera]()
		{
			m_emu_settings->SetSetting(emu_settings_type::CameraID, default_camera);
			ui->cameraIdBox->setCurrentIndex(ui->cameraIdBox->findData(QString::fromStdString(default_camera)));
		});
		SubscribeTooltip(ui->gb_camera_id, tooltips.settings.camera_id);
	}

	m_emu_settings->EnhanceComboBox(ui->musicHandlerBox, emu_settings_type::MusicHandler);
	SubscribeTooltip(ui->gb_music_handler, tooltips.settings.music_handler);

	m_emu_settings->EnhanceComboBox(ui->padModeBox, emu_settings_type::PadHandlerMode);
	SubscribeTooltip(ui->gb_pad_mode, tooltips.settings.pad_mode);

	m_emu_settings->EnhanceComboBox(ui->moveBox, emu_settings_type::Move);
	SubscribeTooltip(ui->gb_move_handler, tooltips.settings.move);

	m_emu_settings->EnhanceComboBox(ui->buzzBox, emu_settings_type::Buzz);
	SubscribeTooltip(ui->gb_buzz_emulated, tooltips.settings.buzz);

	m_emu_settings->EnhanceComboBox(ui->turntableBox, emu_settings_type::Turntable);
	SubscribeTooltip(ui->gb_turntable_emulated, tooltips.settings.turntable);

	m_emu_settings->EnhanceComboBox(ui->ghltarBox, emu_settings_type::GHLtar);
	SubscribeTooltip(ui->gb_ghltar_emulated, tooltips.settings.ghltar);

	m_emu_settings->EnhanceCheckBox(ui->backgroundInputBox, emu_settings_type::BackgroundInput);
	SubscribeTooltip(ui->backgroundInputBox, tooltips.settings.background_input);

	m_emu_settings->EnhanceCheckBox(ui->padConnectionBox, emu_settings_type::PadConnection);
	SubscribeTooltip(ui->padConnectionBox, tooltips.settings.pad_connection);

	m_emu_settings->EnhanceCheckBox(ui->showMoveCursorBox, emu_settings_type::ShowMoveCursor);
	SubscribeTooltip(ui->showMoveCursorBox, tooltips.settings.show_move_cursor);

	m_emu_settings->EnhanceCheckBox(ui->lockOverlayInputToPlayerOne, emu_settings_type::LockOvlIptToP1);
	SubscribeTooltip(ui->lockOverlayInputToPlayerOne, tooltips.settings.lock_overlay_input_to_player_one);

#if HAVE_SDL3
	m_emu_settings->EnhanceCheckBox(ui->loadSdlMappings, emu_settings_type::SDLMappings);
	SubscribeTooltip(ui->loadSdlMappings, tooltips.settings.sdl_mappings);
#else
	ui->loadSdlMappings->setVisible(false);
#endif

#ifndef _WIN32
	// Remove raw mouse handler
	remove_item(ui->mouseHandlerBox, static_cast<int>(mouse_handler::raw), static_cast<int>(g_cfg.io.mouse.def));
	remove_item(ui->moveBox, static_cast<int>(move_handler::raw_mouse), static_cast<int>(g_cfg.io.move.def));
#endif

	// Midi
	const QString midi_none = m_emu_settings->m_midi_creator.get_none();
	const midi_device def_midi_device{ .type = midi_device_type::keyboard, .name = midi_none.toStdString() };
	const std::vector<std::string> midi_device_types = cfg::try_to_enum_list(&fmt_class_string<midi_device_type>::format);

	m_midi_type_combo[0] = ui->midiTypeBox1;
	m_midi_type_combo[1] = ui->midiTypeBox2;
	m_midi_type_combo[2] = ui->midiTypeBox3;
	m_midi_device_combo[0] = ui->midiDeviceBox1;
	m_midi_device_combo[1] = ui->midiDeviceBox2;
	m_midi_device_combo[2] = ui->midiDeviceBox3;

	SubscribeTooltip(ui->gb_midi_1, tooltips.settings.midi_devices);
	SubscribeTooltip(ui->gb_midi_2, tooltips.settings.midi_devices);
	SubscribeTooltip(ui->gb_midi_3, tooltips.settings.midi_devices);

	const auto propagate_midi_devices = [midi_none, this]()
	{
		for (u32 index = 0; index < m_midi_device_combo.size(); index++)
		{
			const QString cur_item = m_midi_device_combo[index]->currentText();
			QStringList cur_list = m_emu_settings->m_midi_creator.get_midi_list();
			for (u32 subindex = 0; subindex < m_midi_device_combo.size(); subindex++)
			{
				if (subindex != index && m_midi_device_combo[subindex]->currentText() != midi_none)
					cur_list.removeOne(m_midi_device_combo[subindex]->currentText());
			}
			m_midi_device_combo[index]->blockSignals(true);
			m_midi_device_combo[index]->clear();
			m_midi_device_combo[index]->addItems(cur_list);
			m_midi_device_combo[index]->setCurrentText(cur_item);
			m_midi_device_combo[index]->blockSignals(false);
		}
	};

	const auto change_midi_device = [propagate_midi_devices, this](u32 index, const midi_device& device)
	{
		m_emu_settings->SetSetting(emu_settings_type::MidiDevices, m_emu_settings->m_midi_creator.set_device(index, device));
		propagate_midi_devices();
	};

	for (u32 i = 0; i < m_midi_type_combo.size(); i++)
	{
		m_midi_type_combo[i]->blockSignals(true);
		for (const std::string& type_str : midi_device_types)
		{
			midi_device_type type{};
			if (u64 result; cfg::try_to_enum_value(&result, &fmt_class_string<midi_device_type>::format, type_str))
			{
				type = static_cast<midi_device_type>(static_cast<std::underlying_type_t<midi_device_type>>(result));
			}
			const QString type_name = m_emu_settings->GetLocalizedSetting(QString::fromStdString(fmt::format("%s", type)), emu_settings_type::MidiDevices, static_cast<int>(type), false);
			m_midi_type_combo[i]->addItem(type_name, static_cast<int>(type));
		}
		m_midi_type_combo[i]->blockSignals(false);

		connect(m_midi_type_combo[i], &QComboBox::currentTextChanged, this, [this, change_midi_device, i](const QString& /*text*/)
		{
			const midi_device device{ .type = static_cast<midi_device_type>(m_midi_type_combo[i]->currentData().toInt()), .name = m_midi_device_combo[i]->currentText().toStdString() };
			change_midi_device(i, device);
		});
		connect(m_midi_device_combo[i], &QComboBox::currentTextChanged, this, [this, change_midi_device, i](const QString& text)
		{
			const midi_device device{ .type = static_cast<midi_device_type>(m_midi_type_combo[i]->currentData().toInt()), .name = text.toStdString() };
			change_midi_device(i, device);
		});
		connect(this, &settings_dialog::signal_restore_dependant_defaults, this, [change_midi_device, i, def_midi_device]() { change_midi_device(i, def_midi_device); });
	}

	m_emu_settings->m_midi_creator.refresh_list();
	propagate_midi_devices(); // Fills comboboxes list

	m_emu_settings->m_midi_creator.parse_devices(m_emu_settings->GetSetting(emu_settings_type::MidiDevices));

	const std::array<midi_device, max_midi_devices> midi_sel_list = m_emu_settings->m_midi_creator.get_selection_list();

	for (s32 index = static_cast<int>(midi_sel_list.size()) - 1; index >= 0; index--)
	{
		const midi_device& device = midi_sel_list[index];
		const QString qmidi = QString::fromStdString(device.name);

		m_midi_type_combo[index]->setCurrentIndex(m_midi_type_combo[index]->findData(static_cast<int>(device.type)));

		if (qmidi.isEmpty() || m_midi_device_combo[index]->findText(qmidi) == -1)
		{
			m_midi_device_combo[index]->setCurrentText(midi_none);
			change_midi_device(index, def_midi_device); // Ensures the value is set in config
		}
		else
		{
			m_midi_device_combo[index]->setCurrentText(qmidi);
		}
	}

	propagate_midi_devices(); // Enables/Disables comboboxes and checks values from config for sanity

	//     _____           _                   _______    _
	//    / ____|         | |                 |__   __|  | |
	//   | (___  _   _ ___| |_ ___ _ __ ___      | | __ _| |__
	//    \___ \| | | / __| __/ _ \ '_ ` _ \     | |/ _` | '_ ＼
	//    ____) | |_| \__ \ ||  __/ | | | | |    | | (_| | |_) |
	//   |_____/ \__, |___/\__\___|_| |_| |_|    |_|\__,_|_.__/
	//            __/ |
	//           |___/

	// Comboboxes

	m_emu_settings->EnhanceComboBox(ui->sysLangBox, emu_settings_type::Language, false, false, 0, true);
	SubscribeTooltip(ui->gb_sysLang, tooltips.settings.system_language);

	m_emu_settings->EnhanceComboBox(ui->console_region, emu_settings_type::LicenseArea, false, false, 0, true);
	SubscribeTooltip(ui->gb_console_region, tooltips.settings.license_area);

	m_emu_settings->EnhanceComboBox(ui->keyboardType, emu_settings_type::KeyboardType, false, false, 0, true);
	SubscribeTooltip(ui->gb_keyboardType, tooltips.settings.keyboard_type);

	// Checkboxes

	m_emu_settings->EnhanceCheckBox(ui->enableHostRoot, emu_settings_type::EnableHostRoot);
	SubscribeTooltip(ui->enableHostRoot, tooltips.settings.enable_host_root);

	m_emu_settings->EnhanceCheckBox(ui->emptyHdd0Tmp, emu_settings_type::EmptyHdd0Tmp);
	SubscribeTooltip(ui->emptyHdd0Tmp, tooltips.settings.empty_hdd0_tmp);

	m_emu_settings->EnhanceCheckBox(ui->enableCacheClearing, emu_settings_type::LimitCacheSize);
	SubscribeTooltip(ui->gb_DiskCacheClearing, tooltips.settings.limit_cache_size);
	if (game)
		ui->gb_DiskCacheClearing->setDisabled(true);
	else
		connect(ui->enableCacheClearing, &QCheckBox::checkStateChanged, ui->maximumCacheSize, &QSlider::setEnabled);

	// Date Time Edit Box
	m_emu_settings->EnhanceDateTimeEdit(ui->console_time_edit, emu_settings_type::ConsoleTimeOffset, tr("dd MMM yyyy HH:mm"), true, true, 15000);
	connect(ui->console_time_reset, &QAbstractButton::clicked, [this]()
	{
		ui->console_time_edit->setDateTime(QDateTime::currentDateTime());
	});
	SubscribeTooltip(ui->gb_console_time, tooltips.settings.console_time_offset);

	// Sliders

	EnhanceSlider(emu_settings_type::MaximumCacheSize, ui->maximumCacheSize, ui->maximumCacheSizeLabel, tr("Maximum size: %0 MB", "Maximum cache size"));
	ui->maximumCacheSize->setEnabled(ui->enableCacheClearing->isChecked());

	// Radio Buttons

	// creating this in ui file keeps scrambling the order...
	QButtonGroup *enter_button_assignment_bg = new QButtonGroup(this);
	enter_button_assignment_bg->addButton(ui->enterButtonAssignCircle, 0);
	enter_button_assignment_bg->addButton(ui->enterButtonAssignCross, 1);

	m_emu_settings->EnhanceRadioButton(enter_button_assignment_bg, emu_settings_type::EnterButtonAssignment);
	SubscribeTooltip(ui->gb_enterButtonAssignment, tooltips.settings.enter_button_assignment);

	//    _   _      _                      _      _______    _
	//   | \ | |    | |                    | |    |__   __|  | |
	//   |  \| | ___| |___      _____  _ __| | __    | | __ _| |__
	//   | . ` |/ _ \ __\ \ /\ / / _ \| '__| |/ /    | |/ _` | '_ ＼
	//   | |\  |  __/ |_ \ V  V / (_) | |  |   <     | | (_| | |_) |
	//   |_| \_|\___|\__| \_/\_/ \___/|_|  |_|\_\    |_|\__,_|_.__/

	// Edits

	m_emu_settings->EnhanceLineEdit(ui->edit_dns, emu_settings_type::DNSAddress);
	SubscribeTooltip(ui->gb_edit_dns, tooltips.settings.dns);

	m_emu_settings->EnhanceLineEdit(ui->edit_bind, emu_settings_type::BindAddress);
	SubscribeTooltip(ui->gb_edit_bind, tooltips.settings.bind);
	ui->gb_edit_bind->setEnabled(!!game);

	m_emu_settings->EnhanceLineEdit(ui->edit_swaps, emu_settings_type::IpSwapList);
	SubscribeTooltip(ui->gb_edit_swaps, tooltips.settings.dns_swap);
	ui->gb_edit_swaps->setEnabled(!!game);

	m_emu_settings->EnhanceCheckBox(ui->enable_upnp, emu_settings_type::EnableUpnp);
	SubscribeTooltip(ui->enable_upnp, tooltips.settings.enable_upnp);

	// Comboboxes

	connect(ui->netStatusBox, QOverload<int>::of(&QComboBox::currentIndexChanged), [this](int index)
	{
		if (index < 0) return;
		const auto [text, value] = get_data(ui->netStatusBox, index);
		ui->gb_edit_dns->setEnabled(static_cast<np_internet_status>(value) != np_internet_status::disabled);
		ui->enable_upnp->setEnabled(static_cast<np_internet_status>(value) != np_internet_status::disabled);

		if (static_cast<np_internet_status>(value) == np_internet_status::disabled)
		{
			ui->psnStatusBox->setCurrentIndex(find_item(ui->psnStatusBox, static_cast<int>(g_cfg.net.psn_status.def)));
			ui->psnStatusBox->setEnabled(false);
		}
		else
		{
			ui->psnStatusBox->setEnabled(true);
		}
	});
	m_emu_settings->EnhanceComboBox(ui->netStatusBox, emu_settings_type::InternetStatus);
	SubscribeTooltip(ui->gb_netStatusBox, tooltips.settings.net_status);

	m_emu_settings->EnhanceComboBox(ui->psnStatusBox, emu_settings_type::PSNStatus);
	SubscribeTooltip(ui->gb_psnStatusBox, tooltips.settings.psn_status);

	settings_dialog::refresh_countrybox();
	connect(ui->psnCountryBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int index)
	{
		if (index < 0)
			return;

		const QVariant country_code = ui->psnCountryBox->itemData(index);

		if (!country_code.isValid() || !country_code.canConvert<QString>())
			return;

		m_emu_settings->SetSetting(emu_settings_type::PSNCountry, country_code.toString().toStdString());
	});

	SubscribeTooltip(ui->gb_psnCountryBox, tooltips.settings.psn_country);

	if (!game)
	{
		remove_item(ui->psnStatusBox, static_cast<int>(np_psn_status::psn_fake), static_cast<int>(g_cfg.net.psn_status.def));
	}

	//                _                               _   _______    _
	//       /\      | |                             | | |__   __|  | |
	//      /  \   __| |_   ____ _ _ __   ___ ___  __| |    | | __ _| |__
	//     / /\ \ / _` \ \ / / _` | '_ \ / __/ _ \/ _` |    | |/ _` | '_ ＼
	//    / ____ \ (_| |\ V / (_| | | | | (_|  __/ (_| |    | | (_| | |_) |
	//   /_/    \_\__,_| \_/ \__,_|_| |_|\___\___|\__,_|    |_|\__,_|_.__/


	// Checkboxes

	m_emu_settings->EnhanceCheckBox(ui->debugConsoleMode, emu_settings_type::DebugConsoleMode);
	SubscribeTooltip(ui->debugConsoleMode, tooltips.settings.debug_console_mode);

	m_emu_settings->EnhanceCheckBox(ui->accurateDFMA, emu_settings_type::AccurateDFMA);
	SubscribeTooltip(ui->accurateDFMA, tooltips.settings.accurate_dfma);
	ui->accurateDFMA->setDisabled(utils::has_fma3() || utils::has_fma4());

	m_emu_settings->EnhanceCheckBox(ui->accurateRSXAccess, emu_settings_type::AccurateRSXAccess);
	SubscribeTooltip(ui->accurateRSXAccess, tooltips.settings.accurate_rsx_access);

	m_emu_settings->EnhanceCheckBox(ui->accurateSpuDMA, emu_settings_type::AccurateSpuDMA);
	SubscribeTooltip(ui->accurateSpuDMA, tooltips.settings.accurate_spu_dma);

	m_emu_settings->EnhanceCheckBox(ui->ppuNJFixup, emu_settings_type::PPUNJFixup);
	SubscribeTooltip(ui->ppuNJFixup, tooltips.settings.fixup_ppunj);

	m_emu_settings->EnhanceCheckBox(ui->fixupPPUVNAN, emu_settings_type::FixupPPUVNAN);
	SubscribeTooltip(ui->fixupPPUVNAN, tooltips.settings.fixup_ppuvnan);

	m_emu_settings->EnhanceCheckBox(ui->llvmPrecompilation, emu_settings_type::LLVMPrecompilation);
	SubscribeTooltip(ui->llvmPrecompilation, tooltips.settings.llvm_precompilation);

	m_emu_settings->EnhanceCheckBox(ui->antiCheatSavestates, emu_settings_type::SuspendEmulationSavestateMode);
	SubscribeTooltip(ui->antiCheatSavestates, tooltips.settings.anti_cheat_savestates);

	m_emu_settings->EnhanceCheckBox(ui->compatibleSavestates, emu_settings_type::CompatibleEmulationSavestateMode);
	SubscribeTooltip(ui->compatibleSavestates, tooltips.settings.compatible_savestates);

	m_emu_settings->EnhanceCheckBox(ui->spuProfiler, emu_settings_type::SPUProfiler);
	SubscribeTooltip(ui->spuProfiler, tooltips.settings.spu_profiler);

	m_emu_settings->EnhanceCheckBox(ui->silenceAllLogs, emu_settings_type::SilenceAllLogs);
	SubscribeTooltip(ui->silenceAllLogs, tooltips.settings.silence_all_logs);

	m_emu_settings->EnhanceCheckBox(ui->readColor, emu_settings_type::ReadColorBuffers);
	SubscribeTooltip(ui->readColor, tooltips.settings.read_color);

	m_emu_settings->EnhanceCheckBox(ui->readDepth, emu_settings_type::ReadDepthBuffer);
	SubscribeTooltip(ui->readDepth, tooltips.settings.read_depth);

	m_emu_settings->EnhanceCheckBox(ui->dumpDepth, emu_settings_type::WriteDepthBuffer);
	SubscribeTooltip(ui->dumpDepth, tooltips.settings.dump_depth);

	m_emu_settings->EnhanceCheckBox(ui->handleTiledMemory, emu_settings_type::HandleRSXTiledMemory);
	SubscribeTooltip(ui->handleTiledMemory, tooltips.settings.handle_tiled_memory);

	m_emu_settings->EnhanceCheckBox(ui->vblankNTSCFixup, emu_settings_type::VBlankNTSCFixup);

	ui->mfcDelayCommand->setChecked(m_emu_settings->GetSetting(emu_settings_type::MFCCommandsShuffling) == "1");
	SubscribeTooltip(ui->mfcDelayCommand, tooltips.settings.mfc_delay_command);
	connect(ui->mfcDelayCommand, &QCheckBox::checkStateChanged, [&](Qt::CheckState val)
	{
		const std::string str = val != Qt::Unchecked ? "1" : "0";
		m_emu_settings->SetSetting(emu_settings_type::MFCCommandsShuffling, str);
	});

	m_emu_settings->EnhanceCheckBox(ui->disableMslFastMath, emu_settings_type::DisableMSLFastMath);
#ifdef __APPLE__
	SubscribeTooltip(ui->disableMslFastMath, tooltips.settings.disable_msl_fast_math);
#else
	ui->disableMslFastMath->setVisible(false);
#endif

	m_emu_settings->EnhanceCheckBox(ui->disableAsyncHostMM, emu_settings_type::DisableAsyncHostMM);
	SubscribeTooltip(ui->disableAsyncHostMM, tooltips.settings.disable_async_host_mm);

	m_emu_settings->EnhanceCheckBox(ui->disableSpinOptimization, emu_settings_type::DisableSpinOptimization);
	SubscribeTooltip(ui->disableSpinOptimization, tooltips.settings.disable_spin_optimization);

	m_emu_settings->EnhanceCheckBox(ui->enableSpuEventsBusyLoop, emu_settings_type::EnabledSPUEventsBusyLoop);
	SubscribeTooltip(ui->enableSpuEventsBusyLoop, tooltips.settings.enable_spu_events_busy_loop);

	// Comboboxes

	m_emu_settings->EnhanceComboBox(ui->maxSPURSThreads, emu_settings_type::MaxSPURSThreads, true);
	ui->maxSPURSThreads->setItemText(ui->maxSPURSThreads->findData(6), tr("Unlimited (Default)", "Max SPURS threads"));
	SubscribeTooltip(ui->gb_max_spurs_threads, tooltips.settings.max_spurs_threads);

	m_emu_settings->EnhanceComboBox(ui->exclusiveFullscreenMode, emu_settings_type::ExclusiveFullscreenMode);
	SubscribeTooltip(ui->gb_exclusiveFullscreen, tooltips.settings.exclusive_fullscreen_mode);

	m_emu_settings->EnhanceComboBox(ui->sleepTimersAccuracy, emu_settings_type::SleepTimersAccuracy);
	SubscribeTooltip(ui->gb_sleep_timers_accuracy, tooltips.settings.sleep_timers_accuracy);

	m_emu_settings->EnhanceComboBox(ui->FIFOAccuracy, emu_settings_type::FIFOAccuracy);
	SubscribeTooltip(ui->gb_rsx_fifo_accuracy, tooltips.settings.rsx_fifo_accuracy);

	// Hide a developers' setting
	remove_item(ui->FIFOAccuracy, static_cast<int>(rsx_fifo_mode::as_ps3), static_cast<int>(g_cfg.core.rsx_fifo_accuracy.def));

	// Sliders

	EnhanceSlider(emu_settings_type::DriverWakeUpDelay, ui->wakeupDelay, ui->wakeupText, tr(reinterpret_cast<const char*>(u8"%0 µs"), "Driver wake up delay"));
	SnapSlider(ui->wakeupDelay, 20);
	ui->wakeupDelay->setMaximum(800); // Very large values must be entered with config.yml changes
	ui->wakeupDelay->setPageStep(20);
	const int wakeup_def = stoi(m_emu_settings->GetSettingDefault(emu_settings_type::DriverWakeUpDelay));
	connect(ui->wakeupReset, &QAbstractButton::clicked, [wakeup_def, this]()
	{
		ui->wakeupDelay->setValue(wakeup_def);
	});

	EnhanceSlider(emu_settings_type::VBlankRate, ui->vblank, ui->vblankText, tr("%0 Hz", "VBlank rate"));
	SnapSlider(ui->vblank, 30);
	ui->vblank->setPageStep(60);
	const int vblank_def = stoi(m_emu_settings->GetSettingDefault(emu_settings_type::VBlankRate));
	connect(ui->vblankReset, &QAbstractButton::clicked, [vblank_def, this]()
	{
		ui->vblank->setValue(vblank_def);
	});

	EnhanceSlider(emu_settings_type::ClocksScale, ui->clockScale, ui->clockScaleText, tr("%0 %", "Clocks scale"));
	SnapSlider(ui->clockScale, 10);
	ui->clockScale->setPageStep(50);
	const int clocks_scale_def = stoi(m_emu_settings->GetSettingDefault(emu_settings_type::ClocksScale));
	connect(ui->clockScaleReset, &QAbstractButton::clicked, [clocks_scale_def, this]()
	{
		ui->clockScale->setValue(clocks_scale_def);
	});

	EnhanceSlider(emu_settings_type::MaxPreemptCount, ui->maxPreemptCount, ui->preemptText, tr(reinterpret_cast<const char*>(u8"%0"), "Max CPU preempt count"));
	SubscribeTooltip(ui->gb_max_preempt_count, tooltips.settings.max_cpu_preempt);

#ifdef _WIN32
	// Windows' thread execution slice is much larger than on other platforms
	SnapSlider(ui->maxPreemptCount, 5);
	ui->maxPreemptCount->setPageStep(20);
#else
	SnapSlider(ui->maxPreemptCount, 10);
	ui->maxPreemptCount->setPageStep(50);
#endif

	const int preempt_def = stoi(m_emu_settings->GetSettingDefault(emu_settings_type::MaxPreemptCount));
	connect(ui->preemptReset, &QAbstractButton::clicked, [preempt_def, this]()
	{
		ui->maxPreemptCount->setValue(preempt_def);
	});

	if (!game) // Prevent users from doing dumb things
	{
		ui->gb_vblank->setDisabled(true);
		SubscribeTooltip(ui->gb_vblank, tooltips.settings.disabled_from_global);
		ui->gb_clockScale->setDisabled(true);
		SubscribeTooltip(ui->gb_clockScale, tooltips.settings.disabled_from_global);
		ui->gb_wakeupDelay->setDisabled(true);
		SubscribeTooltip(ui->gb_wakeupDelay, tooltips.settings.disabled_from_global);
	}
	else
	{
		SubscribeTooltip(ui->vblankNTSCFixup, tooltips.settings.vblank_ntsc_fixup);
		SubscribeTooltip(ui->gb_vblank, tooltips.settings.vblank_rate);
		SubscribeTooltip(ui->gb_clockScale, tooltips.settings.clocks_scale);
		SubscribeTooltip(ui->gb_wakeupDelay, tooltips.settings.wake_up_delay);
	}

	std::vector<std::string> loadedLibs = m_emu_settings->GetLibrariesControl();

	std::set<std::string_view> set(loadedLibs.begin(), loadedLibs.end());

	for (const auto& lib : g_prx_list)
	{
		// -1: Override LLE
		// 1: Override HLE
		// 0: No override
		const int res = static_cast<int>(set.count(std::string(lib.first) + ":hle") - set.count(std::string(lib.first) + ":lle"));

		const auto list = (lib.second ? ui->hleList : ui->lleList);

		QListWidgetItem* item = new QListWidgetItem(QString(lib.first.data()), list);
		item->setFlags(item->flags() | Qt::ItemIsUserCheckable); // set checkable flag

		// If no override selected (res=0), checkbox is unchecked
		// Otherwise if the override does not match the default behaviour, checkbox is checked
		item->setCheckState(res && res != (lib.second * 2 - 1) ? Qt::Checked : Qt::Unchecked); // AND initialize check state

		item->setToolTip(!lib.second ? tooltips.settings.lib_default_lle :
			(lib.first.starts_with("libsysutil") ? tr("Do not touch libsysutil libs, development purposes only, will cause game crashes.") : tooltips.settings.lib_default_hle));

		list->addItem(item);
	}

	SubscribeTooltip(ui->lleList, tooltips.settings.lle_list);
	SubscribeTooltip(ui->hleList, tooltips.settings.hle_list);
	ui->searchBox->setPlaceholderText(tr("Search libraries", "Library search box"));

	const auto on_lib_state_changed = [this](const QString& text)
	{
		const QString search_term = text.toLower();
		std::vector<QListWidgetItem*> items, items2;

		// Take current items
		while (ui->lleList->count())
		{
			items.push_back(ui->lleList->takeItem(0));
		}

		while (ui->hleList->count())
		{
			items2.push_back(ui->hleList->takeItem(0));
		}

		// sort items: checked items first then alphabetical order
		const auto func = [](QListWidgetItem *i1, QListWidgetItem *i2)
		{
			return (i1->checkState() != i2->checkState()) ? (i1->checkState() > i2->checkState()) : (i1->text() < i2->text());
		};

		std::sort(items.begin(), items.end(), func);
		std::sort(items2.begin(), items2.end(), func);

		for (uint i = 0; i < items.size(); i++)
		{
			ui->lleList->addItem(items[i]);

			// only show items filtered for search text
			ui->lleList->setRowHidden(i, !items[i]->text().contains(search_term));
		}

		for (uint i = 0; i < items2.size(); i++)
		{
			ui->hleList->addItem(items2[i]);

			// only show items filtered for search text
			ui->hleList->setRowHidden(i, !items2[i]->text().contains(search_term));
		}
	};

	const auto reset_library_lists = [this, on_lib_state_changed]()
	{
		for (int i = 0; i < ui->lleList->count(); i++)
		{
			ui->lleList->item(i)->setCheckState(Qt::Unchecked);
		}

		for (int i = 0; i < ui->hleList->count(); i++)
		{
			ui->hleList->item(i)->setCheckState(Qt::Unchecked);
		}

		on_lib_state_changed(ui->searchBox->text());
	};

	// Sort libs
	on_lib_state_changed({});

	// Events
	connect(ui->searchBox, &QLineEdit::textChanged, on_lib_state_changed);
	connect(ui->resetLleList, &QAbstractButton::clicked, reset_library_lists);

	// enable multiselection (there must be a better way)
	connect(ui->lleList, &QListWidget::itemChanged, [this](QListWidgetItem* item)
	{
		for (auto cb : ui->lleList->selectedItems())
		{
			cb->setCheckState(item->checkState());
		}
	});

	connect(ui->hleList, &QListWidget::itemChanged, [this](QListWidgetItem* item)
	{
		for (auto cb : ui->hleList->selectedItems())
		{
			cb->setCheckState(item->checkState());
		}
	});

	connect(this, &settings_dialog::signal_restore_dependant_defaults, this, reset_library_lists);

	//    ______                 _       _               _______    _
	//   |  ____|               | |     | |             |__   __|  | |
	//   | |__   _ __ ___  _   _| | __ _| |_ ___  _ __     | | __ _| |__
	//   |  __| | '_ ` _ \| | | | |/ _` | __/ _ \| '__|    | |/ _` | '_ ＼
	//   | |____| | | | | | |_| | | (_| | || (_) | |       | | (_| | |_) |
	//   |______|_| |_| |_|\__,_|_|\__,_|\__\___/|_|       |_|\__,_|_.__/

	// Comboboxes

	m_emu_settings->EnhanceComboBox(ui->maxLLVMThreads, emu_settings_type::MaxLLVMThreads, true, true, utils::get_thread_count());
	SubscribeTooltip(ui->gb_max_llvm, tooltips.settings.max_llvm_threads);
	ui->maxLLVMThreads->setItemText(ui->maxLLVMThreads->findData(0), tr("All (%1)", "Max LLVM Compile Threads").arg(utils::get_thread_count()));

	m_emu_settings->EnhanceComboBox(ui->shaderCompilerThreads, emu_settings_type::ShaderCompilerNumThreads, true);
	SubscribeTooltip(ui->gb_shader_compiler_threads, tooltips.settings.shader_compiler_threads);
	ui->shaderCompilerThreads->setItemText(ui->shaderCompilerThreads->findData(0), tr("Auto", "Max Shader Compile Threads"));

	m_emu_settings->EnhanceComboBox(ui->perfOverlayDetailLevel, emu_settings_type::PerfOverlayDetailLevel);
	SubscribeTooltip(ui->perf_overlay_detail_level, tooltips.settings.perf_overlay_detail_level);

	m_emu_settings->EnhanceComboBox(ui->perfOverlayPosition, emu_settings_type::PerfOverlayPosition);
	SubscribeTooltip(ui->perf_overlay_position, tooltips.settings.perf_overlay_position);

	// Checkboxes

	m_emu_settings->EnhanceCheckBox(ui->exitOnStop, emu_settings_type::ExitRPCS3OnFinish);
	SubscribeTooltip(ui->exitOnStop, tooltips.settings.exit_on_stop);

	m_emu_settings->EnhanceCheckBox(ui->pauseOnFocusLoss, emu_settings_type::PauseOnFocusLoss);
	SubscribeTooltip(ui->pauseOnFocusLoss, tooltips.settings.pause_on_focus_loss);

	m_emu_settings->EnhanceCheckBox(ui->startGameFullscreen, emu_settings_type::StartGameFullscreen);
	SubscribeTooltip(ui->startGameFullscreen, tooltips.settings.start_game_fullscreen);

	m_emu_settings->EnhanceCheckBox(ui->preventDisplaySleep, emu_settings_type::PreventDisplaySleep);
	SubscribeTooltip(ui->preventDisplaySleep, tooltips.settings.prevent_display_sleep);
	ui->preventDisplaySleep->setEnabled(Emu.GetCallbacks().display_sleep_control_supported());

	m_emu_settings->EnhanceCheckBox(ui->showTrophyPopups, emu_settings_type::ShowTrophyPopups);
	SubscribeTooltip(ui->showTrophyPopups, tooltips.settings.show_trophy_popups);

	m_emu_settings->EnhanceCheckBox(ui->showRpcnPopups, emu_settings_type::ShowRpcnPopups);
	SubscribeTooltip(ui->showRpcnPopups, tooltips.settings.show_rpcn_popups);

	m_emu_settings->EnhanceCheckBox(ui->useNativeInterface, emu_settings_type::UseNativeInterface);
	SubscribeTooltip(ui->useNativeInterface, tooltips.settings.use_native_interface);

#if defined(__linux__)
	ui->enableGamemode->setVisible(true);
#endif
#if defined(GAMEMODE_AVAILABLE)
	ui->enableGamemode->setEnabled(true);
	m_emu_settings->EnhanceCheckBox(ui->enableGamemode, emu_settings_type::EnableGamemode);
	SubscribeTooltip(ui->enableGamemode, tooltips.settings.enable_gamemode);
#else
	SubscribeTooltip(ui->enableGamemode, tooltips.settings.no_gamemode);
#endif

	m_emu_settings->EnhanceCheckBox(ui->showShaderCompilationHint, emu_settings_type::ShowShaderCompilationHint);
	SubscribeTooltip(ui->showShaderCompilationHint, tooltips.settings.show_shader_compilation_hint);

	m_emu_settings->EnhanceCheckBox(ui->showPPUCompilationHint, emu_settings_type::ShowPPUCompilationHint);
	SubscribeTooltip(ui->showPPUCompilationHint, tooltips.settings.show_ppu_compilation_hint);

	m_emu_settings->EnhanceCheckBox(ui->showAutosaveAutoloadHint, emu_settings_type::ShowAutosaveAutoloadHint);
	SubscribeTooltip(ui->showAutosaveAutoloadHint, tooltips.settings.show_autosave_autoload_hint);

	m_emu_settings->EnhanceCheckBox(ui->showPressureIntensityToggleHint, emu_settings_type::ShowPressureIntensityToggleHint);
	SubscribeTooltip(ui->showPressureIntensityToggleHint, tooltips.settings.show_pressure_intensity_toggle_hint);

	m_emu_settings->EnhanceCheckBox(ui->showAnalogLimiterToggleHint, emu_settings_type::ShowAnalogLimiterToggleHint);
	SubscribeTooltip(ui->showAnalogLimiterToggleHint, tooltips.settings.show_analog_limiter_toggle_hint);

	m_emu_settings->EnhanceCheckBox(ui->showMouseAndKeyboardToggleHint, emu_settings_type::ShowMouseAndKeyboardToggleHint);
	SubscribeTooltip(ui->showMouseAndKeyboardToggleHint, tooltips.settings.show_mouse_and_keyboard_toggle_hint);

	m_emu_settings->EnhanceCheckBox(ui->pauseDuringHomeMenu, emu_settings_type::PauseDuringHomeMenu);
	SubscribeTooltip(ui->pauseDuringHomeMenu, tooltips.settings.pause_during_home_menu);

	m_emu_settings->EnhanceCheckBox(ui->pausedSavestates, emu_settings_type::StartSavestatePaused);
	SubscribeTooltip(ui->pausedSavestates, tooltips.settings.paused_savestates);

	m_emu_settings->EnhanceCheckBox(ui->perfOverlayCenterX, emu_settings_type::PerfOverlayCenterX);
	SubscribeTooltip(ui->perfOverlayCenterX, tooltips.settings.perf_overlay_center_x);
	connect(ui->perfOverlayCenterX, &QCheckBox::toggled, [this](bool checked)
	{
		ui->perfOverlayMarginX->setEnabled(!checked);
	});
	ui->perfOverlayMarginX->setEnabled(!ui->perfOverlayCenterX->isChecked());

	m_emu_settings->EnhanceCheckBox(ui->perfOverlayCenterY, emu_settings_type::PerfOverlayCenterY);
	SubscribeTooltip(ui->perfOverlayCenterY, tooltips.settings.perf_overlay_center_y);
	connect(ui->perfOverlayCenterY, &QCheckBox::toggled, [this](bool checked)
	{
		ui->perfOverlayMarginY->setEnabled(!checked);
	});
	ui->perfOverlayMarginY->setEnabled(!ui->perfOverlayCenterY->isChecked());

	m_emu_settings->EnhanceCheckBox(ui->perfOverlayFramerateGraphEnabled, emu_settings_type::PerfOverlayFramerateGraphEnabled);
	SubscribeTooltip(ui->perfOverlayFramerateGraphEnabled, tooltips.settings.perf_overlay_framerate_graph_enabled);

	m_emu_settings->EnhanceCheckBox(ui->perfOverlayFrametimeGraphEnabled, emu_settings_type::PerfOverlayFrametimeGraphEnabled);
	SubscribeTooltip(ui->perfOverlayFrametimeGraphEnabled, tooltips.settings.perf_overlay_frametime_graph_enabled);

	m_emu_settings->EnhanceCheckBox(ui->perfOverlayEnabled, emu_settings_type::PerfOverlayEnabled);
	SubscribeTooltip(ui->perfOverlayEnabled, tooltips.settings.perf_overlay_enabled);
	const auto enable_perf_overlay_options = [this](bool enabled)
	{
		ui->label_detail_level->setEnabled(enabled);
		ui->label_update_interval->setEnabled(enabled);
		ui->label_font_size->setEnabled(enabled);
		ui->label_position->setEnabled(enabled);
		ui->label_opacity->setEnabled(enabled);
		ui->label_margin_x->setEnabled(enabled);
		ui->label_margin_y->setEnabled(enabled);
		ui->perfOverlayDetailLevel->setEnabled(enabled);
		ui->perfOverlayPosition->setEnabled(enabled);
		ui->perfOverlayUpdateInterval->setEnabled(enabled);
		ui->perfOverlayFontSize->setEnabled(enabled);
		ui->perfOverlayOpacity->setEnabled(enabled);
		ui->perfOverlayMarginX->setEnabled(enabled && !ui->perfOverlayCenterX->isChecked());
		ui->perfOverlayMarginY->setEnabled(enabled && !ui->perfOverlayCenterY->isChecked());
		ui->perfOverlayCenterX->setEnabled(enabled);
		ui->perfOverlayCenterY->setEnabled(enabled);
		ui->perfOverlayFramerateGraphEnabled->setEnabled(enabled);
		ui->perfOverlayFrametimeGraphEnabled->setEnabled(enabled);
		ui->perf_overlay_framerate_datapoints->setEnabled(enabled);
		ui->perf_overlay_frametime_datapoints->setEnabled(enabled);
	};
	enable_perf_overlay_options(ui->perfOverlayEnabled->isChecked());
	connect(ui->perfOverlayEnabled, &QCheckBox::toggled, enable_perf_overlay_options);

	m_emu_settings->EnhanceCheckBox(ui->shaderLoadBgEnabled, emu_settings_type::ShaderLoadBgEnabled);
	SubscribeTooltip(ui->shaderLoadBgEnabled, tooltips.settings.shader_load_bg_enabled);
	const auto enable_shader_loader_options = [this](bool enabled)
	{
		ui->label_shaderLoadBgDarkening->setEnabled(enabled);
		ui->label_shaderLoadBgBlur->setEnabled(enabled);
		ui->shaderLoadBgDarkening->setEnabled(enabled);
		ui->shaderLoadBgBlur->setEnabled(enabled);
	};
	enable_shader_loader_options(ui->shaderLoadBgEnabled->isChecked());
	connect(ui->shaderLoadBgEnabled, &QCheckBox::toggled, enable_shader_loader_options);

	// Sliders

	EnhanceSlider(emu_settings_type::PerfOverlayUpdateInterval, ui->perfOverlayUpdateInterval, ui->label_update_interval, tr("Update Interval: %0 ms", "Performance overlay update interval"));
	SubscribeTooltip(ui->perf_overlay_update_interval, tooltips.settings.perf_overlay_update_interval);

	EnhanceSlider(emu_settings_type::PerfOverlayFontSize, ui->perfOverlayFontSize, ui->label_font_size, tr("Font Size: %0 px", "Performance overlay font size"));
	SubscribeTooltip(ui->perf_overlay_font_size, tooltips.settings.perf_overlay_font_size);

	EnhanceSlider(emu_settings_type::PerfOverlayOpacity, ui->perfOverlayOpacity, ui->label_opacity, tr("Opacity: %0 %", "Performance overlay opacity"));
	SubscribeTooltip(ui->perf_overlay_opacity, tooltips.settings.perf_overlay_opacity);

	EnhanceSlider(emu_settings_type::PerfOverlayFramerateDatapoints, ui->slider_framerate_datapoints, ui->label_framerate_datapoints, tr("Framerate datapoints: %0", "Framerate graph datapoints"));
	SubscribeTooltip(ui->perf_overlay_framerate_datapoints, tooltips.settings.perf_overlay_framerate_datapoints);

	EnhanceSlider(emu_settings_type::PerfOverlayFrametimeDatapoints, ui->slider_frametime_datapoints, ui->label_frametime_datapoints, tr("Frametime datapoints: %0", "Frametime graph datapoints"));
	SubscribeTooltip(ui->perf_overlay_frametime_datapoints, tooltips.settings.perf_overlay_frametime_datapoints);

	EnhanceSlider(emu_settings_type::ShaderLoadBgDarkening, ui->shaderLoadBgDarkening, ui->label_shaderLoadBgDarkening, tr("Background darkening: %0 %", "Shader load background darkening"));
	SubscribeTooltip(ui->shaderLoadBgDarkening, tooltips.settings.shader_load_bg_darkening);

	EnhanceSlider(emu_settings_type::ShaderLoadBgBlur, ui->shaderLoadBgBlur, ui->label_shaderLoadBgBlur, tr("Background blur: %0 %", "Shader load background blur"));
	SubscribeTooltip(ui->shaderLoadBgBlur, tooltips.settings.shader_load_bg_blur);

	// SpinBoxes

	m_emu_settings->EnhanceSpinBox(ui->perfOverlayMarginX, emu_settings_type::PerfOverlayMarginX, "", tr("px", "Performance overlay margin x"));
	SubscribeTooltip(ui->perfOverlayMarginX, tooltips.settings.perf_overlay_margin_x);

	m_emu_settings->EnhanceSpinBox(ui->perfOverlayMarginY, emu_settings_type::PerfOverlayMarginY, "", tr("px", "Performance overlay margin y"));
	SubscribeTooltip(ui->perfOverlayMarginY, tooltips.settings.perf_overlay_margin_y);

	// Global settings (gui_settings)
	if (!game)
	{
		SubscribeTooltip(ui->gs_resizeOnBoot_widget, tooltips.settings.resize_on_boot);
		SubscribeTooltip(ui->gs_disableMouse, tooltips.settings.disable_mouse);
		SubscribeTooltip(ui->gs_disableKbHotkeys, tooltips.settings.disable_kb_hotkeys);
		SubscribeTooltip(ui->gs_showMouseInFullscreen, tooltips.settings.show_mouse_in_fullscreen);
		SubscribeTooltip(ui->gs_lockMouseInFullscreen, tooltips.settings.lock_mouse_in_fullscreen);
		SubscribeTooltip(ui->gs_hideMouseOnIdle_widget, tooltips.settings.hide_mouse_on_idle);

		ui->gs_disableMouse->setChecked(m_gui_settings->GetValue(gui::gs_disableMouse).toBool());
		connect(ui->gs_disableMouse, &QCheckBox::toggled, [this](bool checked)
		{
			m_gui_settings->SetValue(gui::gs_disableMouse, checked);
		});

		ui->gs_disableKbHotkeys->setChecked(m_gui_settings->GetValue(gui::gs_disableKbHotkeys).toBool());
		connect(ui->gs_disableKbHotkeys, &QCheckBox::toggled, [this](bool checked)
		{
			m_gui_settings->SetValue(gui::gs_disableKbHotkeys, checked);
		});

		ui->gs_showMouseInFullscreen->setChecked(m_gui_settings->GetValue(gui::gs_showMouseFs).toBool());
		connect(ui->gs_showMouseInFullscreen, &QCheckBox::toggled, [this](bool checked)
		{
			m_gui_settings->SetValue(gui::gs_showMouseFs, checked);
		});

		ui->gs_lockMouseInFullscreen->setChecked(m_gui_settings->GetValue(gui::gs_lockMouseFs).toBool());
		connect(ui->gs_lockMouseInFullscreen, &QCheckBox::toggled, [this](bool checked)
		{
			m_gui_settings->SetValue(gui::gs_lockMouseFs, checked);
		});

		ui->gs_hideMouseOnIdle->setChecked(m_gui_settings->GetValue(gui::gs_hideMouseIdle).toBool());
		connect(ui->gs_hideMouseOnIdle, &QCheckBox::toggled, [this](bool checked)
		{
			m_gui_settings->SetValue(gui::gs_hideMouseIdle, checked);
			ui->gs_hideMouseOnIdleTime->setEnabled(checked);
		});
		ui->gs_hideMouseOnIdleTime->setEnabled(ui->gs_hideMouseOnIdle->checkState() == Qt::CheckState::Checked);
		ui->gs_hideMouseOnIdleTime->setValue(m_gui_settings->GetValue(gui::gs_hideMouseIdleTime).toUInt());
		connect(ui->gs_hideMouseOnIdleTime, &QSpinBox::editingFinished, [this]()
		{
			m_gui_settings->SetValue(gui::gs_hideMouseIdleTime, ui->gs_hideMouseOnIdleTime->value());
		});

		const bool enable_buttons = m_gui_settings->GetValue(gui::gs_resize).toBool();
		const bool use_manual_resize = m_gui_settings->GetValue(gui::gs_resize_manual).toBool();
		ui->gs_resizeOnBoot->setChecked(enable_buttons);
		ui->gs_resizeOnBootManual->setChecked(use_manual_resize);
		ui->gs_resizeOnBootManual->setEnabled(enable_buttons);
		ui->gs_width->setEnabled(enable_buttons && use_manual_resize);
		ui->gs_height->setEnabled(enable_buttons && use_manual_resize);

		const QRect screen = QGuiApplication::primaryScreen()->geometry();
		const int width = m_gui_settings->GetValue(gui::gs_width).toInt();
		const int height = m_gui_settings->GetValue(gui::gs_height).toInt();
		ui->gs_width->setValue(std::min(width, screen.width()));
		ui->gs_height->setValue(std::min(height, screen.height()));

		connect(ui->gs_resizeOnBoot, &QCheckBox::toggled, [this](bool checked)
		{
			const bool enabled = checked && ui->gs_resizeOnBootManual->isChecked();
			m_gui_settings->SetValue(gui::gs_resize, checked);
			ui->gs_resizeOnBootManual->setEnabled(checked);
			ui->gs_width->setEnabled(enabled);
			ui->gs_height->setEnabled(enabled);
		});
		connect(ui->gs_resizeOnBootManual, &QCheckBox::toggled, [this](bool checked)
		{
			const bool enabled = checked && ui->gs_resizeOnBoot->isChecked();
			m_gui_settings->SetValue(gui::gs_resize_manual, checked);
			ui->gs_width->setEnabled(enabled);
			ui->gs_height->setEnabled(enabled);
		});
		connect(ui->gs_width, &QSpinBox::editingFinished, [this]()
		{
			ui->gs_width->setValue(std::min(ui->gs_width->value(), QGuiApplication::primaryScreen()->size().width()));
			m_gui_settings->SetValue(gui::gs_width, ui->gs_width->value());
		});
		connect(ui->gs_height, &QSpinBox::editingFinished, [this]()
		{
			ui->gs_height->setValue(std::min(ui->gs_height->value(), QGuiApplication::primaryScreen()->size().height()));
			m_gui_settings->SetValue(gui::gs_height, ui->gs_height->value());
		});
	}
	else
	{
		ui->gb_viewport->setEnabled(false);
		ui->gb_viewport->setVisible(false);
	}

	// Game window title builder

	const auto get_game_window_title = [this, game](const QString& format)
	{
		rpcs3::title_format_data title_data;
		title_data.format = format.toStdString();
		title_data.renderer = m_emu_settings->GetSetting(emu_settings_type::Renderer);
		title_data.vulkan_adapter = m_emu_settings->GetSetting(emu_settings_type::VulkanAdapter);
		title_data.fps = 60.;

		if (game)
		{
			title_data.title = game->name;
			title_data.title_id = game->serial;
		}
		else
		{
			title_data.title = tr("My Game", "Game window title").toStdString();
			title_data.title_id = "ABCD12345";
		}

		const std::string game_window_title = rpcs3::get_formatted_title(title_data);

		if (game_window_title.empty())
		{
			return QStringLiteral("RPCS3");
		}

		return QString::fromStdString(game_window_title);
	};

	const auto set_game_window_title = [get_game_window_title, this](const std::string& format)
	{
		const QString game_window_title_format = QString::fromStdString(format);
		const QString game_window_title = get_game_window_title(game_window_title_format);
		const int width = ui->label_game_window_title_format->sizeHint().width();
		const QFontMetrics metrics = ui->label_game_window_title_format->fontMetrics();
		const QString elided_text = metrics.elidedText(game_window_title_format, Qt::ElideRight, width);
		const QString tooltip = game_window_title_format + QStringLiteral("\n\n") + game_window_title;

		ui->label_game_window_title_format->setText(elided_text);
		ui->label_game_window_title_format->setToolTip(tooltip);
	};

	connect(ui->edit_button_game_window_title_format, &QAbstractButton::clicked, [get_game_window_title, set_game_window_title, this]()
	{
		auto get_game_window_title_label = [get_game_window_title, set_game_window_title, this](const QString& format)
		{
			const QString game_window_title = get_game_window_title(format);

			const std::vector<std::pair<const QString, const QString>> window_title_glossary =
			{
				{ "%G", tr("GPU Model", "Game window title") },
				{ "%C", tr("CPU Model", "Game window title") },
				{ "%c", tr("Thread Count", "Game window title") },
				{ "%M", tr("System Memory", "Game window title") },
				{ "%F", tr("Framerate", "Game window title") },
				{ "%R", tr("Renderer", "Game window title") },
				{ "%T", tr("Title", "Game window title") },
				{ "%t", tr("Title ID", "Game window title") },
				{ "%V", tr("RPCS3 Version", "Game window title") }
			};

			QString glossary;

			for (const auto& entry : window_title_glossary)
			{
				glossary += entry.first + "\t = " + entry.second + "\n";
			}

			return tr("Glossary:\n\n%0\nPreview:\n\n%1\n", "Game window title").arg(glossary).arg(game_window_title);
		};

		const std::string game_title_format = m_emu_settings->GetSetting(emu_settings_type::WindowTitleFormat);

		QString edited_format = QString::fromStdString(game_title_format);

		input_dialog dlg(-1, edited_format, tr("Game Window Title Format", "Game window title"), get_game_window_title_label(edited_format), "", this);
		dlg.resize(width() * .75, dlg.height());

		connect(&dlg, &input_dialog::text_changed, [&edited_format, &dlg, get_game_window_title_label](const QString& text)
		{
			edited_format = text.simplified();
			dlg.set_label_text(get_game_window_title_label(edited_format));
		});

		if (dlg.exec() == QDialog::Accepted)
		{
			m_emu_settings->SetSetting(emu_settings_type::WindowTitleFormat, edited_format.toStdString());
			set_game_window_title(m_emu_settings->GetSetting(emu_settings_type::WindowTitleFormat));
		}
	});

	const auto reset_game_window_title_format = [set_game_window_title, this]()
	{
		const std::string default_game_title_format = m_emu_settings->GetSettingDefault(emu_settings_type::WindowTitleFormat);
		m_emu_settings->SetSetting(emu_settings_type::WindowTitleFormat, default_game_title_format);
		set_game_window_title(default_game_title_format);
	};
	connect(ui->reset_button_game_window_title_format, &QAbstractButton::clicked, this, reset_game_window_title_format);
	connect(this, &settings_dialog::signal_restore_dependant_defaults, this, reset_game_window_title_format);

	// Load and apply the configured game window title format
	set_game_window_title(m_emu_settings->GetSetting(emu_settings_type::WindowTitleFormat));

	SubscribeTooltip(ui->gb_game_window_title, tooltips.settings.game_window_title_format);

	//     _____  _    _  _   _______    _
	//    / ____|| |  | || | |__   __|  | |
	//   | |  __|| |  | || |    | | __ _| |__
	//   | | |_ || |  | || |    | |/ _` | '_ ＼
	//   | |__| || |__| || |    | | (_| | |_) |
	//    \_____| \____/ |_|    |_|\__,_|_.__/

	if (!game)
	{
		SubscribeTooltip(ui->gb_stylesheets, tooltips.settings.stylesheets);
		SubscribeTooltip(ui->cb_custom_colors, tooltips.settings.custom_colors);
		SubscribeTooltip(ui->cb_show_welcome, tooltips.settings.show_welcome);
		SubscribeTooltip(ui->cb_show_exit_game, tooltips.settings.show_exit_game);
		SubscribeTooltip(ui->cb_show_boot_game, tooltips.settings.show_boot_game);
		SubscribeTooltip(ui->cb_show_pkg_install, tooltips.settings.show_pkg_install);
		SubscribeTooltip(ui->cb_show_pup_install, tooltips.settings.show_pup_install);
		SubscribeTooltip(ui->cb_show_obsolete_cfg_dialog, tooltips.settings.show_obsolete_cfg);
		SubscribeTooltip(ui->cb_show_same_buttons_dialog, tooltips.settings.show_same_buttons);
		SubscribeTooltip(ui->cb_show_restart_hint, tooltips.settings.show_restart_hint);
		SubscribeTooltip(ui->gb_updates, tooltips.settings.check_update_start);
		SubscribeTooltip(ui->gb_uuid, tooltips.settings.uuid);

		// Pad navigation
		SubscribeTooltip(ui->cb_pad_navigation, tooltips.settings.pad_navigation);
		SubscribeTooltip(ui->cb_global_pad_navigation, tooltips.settings.global_navigation);
		ui->cb_pad_navigation->setChecked(m_gui_settings->GetValue(gui::nav_enabled).toBool());
		ui->cb_global_pad_navigation->setChecked(m_gui_settings->GetValue(gui::nav_global).toBool());
#if defined(_WIN32) || defined(__linux__) || defined(__APPLE__)
		connect(ui->cb_pad_navigation, &QCheckBox::toggled, [this](bool checked)
		{
			m_gui_settings->SetValue(gui::nav_enabled, checked);
		});
		connect(ui->cb_global_pad_navigation, &QCheckBox::toggled, [this](bool checked)
		{
			m_gui_settings->SetValue(gui::nav_global, checked);
		});
#else
		ui->gb_gui_pad_input->setEnabled(false);
#endif

		// Discord:
		SubscribeTooltip(ui->useRichPresence, tooltips.settings.use_rich_presence);
		SubscribeTooltip(ui->discordState, tooltips.settings.discord_state);
		ui->useRichPresence->setChecked(m_use_discord);
		ui->label_discordState->setEnabled(m_use_discord);
		ui->discordState->setEnabled(m_use_discord);
		ui->discordState->setText(m_discord_state);

		connect(ui->useRichPresence, &QCheckBox::toggled, [this](bool checked)
		{
			ui->discordState->setEnabled(checked);
			ui->label_discordState->setEnabled(checked);
			m_use_discord = checked;
		});

		connect(ui->discordState, &QLineEdit::editingFinished, [this]()
		{
			m_discord_state = ui->discordState->text();
		});

		connect(ui->pb_uuid, &QAbstractButton::clicked, [this]()
		{
			std::string uuid;
			if (!gui::utils::create_new_uuid(uuid))
			{
				QMessageBox::critical(this, tr("Error"), tr("Failed to create new installation ID!"), QMessageBox::Ok);
				return;
			}

			ui->label_uuid->setText(QString::fromStdString(uuid));
			gui::utils::log_uuid();
		});
		ui->label_uuid->setText(QString::fromStdString(gui::utils::load_uuid()));

		// Log and TTY:
		SubscribeTooltip(ui->log_limit, tooltips.settings.log_limit);
		SubscribeTooltip(ui->tty_limit, tooltips.settings.tty_limit);

		ui->spinbox_log_limit->setValue(m_gui_settings->GetValue(gui::l_limit).toInt());
		connect(ui->spinbox_log_limit, &QSpinBox::editingFinished, [this]()
		{
			m_gui_settings->SetValue(gui::l_limit, ui->spinbox_log_limit->value());
		});

		ui->spinbox_tty_limit->setValue(m_gui_settings->GetValue(gui::l_limit_tty).toInt());
		connect(ui->spinbox_tty_limit, &QSpinBox::editingFinished, [this]()
		{
			m_gui_settings->SetValue(gui::l_limit_tty, ui->spinbox_tty_limit->value());
		});

		// colorize preview icons
		const auto add_colored_icon = [this](QPushButton *button, const QColor& color, const QIcon& icon = QIcon(), const QColor& iconColor = QColor())
		{
			QLabel* text = new QLabel(button->text());
			text->setObjectName("color_button");
			text->setAlignment(Qt::AlignCenter);
			text->setAttribute(Qt::WA_TransparentForMouseEvents, true);
			delete button->layout();
			if (icon.isNull())
			{
				QPixmap pixmap(100, 100);
				pixmap.fill(color);
				button->setIcon(pixmap);
			}
			else
			{
				button->setIcon(gui::utils::get_colorized_icon(icon, iconColor, color, true));
			}
			button->setText("");
			button->setStyleSheet(styleSheet().append("text-align:left;"));
			button->setLayout(new QGridLayout);
			button->layout()->setContentsMargins(0, 0, 0, 0);
			button->layout()->addWidget(text);
		};

		add_colored_icon(ui->pb_gl_icon_color, m_gui_settings->GetValue(gui::gl_iconColor).value<QColor>());
		add_colored_icon(ui->pb_sd_icon_color, m_gui_settings->GetValue(gui::sd_icon_color).value<QColor>());
		add_colored_icon(ui->pb_tr_icon_color, m_gui_settings->GetValue(gui::tr_icon_color).value<QColor>());

		ui->cb_show_welcome->setChecked(m_gui_settings->GetValue(gui::ib_show_welcome).toBool());
		ui->cb_show_exit_game->setChecked(m_gui_settings->GetValue(gui::ib_confirm_exit).toBool());
		ui->cb_show_boot_game->setChecked(m_gui_settings->GetValue(gui::ib_confirm_boot).toBool());
		ui->cb_show_pkg_install->setChecked(m_gui_settings->GetValue(gui::ib_pkg_success).toBool());
		ui->cb_show_pup_install->setChecked(m_gui_settings->GetValue(gui::ib_pup_success).toBool());
		ui->cb_show_obsolete_cfg_dialog->setChecked(m_gui_settings->GetValue(gui::ib_obsolete_cfg).toBool());
		ui->cb_show_same_buttons_dialog->setChecked(m_gui_settings->GetValue(gui::ib_same_buttons).toBool());
		ui->cb_show_restart_hint->setChecked(m_gui_settings->GetValue(gui::ib_restart_hint).toBool());

		ui->combo_updates->addItem(tr("Yes", "Updates"), gui::update_on);
		ui->combo_updates->addItem(tr("Background", "Updates"), gui::update_bkg);
		ui->combo_updates->addItem(tr("Automatic", "Updates"), gui::update_auto);
		ui->combo_updates->addItem(tr("No", "Updates"), gui::update_off);
		ui->combo_updates->setCurrentIndex(ui->combo_updates->findData(m_gui_settings->GetValue(gui::m_check_upd_start).toString()));
		connect(ui->combo_updates, QOverload<int>::of(&QComboBox::currentIndexChanged), [this](int index)
		{
			if (index >= 0) m_gui_settings->SetValue(gui::m_check_upd_start, ui->combo_updates->itemData(index));
		});

		const bool enable_ui_colors = m_gui_settings->GetValue(gui::m_enableUIColors).toBool();
		ui->cb_custom_colors->setChecked(enable_ui_colors);
		ui->pb_gl_icon_color->setEnabled(enable_ui_colors);
		ui->pb_sd_icon_color->setEnabled(enable_ui_colors);
		ui->pb_tr_icon_color->setEnabled(enable_ui_colors);

		connect(ui->pb_apply_stylesheet, &QAbstractButton::clicked, this, [this]() { ApplyStylesheet(false); });

		connect(ui->cb_show_welcome, &QCheckBox::toggled, [this](bool checked)
		{
			m_gui_settings->SetValue(gui::ib_show_welcome, checked);
		});
		connect(ui->cb_show_exit_game, &QCheckBox::toggled, [this](bool checked)
		{
			m_gui_settings->SetValue(gui::ib_confirm_exit, checked);
		});
		connect(ui->cb_show_boot_game, &QCheckBox::toggled, [this](bool checked)
		{
			m_gui_settings->SetValue(gui::ib_confirm_boot, checked);
		});
		connect(ui->cb_show_pkg_install, &QCheckBox::toggled, [this](bool checked)
		{
			m_gui_settings->SetValue(gui::ib_pkg_success, checked);
		});
		connect(ui->cb_show_pup_install, &QCheckBox::toggled, [this](bool checked)
		{
			m_gui_settings->SetValue(gui::ib_pup_success, checked);
		});
		connect(ui->cb_show_obsolete_cfg_dialog, &QCheckBox::toggled, [this](bool checked)
		{
			m_gui_settings->SetValue(gui::ib_obsolete_cfg, checked);
		});
		connect(ui->cb_show_same_buttons_dialog, &QCheckBox::toggled, [this](bool checked)
		{
			m_gui_settings->SetValue(gui::ib_same_buttons, checked);
		});
		connect(ui->cb_show_restart_hint, &QCheckBox::toggled, [this](bool checked)
		{
			m_gui_settings->SetValue(gui::ib_restart_hint, checked);
		});

		connect(ui->cb_custom_colors, &QCheckBox::toggled, [this](bool checked)
		{
			m_gui_settings->SetValue(gui::m_enableUIColors, checked);
			ui->pb_gl_icon_color->setEnabled(checked);
			ui->pb_sd_icon_color->setEnabled(checked);
			ui->pb_tr_icon_color->setEnabled(checked);
			Q_EMIT GuiRepaintRequest();
		});

		const auto color_dialog = [&](const gui_save& color, const QString& title, QPushButton *button)
		{
			const QColor old_color = m_gui_settings->GetValue(color).value<QColor>();
			QColorDialog dlg(old_color, this);
			dlg.setWindowTitle(title);
			dlg.setOptions(QColorDialog::ShowAlphaChannel);
			for (int i = 0; i < QColorDialog::customCount(); i++)
			{
				QColorDialog::setCustomColor(i, m_gui_settings->GetCustomColor(i));
			}
			if (dlg.exec() == QColorDialog::Accepted)
			{
				for (int i = 0; i < QColorDialog::customCount(); i++)
				{
					m_gui_settings->SetCustomColor(i, QColorDialog::customColor(i));
				}
				m_gui_settings->SetValue(color, dlg.selectedColor());
				button->setIcon(gui::utils::get_colorized_icon(button->icon(), old_color, dlg.selectedColor(), true));
				Q_EMIT GuiRepaintRequest();
			}
		};

		connect(ui->pb_gl_icon_color, &QAbstractButton::clicked, [color_dialog, this]()
		{
			color_dialog(gui::gl_iconColor, tr("Choose gamelist icon color", "Settings: color dialog"), ui->pb_gl_icon_color);
		});
		connect(ui->pb_sd_icon_color, &QAbstractButton::clicked, [color_dialog, this]()
		{
			color_dialog(gui::sd_icon_color, tr("Choose save manager icon color", "Settings: color dialog"), ui->pb_sd_icon_color);
		});
		connect(ui->pb_tr_icon_color, &QAbstractButton::clicked, [color_dialog, this]()
		{
			color_dialog(gui::tr_icon_color, tr("Choose trophy manager icon color", "Settings: color dialog"), ui->pb_tr_icon_color);
		});

		AddStylesheets();
	}

	//    _____       _                   _______    _
	//   |  __ \     | |                 |__   __|  | |
	//   | |  | | ___| |__  _   _  __ _     | | __ _| |__
	//   | |  | |/ _ \ '_ \| | | |/ _` |    | |/ _` | '_ ＼
	//   | |__| |  __/ |_) | |_| | (_| |    | | (_| | |_) |
	//   |_____/ \___|_.__/ \__,_|\__, |    |_|\__,_|_.__/
	//                             __/ |
	//                            |___/

	// Checkboxes: gpu debug options
	m_emu_settings->EnhanceCheckBox(ui->renderdocCompatibility, emu_settings_type::RenderdocCompatibility);
	SubscribeTooltip(ui->renderdocCompatibility, tooltips.settings.renderdoc_compatibility);

	m_emu_settings->EnhanceCheckBox(ui->forceHighpZ, emu_settings_type::ForceHighpZ);
	SubscribeTooltip(ui->forceHighpZ, tooltips.settings.force_high_pz);

	m_emu_settings->EnhanceCheckBox(ui->debugOutput, emu_settings_type::DebugOutput);
	SubscribeTooltip(ui->debugOutput, tooltips.settings.debug_output);

	m_emu_settings->EnhanceCheckBox(ui->debugOverlay, emu_settings_type::DebugOverlay);
	SubscribeTooltip(ui->debugOverlay, tooltips.settings.debug_overlay);

	m_emu_settings->EnhanceCheckBox(ui->logProg, emu_settings_type::LogShaderPrograms);
	SubscribeTooltip(ui->logProg, tooltips.settings.log_shader_programs);

	m_emu_settings->EnhanceCheckBox(ui->disableHwOcclusionQueries, emu_settings_type::DisableOcclusionQueries);
	SubscribeTooltip(ui->disableHwOcclusionQueries, tooltips.settings.disable_occlusion_queries);

	m_emu_settings->EnhanceCheckBox(ui->disableVideoOutput, emu_settings_type::DisableVideoOutput);
	SubscribeTooltip(ui->disableVideoOutput, tooltips.settings.disable_video_output);

	m_emu_settings->EnhanceCheckBox(ui->forceCpuBlitEmulation, emu_settings_type::ForceCPUBlitEmulation);
	SubscribeTooltip(ui->forceCpuBlitEmulation, tooltips.settings.force_cpu_blit_emulation);

	m_emu_settings->EnhanceCheckBox(ui->disableVulkanMemAllocator, emu_settings_type::DisableVulkanMemAllocator);
	SubscribeTooltip(ui->disableVulkanMemAllocator, tooltips.settings.disable_vulkan_mem_allocator);

	m_emu_settings->EnhanceCheckBox(ui->disableFIFOReordering, emu_settings_type::DisableFIFOReordering);
	SubscribeTooltip(ui->disableFIFOReordering, tooltips.settings.disable_fifo_reordering);

	m_emu_settings->EnhanceCheckBox(ui->strictTextureFlushing, emu_settings_type::StrictTextureFlushing);
	SubscribeTooltip(ui->strictTextureFlushing, tooltips.settings.strict_texture_flushing);

	m_emu_settings->EnhanceCheckBox(ui->gpuTextureScaling, emu_settings_type::GPUTextureScaling);
	SubscribeTooltip(ui->gpuTextureScaling, tooltips.settings.gpu_texture_scaling);

	m_emu_settings->EnhanceCheckBox(ui->allowHostGPULabels, emu_settings_type::AllowHostGPULabels);
	SubscribeTooltip(ui->allowHostGPULabels, tooltips.settings.allow_host_labels);

	m_emu_settings->EnhanceCheckBox(ui->disableVertexCache, emu_settings_type::DisableVertexCache);
	SubscribeTooltip(ui->disableVertexCache, tooltips.settings.disable_vertex_cache);

	m_emu_settings->EnhanceCheckBox(ui->forceHwMSAAResolve, emu_settings_type::ForceHwMSAAResolve);
	SubscribeTooltip(ui->forceHwMSAAResolve, tooltips.settings.force_hw_MSAA);

	m_emu_settings->EnhanceCheckBox(ui->disableOnDiskShaderCache, emu_settings_type::DisableOnDiskShaderCache);
	SubscribeTooltip(ui->disableOnDiskShaderCache, tooltips.settings.disable_on_disk_shader_cache);

	// Checkboxes: core debug options
	m_emu_settings->EnhanceCheckBox(ui->alwaysStart, emu_settings_type::StartOnBoot);
	SubscribeTooltip(ui->alwaysStart, tooltips.settings.start_on_boot);

	m_emu_settings->EnhanceCheckBox(ui->ppuDebug, emu_settings_type::PPUDebug);
	SubscribeTooltip(ui->ppuDebug, tooltips.settings.ppu_debug);

	m_emu_settings->EnhanceCheckBox(ui->spuDebug, emu_settings_type::SPUDebug);
	SubscribeTooltip(ui->spuDebug, tooltips.settings.spu_debug);

	m_emu_settings->EnhanceCheckBox(ui->mfcDebug, emu_settings_type::MFCDebug);
	SubscribeTooltip(ui->mfcDebug, tooltips.settings.mfc_debug);

	m_emu_settings->EnhanceCheckBox(ui->setDAZandFTZ, emu_settings_type::SetDAZandFTZ);
	SubscribeTooltip(ui->setDAZandFTZ, tooltips.settings.set_daz_and_ftz);

	m_emu_settings->EnhanceCheckBox(ui->accuratePPUSAT, emu_settings_type::AccuratePPUSAT);
	SubscribeTooltip(ui->accuratePPUSAT, tooltips.settings.accurate_ppusat);

	m_emu_settings->EnhanceCheckBox(ui->accuratePPUNJ, emu_settings_type::AccuratePPUNJ);
	SubscribeTooltip(ui->accuratePPUNJ, tooltips.settings.accurate_ppunj);

	m_emu_settings->EnhanceCheckBox(ui->accuratePPUVNAN, emu_settings_type::AccuratePPUVNAN);
	SubscribeTooltip(ui->accuratePPUVNAN, tooltips.settings.accurate_ppuvnan);

	m_emu_settings->EnhanceCheckBox(ui->accuratePPUFPCC, emu_settings_type::AccuratePPUFPCC);
	SubscribeTooltip(ui->accuratePPUFPCC, tooltips.settings.accurate_ppufpcc);

	m_emu_settings->EnhanceCheckBox(ui->accurateClineStores, emu_settings_type::AccurateClineStores);
	SubscribeTooltip(ui->accurateClineStores, tooltips.settings.accurate_cache_line_stores);

	m_emu_settings->EnhanceCheckBox(ui->hookStFunc, emu_settings_type::HookStaticFuncs);
	SubscribeTooltip(ui->hookStFunc, tooltips.settings.hook_static_functions);

	m_emu_settings->EnhanceCheckBox(ui->perfReport, emu_settings_type::PerformanceReport);
	SubscribeTooltip(ui->perfReport, tooltips.settings.enable_performance_report);

	// Checkboxes: IO debug options
	m_emu_settings->EnhanceCheckBox(ui->debugOverlayIO, emu_settings_type::IoDebugOverlay);
	SubscribeTooltip(ui->debugOverlayIO, tooltips.settings.debug_overlay_io);

	// Comboboxes

	m_emu_settings->EnhanceComboBox(ui->combo_accurate_ppu_128, emu_settings_type::AccuratePPU128Loop, true);
	SubscribeTooltip(ui->gb_accurate_ppu_128, tooltips.settings.accurate_ppu_128_loop);
	ui->combo_accurate_ppu_128->setItemText(ui->combo_accurate_ppu_128->findData(-1), tr("Always Enabled", "Accurate PPU 128 Reservations"));
	ui->combo_accurate_ppu_128->setItemText(ui->combo_accurate_ppu_128->findData(0), tr("Disabled", "Accurate PPU 128 Reservations"));

	m_emu_settings->EnhanceComboBox(ui->combo_num_ppu_threads, emu_settings_type::NumPPUThreads, true);
	SubscribeTooltip(ui->gb_num_ppu_threads, tooltips.settings.num_ppu_threads);

	m_emu_settings->EnhanceComboBox(ui->vulkansched, emu_settings_type::VulkanAsyncSchedulerDriver);
	SubscribeTooltip(ui->gb_vulkansched, tooltips.settings.vulkan_async_scheduler);

	if (!restoreGeometry(m_gui_settings->GetValue(gui::cfg_geometry).toByteArray()))
	{
		// Ignore. This will most likely only fail if the setting doesn't contain any values
	}
}

void settings_dialog::refresh_countrybox()
{
	const auto& vec_countries = countries::g_countries;
	const std::string cur_country = m_emu_settings->GetSetting(emu_settings_type::PSNCountry);

	ui->psnCountryBox->clear();

	for (const auto& [cnty, code] : vec_countries)
	{
		ui->psnCountryBox->addItem(QString::fromUtf8(cnty.data(), static_cast<int>(cnty.size())), QString::fromUtf8(code.data(), static_cast<int>(code.size())));
	}
	ui->psnCountryBox->setCurrentIndex(ui->psnCountryBox->findData(QString::fromStdString(cur_country)));
	ui->psnCountryBox->model()->sort(0, Qt::AscendingOrder);
}

void settings_dialog::closeEvent([[maybe_unused]] QCloseEvent* event)
{
	m_gui_settings->SetValue(gui::cfg_geometry, saveGeometry());
}

settings_dialog::~settings_dialog()
{
}

void settings_dialog::EnhanceSlider(emu_settings_type settings_type, QSlider* slider, QLabel* label, const QString& label_text) const
{
	m_emu_settings->EnhanceSlider(slider, settings_type);

	if (slider && label)
	{
		label->setText(label_text.arg(slider->value()));
		connect(slider, &QSlider::valueChanged, [label, label_text](int value)
		{
			label->setText(label_text.arg(value));
		});
	}
}

void settings_dialog::SnapSlider(QSlider *slider, int interval)
{
	if (!slider)
	{
		return;
	}

	// Snap the slider to the next best interval position if the slider is currently modified with the mouse.
	connect(slider, &QSlider::valueChanged, [this, slider, interval](int value)
	{
		if (!slider || slider != m_current_slider)
		{
			return;
		}
		slider->setValue(utils::rounded_div(value, interval) * interval);
	});

	// Register the slider for the event filter which updates m_current_slider if a QSlider was pressed or released with the mouse.
	// We can't just use sliderPressed and sliderReleased signals to update m_current_slider because those only trigger if the handle was clicked.
	slider->installEventFilter(this);
	m_snap_sliders.insert(slider);
}

void settings_dialog::AddStylesheets()
{
	ui->combo_stylesheets->clear();

	ui->combo_stylesheets->addItem(tr("None", "Stylesheets"), gui::NoStylesheet);

	for (const QString& key : QStyleFactory::keys())
	{
		// Variant value: "native (<style>)"
		ui->combo_stylesheets->addItem(tr("Native (%0)", "Stylesheets").arg(key), QString("%0 (%1)").arg(gui::NativeStylesheet, key));
	}

	ui->combo_stylesheets->addItem(tr("Default (Bright)", "Stylesheets"), gui::DefaultStylesheet);

	for (const QString& entry : m_gui_settings->GetStylesheetEntries())
	{
		if (entry != gui::DefaultStylesheet)
		{
			ui->combo_stylesheets->addItem(entry, entry);
		}
	}

	m_current_stylesheet = m_gui_settings->GetValue(gui::m_currentStylesheet).toString();

	const int index = ui->combo_stylesheets->findData(m_current_stylesheet);
	if (index >= 0)
	{
		ui->combo_stylesheets->setCurrentIndex(index);
	}
	else
	{
		cfg_log.warning("Trying to set an invalid stylesheets index: %d (%s)", index, m_current_stylesheet);
	}
}

void settings_dialog::ApplyStylesheet(bool reset)
{
	if (reset)
	{
		m_current_stylesheet = gui::DefaultStylesheet;
		ui->combo_stylesheets->setCurrentIndex(0);
	}

	// NOTE: We're deliberately not using currentText() here. The actual stylesheet is stored in user data.
	m_current_stylesheet = ui->combo_stylesheets->currentData().toString();

	if (m_current_stylesheet != m_gui_settings->GetValue(gui::m_currentStylesheet).toString())
	{
		m_gui_settings->SetValue(gui::m_currentStylesheet, m_current_stylesheet);
		Q_EMIT GuiStylesheetRequest();
	}
}

void settings_dialog::open()
{
	QDialog::open();

	ui->tab_widget_settings->setCurrentIndex(m_tab_index);

	// Open a dialog if your config file contained invalid entries
	QTimer::singleShot(10, [this]
	{
		m_emu_settings->OpenCorrectionDialog(this);

		if (!m_emu_settings->ValidateSettings(false))
		{
			int result = QMessageBox::No;
			m_gui_settings->ShowConfirmationBox(
				tr("Remove obsolete settings?"),
				tr(
					"Your config file contains one or more obsolete entries.<br>"
					"Consider that a removal might render them invalid for other versions of RPCS3.<br>"
					"<br>"
					"Do you wish to let the program remove them for you now?<br>"
					"This change will only be final when you save the config."
				), gui::ib_obsolete_cfg, &result, this);

			if (result == QMessageBox::Yes)
			{
				m_emu_settings->ValidateSettings(true);
			}
		}
	});
}

void settings_dialog::SubscribeDescription(QLabel* description)
{
	description->setFixedHeight(description->sizeHint().height());
	m_description_labels.push_back(std::pair<QLabel*, QString>(description, description->text()));
}

void settings_dialog::SubscribeTooltip(QObject* object, const QString& tooltip)
{
	m_descriptions[object] = tooltip;
	object->installEventFilter(this);
}

// Thanks Dolphin
bool settings_dialog::eventFilter(QObject* object, QEvent* event)
{
	if (m_snap_sliders.contains(object))
	{
		if (event->type() == QEvent::MouseButtonPress)
		{
			m_current_slider = static_cast<QSlider*>(object);
		}
		else if (event->type() == QEvent::MouseButtonRelease)
		{
			m_current_slider = nullptr;
		}
	}

	if (!m_descriptions.contains(object))
	{
		return QDialog::eventFilter(object, event);
	}

	if (event->type() == QEvent::Enter || event->type() == QEvent::Leave)
	{
		const int i = ui->tab_widget_settings->currentIndex();

		if (i >= 0 && static_cast<usz>(i) < m_description_labels.size())
		{
			if (QLabel* label = m_description_labels[i].first)
			{
				if (event->type() == QEvent::Enter)
				{
					label->setText(m_descriptions[object]);
				}
				else if (event->type() == QEvent::Leave)
				{
					label->setText(m_description_labels[i].second);
				}
			}
		}
	}

	return QDialog::eventFilter(object, event);
}

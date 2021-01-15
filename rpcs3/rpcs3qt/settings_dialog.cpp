#include <QButtonGroup>
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
#include <QUrl>

#include "gui_settings.h"
#include "display_sleep_control.h"
#include "qt_utils.h"
#include "settings_dialog.h"
#include "ui_settings_dialog.h"
#include "tooltips.h"
#include "input_dialog.h"
#include "emu_settings_type.h"
#include "render_creator.h"
#include "microphone_creator.h"

#include "Emu/GameInfo.h"
#include "Emu/System.h"
#include "Emu/system_config.h"
#include "Emu/title.h"
#include "Crypto/unself.h"

#include <set>
#include <unordered_set>
#include <thread>

#include "util/sysinfo.hpp"
#include "util/asm.hpp"

#ifdef WITH_DISCORD_RPC
#include "_discord_utils.h"
#endif

LOG_CHANNEL(cfg_log, "CFG");

inline std::string sstr(const QString& _in) { return _in.toStdString(); }
inline std::string sstr(const QVariant& _in) { return sstr(_in.toString()); }
inline QString qsv(std::string_view sv) { return QString(sv.data()); }

extern const std::map<std::string_view, int> g_prx_list;

settings_dialog::settings_dialog(std::shared_ptr<gui_settings> gui_settings, std::shared_ptr<emu_settings> emu_settings, const int& tab_index, QWidget *parent, const GameInfo* game)
	: QDialog(parent)
	, m_tab_index(tab_index)
	, ui(new Ui::settings_dialog)
	, m_gui_settings(gui_settings)
	, m_emu_settings(emu_settings)
{
	ui->setupUi(this);
	ui->buttonBox->button(QDialogButtonBox::StandardButton::Close)->setFocus();
	ui->tab_widget_settings->setUsesScrollButtons(false);
	ui->tab_widget_settings->tabBar()->setObjectName("tab_bar_settings");

	if (!m_gui_settings->GetValue(gui::m_showDebugTab).toBool())
	{
		ui->tab_widget_settings->removeTab(9);
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
		m_emu_settings->LoadSettings(game->serial);
		setWindowTitle(tr("Settings: [%0] %1", "Settings dialog").arg(qstr(game->serial)).arg(qstr(game->name)));
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

	const auto apply_configs = [this, use_discord_old = m_use_discord, discord_state_old = m_discord_state](bool do_exit)
	{
		std::set<std::string> selected;
		for (int i = 0; i < ui->lleList->count(); ++i)
		{
			const auto& item = ui->lleList->item(i);
			if (item->checkState() != Qt::CheckState::Unchecked)
			{
				// suffix indicates forced HLE mode
				selected.emplace(sstr(item->text()) + ":hle");
			}
		}

		for (int i = 0; i < ui->hleList->count(); ++i)
		{
			const auto& item = ui->hleList->item(i);
			if (item->checkState() != Qt::CheckState::Unchecked)
			{
				// suffix indicates forced LLE mode
				selected.emplace(sstr(item->text()) + ":lle");
			}
		}

		std::vector<std::string> selected_ls(selected.begin(), selected.end());
		m_emu_settings->SaveSelectedLibraries(selected_ls);
		m_emu_settings->SaveSettings();

		if (do_exit)
		{
			accept();
		}

		Q_EMIT EmuSettingsApplied();

		// Discord Settings can be saved regardless of WITH_DISCORD_RPC
		m_gui_settings->SetValue(gui::m_richPresence, m_use_discord);
		m_gui_settings->SetValue(gui::m_discordState, m_discord_state);

#ifdef WITH_DISCORD_RPC
		if (m_use_discord != use_discord_old)
		{
			if (m_use_discord)
			{
				discord::initialize();
				discord::update_presence(sstr(m_discord_state));
			}
			else
			{
				discord::shutdown();
			}
		}
		else if (m_discord_state != discord_state_old && Emu.IsStopped())
		{
			discord::update_presence(sstr(m_discord_state), "Idle", false);
		}
#endif
	};

	connect(ui->buttonBox, &QDialogButtonBox::clicked, [apply_configs, this](QAbstractButton* button)
	{
		if (button == ui->buttonBox->button(QDialogButtonBox::Save))
		{
			apply_configs(true);
		}
		else if (button == ui->buttonBox->button(QDialogButtonBox::Apply))
		{
			apply_configs(false);
		}
	});

	connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &QWidget::close);

	connect(ui->tab_widget_settings, &QTabWidget::currentChanged, [this]()
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

	m_emu_settings->EnhanceCheckBox(ui->spuCache, emu_settings_type::SPUCache);
	SubscribeTooltip(ui->spuCache, tooltips.settings.spu_cache);

	m_emu_settings->EnhanceCheckBox(ui->enableScheduler, emu_settings_type::EnableThreadScheduler);
	SubscribeTooltip(ui->enableScheduler, tooltips.settings.enable_thread_scheduler);

	m_emu_settings->EnhanceCheckBox(ui->lowerSPUThrPrio, emu_settings_type::LowerSPUThreadPrio);
	SubscribeTooltip(ui->lowerSPUThrPrio, tooltips.settings.lower_spu_thread_priority);

	m_emu_settings->EnhanceCheckBox(ui->spuLoopDetection, emu_settings_type::SPULoopDetection);
	SubscribeTooltip(ui->spuLoopDetection, tooltips.settings.spu_loop_detection);

	m_emu_settings->EnhanceCheckBox(ui->accurateXFloat, emu_settings_type::AccurateXFloat);
	SubscribeTooltip(ui->accurateXFloat, tooltips.settings.accurate_xfloat);

	// Comboboxes

	m_emu_settings->EnhanceComboBox(ui->spuBlockSize, emu_settings_type::SPUBlockSize);
	SubscribeTooltip(ui->gb_spuBlockSize, tooltips.settings.spu_block_size);

	m_emu_settings->EnhanceComboBox(ui->preferredSPUThreads, emu_settings_type::PreferredSPUThreads, true);
	SubscribeTooltip(ui->gb_spu_threads, tooltips.settings.preferred_spu_threads);
	ui->preferredSPUThreads->setItemText(ui->preferredSPUThreads->findData(0), tr("Auto", "Preferred SPU threads"));

	if (utils::has_rtm())
	{
		m_emu_settings->EnhanceComboBox(ui->enableTSX, emu_settings_type::EnableTSX);
		SubscribeTooltip(ui->gb_tsx, tooltips.settings.enable_tsx);

		static const QString tsx_forced = qstr(fmt::format("%s", tsx_usage::forced));
		static const QString tsx_default = qstr(m_emu_settings->GetSettingDefault(emu_settings_type::EnableTSX));

		// connect the toogled signal so that the stateChanged signal in EnhanceCheckBox can be prevented
		connect(ui->enableTSX, &QComboBox::currentTextChanged, [this](const QString& text)
		{
			if (text == tsx_forced && !utils::has_mpx() && QMessageBox::No == QMessageBox::critical(this, tr("Haswell/Broadwell TSX Warning"), tr(
				R"(
					<p style="white-space: nowrap;">
						RPCS3 has detected you are using TSX functions on a Haswell or Broadwell CPU.<br>
						Intel has deactivated these functions in newer Microcode revisions, since they can lead to unpredicted behaviour.<br>
						That means using TSX may break games or even <font color="red"><b>damage</b></font> your data.<br>
						We recommend to disable this feature and update your computer BIOS.<br><br>
						Do you wish to use TSX anyway?
					</p>
				)"
			), QMessageBox::Yes, QMessageBox::No))
			{
				// Reset if the messagebox was answered with no. This prevents the currentIndexChanged signal in EnhanceComboBox
				ui->enableTSX->setCurrentText(tsx_default);
			}
		});
	}
	else
	{
		ui->enableTSX->setEnabled(false);
		ui->enableTSX->setPlaceholderText(tr("Not supported", "Enable TSX"));
		SubscribeTooltip(ui->enableTSX, tr("Unfortunately your CPU model does not support this instruction set.", "Enable TSX"));

		m_emu_settings->SetSetting(emu_settings_type::EnableTSX, fmt::format("%s", tsx_usage::disabled));
	}

	// PPU tool tips
	SubscribeTooltip(ui->ppu_precise, tooltips.settings.ppu_precise);
	SubscribeTooltip(ui->ppu_fast,    tooltips.settings.ppu_fast);
	SubscribeTooltip(ui->ppu_llvm,    tooltips.settings.ppu_llvm);

	QButtonGroup *ppu_bg = new QButtonGroup(this);
	ppu_bg->addButton(ui->ppu_precise, static_cast<int>(ppu_decoder_type::precise));
	ppu_bg->addButton(ui->ppu_fast,    static_cast<int>(ppu_decoder_type::fast));
	ppu_bg->addButton(ui->ppu_llvm,    static_cast<int>(ppu_decoder_type::llvm));

	m_emu_settings->EnhanceRadioButton(ppu_bg, emu_settings_type::PPUDecoder);

	// SPU tool tips
	SubscribeTooltip(ui->spu_precise, tooltips.settings.spu_precise);
	SubscribeTooltip(ui->spu_fast,    tooltips.settings.spu_fast);
	SubscribeTooltip(ui->spu_asmjit,  tooltips.settings.spu_asmjit);
	SubscribeTooltip(ui->spu_llvm,    tooltips.settings.spu_llvm);

	QButtonGroup *spu_bg = new QButtonGroup(this);
	spu_bg->addButton(ui->spu_precise, static_cast<int>(spu_decoder_type::precise));
	spu_bg->addButton(ui->spu_fast,    static_cast<int>(spu_decoder_type::fast));
	spu_bg->addButton(ui->spu_asmjit,  static_cast<int>(spu_decoder_type::asmjit));
	spu_bg->addButton(ui->spu_llvm,    static_cast<int>(spu_decoder_type::llvm));

	m_emu_settings->EnhanceRadioButton(spu_bg, emu_settings_type::SPUDecoder);

	connect(ui->spu_llvm, &QAbstractButton::toggled, [this](bool checked)
	{
		ui->accurateXFloat->setEnabled(checked);
	});

	connect(ui->spu_fast, &QAbstractButton::toggled, [this](bool checked)
	{
		ui->accurateXFloat->setEnabled(checked);
	});

	ui->accurateXFloat->setEnabled(ui->spu_llvm->isChecked() || ui->spu_fast->isChecked());

#ifndef LLVM_AVAILABLE
	ui->ppu_llvm->setEnabled(false);
	ui->spu_llvm->setEnabled(false);
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
		m_emu_settings->GetLocalizedSetting("Vulkan", emu_settings_type::Renderer, static_cast<int>(video_renderer::vulkan)),
		m_emu_settings->GetLocalizedSetting("OpenGl", emu_settings_type::Renderer, static_cast<int>(video_renderer::opengl)),
		m_emu_settings->GetLocalizedSetting("Null", emu_settings_type::Renderer, static_cast<int>(video_renderer::null))
	});

	// Comboboxes
	m_emu_settings->EnhanceComboBox(ui->renderBox, emu_settings_type::Renderer);
	SubscribeTooltip(ui->gb_renderer, tooltips.settings.renderer);
	SubscribeTooltip(ui->gb_graphicsAdapter, tooltips.settings.graphics_adapter);

	m_emu_settings->EnhanceComboBox(ui->resBox, emu_settings_type::Resolution);
	SubscribeTooltip(ui->gb_default_resolution, tooltips.settings.resolution);
	// remove unsupported resolutions from the dropdown
	const int saved_index = ui->resBox->currentIndex();
	bool saved_index_removed = false;
	if (game && game->resolution > 0)
	{
		const std::map<u32, std::string> resolutions
		{
			{ 1 << 0, fmt::format("%s", video_resolution::_480) },
			{ 1 << 1, fmt::format("%s", video_resolution::_576) },
			{ 1 << 2, fmt::format("%s", video_resolution::_720) },
			{ 1 << 3, fmt::format("%s", video_resolution::_1080) },
			// { 1 << 4, fmt::format("%s", video_resolution::_480p_16:9) },
			// { 1 << 5, fmt::format("%s", video_resolution::_576p_16:9) },
		};

		for (int i = ui->resBox->count() - 1; i >= 0; i--)
		{
			bool has_resolution = false;
			for (const auto& res : resolutions)
			{
				if ((game->resolution & res.first) && res.second == sstr(ui->resBox->itemText(i)))
				{
					has_resolution = true;
					break;
				}
			}
			if (!has_resolution)
			{
				ui->resBox->removeItem(i);
				if (i == saved_index)
				{
					saved_index_removed = true;
				}
			}
		}
	}
	for (int i = 0; i < ui->resBox->count(); i++)
	{
		const QVariantList var_list = ui->resBox->itemData(i).toList();
		ensure(var_list.size() == 2 && var_list[0].canConvert<QString>());

		if (var_list[0].toString() == "1280x720")
		{
			// Rename the default resolution for users
			ui->resBox->setItemText(i, tr("1280x720 (Recommended)", "Resolution"));

			// Set the current selection to the default if the original setting wasn't valid
			if (saved_index_removed)
			{
				ui->resBox->setCurrentIndex(i);
			}
			break;
		}
	}

	m_emu_settings->EnhanceComboBox(ui->aspectBox, emu_settings_type::AspectRatio);
	SubscribeTooltip(ui->gb_aspectRatio, tooltips.settings.aspect_ratio);

	m_emu_settings->EnhanceComboBox(ui->frameLimitBox, emu_settings_type::FrameLimit);
	SubscribeTooltip(ui->gb_frameLimit, tooltips.settings.frame_limit);

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

	// Comboboxes
	m_emu_settings->EnhanceComboBox(ui->shaderCompilerThreads, emu_settings_type::ShaderCompilerNumThreads, true);
	SubscribeTooltip(ui->gb_shader_compiler_threads, tooltips.settings.shader_compiler_threads);
	ui->shaderCompilerThreads->setItemText(ui->shaderCompilerThreads->findData(0), tr("Auto", "Number of Shader Compiler Threads"));

	// Checkboxes: main options
	m_emu_settings->EnhanceCheckBox(ui->dumpColor, emu_settings_type::WriteColorBuffers);
	SubscribeTooltip(ui->dumpColor, tooltips.settings.dump_color);

	m_emu_settings->EnhanceCheckBox(ui->vsync, emu_settings_type::VSync);
	SubscribeTooltip(ui->vsync, tooltips.settings.vsync);

	m_emu_settings->EnhanceCheckBox(ui->stretchToDisplayArea, emu_settings_type::StretchToDisplayArea);
	SubscribeTooltip(ui->stretchToDisplayArea, tooltips.settings.stretch_to_display_area);

	m_emu_settings->EnhanceCheckBox(ui->disableVertexCache, emu_settings_type::DisableVertexCache);
	SubscribeTooltip(ui->disableVertexCache, tooltips.settings.disable_vertex_cache);

	m_emu_settings->EnhanceCheckBox(ui->multithreadedRSX, emu_settings_type::MultithreadedRSX);
	SubscribeTooltip(ui->multithreadedRSX, tooltips.settings.multithreaded_rsx);
	connect(ui->multithreadedRSX, &QCheckBox::clicked, [this](bool checked)
	{
		ui->disableVertexCache->setEnabled(!checked);
	});
	ui->disableVertexCache->setEnabled(!ui->multithreadedRSX->isChecked());

	m_emu_settings->EnhanceCheckBox(ui->strictModeRendering, emu_settings_type::StrictRenderingMode);
	SubscribeTooltip(ui->strictModeRendering, tooltips.settings.strict_rendering_mode);
	const auto onStrictRenderingMode = [this](bool checked)
	{
		ui->gb_resolutionScale->setEnabled(!checked);
		ui->gb_minimumScalableDimension->setEnabled(!checked);
		ui->gb_anisotropicFilter->setEnabled(!checked);
	};
	connect(ui->strictModeRendering, &QCheckBox::clicked, this, onStrictRenderingMode);

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

	// Sliders

	m_emu_settings->EnhanceSlider(ui->resolutionScale, emu_settings_type::ResolutionScale);
	SubscribeTooltip(ui->gb_resolutionScale, tooltips.settings.resolution_scale);
	// rename label texts to fit current state of Resolution Scale
	const int resolution_scale_def = stoi(m_emu_settings->GetSettingDefault(emu_settings_type::ResolutionScale));
	auto scaled_resolution = [resolution_scale_def](int percentage)
	{
		if (percentage == resolution_scale_def)
		{
			return tr("100% (Default)", "Resolution scale");
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
	auto min_scalable_dimension = [minimum_scalable_dimension_def](int dim)
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

	// Remove renderers from the renderer Combobox if not supported
	for (const auto& renderer : r_creator->renderers)
	{
		if (renderer->supported)
		{
			if (renderer->has_adapters)
			{
				renderer->old_adapter = qstr(m_emu_settings->GetSetting(renderer->type));
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

	auto set_renderer = [r_creator, this](QString text)
	{
		if (text.isEmpty())
		{
			return;
		}

		auto switchTo = [r_creator, text, this](render_creator::render_info renderer)
		{
			// Reset other adapters to old config
			for (const auto& render : r_creator->renderers)
			{
				if (renderer.name != render->name && render->has_adapters && render->supported)
				{
					m_emu_settings->SetSetting(render->type, sstr(render->old_adapter));
				}
			}

			// Enable/disable MSAA depending on renderer
			ui->antiAliasing->setEnabled(renderer.has_msaa);
			ui->antiAliasing->blockSignals(true);
			ui->antiAliasing->setCurrentText(renderer.has_msaa ? qstr(m_emu_settings->GetSetting(emu_settings_type::MSAA)) : tr("Disabled", "MSAA"));
			ui->antiAliasing->blockSignals(false);

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
					rsx_log.warning("%s adapter config empty: setting to default!", sstr(renderer.name));
				}
				else
				{
					rsx_log.warning("Last used %s adapter not found: setting to default!", sstr(renderer.name));
				}
			}
			ui->graphicsAdapterBox->setCurrentIndex(idx);
			m_emu_settings->SetSetting(renderer.type, sstr(ui->graphicsAdapterBox->currentText()));
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

	auto set_adapter = [r_creator, this](QString text)
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
				m_emu_settings->SetSetting(render->type, sstr(text));
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

	auto fix_gl_legacy = [=, this](const QString& text)
	{
		ui->glLegacyBuffers->setEnabled(text == r_creator->OpenGL.name);
	};

	// Handle connects to disable specific checkboxes that depend on GUI state.
	onStrictRenderingMode(ui->strictModeRendering->isChecked());
	fix_gl_legacy(ui->renderBox->currentText()); // Init
	connect(ui->renderBox, &QComboBox::currentTextChanged, fix_gl_legacy);

	//                      _ _         _______    _
	//       /\            | (_)       |__   __|  | |
	//      /  \  _   _  __| |_  ___      | | __ _| |__
	//     / /\ \| | | |/ _` | |/ _ \     | |/ _` | '_ ＼
	//    / ____ \ |_| | (_| | | (_) |    | | (_| | |_) |
	//   /_/    \_\__,_|\__,_|_|\___/     |_|\__,_|_.__/

	auto enable_time_stretching_options = [this](bool enabled)
	{
		ui->timeStretchingThresholdLabel->setEnabled(enabled);
		ui->timeStretchingThreshold->setEnabled(enabled);
	};

	auto enable_buffering_options = [this, enable_time_stretching_options](bool enabled)
	{
		ui->audioBufferDuration->setEnabled(enabled);
		ui->audioBufferDurationLabel->setEnabled(enabled);
		ui->enableTimeStretching->setEnabled(enabled);
		enable_time_stretching_options(enabled && ui->enableTimeStretching->isChecked());
	};

	auto enable_buffering = [this, enable_buffering_options](int index)
	{
		const QVariantList var_list = ui->audioOutBox->itemData(index).toList();
		ensure(var_list.size() == 2 && var_list[0].canConvert<QString>());
		const QString text = var_list[0].toString();
		const bool enabled = text == "XAudio2" || text == "OpenAL" || text == "FAudio";
		ui->enableBuffering->setEnabled(enabled);
		enable_buffering_options(enabled && ui->enableBuffering->isChecked());
	};

	const QString mic_none = m_emu_settings->m_microphone_creator.get_none();

	auto change_microphone_type = [mic_none, this](int index)
	{
		if (index < 0)
		{
			return;
		}

		const QVariantList var_list = ui->microphoneBox->itemData(index).toList();
		ensure(var_list.size() == 2 && var_list[1].canConvert<int>());
		const int handler_id = var_list[1].toInt();
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
		default:
			break;
		}

		ui->microphone1Box->setEnabled(max > 0);
		ui->microphone2Box->setEnabled(max > 1 && ui->microphone1Box->currentText() != mic_none);
		ui->microphone3Box->setEnabled(max > 2 && ui->microphone2Box->currentText() != mic_none);
		ui->microphone4Box->setEnabled(ui->microphone3Box->isEnabled() && ui->microphone3Box->currentText() != mic_none);
	};

	auto propagate_used_devices = [mic_none, change_microphone_type, this]()
	{
		for (u32 index = 0; index < 4; index++)
		{
			const QString cur_item = m_mics_combo[index]->currentText();
			QStringList cur_list = m_emu_settings->m_microphone_creator.get_microphone_list();
			for (u32 subindex = 0; subindex < 4; subindex++)
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

	auto change_microphone_device = [mic_none, propagate_used_devices, this](u32 next_index, QString text)
	{
		m_emu_settings->SetSetting(emu_settings_type::MicrophoneDevices, m_emu_settings->m_microphone_creator.set_device(next_index, text));
		if (next_index < 4 && text == mic_none)
			m_mics_combo[next_index]->setCurrentText(mic_none);
		propagate_used_devices();
	};

	// Comboboxes

	m_emu_settings->EnhanceComboBox(ui->audioOutBox, emu_settings_type::AudioRenderer);
#ifdef WIN32
	SubscribeTooltip(ui->gb_audio_out, tooltips.settings.audio_out);
#else
	SubscribeTooltip(ui->gb_audio_out, tooltips.settings.audio_out_linux);
#endif
	connect(ui->audioOutBox, QOverload<int>::of(&QComboBox::currentIndexChanged), enable_buffering);

	m_emu_settings->EnhanceComboBox(ui->combo_audio_downmix, emu_settings_type::AudioChannels);
	SubscribeTooltip(ui->gb_audio_downmix, tooltips.settings.downmix);
	// TODO: enable this setting once cellAudioOutConfigure can change downmix on the fly
	ui->combo_audio_downmix->removeItem(static_cast<int>(audio_downmix::use_application_settings));

	// Microphone Comboboxes
	m_mics_combo[0] = ui->microphone1Box;
	m_mics_combo[1] = ui->microphone2Box;
	m_mics_combo[2] = ui->microphone3Box;
	m_mics_combo[3] = ui->microphone4Box;
	connect(m_mics_combo[0], &QComboBox::currentTextChanged, [=, this](const QString& text) { change_microphone_device(1, text); });
	connect(m_mics_combo[1], &QComboBox::currentTextChanged, [=, this](const QString& text) { change_microphone_device(2, text); });
	connect(m_mics_combo[2], &QComboBox::currentTextChanged, [=, this](const QString& text) { change_microphone_device(3, text); });
	connect(m_mics_combo[3], &QComboBox::currentTextChanged, [=, this](const QString& text) { change_microphone_device(4, text); });
	m_emu_settings->m_microphone_creator.refresh_list();
	propagate_used_devices(); // Fills comboboxes list

	m_emu_settings->m_microphone_creator.parse_devices(m_emu_settings->GetSetting(emu_settings_type::MicrophoneDevices));

	const auto mic_sel_list = m_emu_settings->m_microphone_creator.get_selection_list();

	for (s32 index = 3; index >= 0; index--)
	{
		const auto mic = mic_sel_list[index];
		const auto qmic = qstr(mic);

		if (mic.empty() || m_mics_combo[index]->findText(qmic) == -1)
		{
			m_mics_combo[index]->setCurrentText(mic_none);
			change_microphone_device(index+1, mic_none); // Ensures the value is set in config
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
	connect(ui->enableBuffering, &QCheckBox::clicked, enable_buffering_options);

	m_emu_settings->EnhanceCheckBox(ui->enableTimeStretching, emu_settings_type::EnableTimeStretching);
	SubscribeTooltip(ui->enableTimeStretching, tooltips.settings.enable_time_stretching);
	connect(ui->enableTimeStretching, &QCheckBox::clicked, enable_time_stretching_options);

	enable_buffering(ui->audioOutBox->currentIndex());

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

	m_emu_settings->EnhanceComboBox(ui->moveBox, emu_settings_type::Move);
	SubscribeTooltip(ui->gb_move_handler, tooltips.settings.move);

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

	m_emu_settings->EnhanceCheckBox(ui->enableCacheClearing, emu_settings_type::LimitCacheSize);
	SubscribeTooltip(ui->gb_DiskCacheClearing, tooltips.settings.limit_cache_size);
	connect(ui->enableCacheClearing, &QCheckBox::stateChanged, ui->maximumCacheSize, &QSlider::setEnabled);

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

	m_emu_settings->EnhanceLineEdit(ui->edit_swaps, emu_settings_type::IpSwapList);
	SubscribeTooltip(ui->gb_edit_swaps, tooltips.settings.dns_swap);

	// Comboboxes

	m_emu_settings->EnhanceComboBox(ui->netStatusBox, emu_settings_type::InternetStatus);
	SubscribeTooltip(ui->gb_netStatusBox, tooltips.settings.net_status);

	connect(ui->netStatusBox, QOverload<int>::of(&QComboBox::currentIndexChanged), [this](int index)
	{
		ui->edit_dns->setEnabled(index > 0);
	});
	ui->edit_dns->setEnabled(ui->netStatusBox->currentIndex() > 0);

	m_emu_settings->EnhanceComboBox(ui->psnStatusBox, emu_settings_type::PSNStatus);
	SubscribeTooltip(ui->gb_psnStatusBox, tooltips.settings.psn_status);

	//                _                               _   _______    _
	//       /\      | |                             | | |__   __|  | |
	//      /  \   __| |_   ____ _ _ __   ___ ___  __| |    | | __ _| |__
	//     / /\ \ / _` \ \ / / _` | '_ \ / __/ _ \/ _` |    | |/ _` | '_ ＼
	//    / ____ \ (_| |\ V / (_| | | | | (_|  __/ (_| |    | | (_| | |_) |
	//   /_/    \_\__,_| \_/ \__,_|_| |_|\___\___|\__,_|    |_|\__,_|_.__/


	// Checkboxes

	m_emu_settings->EnhanceCheckBox(ui->debugConsoleMode, emu_settings_type::DebugConsoleMode);
	SubscribeTooltip(ui->debugConsoleMode, tooltips.settings.debug_console_mode);

	m_emu_settings->EnhanceCheckBox(ui->accurateLLVMdfma, emu_settings_type::AccurateLLVMdfma);
	SubscribeTooltip(ui->accurateLLVMdfma, tooltips.settings.accurate_llvm_dfma);
	ui->accurateLLVMdfma->setDisabled(utils::has_fma3() || utils::has_fma4());

	m_emu_settings->EnhanceCheckBox(ui->AccurateVectorNaN, emu_settings_type::AccurateVectorNaN);
	SubscribeTooltip(ui->AccurateVectorNaN, tooltips.settings.accurate_vector_nan);

	m_emu_settings->EnhanceCheckBox(ui->silenceAllLogs, emu_settings_type::SilenceAllLogs);
	SubscribeTooltip(ui->silenceAllLogs, tooltips.settings.silence_all_logs);

	m_emu_settings->EnhanceCheckBox(ui->readColor, emu_settings_type::ReadColorBuffers);
	SubscribeTooltip(ui->readColor, tooltips.settings.read_color);

	m_emu_settings->EnhanceCheckBox(ui->readDepth, emu_settings_type::ReadDepthBuffer);
	SubscribeTooltip(ui->readDepth, tooltips.settings.read_depth);

	m_emu_settings->EnhanceCheckBox(ui->dumpDepth, emu_settings_type::WriteDepthBuffer);
	SubscribeTooltip(ui->dumpDepth, tooltips.settings.dump_depth);

	m_emu_settings->EnhanceCheckBox(ui->disableOnDiskShaderCache, emu_settings_type::DisableOnDiskShaderCache);
	SubscribeTooltip(ui->disableOnDiskShaderCache, tooltips.settings.disable_on_disk_shader_cache);

	m_emu_settings->EnhanceCheckBox(ui->relaxedZCULL, emu_settings_type::RelaxedZCULL);
	SubscribeTooltip(ui->relaxedZCULL, tooltips.settings.relaxed_zcull);

	// Comboboxes

	m_emu_settings->EnhanceComboBox(ui->maxSPURSThreads, emu_settings_type::MaxSPURSThreads, true);
	ui->maxSPURSThreads->setItemText(ui->maxSPURSThreads->findData(6), tr("Unlimited (Default)", "Max SPURS threads"));
	SubscribeTooltip(ui->gb_max_spurs_threads, tooltips.settings.max_spurs_threads);

	m_emu_settings->EnhanceComboBox(ui->sleepTimersAccuracy, emu_settings_type::SleepTimersAccuracy);
	SubscribeTooltip(ui->gb_sleep_timers_accuracy, tooltips.settings.sleep_timers_accuracy);

	// Sliders

	EnhanceSlider(emu_settings_type::DriverWakeUpDelay, ui->wakeupDelay, ui->wakeupText, tr(reinterpret_cast<const char*>(u8"%0 µs"), "Driver wake up delay"));
	SnapSlider(ui->wakeupDelay, 200);
	ui->wakeupDelay->setMaximum(7000); // Very large values must be entered with config.yml changes
	ui->wakeupDelay->setPageStep(200);
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
	connect(ui->clockScaleReset, &QAbstractButton::clicked, [=, this]()
	{
		ui->clockScale->setValue(clocks_scale_def);
	});

	if (!game) // Prevent users from doing dumb things
	{
		ui->vblank->setDisabled(true);
		ui->vblankReset->setDisabled(true);
		SubscribeTooltip(ui->gb_vblank, tooltips.settings.disabled_from_global);
		ui->clockScale->setDisabled(true);
		ui->clockScaleReset->setDisabled(true);
		SubscribeTooltip(ui->gb_clockScale, tooltips.settings.disabled_from_global);
		ui->wakeupDelay->setDisabled(true);
		ui->wakeupReset->setDisabled(true);
		SubscribeTooltip(ui->gb_wakeupDelay, tooltips.settings.disabled_from_global);
	}
	else
	{
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

		QListWidgetItem* item = new QListWidgetItem(qsv(lib.first), list);
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

	auto on_lib_state_changed = [this](QString text)
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

	// Sort libs
	on_lib_state_changed({});

	// Events
	connect(ui->searchBox, &QLineEdit::textChanged, on_lib_state_changed);
	connect(ui->resetLleList, &QAbstractButton::clicked, [this, on_lib_state_changed]()
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
	});

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

	//    ______                 _       _               _______    _
	//   |  ____|               | |     | |             |__   __|  | |
	//   | |__   _ __ ___  _   _| | __ _| |_ ___  _ __     | | __ _| |__
	//   |  __| | '_ ` _ \| | | | |/ _` | __/ _ \| '__|    | |/ _` | '_ ＼
	//   | |____| | | | | | |_| | | (_| | || (_) | |       | | (_| | |_) |
	//   |______|_| |_| |_|\__,_|_|\__,_|\__\___/|_|       |_|\__,_|_.__/

	// Comboboxes

	m_emu_settings->EnhanceComboBox(ui->maxLLVMThreads, emu_settings_type::MaxLLVMThreads, true, true, std::thread::hardware_concurrency());
	SubscribeTooltip(ui->gb_max_llvm, tooltips.settings.max_llvm_threads);
	ui->maxLLVMThreads->setItemText(ui->maxLLVMThreads->findData(0), tr("All (%1)", "Max LLVM threads").arg(std::thread::hardware_concurrency()));

	m_emu_settings->EnhanceComboBox(ui->perfOverlayDetailLevel, emu_settings_type::PerfOverlayDetailLevel);
	SubscribeTooltip(ui->perf_overlay_detail_level, tooltips.settings.perf_overlay_detail_level);

	m_emu_settings->EnhanceComboBox(ui->perfOverlayPosition, emu_settings_type::PerfOverlayPosition);
	SubscribeTooltip(ui->perf_overlay_position, tooltips.settings.perf_overlay_position);

	// Checkboxes

	m_emu_settings->EnhanceCheckBox(ui->exitOnStop, emu_settings_type::ExitRPCS3OnFinish);
	SubscribeTooltip(ui->exitOnStop, tooltips.settings.exit_on_stop);

	m_emu_settings->EnhanceCheckBox(ui->alwaysStart, emu_settings_type::StartOnBoot);
	SubscribeTooltip(ui->alwaysStart, tooltips.settings.start_on_boot);

	m_emu_settings->EnhanceCheckBox(ui->startGameFullscreen, emu_settings_type::StartGameFullscreen);
	SubscribeTooltip(ui->startGameFullscreen, tooltips.settings.start_game_fullscreen);

	m_emu_settings->EnhanceCheckBox(ui->preventDisplaySleep, emu_settings_type::PreventDisplaySleep);
	SubscribeTooltip(ui->preventDisplaySleep, tooltips.settings.prevent_display_sleep);
	ui->preventDisplaySleep->setEnabled(display_sleep_control_supported());

	m_emu_settings->EnhanceCheckBox(ui->showTrophyPopups, emu_settings_type::ShowTrophyPopups);
	SubscribeTooltip(ui->showTrophyPopups, tooltips.settings.show_trophy_popups);

	m_emu_settings->EnhanceCheckBox(ui->useNativeInterface, emu_settings_type::UseNativeInterface);
	SubscribeTooltip(ui->useNativeInterface, tooltips.settings.use_native_interface);

	m_emu_settings->EnhanceCheckBox(ui->showShaderCompilationHint, emu_settings_type::ShowShaderCompilationHint);
	SubscribeTooltip(ui->showShaderCompilationHint, tooltips.settings.show_shader_compilation_hint);

	m_emu_settings->EnhanceCheckBox(ui->perfOverlayCenterX, emu_settings_type::PerfOverlayCenterX);
	SubscribeTooltip(ui->perfOverlayCenterX, tooltips.settings.perf_overlay_center_x);
	connect(ui->perfOverlayCenterX, &QCheckBox::clicked, [this](bool checked)
	{
		ui->perfOverlayMarginX->setEnabled(!checked);
	});
	ui->perfOverlayMarginX->setEnabled(!ui->perfOverlayCenterX->isChecked());

	m_emu_settings->EnhanceCheckBox(ui->perfOverlayCenterY, emu_settings_type::PerfOverlayCenterY);
	SubscribeTooltip(ui->perfOverlayCenterY, tooltips.settings.perf_overlay_center_y);
	connect(ui->perfOverlayCenterY, &QCheckBox::clicked, [this](bool checked)
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
	auto enable_perf_overlay_options = [this](bool enabled)
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
	};
	enable_perf_overlay_options(ui->perfOverlayEnabled->isChecked());
	connect(ui->perfOverlayEnabled, &QCheckBox::clicked, enable_perf_overlay_options);

	m_emu_settings->EnhanceCheckBox(ui->shaderLoadBgEnabled, emu_settings_type::ShaderLoadBgEnabled);
	SubscribeTooltip(ui->shaderLoadBgEnabled, tooltips.settings.shader_load_bg_enabled);
	auto enable_shader_loader_options = [this](bool enabled)
	{
		ui->label_shaderLoadBgDarkening->setEnabled(enabled);
		ui->label_shaderLoadBgBlur->setEnabled(enabled);
		ui->shaderLoadBgDarkening->setEnabled(enabled);
		ui->shaderLoadBgBlur->setEnabled(enabled);
	};
	enable_shader_loader_options(ui->shaderLoadBgEnabled->isChecked());
	connect(ui->shaderLoadBgEnabled, &QCheckBox::clicked, enable_shader_loader_options);

	// Sliders

	EnhanceSlider(emu_settings_type::PerfOverlayUpdateInterval, ui->perfOverlayUpdateInterval, ui->label_update_interval, tr("Update Interval: %0 ms", "Performance overlay update interval"));
	SubscribeTooltip(ui->perf_overlay_update_interval, tooltips.settings.perf_overlay_update_interval);

	EnhanceSlider(emu_settings_type::PerfOverlayFontSize, ui->perfOverlayFontSize, ui->label_font_size, tr("Font Size: %0 px", "Performance overlay font size"));
	SubscribeTooltip(ui->perf_overlay_font_size, tooltips.settings.perf_overlay_font_size);

	EnhanceSlider(emu_settings_type::PerfOverlayOpacity, ui->perfOverlayOpacity, ui->label_opacity, tr("Opacity: %0 %", "Performance overlay opacity"));
	SubscribeTooltip(ui->perf_overlay_opacity, tooltips.settings.perf_overlay_opacity);

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
		SubscribeTooltip(ui->gs_hideMouseOnIdle_widget, tooltips.settings.hide_mouse_on_idle);

		ui->gs_disableMouse->setChecked(m_gui_settings->GetValue(gui::gs_disableMouse).toBool());
		connect(ui->gs_disableMouse, &QCheckBox::clicked, [this](bool val)
		{
			m_gui_settings->SetValue(gui::gs_disableMouse, val);
		});

		ui->gs_disableKbHotkeys->setChecked(m_gui_settings->GetValue(gui::gs_disableKbHotkeys).toBool());
		connect(ui->gs_disableKbHotkeys, &QCheckBox::clicked, [this](bool val)
		{
			m_gui_settings->SetValue(gui::gs_disableKbHotkeys, val);
		});

		ui->gs_showMouseInFullscreen->setChecked(m_gui_settings->GetValue(gui::gs_showMouseFs).toBool());
		connect(ui->gs_showMouseInFullscreen, &QCheckBox::clicked, [this](bool val)
		{
			m_gui_settings->SetValue(gui::gs_showMouseFs, val);
		});

		ui->gs_hideMouseOnIdle->setChecked(m_gui_settings->GetValue(gui::gs_hideMouseIdle).toBool());
		connect(ui->gs_hideMouseOnIdle, &QCheckBox::clicked, [this](bool checked)
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
		ui->gs_resizeOnBoot->setChecked(enable_buttons);
		ui->gs_width->setEnabled(enable_buttons);
		ui->gs_height->setEnabled(enable_buttons);

		const QRect screen = QGuiApplication::primaryScreen()->geometry();
		const int width = m_gui_settings->GetValue(gui::gs_width).toInt();
		const int height = m_gui_settings->GetValue(gui::gs_height).toInt();
		ui->gs_width->setValue(std::min(width, screen.width()));
		ui->gs_height->setValue(std::min(height, screen.height()));

		connect(ui->gs_resizeOnBoot, &QCheckBox::clicked, [=, this](bool val)
		{
			m_gui_settings->SetValue(gui::gs_resize, val);
			ui->gs_width->setEnabled(val);
			ui->gs_height->setEnabled(val);
		});
		connect(ui->gs_width, &QSpinBox::editingFinished, [=, this]()
		{
			ui->gs_width->setValue(std::min(ui->gs_width->value(), QGuiApplication::primaryScreen()->size().width()));
			m_gui_settings->SetValue(gui::gs_width, ui->gs_width->value());
		});
		connect(ui->gs_height, &QSpinBox::editingFinished, [=, this]()
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
		title_data.format = sstr(format);
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
			title_data.title = sstr(tr("My Game", "Game window title"));
			title_data.title_id = "ABCD12345";
		}

		const std::string game_window_title = rpcs3::get_formatted_title(title_data);

		if (game_window_title.empty())
		{
			return QStringLiteral("RPCS3");
		}

		return qstr(game_window_title);
	};

	const auto set_game_window_title = [get_game_window_title, this](const std::string& format)
	{
		const auto game_window_title_format = qstr(format);
		const auto game_window_title = get_game_window_title(game_window_title_format);
		const auto width = ui->label_game_window_title_format->sizeHint().width();
		const auto metrics = ui->label_game_window_title_format->fontMetrics();
		const auto elided_text = metrics.elidedText(game_window_title_format, Qt::ElideRight, width);
		const auto tooltip = game_window_title_format + QStringLiteral("\n\n") + game_window_title;

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

			for (const auto& [format, description] : window_title_glossary)
			{
				glossary += format + "\t = " + description + "\n";
			}

			return tr("Glossary:\n\n%0\nPreview:\n\n%1\n", "Game window title").arg(glossary).arg(game_window_title);
		};

		const std::string game_title_format = m_emu_settings->GetSetting(emu_settings_type::WindowTitleFormat);

		QString edited_format = qstr(game_title_format);

		input_dialog dlg(-1, edited_format, tr("Game Window Title Format", "Game window title"), get_game_window_title_label(edited_format), "", this);
		dlg.resize(width() * .75, dlg.height());

		connect(&dlg, &input_dialog::text_changed, [&edited_format, &dlg, get_game_window_title_label](const QString& text)
		{
			edited_format = text.simplified();
			dlg.set_label_text(get_game_window_title_label(edited_format));
		});

		if (dlg.exec() == QDialog::Accepted)
		{
			m_emu_settings->SetSetting(emu_settings_type::WindowTitleFormat, sstr(edited_format));
			set_game_window_title(m_emu_settings->GetSetting(emu_settings_type::WindowTitleFormat));
		}
	});

	connect(ui->reset_button_game_window_title_format, &QAbstractButton::clicked, [set_game_window_title, this]()
	{
		const std::string default_game_title_format = m_emu_settings->GetSettingDefault(emu_settings_type::WindowTitleFormat);
		m_emu_settings->SetSetting(emu_settings_type::WindowTitleFormat, default_game_title_format);
		set_game_window_title(default_game_title_format);
	});

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
		// Comboboxes
		SubscribeTooltip(ui->combo_configs, tooltips.settings.configs);

		SubscribeTooltip(ui->gb_stylesheets, tooltips.settings.stylesheets);

		// Checkboxes:
		SubscribeTooltip(ui->cb_custom_colors, tooltips.settings.custom_colors);

		// Checkboxes: gui options
		SubscribeTooltip(ui->cb_show_welcome, tooltips.settings.show_welcome);

		SubscribeTooltip(ui->cb_show_exit_game, tooltips.settings.show_exit_game);

		SubscribeTooltip(ui->cb_show_boot_game, tooltips.settings.show_boot_game);

		SubscribeTooltip(ui->cb_show_pkg_install, tooltips.settings.show_pkg_install);

		SubscribeTooltip(ui->cb_show_pup_install, tooltips.settings.show_pup_install);

		SubscribeTooltip(ui->gb_updates, tooltips.settings.check_update_start);

		SubscribeTooltip(ui->useRichPresence, tooltips.settings.use_rich_presence);

		SubscribeTooltip(ui->discordState, tooltips.settings.discord_state);

		// Discord:
		ui->useRichPresence->setChecked(m_use_discord);
		ui->label_discordState->setEnabled(m_use_discord);
		ui->discordState->setEnabled(m_use_discord);
		ui->discordState->setText(m_discord_state);

		connect(ui->useRichPresence, &QCheckBox::clicked, [this](bool checked)
		{
			ui->discordState->setEnabled(checked);
			ui->label_discordState->setEnabled(checked);
			m_use_discord = checked;
		});

		connect(ui->discordState, &QLineEdit::editingFinished, [this]()
		{
			m_discord_state = ui->discordState->text();
		});

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
		auto add_colored_icon = [this](QPushButton *button, const QColor& color, const QIcon& icon = QIcon(), const QColor& iconColor = QColor())
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

		auto add_colored_icons = [add_colored_icon, this]()
		{
			add_colored_icon(ui->pb_gl_icon_color, m_gui_settings->GetValue(gui::gl_iconColor).value<QColor>());
			add_colored_icon(ui->pb_sd_icon_color, m_gui_settings->GetValue(gui::sd_icon_color).value<QColor>());
			add_colored_icon(ui->pb_tr_icon_color, m_gui_settings->GetValue(gui::tr_icon_color).value<QColor>());
		};
		add_colored_icons();

		ui->cb_show_welcome->setChecked(m_gui_settings->GetValue(gui::ib_show_welcome).toBool());
		ui->cb_show_exit_game->setChecked(m_gui_settings->GetValue(gui::ib_confirm_exit).toBool());
		ui->cb_show_boot_game->setChecked(m_gui_settings->GetValue(gui::ib_confirm_boot).toBool());
		ui->cb_show_pkg_install->setChecked(m_gui_settings->GetValue(gui::ib_pkg_success).toBool());
		ui->cb_show_pup_install->setChecked(m_gui_settings->GetValue(gui::ib_pup_success).toBool());

		const QString updates_yes        = tr("Yes", "Updates");
		const QString updates_background = tr("Background", "Updates");
		const QString updates_no         = tr("No", "Updates");

		ui->combo_updates->addItem(updates_yes, "true");
		ui->combo_updates->addItem(updates_background, "background");
		ui->combo_updates->addItem(updates_no, "false");
		ui->combo_updates->setCurrentIndex(ui->combo_updates->findData(m_gui_settings->GetValue(gui::m_check_upd_start).toString()));
		connect(ui->combo_updates, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged), [this](int index)
		{
			m_gui_settings->SetValue(gui::m_check_upd_start, ui->combo_updates->itemData(index));
		});

		const bool enable_ui_colors = m_gui_settings->GetValue(gui::m_enableUIColors).toBool();
		ui->cb_custom_colors->setChecked(enable_ui_colors);
		ui->pb_gl_icon_color->setEnabled(enable_ui_colors);
		ui->pb_sd_icon_color->setEnabled(enable_ui_colors);
		ui->pb_tr_icon_color->setEnabled(enable_ui_colors);

		auto apply_gui_options = [this](bool reset = false)
		{
			if (reset)
			{
				m_current_stylesheet = gui::Default;
				ui->combo_configs->setCurrentIndex(0);
				ui->combo_stylesheets->setCurrentIndex(0);
			}
			// Only attempt to load a config if changes occurred.
			if (m_current_gui_config != ui->combo_configs->currentText())
			{
				OnApplyGuiConfig();
			}
			if (m_current_stylesheet != m_gui_settings->GetValue(gui::m_currentStylesheet).toString())
			{
				OnApplyStylesheet();
			}
		};

		connect(ui->buttonBox, &QDialogButtonBox::accepted, [apply_gui_options, this]()
		{
			apply_gui_options();
		});

		connect(ui->pb_reset_default, &QAbstractButton::clicked, [apply_gui_options, this]
		{
			if (QMessageBox::question(this, tr("Reset GUI to default?", "Reset"), tr("This will include your stylesheet as well. Do you wish to proceed?", "Reset"),
				QMessageBox::Yes | QMessageBox::No, QMessageBox::No) == QMessageBox::Yes)
			{
				apply_gui_options(true);
				m_gui_settings->Reset(true);
				Q_EMIT GuiSettingsSyncRequest(true);
				AddGuiConfigs();
				AddStylesheets();
				apply_gui_options();
			}
		});

		connect(ui->pb_backup_config, &QAbstractButton::clicked, this, &settings_dialog::OnBackupCurrentGuiConfig);
		connect(ui->pb_apply_config, &QAbstractButton::clicked, this, &settings_dialog::OnApplyGuiConfig);
		connect(ui->pb_apply_stylesheet, &QAbstractButton::clicked, this, &settings_dialog::OnApplyStylesheet);

		connect(ui->pb_open_folder, &QAbstractButton::clicked, [this]()
		{
			QDesktopServices::openUrl(m_gui_settings->GetSettingsDir());
		});

		connect(ui->cb_show_welcome, &QCheckBox::clicked, [this](bool val)
		{
			m_gui_settings->SetValue(gui::ib_show_welcome, val);
		});
		connect(ui->cb_show_exit_game, &QCheckBox::clicked, [this](bool val)
		{
			m_gui_settings->SetValue(gui::ib_confirm_exit, val);
		});
		connect(ui->cb_show_boot_game, &QCheckBox::clicked, [this](bool val)
		{
			m_gui_settings->SetValue(gui::ib_confirm_boot, val);
		});
		connect(ui->cb_show_pkg_install, &QCheckBox::clicked, [this](bool val)
		{
			m_gui_settings->SetValue(gui::ib_pkg_success, val);
		});
		connect(ui->cb_show_pup_install, &QCheckBox::clicked, [this](bool val)
		{
			m_gui_settings->SetValue(gui::ib_pup_success, val);
		});

		connect(ui->cb_custom_colors, &QCheckBox::clicked, [this](bool val)
		{
			m_gui_settings->SetValue(gui::m_enableUIColors, val);
			ui->pb_gl_icon_color->setEnabled(val);
			ui->pb_sd_icon_color->setEnabled(val);
			ui->pb_tr_icon_color->setEnabled(val);
			Q_EMIT GuiRepaintRequest();
		});
		auto color_dialog = [&](const gui_save& color, const QString& title, QPushButton *button)
		{
			const QColor old_color = m_gui_settings->GetValue(color).value<QColor>();
			QColorDialog dlg(old_color, this);
			dlg.setWindowTitle(title);
			dlg.setOptions(QColorDialog::ShowAlphaChannel);
			for (int i = 0; i < dlg.customCount(); i++)
			{
				dlg.setCustomColor(i, m_gui_settings->GetCustomColor(i));
			}
			if (dlg.exec() == QColorDialog::Accepted)
			{
				for (int i = 0; i < dlg.customCount(); i++)
				{
					m_gui_settings->SetCustomColor(i, dlg.customColor(i));
				}
				m_gui_settings->SetValue(color, dlg.selectedColor());
				button->setIcon(gui::utils::get_colorized_icon(button->icon(), old_color, dlg.selectedColor(), true));
				Q_EMIT GuiRepaintRequest();
			}
		};

		connect(ui->pb_gl_icon_color, &QAbstractButton::clicked, [=, this]()
		{
			color_dialog(gui::gl_iconColor, tr("Choose gamelist icon color", "Settings: color dialog"), ui->pb_gl_icon_color);
		});
		connect(ui->pb_sd_icon_color, &QAbstractButton::clicked, [=, this]()
		{
			color_dialog(gui::sd_icon_color, tr("Choose save manager icon color", "Settings: color dialog"), ui->pb_sd_icon_color);
		});
		connect(ui->pb_tr_icon_color, &QAbstractButton::clicked, [=, this]()
		{
			color_dialog(gui::tr_icon_color, tr("Choose trophy manager icon color", "Settings: color dialog"), ui->pb_tr_icon_color);
		});

		AddGuiConfigs();
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
	m_emu_settings->EnhanceCheckBox(ui->glLegacyBuffers, emu_settings_type::LegacyBuffers);
	SubscribeTooltip(ui->glLegacyBuffers, tooltips.settings.gl_legacy_buffers);

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

	m_emu_settings->EnhanceCheckBox(ui->forceCpuBlitEmulation, emu_settings_type::ForceCPUBlitEmulation);
	SubscribeTooltip(ui->forceCpuBlitEmulation, tooltips.settings.force_cpu_blit_emulation);

	m_emu_settings->EnhanceCheckBox(ui->disableVulkanMemAllocator, emu_settings_type::DisableVulkanMemAllocator);
	SubscribeTooltip(ui->disableVulkanMemAllocator, tooltips.settings.disable_vulkan_mem_allocator);

	m_emu_settings->EnhanceCheckBox(ui->disableFIFOReordering, emu_settings_type::DisableFIFOReordering);
	SubscribeTooltip(ui->disableFIFOReordering, tooltips.settings.disable_fifo_reordering);

	m_emu_settings->EnhanceCheckBox(ui->strictTextureFlushing, emu_settings_type::StrictTextureFlushing);
	SubscribeTooltip(ui->strictTextureFlushing, tooltips.settings.strict_texture_flushing);

	m_emu_settings->EnhanceCheckBox(ui->DisableNativefp16, emu_settings_type::DisableNativefloat16);
	SubscribeTooltip(ui->DisableNativefp16, tooltips.settings.disable_native_fp16);

	m_emu_settings->EnhanceCheckBox(ui->Enable3D, emu_settings_type::Enable3D);
	SubscribeTooltip(ui->Enable3D, tooltips.settings.enable_3d);

	m_emu_settings->EnhanceCheckBox(ui->gpuTextureScaling, emu_settings_type::GPUTextureScaling);
	SubscribeTooltip(ui->gpuTextureScaling, tooltips.settings.gpu_texture_scaling);

	// Checkboxes: core debug options
	m_emu_settings->EnhanceCheckBox(ui->ppuDebug, emu_settings_type::PPUDebug);
	SubscribeTooltip(ui->ppuDebug, tooltips.settings.ppu_debug);

	m_emu_settings->EnhanceCheckBox(ui->spuDebug, emu_settings_type::SPUDebug);
	SubscribeTooltip(ui->spuDebug, tooltips.settings.spu_debug);

	m_emu_settings->EnhanceCheckBox(ui->setDAZandFTZ, emu_settings_type::SetDAZandFTZ);
	SubscribeTooltip(ui->setDAZandFTZ, tooltips.settings.set_daz_and_ftz);

	m_emu_settings->EnhanceCheckBox(ui->accurateGETLLAR, emu_settings_type::AccurateGETLLAR);
	SubscribeTooltip(ui->accurateGETLLAR, tooltips.settings.accurate_getllar);

	m_emu_settings->EnhanceCheckBox(ui->accurateSpuDMA, emu_settings_type::AccurateSpuDMA);
	SubscribeTooltip(ui->accurateSpuDMA, tooltips.settings.accurate_spu_dma);

	m_emu_settings->EnhanceCheckBox(ui->accurateClineStores, emu_settings_type::AccurateClineStores);
	SubscribeTooltip(ui->accurateClineStores, tooltips.settings.accurate_cache_line_stores);

	m_emu_settings->EnhanceCheckBox(ui->accurateRSXAccess, emu_settings_type::AccurateRSXAccess);
	SubscribeTooltip(ui->accurateRSXAccess, tooltips.settings.accurate_rsx_access);

	m_emu_settings->EnhanceCheckBox(ui->hookStFunc, emu_settings_type::HookStaticFuncs);
	SubscribeTooltip(ui->hookStFunc, tooltips.settings.hook_static_functions);

	m_emu_settings->EnhanceCheckBox(ui->perfReport, emu_settings_type::PerformanceReport);
	SubscribeTooltip(ui->perfReport, tooltips.settings.enable_performance_report);

	// Comboboxes

	m_emu_settings->EnhanceComboBox(ui->combo_accurate_ppu_128, emu_settings_type::AccuratePPU128Loop, true);
	SubscribeTooltip(ui->gb_accurate_ppu_128, tooltips.settings.accurate_ppu_128_loop);
	ui->combo_accurate_ppu_128->setItemText(ui->combo_accurate_ppu_128->findData(-1), tr("Always Enabled", "Accurate PPU 128 Reservations"));
	ui->combo_accurate_ppu_128->setItemText(ui->combo_accurate_ppu_128->findData(0), tr("Disabled", "Accurate PPU 128 Reservations"));

	// Layout fix for High Dpi
	layout()->setSizeConstraint(QLayout::SetFixedSize);
}

settings_dialog::~settings_dialog()
{
	delete ui;
}

void settings_dialog::EnhanceSlider(emu_settings_type settings_type, QSlider* slider, QLabel* label, const QString& label_text)
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
	connect(slider, &QSlider::sliderPressed, [this, slider]()
	{
		m_current_slider = slider;
	});

	connect(slider, &QSlider::sliderReleased, [this]()
	{
		m_current_slider = nullptr;
	});

	connect(slider, &QSlider::valueChanged, [this, slider, interval](int value)
	{
		if (slider != m_current_slider)
		{
			return;
		}
		slider->setValue(utils::rounded_div(value, interval) * interval);
	});
}

void settings_dialog::AddGuiConfigs()
{
	ui->combo_configs->clear();
	ui->combo_configs->addItems(m_gui_settings->GetConfigEntries());

	m_current_gui_config = m_gui_settings->GetValue(gui::m_currentConfig).toString();

	const int index = ui->combo_configs->findText(m_current_gui_config);
	if (index >= 0)
	{
		ui->combo_configs->setCurrentIndex(index);
	}
	else
	{
		cfg_log.warning("Trying to set an invalid config index %d", index);
	}
}

void settings_dialog::AddStylesheets()
{
	ui->combo_stylesheets->clear();

	ui->combo_stylesheets->addItem(tr("None", "Stylesheets"), gui::None);
	ui->combo_stylesheets->addItem(tr("Default (Bright)", "Stylesheets"), gui::Default);

	for (const QString& entry : m_gui_settings->GetStylesheetEntries())
	{
		if (entry != gui::Default)
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
		cfg_log.warning("Trying to set an invalid stylesheets index: %d (%s)", index, sstr(m_current_stylesheet));
	}
}

void settings_dialog::OnBackupCurrentGuiConfig()
{
	QInputDialog* dialog = new QInputDialog(this);
	dialog->setWindowTitle(tr("Choose a unique name", "Backup GUI config"));
	dialog->setLabelText(tr("Configuration Name: ", "Backup GUI config"));
	dialog->resize(500, 100);

	while (dialog->exec() != QDialog::Rejected)
	{
		dialog->resize(500, 100);

		const QString gui_config_name = dialog->textValue();

		if (gui_config_name.isEmpty())
		{
			QMessageBox::warning(this, tr("Error", "Backup GUI config warning 1"), tr("Name cannot be empty", "Backup GUI config warning 1"));
			continue;
		}
		if (gui_config_name.contains("."))
		{
			QMessageBox::warning(this, tr("Error", "Backup GUI config warning 2"), tr("Must choose a name with no '.'", "Backup GUI config warning 2"));
			continue;
		}
		if (ui->combo_configs->findText(gui_config_name) != -1)
		{
			QMessageBox::warning(this, tr("Error", "Backup GUI config warning 3"), tr("Please choose a non-existing name", "Backup GUI config warning 3"));
			continue;
		}
		Q_EMIT GuiSettingsSaveRequest();
		m_gui_settings->SaveCurrentConfig(gui_config_name);
		ui->combo_configs->addItem(gui_config_name);
		ui->combo_configs->setCurrentText(gui_config_name);
		m_current_gui_config = gui_config_name;
		break;
	}
}

void settings_dialog::OnApplyGuiConfig()
{
	const QString new_config = ui->combo_configs->currentText();

	if (new_config == m_current_gui_config)
	{
		return;
	}

	// Backup current window states
	Q_EMIT GuiSettingsSaveRequest();

	if (!m_gui_settings->ChangeToConfig(new_config))
	{
		const int new_config_idx = ui->combo_configs->currentIndex();
		ui->combo_configs->setCurrentText(m_current_gui_config);
		ui->combo_configs->removeItem(new_config_idx);
		return;
	}

	m_current_gui_config = new_config;
	Q_EMIT GuiSettingsSyncRequest(true);
}

void settings_dialog::OnApplyStylesheet()
{
	m_current_stylesheet = ui->combo_stylesheets->currentData().toString();
	m_gui_settings->SetValue(gui::m_currentStylesheet, m_current_stylesheet);
	Q_EMIT GuiStylesheetRequest(m_gui_settings->GetCurrentStylesheetPath());
}

int settings_dialog::exec()
{
	// singleShot Hack to fix following bug:
	// If we use setCurrentIndex now we will miraculously see a resize of the dialog as soon as we
	// switch to the cpu tab after conjuring the settings_dialog with another tab opened first.
	// Weirdly enough this won't happen if we change the tab order so that anything else is at index 0.
	ui->tab_widget_settings->setCurrentIndex(0);
	QTimer::singleShot(0, [this]{ ui->tab_widget_settings->setCurrentIndex(m_tab_index); });

	// Open a dialog if your config file contained invalid entries
	QTimer::singleShot(10, [this] { m_emu_settings->OpenCorrectionDialog(this); });

	return QDialog::exec();
}

void settings_dialog::SubscribeDescription(QLabel* description)
{
	description->setFixedHeight(description->sizeHint().height());
	m_description_labels.append(QPair<QLabel*, QString>(description, description->text()));
}

void settings_dialog::SubscribeTooltip(QObject* object, const QString& tooltip)
{
	m_descriptions[object] = tooltip;
	object->installEventFilter(this);
}

// Thanks Dolphin
bool settings_dialog::eventFilter(QObject* object, QEvent* event)
{
	if (!m_descriptions.contains(object))
	{
		return QDialog::eventFilter(object, event);
	}

	if (event->type() == QEvent::Enter || event->type() == QEvent::Leave)
	{
		const int i = ui->tab_widget_settings->currentIndex();

		if (i < m_description_labels.size())
		{
			QLabel* label = m_description_labels[i].first;

			if (label)
			{
				if (event->type() == QEvent::Enter)
				{
					label->setText(m_descriptions[object]);
				}
				else if (event->type() == QEvent::Leave)
				{
					const QString description = m_description_labels[i].second;
					label->setText(description);
				}
			}
		}
	}

	return QDialog::eventFilter(object, event);
}

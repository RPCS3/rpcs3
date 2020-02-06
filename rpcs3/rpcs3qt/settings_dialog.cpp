#include <QVBoxLayout>
#include <QButtonGroup>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QMessageBox>
#include <QInputDialog>
#include <QDesktopServices>
#include <QColorDialog>
#include <QSpinBox>
#include <QApplication>
#include <QDesktopWidget>
#include <QTimer>
#include <QScreen>

#include "display_sleep_control.h"
#include "qt_utils.h"
#include "settings_dialog.h"
#include "ui_settings_dialog.h"
#include "tooltips.h"

#include "stdafx.h"
#include "Emu/System.h"
#include "Crypto/unself.h"
#include "Utilities/sysinfo.h"

#include <unordered_set>
#include <thread>

#ifdef WITH_DISCORD_RPC
#include "_discord_utils.h"
#endif

LOG_CHANNEL(cfg_log, "CFG");

inline std::string sstr(const QString& _in) { return _in.toStdString(); }
inline std::string sstr(const QVariant& _in) { return sstr(_in.toString()); }

settings_dialog::settings_dialog(std::shared_ptr<gui_settings> guiSettings, std::shared_ptr<emu_settings> emuSettings, const int& tabIndex, QWidget *parent, const GameInfo* game)
	: QDialog(parent), xgui_settings(guiSettings), xemu_settings(emuSettings), ui(new Ui::settings_dialog), m_tab_Index(tabIndex)
{
	ui->setupUi(this);
	ui->buttonBox->button(QDialogButtonBox::StandardButton::Close)->setFocus();
	ui->tab_widget_settings->setUsesScrollButtons(false);
	ui->tab_widget_settings->tabBar()->setObjectName("tab_bar_settings");

	bool showDebugTab = xgui_settings->GetValue(gui::m_showDebugTab).toBool();
	xgui_settings->SetValue(gui::m_showDebugTab, showDebugTab);
	if (!showDebugTab)
	{
		ui->tab_widget_settings->removeTab(9);
	}
	if (game)
	{
		ui->tab_widget_settings->removeTab(8);
		ui->buttonBox->button(QDialogButtonBox::StandardButton::Save)->setText(tr("Save custom configuration"));
	}

	// Localized tooltips
	Tooltips tooltips;

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
		xemu_settings->LoadSettings(game->serial);
		setWindowTitle(tr("Settings: [") + qstr(game->serial) + "] " + qstr(game->name));
	}
	else
	{
		xemu_settings->LoadSettings();
		setWindowTitle(tr("Settings"));
	}

	// Discord variables
	m_use_discord = xgui_settings->GetValue(gui::m_richPresence).toBool();
	m_discord_state = xgui_settings->GetValue(gui::m_discordState).toString();

	// Various connects
	connect(ui->buttonBox, &QDialogButtonBox::accepted, [=, use_discord_old = m_use_discord, discord_state_old = m_discord_state]
	{
		std::set<std::string> selectedlle;
		for (int i = 0; i<ui->lleList->count(); ++i)
		{
			const auto& item = ui->lleList->item(i);
			if (item->checkState() != Qt::CheckState::Unchecked)
			{
				selectedlle.emplace(sstr(item->text()));
			}
		}
		std::vector<std::string> selected_ls = std::vector<std::string>(selectedlle.begin(), selectedlle.end());
		xemu_settings->SaveSelectedLibraries(selected_ls);
		xemu_settings->SaveSettings();
		accept();

		// Discord Settings can be saved regardless of WITH_DISCORD_RPC
		xgui_settings->SetValue(gui::m_richPresence, m_use_discord);
		xgui_settings->SetValue(gui::m_discordState, m_discord_state);

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
	});

	connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &QWidget::close);

	connect(ui->tab_widget_settings, &QTabWidget::currentChanged, [=]()
	{
		ui->buttonBox->button(QDialogButtonBox::StandardButton::Close)->setFocus();
	});

	//     _____ _____  _    _   _______    _
	//    / ____|  __ \| |  | | |__   __|  | |
	//   | |    | |__) | |  | |    | | __ _| |__
	//   | |    |  ___/| |  | |    | |/ _` | '_ \
	//   | |____| |    | |__| |    | | (_| | |_) |
	//    \_____|_|     \____/     |_|\__,_|_.__/

	// Checkboxes

	xemu_settings->EnhanceCheckBox(ui->spuCache, emu_settings::SPUCache);
	SubscribeTooltip(ui->spuCache, tooltips.settings.spu_cache);

	xemu_settings->EnhanceCheckBox(ui->enableScheduler, emu_settings::EnableThreadScheduler);
	SubscribeTooltip(ui->enableScheduler, tooltips.settings.enable_thread_scheduler);

	xemu_settings->EnhanceCheckBox(ui->lowerSPUThrPrio, emu_settings::LowerSPUThreadPrio);
	SubscribeTooltip(ui->lowerSPUThrPrio, tooltips.settings.lower_spu_thread_priority);

	xemu_settings->EnhanceCheckBox(ui->spuLoopDetection, emu_settings::SPULoopDetection);
	SubscribeTooltip(ui->spuLoopDetection, tooltips.settings.spu_loop_detection);

	xemu_settings->EnhanceCheckBox(ui->accurateXFloat, emu_settings::AccurateXFloat);
	SubscribeTooltip(ui->accurateXFloat, tooltips.settings.accurate_xfloat);

	// Comboboxes

	xemu_settings->EnhanceComboBox(ui->spuBlockSize, emu_settings::SPUBlockSize);
	SubscribeTooltip(ui->gb_spuBlockSize, tooltips.settings.spu_block_size);

	xemu_settings->EnhanceComboBox(ui->preferredSPUThreads, emu_settings::PreferredSPUThreads, true);
	SubscribeTooltip(ui->gb_spu_threads, tooltips.settings.preferred_spu_threads);
	ui->preferredSPUThreads->setItemText(ui->preferredSPUThreads->findData("0"), tr("Auto"));

	if (utils::has_rtm())
	{
		xemu_settings->EnhanceComboBox(ui->enableTSX, emu_settings::EnableTSX);
		SubscribeTooltip(ui->gb_tsx, tooltips.settings.enable_tsx);

		static const QString tsx_forced = qstr(fmt::format("%s", tsx_usage::forced));
		static const QString tsx_default = qstr(xemu_settings->GetSettingDefault(emu_settings::EnableTSX));

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
		ui->enableTSX->addItem(tr("Not supported"));
		SubscribeTooltip(ui->enableTSX, tr("Unfortunately your CPU model does not support this instruction set."));
	}

	// PPU tool tips
	SubscribeTooltip(ui->ppu_precise, tooltips.settings.ppu_precise);
	SubscribeTooltip(ui->ppu_fast,    tooltips.settings.ppu_fast);
	SubscribeTooltip(ui->ppu_llvm,    tooltips.settings.ppu_llvm);

	QButtonGroup *ppuBG = new QButtonGroup(this);
	ppuBG->addButton(ui->ppu_precise, static_cast<int>(ppu_decoder_type::precise));
	ppuBG->addButton(ui->ppu_fast,    static_cast<int>(ppu_decoder_type::fast));
	ppuBG->addButton(ui->ppu_llvm,    static_cast<int>(ppu_decoder_type::llvm));

	{ // PPU Stuff
		QString selectedPPU = qstr(xemu_settings->GetSetting(emu_settings::PPUDecoder));
		QStringList ppu_list = xemu_settings->GetSettingOptions(emu_settings::PPUDecoder);

		for (int i = 0; i < ppu_list.count(); i++)
		{
			if (ppu_list[i] == selectedPPU)
			{
				ppuBG->button(i)->setChecked(true);
			}

			connect(ppuBG->button(i), &QAbstractButton::clicked, [=]()
			{
				xemu_settings->SetSetting(emu_settings::PPUDecoder, sstr(ppu_list[i]));
			});
		}
	}

	// SPU tool tips
	SubscribeTooltip(ui->spu_precise, tooltips.settings.spu_precise);
	SubscribeTooltip(ui->spu_fast,    tooltips.settings.spu_fast);
	SubscribeTooltip(ui->spu_asmjit,  tooltips.settings.spu_asmjit);
	SubscribeTooltip(ui->spu_llvm,    tooltips.settings.spu_llvm);

	QButtonGroup *spuBG = new QButtonGroup(this);
	spuBG->addButton(ui->spu_precise, static_cast<int>(spu_decoder_type::precise));
	spuBG->addButton(ui->spu_fast,    static_cast<int>(spu_decoder_type::fast));
	spuBG->addButton(ui->spu_asmjit,  static_cast<int>(spu_decoder_type::asmjit));
	spuBG->addButton(ui->spu_llvm,    static_cast<int>(spu_decoder_type::llvm));

	{ // Spu stuff
		QString selectedSPU = qstr(xemu_settings->GetSetting(emu_settings::SPUDecoder));
		QStringList spu_list = xemu_settings->GetSettingOptions(emu_settings::SPUDecoder);

		for (int i = 0; i < spu_list.count(); i++)
		{
			if (spu_list[i] == selectedSPU)
			{
				spuBG->button(i)->setChecked(true);
			}

			connect(spuBG->button(i), &QAbstractButton::clicked, [=]()
			{
				xemu_settings->SetSetting(emu_settings::SPUDecoder, sstr(spu_list[i]));
			});
		}
	}

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
	//   | | |_ |  ___/| |  | |    | |/ _` | '_ \
	//   | |__| | |    | |__| |    | | (_| | |_) |
	//    \_____|_|     \____/     |_|\__,_|_.__/

	emu_settings::Render_Creator render_creator = xemu_settings.get()->m_render_creator;

	// Comboboxes
	xemu_settings->EnhanceComboBox(ui->renderBox, emu_settings::Renderer);
	SubscribeTooltip(ui->gb_renderer, tooltips.settings.renderer);
	SubscribeTooltip(ui->gb_graphicsAdapter, tooltips.settings.graphics_adapter);

	// Change displayed renderer names
	ui->renderBox->setItemText(ui->renderBox->findData("Null"), render_creator.name_Null);

	xemu_settings->EnhanceComboBox(ui->resBox, emu_settings::Resolution);
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
			bool hasResolution = false;
			for (const auto& res : resolutions)
			{
				if ((game->resolution & res.first) && res.second == sstr(ui->resBox->itemText(i)))
				{
					hasResolution = true;
					break;
				}
			}
			if (!hasResolution)
			{
				ui->resBox->removeItem(i);
				if (i == saved_index)
				{
					saved_index_removed = true;
				}
			}
		}
	}
	const int res_index = ui->resBox->findData("1280x720");
	if (res_index >= 0)
	{
		// Rename the default resolution for users
		ui->resBox->setItemText(res_index, tr("1280x720 (Recommended)"));

		// Set the current selection to the default if the original setting wasn't valid
		if (saved_index_removed)
		{
			ui->resBox->setCurrentIndex(res_index);
		}
	}

	xemu_settings->EnhanceComboBox(ui->aspectBox, emu_settings::AspectRatio);
	SubscribeTooltip(ui->gb_aspectRatio, tooltips.settings.aspect_ratio);

	xemu_settings->EnhanceComboBox(ui->frameLimitBox, emu_settings::FrameLimit);
	SubscribeTooltip(ui->gb_frameLimit, tooltips.settings.frame_limit);

	xemu_settings->EnhanceComboBox(ui->antiAliasing, emu_settings::MSAA);
	SubscribeTooltip(ui->gb_antiAliasing, tooltips.settings.anti_aliasing);

	xemu_settings->EnhanceComboBox(ui->anisotropicFilterOverride, emu_settings::AnisotropicFilterOverride, true);
	SubscribeTooltip(ui->gb_anisotropicFilter, tooltips.settings.anisotropic_filter);
	// only allow values 0,2,4,8,16
	for (int i = ui->anisotropicFilterOverride->count() - 1; i >= 0; i--)
	{
		switch (int val = ui->anisotropicFilterOverride->itemData(i).toInt())
		{
		case 0:
			ui->anisotropicFilterOverride->setItemText(i, tr("Automatic"));
			break;
		case 2:
		case 4:
		case 8:
		case 16:
			ui->anisotropicFilterOverride->setItemText(i, tr("%1x").arg(val));
			break;
		default:
			ui->anisotropicFilterOverride->removeItem(i);
			break;
		}
	}

	// Checkboxes: main options
	xemu_settings->EnhanceCheckBox(ui->dumpColor, emu_settings::WriteColorBuffers);
	SubscribeTooltip(ui->dumpColor, tooltips.settings.dump_color);

	xemu_settings->EnhanceCheckBox(ui->vsync, emu_settings::VSync);
	SubscribeTooltip(ui->vsync, tooltips.settings.vsync);

	xemu_settings->EnhanceCheckBox(ui->stretchToDisplayArea, emu_settings::StretchToDisplayArea);
	SubscribeTooltip(ui->stretchToDisplayArea, tooltips.settings.stretch_to_display_area);

	xemu_settings->EnhanceCheckBox(ui->disableVertexCache, emu_settings::DisableVertexCache);
	SubscribeTooltip(ui->disableVertexCache, tooltips.settings.disable_vertex_cache);

	xemu_settings->EnhanceCheckBox(ui->multithreadedRSX, emu_settings::MultithreadedRSX);
	SubscribeTooltip(ui->multithreadedRSX, tooltips.settings.multithreaded_rsx);

	xemu_settings->EnhanceCheckBox(ui->disableAsyncShaders, emu_settings::DisableAsyncShaderCompiler);
	SubscribeTooltip(ui->disableAsyncShaders, tooltips.settings.disable_async_shaders);

	xemu_settings->EnhanceCheckBox(ui->scrictModeRendering, emu_settings::StrictRenderingMode);
	SubscribeTooltip(ui->scrictModeRendering, tooltips.settings.strict_rendering_mode);
	connect(ui->scrictModeRendering, &QCheckBox::clicked, [=](bool checked)
	{
		ui->gb_resolutionScale->setEnabled(!checked);
		ui->gb_minimumScalableDimension->setEnabled(!checked);
	});

	// Sliders
	static const auto& minmaxLabelWidth = [](const QString& sizer)
	{
		return QLabel(sizer).sizeHint().width();
	};

	xemu_settings->EnhanceSlider(ui->resolutionScale, emu_settings::ResolutionScale);
	SubscribeTooltip(ui->gb_resolutionScale, tooltips.settings.resolution_scale);
	ui->gb_resolutionScale->setEnabled(!ui->scrictModeRendering->isChecked());
	// rename label texts to fit current state of Resolution Scale
	int resolutionScaleDef = stoi(xemu_settings->GetSettingDefault(emu_settings::ResolutionScale));
	auto ScaledResolution = [resolutionScaleDef](int percentage)
	{
		if (percentage == resolutionScaleDef)
		{
			return QString(tr("100% (Default)"));
		}
		return QString("%1% (%2x%3)").arg(percentage).arg(1280 * percentage / 100).arg(720 * percentage / 100);
	};
	ui->resolutionScale->setPageStep(50);
	ui->resolutionScaleMin->setText(QString::number(ui->resolutionScale->minimum()));
	ui->resolutionScaleMin->setFixedWidth(minmaxLabelWidth("00"));
	ui->resolutionScaleMax->setText(QString::number(ui->resolutionScale->maximum()));
	ui->resolutionScaleMax->setFixedWidth(minmaxLabelWidth("0000"));
	ui->resolutionScaleVal->setText(ScaledResolution(ui->resolutionScale->value()));
	connect(ui->resolutionScale, &QSlider::valueChanged, [=](int value)
	{
		ui->resolutionScaleVal->setText(ScaledResolution(value));
	});
	connect(ui->resolutionScaleReset, &QAbstractButton::clicked, [=]()
	{
		ui->resolutionScale->setValue(resolutionScaleDef);
	});
	SnapSlider(ui->resolutionScale, 25);

	xemu_settings->EnhanceSlider(ui->minimumScalableDimension, emu_settings::MinimumScalableDimension);
	SubscribeTooltip(ui->gb_minimumScalableDimension, tooltips.settings.minimum_scalable_dimension);
	ui->gb_minimumScalableDimension->setEnabled(!ui->scrictModeRendering->isChecked());
	// rename label texts to fit current state of Minimum Scalable Dimension
	int minimumScalableDimensionDef = stoi(xemu_settings->GetSettingDefault(emu_settings::MinimumScalableDimension));
	auto MinScalableDimension = [minimumScalableDimensionDef](int dim)
	{
		if (dim == minimumScalableDimensionDef)
		{
			return tr("%1x%1 (Default)").arg(dim);
		}
		return QString("%1x%1").arg(dim);
	};
	ui->minimumScalableDimension->setPageStep(64);
	ui->minimumScalableDimensionMin->setText(QString::number(ui->minimumScalableDimension->minimum()));
	ui->minimumScalableDimensionMin->setFixedWidth(minmaxLabelWidth("00"));
	ui->minimumScalableDimensionMax->setText(QString::number(ui->minimumScalableDimension->maximum()));
	ui->minimumScalableDimensionMax->setFixedWidth(minmaxLabelWidth("0000"));
	ui->minimumScalableDimensionVal->setText(MinScalableDimension(ui->minimumScalableDimension->value()));
	connect(ui->minimumScalableDimension, &QSlider::valueChanged, [=](int value)
	{
		ui->minimumScalableDimensionVal->setText(MinScalableDimension(value));
	});
	connect(ui->minimumScalableDimensionReset, &QAbstractButton::clicked, [=]()
	{
		ui->minimumScalableDimension->setValue(minimumScalableDimensionDef);
	});

	// Remove renderers from the renderer Combobox if not supported
	for (const auto& renderer : render_creator.renderers)
	{
		if (renderer->supported)
		{
			if (renderer->has_adapters)
			{
				renderer->old_adapter = qstr(xemu_settings->GetSetting(renderer->type));
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

	m_oldRender = ui->renderBox->currentText();

	auto setRenderer = [=](QString text)
	{
		if (text.isEmpty())
		{
			return;
		}

		auto switchTo = [=](emu_settings::Render_Info renderer)
		{
			// Reset other adapters to old config
			for (const auto& render : render_creator.renderers)
			{
				if (renderer.name != render->name && render->has_adapters && render->supported)
				{
					xemu_settings->SetSetting(render->type, sstr(render->old_adapter));
				}
			}

			// Enable/disable MSAA depending on renderer
			ui->antiAliasing->setEnabled(renderer.has_msaa);
			ui->antiAliasing->blockSignals(true);
			ui->antiAliasing->setCurrentText(renderer.has_msaa ? qstr(xemu_settings->GetSetting(emu_settings::MSAA)) : tr("Disabled"));
			ui->antiAliasing->blockSignals(false);

			// Fill combobox with placeholder if no adapters needed
			if (!renderer.has_adapters)
			{
				ui->graphicsAdapterBox->clear();
				ui->graphicsAdapterBox->addItem(tr("Not needed for %1 renderer").arg(text));
				return;
			}
			// Fill combobox
			ui->graphicsAdapterBox->clear();
			for (const auto& adapter : renderer.adapters)
			{
				ui->graphicsAdapterBox->addItem(adapter);
			}
			// Reset Adapter to old config
			int idx = ui->graphicsAdapterBox->findText(renderer.old_adapter);
			if (idx == -1)
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
			xemu_settings->SetSetting(renderer.type, sstr(ui->graphicsAdapterBox->currentText()));
		};

		for (const auto& renderer : render_creator.renderers)
		{
			if (renderer->name == text)
			{
				switchTo(*renderer);
				ui->graphicsAdapterBox->setEnabled(renderer->has_adapters);
			}
		}
	};

	auto setAdapter = [=](QString text)
	{
		if (text.isEmpty())
		{
			return;
		}

		// don't set adapter if signal was created by switching render
		QString newRender = ui->renderBox->currentText();
		if (m_oldRender != newRender)
		{
			m_oldRender = newRender;
			return;
		}
		for (const auto& render : render_creator.renderers)
		{
			if (render->name == newRender && render->has_adapters && render->adapters.contains(text))
			{
				xemu_settings->SetSetting(render->type, sstr(text));
				break;
			}
		}
	};

	// Init
	setRenderer(ui->renderBox->currentText());
	setAdapter(ui->graphicsAdapterBox->currentText());

	// Events
	connect(ui->graphicsAdapterBox, &QComboBox::currentTextChanged, setAdapter);
	connect(ui->renderBox, &QComboBox::currentTextChanged, setRenderer);

	auto fixGLLegacy = [=](const QString& text)
	{
		ui->glLegacyBuffers->setEnabled(text == render_creator.name_OpenGL);
	};

	// Handle connects to disable specific checkboxes that depend on GUI state.
	fixGLLegacy(ui->renderBox->currentText()); // Init
	connect(ui->renderBox, &QComboBox::currentTextChanged, fixGLLegacy);

	//                      _ _         _______    _
	//       /\            | (_)       |__   __|  | |
	//      /  \  _   _  __| |_  ___      | | __ _| |__
	//     / /\ \| | | |/ _` | |/ _ \     | |/ _` | '_ \
	//    / ____ \ |_| | (_| | | (_) |    | | (_| | |_) |
	//   /_/    \_\__,_|\__,_|_|\___/     |_|\__,_|_.__/

	auto EnableTimeStretchingOptions = [this](bool enabled)
	{
		ui->timeStretchingThresholdLabel->setEnabled(enabled);
		ui->timeStretchingThreshold->setEnabled(enabled);
	};

	auto EnableBufferingOptions = [this, EnableTimeStretchingOptions](bool enabled)
	{
		ui->audioBufferDuration->setEnabled(enabled);
		ui->audioBufferDurationLabel->setEnabled(enabled);
		ui->enableTimeStretching->setEnabled(enabled);
		EnableTimeStretchingOptions(enabled && ui->enableTimeStretching->isChecked());
	};

	auto EnableBuffering = [this, EnableBufferingOptions](const QString& text)
	{
		const bool enabled = text == "XAudio2" || text == "OpenAL" || text == "FAudio";
		ui->enableBuffering->setEnabled(enabled);
		EnableBufferingOptions(enabled && ui->enableBuffering->isChecked());
	};

	auto ChangeMicrophoneType = [=](QString text)
	{
		std::string s_standard, s_singstar, s_realsingstar, s_rocksmith;

		auto enableMicsCombo = [=](u32 max)
		{
			ui->microphone1Box->setEnabled(true);

			if (max == 1 || ui->microphone1Box->currentText() == xemu_settings->m_microphone_creator.mic_none)
				return;

			ui->microphone2Box->setEnabled(true);

			if (max > 2 && ui->microphone2Box->currentText() != xemu_settings->m_microphone_creator.mic_none)
			{
				ui->microphone3Box->setEnabled(true);
				if (ui->microphone3Box->currentText() != xemu_settings->m_microphone_creator.mic_none)
				{
					ui->microphone4Box->setEnabled(true);
				}
			}
		};

		ui->microphone1Box->setEnabled(false);
		ui->microphone2Box->setEnabled(false);
		ui->microphone3Box->setEnabled(false);
		ui->microphone4Box->setEnabled(false);

		fmt_class_string<microphone_handler>::format(s_standard, static_cast<u64>(microphone_handler::standard));
		fmt_class_string<microphone_handler>::format(s_singstar, static_cast<u64>(microphone_handler::singstar));
		fmt_class_string<microphone_handler>::format(s_realsingstar, static_cast<u64>(microphone_handler::real_singstar));
		fmt_class_string<microphone_handler>::format(s_rocksmith, static_cast<u64>(microphone_handler::rocksmith));

		if (text == s_standard.c_str())
		{
			enableMicsCombo(4);
			return;
		}
		if (text == s_singstar.c_str())
		{
			enableMicsCombo(2);
			return;
		}
		if (text == s_realsingstar.c_str() || text == s_rocksmith.c_str())
		{
			enableMicsCombo(1);
			return;
		}
	};

	auto PropagateUsedDevices = [=]()
	{
		for (u32 index = 0; index < 4; index++)
		{
			const QString cur_item = mics_combo[index]->currentText();
			QStringList cur_list = xemu_settings->m_microphone_creator.microphones_list;
			for (u32 subindex = 0; subindex < 4; subindex++)
			{
				if (subindex != index && mics_combo[subindex]->currentText() != xemu_settings->m_microphone_creator.mic_none)
					cur_list.removeOne(mics_combo[subindex]->currentText());
			}
			mics_combo[index]->blockSignals(true);
			mics_combo[index]->clear();
			mics_combo[index]->addItems(cur_list);
			mics_combo[index]->setCurrentText(cur_item);
			mics_combo[index]->blockSignals(false);
		}
		ChangeMicrophoneType(ui->microphoneBox->currentText());
	};

	auto ChangeMicrophoneDevice = [=](u32 next_index, QString text)
	{
		xemu_settings->SetSetting(emu_settings::MicrophoneDevices, xemu_settings->m_microphone_creator.SetDevice(next_index, text));
		if (next_index < 4 && text == xemu_settings->m_microphone_creator.mic_none)
			mics_combo[next_index]->setCurrentText(xemu_settings->m_microphone_creator.mic_none);
		PropagateUsedDevices();
	};

	// Comboboxes

	xemu_settings->EnhanceComboBox(ui->audioOutBox, emu_settings::AudioRenderer);
#ifdef WIN32
	SubscribeTooltip(ui->gb_audio_out, tooltips.settings.audio_out);
#else
	SubscribeTooltip(ui->gb_audio_out, tooltips.settings.audio_out_linux);
#endif
	// Change displayed backend names
	ui->audioOutBox->setItemText(ui->renderBox->findData("Null"), tr("Disable Audio Output"));
	connect(ui->audioOutBox, &QComboBox::currentTextChanged, EnableBuffering);

	// Microphone Comboboxes
	mics_combo[0] = ui->microphone1Box;
	mics_combo[1] = ui->microphone2Box;
	mics_combo[2] = ui->microphone3Box;
	mics_combo[3] = ui->microphone4Box;
	connect(mics_combo[0], &QComboBox::currentTextChanged, [=](const QString& text) { ChangeMicrophoneDevice(1, text); });
	connect(mics_combo[1], &QComboBox::currentTextChanged, [=](const QString& text) { ChangeMicrophoneDevice(2, text); });
	connect(mics_combo[2], &QComboBox::currentTextChanged, [=](const QString& text) { ChangeMicrophoneDevice(3, text); });
	connect(mics_combo[3], &QComboBox::currentTextChanged, [=](const QString& text) { ChangeMicrophoneDevice(4, text); });
	xemu_settings->m_microphone_creator.RefreshList();
	PropagateUsedDevices(); // Fills comboboxes list

	xemu_settings->m_microphone_creator.ParseDevices(xemu_settings->GetSetting(emu_settings::MicrophoneDevices));

	for (s32 index = 3; index >= 0; index--)
	{
		if (xemu_settings->m_microphone_creator.sel_list[index].empty() || mics_combo[index]->findText(qstr(xemu_settings->m_microphone_creator.sel_list[index])) == -1)
		{
			mics_combo[index]->setCurrentText(xemu_settings->m_microphone_creator.mic_none);
			ChangeMicrophoneDevice(index+1, xemu_settings->m_microphone_creator.mic_none); // Ensures the value is set in config
		}
		else
			mics_combo[index]->setCurrentText(qstr(xemu_settings->m_microphone_creator.sel_list[index]));
	}

	xemu_settings->EnhanceComboBox(ui->microphoneBox, emu_settings::MicrophoneType);
	ui->microphoneBox->setItemText(ui->microphoneBox->findData("Null"), tr("Disabled"));
	SubscribeTooltip(ui->microphoneBox, tooltips.settings.microphone);
	connect(ui->microphoneBox, &QComboBox::currentTextChanged, ChangeMicrophoneType);
	PropagateUsedDevices(); // Enables/Disables comboboxes and checks values from config for sanity

	// Checkboxes

	xemu_settings->EnhanceCheckBox(ui->audioDump, emu_settings::DumpToFile);
	SubscribeTooltip(ui->audioDump, tooltips.settings.audio_dump);

	xemu_settings->EnhanceCheckBox(ui->convert, emu_settings::ConvertTo16Bit);
	SubscribeTooltip(ui->convert, tooltips.settings.convert);

	xemu_settings->EnhanceCheckBox(ui->downmix, emu_settings::DownmixStereo);
	SubscribeTooltip(ui->downmix, tooltips.settings.downmix);

	xemu_settings->EnhanceCheckBox(ui->enableBuffering, emu_settings::EnableBuffering);
	SubscribeTooltip(ui->enableBuffering, tooltips.settings.enable_buffering);
	connect(ui->enableBuffering, &QCheckBox::clicked, EnableBufferingOptions);

	xemu_settings->EnhanceCheckBox(ui->enableTimeStretching, emu_settings::EnableTimeStretching);
	SubscribeTooltip(ui->enableTimeStretching, tooltips.settings.enable_time_stretching);
	connect(ui->enableTimeStretching, &QCheckBox::clicked, EnableTimeStretchingOptions);

	EnableBuffering(ui->audioOutBox->currentText());

	// Sliders

	EnhanceSlider(emu_settings::MasterVolume, ui->masterVolume, ui->masterVolumeLabel, tr("Master: %0 %"));
	SubscribeTooltip(ui->master_volume, tooltips.settings.master_volume);

	EnhanceSlider(emu_settings::AudioBufferDuration, ui->audioBufferDuration, ui->audioBufferDurationLabel, tr("Audio Buffer Duration: %0 ms"));
	SubscribeTooltip(ui->audio_buffer_duration, tooltips.settings.audio_buffer_duration);

	EnhanceSlider(emu_settings::TimeStretchingThreshold, ui->timeStretchingThreshold, ui->timeStretchingThresholdLabel, tr("Time Stretching Threshold: %0 %"));
	SubscribeTooltip(ui->time_stretching_threshold, tooltips.settings.time_stretching_threshold);

	//    _____       __   ____    _______    _
	//   |_   _|     / /  / __ \  |__   __|  | |
	//     | |      / /  | |  | |    | | __ _| |__
	//     | |     / /   | |  | |    | |/ _` | '_ \
	//    _| |_   / /    | |__| |    | | (_| | |_) |
	//   |_____| /_/      \____/     |_|\__,_|_.__/

	// Comboboxes

	xemu_settings->EnhanceComboBox(ui->keyboardHandlerBox, emu_settings::KeyboardHandler);
	SubscribeTooltip(ui->gb_keyboard_handler, tooltips.settings.keyboard_handler);

	xemu_settings->EnhanceComboBox(ui->mouseHandlerBox, emu_settings::MouseHandler);
	SubscribeTooltip(ui->gb_mouse_handler, tooltips.settings.mouse_handler);

	xemu_settings->EnhanceComboBox(ui->cameraTypeBox, emu_settings::CameraType);
	SubscribeTooltip(ui->gb_camera_type, tooltips.settings.camera_type);

	xemu_settings->EnhanceComboBox(ui->cameraBox, emu_settings::Camera);
	SubscribeTooltip(ui->gb_camera_setting, tooltips.settings.camera);

	xemu_settings->EnhanceComboBox(ui->moveBox, emu_settings::Move);
	SubscribeTooltip(ui->gb_move_handler, tooltips.settings.move);

	//     _____           _                   _______    _
	//    / ____|         | |                 |__   __|  | |
	//   | (___  _   _ ___| |_ ___ _ __ ___      | | __ _| |__
	//    \___ \| | | / __| __/ _ \ '_ ` _ \     | |/ _` | '_ \
	//    ____) | |_| \__ \ ||  __/ | | | | |    | | (_| | |_) |
	//   |_____/ \__, |___/\__\___|_| |_| |_|    |_|\__,_|_.__/
	//            __/ |
	//           |___/

	// Comboboxes

	xemu_settings->EnhanceComboBox(ui->sysLangBox, emu_settings::Language, false, false, 0, true);
	SubscribeTooltip(ui->gb_sysLang, tooltips.settings.system_language);

	xemu_settings->EnhanceComboBox(ui->keyboardType, emu_settings::KeyboardType, false, false, 0, true);
	SubscribeTooltip(ui->gb_keyboardType, tooltips.settings.keyboard_type);

	// Checkboxes

	xemu_settings->EnhanceCheckBox(ui->enableHostRoot, emu_settings::EnableHostRoot);
	SubscribeTooltip(ui->enableHostRoot, tooltips.settings.enable_host_root);

	xemu_settings->EnhanceCheckBox(ui->enableCacheClearing, emu_settings::LimitCacheSize);
	SubscribeTooltip(ui->gb_DiskCacheClearing, tooltips.settings.limit_cache_size);
	connect(ui->enableCacheClearing, &QCheckBox::stateChanged, ui->maximumCacheSize, &QSlider::setEnabled);

	// Sliders

	EnhanceSlider(emu_settings::MaximumCacheSize, ui->maximumCacheSize, ui->maximumCacheSizeLabel, tr("Maximum size: %0 MB"));
	ui->maximumCacheSize->setEnabled(ui->enableCacheClearing->isChecked());

	// Radio Buttons

	SubscribeTooltip(ui->gb_enterButtonAssignment, tooltips.settings.enter_button_assignment);

	// creating this in ui file keeps scrambling the order...
	QButtonGroup *enterButtonAssignmentBG = new QButtonGroup(this);
	enterButtonAssignmentBG->addButton(ui->enterButtonAssignCircle, 0);
	enterButtonAssignmentBG->addButton(ui->enterButtonAssignCross, 1);

	{ // EnterButtonAssignment options
		QString assigned_button = qstr(xemu_settings->GetSetting(emu_settings::EnterButtonAssignment));
		QStringList assignable_buttons = xemu_settings->GetSettingOptions(emu_settings::EnterButtonAssignment);

		for (int i = 0; i < assignable_buttons.count(); i++)
		{
			enterButtonAssignmentBG->button(i)->setText(assignable_buttons[i]);

			if (assignable_buttons[i] == assigned_button)
			{
				enterButtonAssignmentBG->button(i)->setChecked(true);
			}

			connect(enterButtonAssignmentBG->button(i), &QAbstractButton::clicked, [=]()
			{
				xemu_settings->SetSetting(emu_settings::EnterButtonAssignment, sstr(assignable_buttons[i]));
			});
		}
	}

	//    _   _      _                      _      _______    _
	//   | \ | |    | |                    | |    |__   __|  | |
	//   |  \| | ___| |___      _____  _ __| | __    | | __ _| |__
	//   | . ` |/ _ \ __\ \ /\ / / _ \| '__| |/ /    | |/ _` | '_ \
	//   | |\  |  __/ |_ \ V  V / (_) | |  |   <     | | (_| | |_) |
	//   |_| \_|\___|\__| \_/\_/ \___/|_|  |_|\_\    |_|\__,_|_.__/

	// Comboboxes

	xemu_settings->EnhanceComboBox(ui->netStatusBox, emu_settings::ConnectionStatus);
	SubscribeTooltip(ui->gb_network_status, tooltips.settings.net_status);


	//                _                               _   _______    _
	//       /\      | |                             | | |__   __|  | |
	//      /  \   __| |_   ____ _ _ __   ___ ___  __| |    | | __ _| |__
	//     / /\ \ / _` \ \ / / _` | '_ \ / __/ _ \/ _` |    | |/ _` | '_ \
	//    / ____ \ (_| |\ V / (_| | | | | (_|  __/ (_| |    | | (_| | |_) |
	//   /_/    \_\__,_| \_/ \__,_|_| |_|\___\___|\__,_|    |_|\__,_|_.__/


	// Checkboxes

	xemu_settings->EnhanceCheckBox(ui->debugConsoleMode, emu_settings::DebugConsoleMode);
	SubscribeTooltip(ui->debugConsoleMode, tooltips.settings.debug_console_mode);

	xemu_settings->EnhanceCheckBox(ui->silenceAllLogs, emu_settings::SilenceAllLogs);
	SubscribeTooltip(ui->silenceAllLogs, tooltips.settings.silence_all_logs);

	xemu_settings->EnhanceCheckBox(ui->readColor, emu_settings::ReadColorBuffers);
	SubscribeTooltip(ui->readColor, tooltips.settings.read_color);

	xemu_settings->EnhanceCheckBox(ui->readDepth, emu_settings::ReadDepthBuffer);
	SubscribeTooltip(ui->readDepth, tooltips.settings.read_depth);

	xemu_settings->EnhanceCheckBox(ui->dumpDepth, emu_settings::WriteDepthBuffer);
	SubscribeTooltip(ui->dumpDepth, tooltips.settings.dump_depth);

	xemu_settings->EnhanceCheckBox(ui->disableOnDiskShaderCache, emu_settings::DisableOnDiskShaderCache);
	SubscribeTooltip(ui->disableOnDiskShaderCache, tooltips.settings.disable_on_disk_shader_cache);

	xemu_settings->EnhanceCheckBox(ui->relaxedZCULL, emu_settings::RelaxedZCULL);
	SubscribeTooltip(ui->relaxedZCULL, tooltips.settings.relaxed_zcull);

	// Comboboxes

	xemu_settings->EnhanceComboBox(ui->maxSPURSThreads, emu_settings::MaxSPURSThreads, true);
	ui->maxSPURSThreads->setItemText(ui->maxSPURSThreads->findData("6"), tr("Unlimited (Default)"));
	SubscribeTooltip(ui->gb_max_spurs_threads, tooltips.settings.max_spurs_threads);

	xemu_settings->EnhanceComboBox(ui->sleepTimersAccuracy, emu_settings::SleepTimersAccuracy);
	SubscribeTooltip(ui->gb_sleep_timers_accuracy, tooltips.settings.sleep_timers_accuracy);

	// Sliders

	EnhanceSlider(emu_settings::DriverWakeUpDelay, ui->wakeupDelay, ui->wakeupText, tr(reinterpret_cast<const char*>(u8"%0 µs")));
	SnapSlider(ui->wakeupDelay, 200);
	ui->wakeupDelay->setMaximum(7000); // Very large values must be entered with config.yml changes
	ui->wakeupDelay->setPageStep(200);
	int wakeupDef = stoi(xemu_settings->GetSettingDefault(emu_settings::DriverWakeUpDelay));
	connect(ui->wakeupReset, &QAbstractButton::clicked, [=]()
	{
		ui->wakeupDelay->setValue(wakeupDef);
	});

	EnhanceSlider(emu_settings::VBlankRate, ui->vblank, ui->vblankText, tr("%0 Hz"));
	SnapSlider(ui->vblank, 30);
	ui->vblank->setPageStep(60);
	int vblankDef = stoi(xemu_settings->GetSettingDefault(emu_settings::VBlankRate));
	connect(ui->vblankReset, &QAbstractButton::clicked, [=]()
	{
		ui->vblank->setValue(vblankDef);
	});

	EnhanceSlider(emu_settings::ClocksScale, ui->clockScale, ui->clockScaleText, tr("%0 %"));
	SnapSlider(ui->clockScale, 10);
	ui->clockScale->setPageStep(50);
	int clocksScaleDef = stoi(xemu_settings->GetSettingDefault(emu_settings::ResolutionScale));
	connect(ui->clockScaleReset, &QAbstractButton::clicked, [=]()
	{
		ui->clockScale->setValue(clocksScaleDef);
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

	// lib options tool tips
	SubscribeTooltip(ui->lib_manu, tooltips.settings.libraries_manual);
	SubscribeTooltip(ui->lib_both, tooltips.settings.libraries_both);
	SubscribeTooltip(ui->lib_lv2,  tooltips.settings.libraries_liblv2);
	SubscribeTooltip(ui->lib_lv2b, tooltips.settings.libraries_liblv2both);
	SubscribeTooltip(ui->lib_lv2l, tooltips.settings.libraries_liblv2list);

	// creating this in ui file keeps scrambling the order...
	QButtonGroup *libModeBG = new QButtonGroup(this);
	libModeBG->addButton(ui->lib_manu, static_cast<int>(lib_loading_type::manual));
	libModeBG->addButton(ui->lib_both, static_cast<int>(lib_loading_type::hybrid));
	libModeBG->addButton(ui->lib_lv2,  static_cast<int>(lib_loading_type::liblv2only));
	libModeBG->addButton(ui->lib_lv2b, static_cast<int>(lib_loading_type::liblv2both));
	libModeBG->addButton(ui->lib_lv2l, static_cast<int>(lib_loading_type::liblv2list));

	{// Handle lib loading options
		QString selectedLib = qstr(xemu_settings->GetSetting(emu_settings::LibLoadOptions));
		QStringList libmode_list = xemu_settings->GetSettingOptions(emu_settings::LibLoadOptions);

		for (int i = 0; i < libmode_list.count(); i++)
		{
			libModeBG->button(i)->setText(libmode_list[i]);

			if (libmode_list[i] == selectedLib)
			{
				libModeBG->button(i)->setChecked(true);
			}

			connect(libModeBG->button(i), &QAbstractButton::clicked, [=]()
			{
				xemu_settings->SetSetting(emu_settings::LibLoadOptions, sstr(libmode_list[i]));
			});
		}
	}

	// Sort string vector alphabetically
	static const auto sort_string_vector = [](std::vector<std::string>& vec)
	{
		std::sort(vec.begin(), vec.end(), [](const std::string &str1, const std::string &str2) { return str1 < str2; });
	};

	std::vector<std::string> loadedLibs = xemu_settings->GetLoadedLibraries();

	sort_string_vector(loadedLibs);

	for (const auto& lib : loadedLibs)
	{
		QListWidgetItem* item = new QListWidgetItem(qstr(lib), ui->lleList);
		item->setFlags(item->flags() | Qt::ItemIsUserCheckable); // set checkable flag
		item->setCheckState(Qt::Checked); // AND initialize check state
		ui->lleList->addItem(item);
	}

	const std::string lle_dir = g_cfg.vfs.get_dev_flash() + "sys/external/";

	std::unordered_set<std::string> set(loadedLibs.begin(), loadedLibs.end());
	std::vector<std::string> lle_module_list_unselected;

	for (const auto& prxf : fs::dir(lle_dir))
	{
		// List found unselected modules
		if (prxf.is_directory || (prxf.name.substr(std::max<size_t>(size_t(3), prxf.name.length()) - 4)) != "sprx")
		{
			continue;
		}
		if (verify_npdrm_self_headers(fs::file(lle_dir + prxf.name)) && !set.count(prxf.name))
		{
			lle_module_list_unselected.push_back(prxf.name);
		}
	}

	sort_string_vector(lle_module_list_unselected);

	for (const auto& lib : lle_module_list_unselected)
	{
		QListWidgetItem* item = new QListWidgetItem(qstr(lib), ui->lleList);
		item->setFlags(item->flags() | Qt::ItemIsUserCheckable); // set checkable flag
		item->setCheckState(Qt::Unchecked); // AND initialize check state
		ui->lleList->addItem(item);
	}

	ui->searchBox->setPlaceholderText(tr("Search libraries"));

	auto l_OnLibButtonClicked = [=](int ind)
	{
		if (ind != static_cast<int>(lib_loading_type::liblv2only))
		{
			ui->searchBox->setEnabled(true);
			ui->lleList->setEnabled(true);
		}
		else
		{
			ui->searchBox->setEnabled(false);
			ui->lleList->setEnabled(false);
		}
	};

	auto l_OnSearchBoxTextChanged = [=](QString text)
	{
		QString searchTerm = text.toLower();
		std::vector<QListWidgetItem*> items;

		// duplicate current items, we need clones to preserve checkstates
		for (int i = 0; i < ui->lleList->count(); i++)
		{
			items.push_back(ui->lleList->item(i)->clone());
		}

		// sort items: checked items first then alphabetical order
		std::sort(items.begin(), items.end(), [](QListWidgetItem *i1, QListWidgetItem *i2)
		{
			return (i1->checkState() != i2->checkState()) ? (i1->checkState() > i2->checkState()) : (i1->text() < i2->text());
		});

		// refill library list
		ui->lleList->clear();

		for (uint i = 0; i < items.size(); i++)
		{
			ui->lleList->addItem(items[i]);

			// only show items filtered for search text
			ui->lleList->setRowHidden(i, !items[i]->text().contains(searchTerm));
		}
	};

	// Events
	connect(libModeBG, static_cast<void(QButtonGroup::*)(int)>(&QButtonGroup::buttonClicked), l_OnLibButtonClicked);
	connect(ui->searchBox, &QLineEdit::textChanged, l_OnSearchBoxTextChanged);

	// enable multiselection (there must be a better way)
	connect(ui->lleList, &QListWidget::itemChanged, [&](QListWidgetItem* item)
	{
		for (auto cb : ui->lleList->selectedItems())
		{
			cb->setCheckState(item->checkState());
		}
	});

	int buttid = libModeBG->checkedId();
	if (buttid != -1)
	{
		l_OnLibButtonClicked(buttid);
	}

	//    ______                 _       _               _______    _
	//   |  ____|               | |     | |             |__   __|  | |
	//   | |__   _ __ ___  _   _| | __ _| |_ ___  _ __     | | __ _| |__
	//   |  __| | '_ ` _ \| | | | |/ _` | __/ _ \| '__|    | |/ _` | '_ \
	//   | |____| | | | | | |_| | | (_| | || (_) | |       | | (_| | |_) |
	//   |______|_| |_| |_|\__,_|_|\__,_|\__\___/|_|       |_|\__,_|_.__/

	// Comboboxes

	xemu_settings->EnhanceComboBox(ui->maxLLVMThreads, emu_settings::MaxLLVMThreads, true, true, std::thread::hardware_concurrency());
	SubscribeTooltip(ui->gb_max_llvm, tooltips.settings.max_llvm_threads);
	ui->maxLLVMThreads->setItemText(ui->maxLLVMThreads->findData("0"), tr("All (%1)").arg(std::thread::hardware_concurrency()));

	xemu_settings->EnhanceComboBox(ui->perfOverlayDetailLevel, emu_settings::PerfOverlayDetailLevel);
	SubscribeTooltip(ui->perf_overlay_detail_level, tooltips.settings.perf_overlay_detail_level);

	xemu_settings->EnhanceComboBox(ui->perfOverlayPosition, emu_settings::PerfOverlayPosition);
	SubscribeTooltip(ui->perf_overlay_position, tooltips.settings.perf_overlay_position);

	// Checkboxes

	xemu_settings->EnhanceCheckBox(ui->exitOnStop, emu_settings::ExitRPCS3OnFinish);
	SubscribeTooltip(ui->exitOnStop, tooltips.settings.exit_on_stop);

	xemu_settings->EnhanceCheckBox(ui->alwaysStart, emu_settings::StartOnBoot);
	SubscribeTooltip(ui->alwaysStart, tooltips.settings.start_on_boot);

	xemu_settings->EnhanceCheckBox(ui->startGameFullscreen, emu_settings::StartGameFullscreen);
	SubscribeTooltip(ui->startGameFullscreen, tooltips.settings.start_game_fullscreen);

	xemu_settings->EnhanceCheckBox(ui->preventDisplaySleep, emu_settings::PreventDisplaySleep);
	SubscribeTooltip(ui->preventDisplaySleep, tooltips.settings.prevent_display_sleep);
	ui->preventDisplaySleep->setEnabled(display_sleep_control_supported());

	xemu_settings->EnhanceCheckBox(ui->showFPSInTitle, emu_settings::ShowFPSInTitle);
	SubscribeTooltip(ui->showFPSInTitle, tooltips.settings.show_fps_in_title);

	xemu_settings->EnhanceCheckBox(ui->showTrophyPopups, emu_settings::ShowTrophyPopups);
	SubscribeTooltip(ui->showTrophyPopups, tooltips.settings.show_trophy_popups);

	xemu_settings->EnhanceCheckBox(ui->useNativeInterface, emu_settings::UseNativeInterface);
	SubscribeTooltip(ui->useNativeInterface, tooltips.settings.use_native_interface);

	xemu_settings->EnhanceCheckBox(ui->showShaderCompilationHint, emu_settings::ShowShaderCompilationHint);
	SubscribeTooltip(ui->showShaderCompilationHint, tooltips.settings.show_shader_compilation_hint);

	xemu_settings->EnhanceCheckBox(ui->perfOverlayCenterX, emu_settings::PerfOverlayCenterX);
	SubscribeTooltip(ui->perfOverlayCenterX, tooltips.settings.perf_overlay_center_x);
	connect(ui->perfOverlayCenterX, &QCheckBox::clicked, [this](bool checked)
	{
		ui->perfOverlayMarginX->setEnabled(!checked);
	});
	ui->perfOverlayMarginX->setEnabled(!ui->perfOverlayCenterX->isChecked());

	xemu_settings->EnhanceCheckBox(ui->perfOverlayCenterY, emu_settings::PerfOverlayCenterY);
	SubscribeTooltip(ui->perfOverlayCenterY, tooltips.settings.perf_overlay_center_y);
	connect(ui->perfOverlayCenterY, &QCheckBox::clicked, [this](bool checked)
	{
		ui->perfOverlayMarginY->setEnabled(!checked);
	});
	ui->perfOverlayMarginY->setEnabled(!ui->perfOverlayCenterY->isChecked());

	xemu_settings->EnhanceCheckBox(ui->perfOverlayFramerateGraphEnabled, emu_settings::PerfOverlayFramerateGraphEnabled);
	SubscribeTooltip(ui->perfOverlayFramerateGraphEnabled, tooltips.settings.perf_overlay_framerate_graph_enabled);

	xemu_settings->EnhanceCheckBox(ui->perfOverlayFrametimeGraphEnabled, emu_settings::PerfOverlayFrametimeGraphEnabled);
	SubscribeTooltip(ui->perfOverlayFrametimeGraphEnabled, tooltips.settings.perf_overlay_frametime_graph_enabled);

	xemu_settings->EnhanceCheckBox(ui->perfOverlayEnabled, emu_settings::PerfOverlayEnabled);
	SubscribeTooltip(ui->perfOverlayEnabled, tooltips.settings.perf_overlay_enabled);
	auto EnablePerfOverlayOptions = [this](bool enabled)
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
	EnablePerfOverlayOptions(ui->perfOverlayEnabled->isChecked());
	connect(ui->perfOverlayEnabled, &QCheckBox::clicked, EnablePerfOverlayOptions);

	xemu_settings->EnhanceCheckBox(ui->shaderLoadBgEnabled, emu_settings::ShaderLoadBgEnabled);
	SubscribeTooltip(ui->shaderLoadBgEnabled, tooltips.settings.shader_load_bg_enabled);
	auto EnableShaderLoaderOptions = [this](bool enabled)
	{
		ui->label_shaderLoadBgDarkening->setEnabled(enabled);
		ui->label_shaderLoadBgBlur->setEnabled(enabled);
		ui->shaderLoadBgDarkening->setEnabled(enabled);
		ui->shaderLoadBgBlur->setEnabled(enabled);
	};
	EnableShaderLoaderOptions(ui->shaderLoadBgEnabled->isChecked());
	connect(ui->shaderLoadBgEnabled, &QCheckBox::clicked, EnableShaderLoaderOptions);

	// Sliders

	EnhanceSlider(emu_settings::PerfOverlayUpdateInterval, ui->perfOverlayUpdateInterval, ui->label_update_interval, tr("Update Interval: %0 ms"));
	SubscribeTooltip(ui->perf_overlay_update_interval, tooltips.settings.perf_overlay_update_interval);

	EnhanceSlider(emu_settings::PerfOverlayFontSize, ui->perfOverlayFontSize, ui->label_font_size, tr("Font Size: %0 px"));
	SubscribeTooltip(ui->perf_overlay_font_size, tooltips.settings.perf_overlay_font_size);

	EnhanceSlider(emu_settings::PerfOverlayOpacity, ui->perfOverlayOpacity, ui->label_opacity, tr("Opacity: %0 %"));
	SubscribeTooltip(ui->perf_overlay_opacity, tooltips.settings.perf_overlay_opacity);

	EnhanceSlider(emu_settings::ShaderLoadBgDarkening, ui->shaderLoadBgDarkening, ui->label_shaderLoadBgDarkening, tr("Background darkening: %0 %"));
	SubscribeTooltip(ui->shaderLoadBgDarkening, tooltips.settings.shader_load_bg_darkening);

	EnhanceSlider(emu_settings::ShaderLoadBgBlur, ui->shaderLoadBgBlur, ui->label_shaderLoadBgBlur, tr("Background blur: %0 %"));
	SubscribeTooltip(ui->shaderLoadBgBlur, tooltips.settings.shader_load_bg_blur);

	// SpinBoxes

	xemu_settings->EnhanceSpinBox(ui->perfOverlayMarginX, emu_settings::PerfOverlayMarginX, "", tr("px"));
	SubscribeTooltip(ui->perfOverlayMarginX, tooltips.settings.perf_overlay_margin_x);

	xemu_settings->EnhanceSpinBox(ui->perfOverlayMarginY, emu_settings::PerfOverlayMarginY, "", tr("px"));
	SubscribeTooltip(ui->perfOverlayMarginY, tooltips.settings.perf_overlay_margin_y);

	// Global settings (gui_settings)
	if (!game)
	{
		SubscribeTooltip(ui->gs_resizeOnBoot, tooltips.settings.resize_on_boot);

		SubscribeTooltip(ui->gs_disableMouse, tooltips.settings.disable_mouse);

		ui->gs_disableMouse->setChecked(xgui_settings->GetValue(gui::gs_disableMouse).toBool());
		connect(ui->gs_disableMouse, &QCheckBox::clicked, [=](bool val)
		{
			xgui_settings->SetValue(gui::gs_disableMouse, val);
		});

		bool enableButtons = xgui_settings->GetValue(gui::gs_resize).toBool();
		ui->gs_resizeOnBoot->setChecked(enableButtons);
		ui->gs_width->setEnabled(enableButtons);
		ui->gs_height->setEnabled(enableButtons);

		QRect screen = QGuiApplication::primaryScreen()->geometry();
		int width = xgui_settings->GetValue(gui::gs_width).toInt();
		int height = xgui_settings->GetValue(gui::gs_height).toInt();
		ui->gs_width->setValue(std::min(width, screen.width()));
		ui->gs_height->setValue(std::min(height, screen.height()));

		connect(ui->gs_resizeOnBoot, &QCheckBox::clicked, [=](bool val)
		{
			xgui_settings->SetValue(gui::gs_resize, val);
			ui->gs_width->setEnabled(val);
			ui->gs_height->setEnabled(val);
		});
		connect(ui->gs_width, &QSpinBox::editingFinished, [=]()
		{
			ui->gs_width->setValue(std::min(ui->gs_width->value(), QGuiApplication::primaryScreen()->size().width()));
			xgui_settings->SetValue(gui::gs_width, ui->gs_width->value());
		});
		connect(ui->gs_height, &QSpinBox::editingFinished, [=]()
		{
			ui->gs_height->setValue(std::min(ui->gs_height->value(), QGuiApplication::primaryScreen()->size().height()));
			xgui_settings->SetValue(gui::gs_height, ui->gs_height->value());
		});
	}
	else
	{
		ui->gb_viewport->setEnabled(false);
		ui->gb_viewport->setVisible(false);
	}

	//     _____  _    _  _   _______    _
	//    / ____|| |  | || | |__   __|  | |
	//   | |  __|| |  | || |    | | __ _| |__
	//   | | |_ || |  | || |    | |/ _` | '_ \
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

		SubscribeTooltip(ui->cb_check_update_start, tooltips.settings.check_update_start);

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

		ui->spinbox_log_limit->setValue(xgui_settings->GetValue(gui::l_limit).toInt());
		connect(ui->spinbox_log_limit, &QSpinBox::editingFinished, [=]()
		{
			xgui_settings->SetValue(gui::l_limit, ui->spinbox_log_limit->value());
		});

		ui->spinbox_tty_limit->setValue(xgui_settings->GetValue(gui::l_limit_tty).toInt());
		connect(ui->spinbox_tty_limit, &QSpinBox::editingFinished, [=]()
		{
			xgui_settings->SetValue(gui::l_limit_tty, ui->spinbox_tty_limit->value());
		});

		// colorize preview icons
		auto addColoredIcon = [&](QPushButton *button, const QColor& color, const QIcon& icon = QIcon(), const QColor& iconColor = QColor())
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

		auto AddColoredIcons = [=]()
		{
			addColoredIcon(ui->pb_gl_icon_color, xgui_settings->GetValue(gui::gl_iconColor).value<QColor>());
			addColoredIcon(ui->pb_sd_icon_color, xgui_settings->GetValue(gui::sd_icon_color).value<QColor>());
			addColoredIcon(ui->pb_tr_icon_color, xgui_settings->GetValue(gui::tr_icon_color).value<QColor>());
		};
		AddColoredIcons();

		ui->cb_show_welcome->setChecked(xgui_settings->GetValue(gui::ib_show_welcome).toBool());
		ui->cb_show_exit_game->setChecked(xgui_settings->GetValue(gui::ib_confirm_exit).toBool());
		ui->cb_show_boot_game->setChecked(xgui_settings->GetValue(gui::ib_confirm_boot).toBool());
		ui->cb_show_pkg_install->setChecked(xgui_settings->GetValue(gui::ib_pkg_success).toBool());
		ui->cb_show_pup_install->setChecked(xgui_settings->GetValue(gui::ib_pup_success).toBool());

		ui->cb_check_update_start->setChecked(xgui_settings->GetValue(gui::m_check_upd_start).toBool());

		bool enableUIColors = xgui_settings->GetValue(gui::m_enableUIColors).toBool();
		ui->cb_custom_colors->setChecked(enableUIColors);
		ui->pb_gl_icon_color->setEnabled(enableUIColors);
		ui->pb_sd_icon_color->setEnabled(enableUIColors);
		ui->pb_tr_icon_color->setEnabled(enableUIColors);

		auto ApplyGuiOptions = [&](bool reset = false)
		{
			if (reset)
			{
				m_currentConfig = gui::Default;
				m_currentStylesheet = gui::Default;
				ui->combo_configs->setCurrentIndex(0);
				ui->combo_stylesheets->setCurrentIndex(0);
			}
			// Only attempt to load a config if changes occurred.
			if (m_currentConfig != ui->combo_configs->currentText())
			{
				OnApplyConfig();
			}
			if (m_currentStylesheet != xgui_settings->GetValue(gui::m_currentStylesheet).toString())
			{
				OnApplyStylesheet();
			}
		};

		connect(ui->buttonBox, &QDialogButtonBox::accepted, [=]()
		{
			ApplyGuiOptions();
		});

		connect(ui->pb_reset_default, &QAbstractButton::clicked, [=]
		{
			if (QMessageBox::question(this, tr("Reset GUI to default?"), tr("This will include your stylesheet as well. Do you wish to proceed?"),
				QMessageBox::Yes | QMessageBox::No, QMessageBox::No) == QMessageBox::Yes)
			{
				ApplyGuiOptions(true);
				xgui_settings->Reset(true);
				xgui_settings->ChangeToConfig(gui::Default);
				Q_EMIT GuiSettingsSyncRequest(true);
				AddConfigs();
				AddStylesheets();
				AddColoredIcons();
			}
		});

		connect(ui->pb_backup_config, &QAbstractButton::clicked, this, &settings_dialog::OnBackupCurrentConfig);
		connect(ui->pb_apply_config, &QAbstractButton::clicked, this, &settings_dialog::OnApplyConfig);
		connect(ui->pb_apply_stylesheet, &QAbstractButton::clicked, this, &settings_dialog::OnApplyStylesheet);

		connect(ui->pb_open_folder, &QAbstractButton::clicked, [=]()
		{
			QDesktopServices::openUrl(xgui_settings->GetSettingsDir());
		});

		connect(ui->cb_show_welcome, &QCheckBox::clicked, [=](bool val)
		{
			xgui_settings->SetValue(gui::ib_show_welcome, val);
		});
		connect(ui->cb_show_exit_game, &QCheckBox::clicked, [=](bool val)
		{
			xgui_settings->SetValue(gui::ib_confirm_exit, val);
		});
		connect(ui->cb_show_boot_game, &QCheckBox::clicked, [=](bool val)
		{
			xgui_settings->SetValue(gui::ib_confirm_boot, val);
		});
		connect(ui->cb_show_pkg_install, &QCheckBox::clicked, [=](bool val)
		{
			xgui_settings->SetValue(gui::ib_pkg_success, val);
		});
		connect(ui->cb_show_pup_install, &QCheckBox::clicked, [=](bool val)
		{
			xgui_settings->SetValue(gui::ib_pup_success, val);
		});
		connect(ui->cb_check_update_start, &QCheckBox::clicked, [=](bool val)
		{
			xgui_settings->SetValue(gui::m_check_upd_start, val);
		});

		connect(ui->cb_custom_colors, &QCheckBox::clicked, [=](bool val)
		{
			xgui_settings->SetValue(gui::m_enableUIColors, val);
			ui->pb_gl_icon_color->setEnabled(val);
			ui->pb_sd_icon_color->setEnabled(val);
			ui->pb_tr_icon_color->setEnabled(val);
			Q_EMIT GuiRepaintRequest();
		});
		auto colorDialog = [&](const gui_save& color, const QString& title, QPushButton *button)
		{
			QColor oldColor = xgui_settings->GetValue(color).value<QColor>();
			QColorDialog dlg(oldColor, this);
			dlg.setWindowTitle(title);
			dlg.setOptions(QColorDialog::ShowAlphaChannel);
			for (int i = 0; i < dlg.customCount(); i++)
			{
				dlg.setCustomColor(i, xgui_settings->GetCustomColor(i));
			}
			if (dlg.exec() == QColorDialog::Accepted)
			{
				for (int i = 0; i < dlg.customCount(); i++)
				{
					xgui_settings->SetCustomColor(i, dlg.customColor(i));
				}
				xgui_settings->SetValue(color, dlg.selectedColor());
				button->setIcon(gui::utils::get_colorized_icon(button->icon(), oldColor, dlg.selectedColor(), true));
				Q_EMIT GuiRepaintRequest();
			}
		};

		connect(ui->pb_gl_icon_color, &QAbstractButton::clicked, [=]()
		{
			colorDialog(gui::gl_iconColor, tr("Choose gamelist icon color"), ui->pb_gl_icon_color);
		});
		connect(ui->pb_sd_icon_color, &QAbstractButton::clicked, [=]()
		{
			colorDialog(gui::sd_icon_color, tr("Choose save manager icon color"), ui->pb_sd_icon_color);
		});
		connect(ui->pb_tr_icon_color, &QAbstractButton::clicked, [=]()
		{
			colorDialog(gui::tr_icon_color, tr("Choose trophy manager icon color"), ui->pb_tr_icon_color);
		});

		AddConfigs();
		AddStylesheets();
	}

	//    _____       _                   _______    _
	//   |  __ \     | |                 |__   __|  | |
	//   | |  | | ___| |__  _   _  __ _     | | __ _| |__
	//   | |  | |/ _ \ '_ \| | | |/ _` |    | |/ _` | '_ \
	//   | |__| |  __/ |_) | |_| | (_| |    | | (_| | |_) |
	//   |_____/ \___|_.__/ \__,_|\__, |    |_|\__,_|_.__/
	//                             __/ |
	//                            |___/

	// Checkboxes: gpu debug options
	xemu_settings->EnhanceCheckBox(ui->glLegacyBuffers, emu_settings::LegacyBuffers);
	SubscribeTooltip(ui->glLegacyBuffers, tooltips.settings.gl_legacy_buffers);

	xemu_settings->EnhanceCheckBox(ui->forceHighpZ, emu_settings::ForceHighpZ);
	SubscribeTooltip(ui->forceHighpZ, tooltips.settings.force_high_pz);

	xemu_settings->EnhanceCheckBox(ui->debugOutput, emu_settings::DebugOutput);
	SubscribeTooltip(ui->debugOutput, tooltips.settings.debug_output);

	xemu_settings->EnhanceCheckBox(ui->debugOverlay, emu_settings::DebugOverlay);
	SubscribeTooltip(ui->debugOverlay, tooltips.settings.debug_overlay);

	xemu_settings->EnhanceCheckBox(ui->logProg, emu_settings::LogShaderPrograms);
	SubscribeTooltip(ui->logProg, tooltips.settings.log_shader_programs);

	xemu_settings->EnhanceCheckBox(ui->disableHwOcclusionQueries, emu_settings::DisableOcclusionQueries);
	SubscribeTooltip(ui->disableHwOcclusionQueries, tooltips.settings.disable_occlusion_queries);

	xemu_settings->EnhanceCheckBox(ui->forceCpuBlitEmulation, emu_settings::ForceCPUBlitEmulation);
	SubscribeTooltip(ui->forceCpuBlitEmulation, tooltips.settings.force_cpu_blit_emulation);

	xemu_settings->EnhanceCheckBox(ui->disableVulkanMemAllocator, emu_settings::DisableVulkanMemAllocator);
	SubscribeTooltip(ui->disableVulkanMemAllocator, tooltips.settings.disable_vulkan_mem_allocator);

	xemu_settings->EnhanceCheckBox(ui->disableFIFOReordering, emu_settings::DisableFIFOReordering);
	SubscribeTooltip(ui->disableFIFOReordering, tooltips.settings.disable_fifo_reordering);

	xemu_settings->EnhanceCheckBox(ui->strictTextureFlushing, emu_settings::StrictTextureFlushing);
	SubscribeTooltip(ui->strictTextureFlushing, tooltips.settings.strict_texture_flushing);

	xemu_settings->EnhanceCheckBox(ui->gpuTextureScaling, emu_settings::GPUTextureScaling);
	SubscribeTooltip(ui->gpuTextureScaling, tooltips.settings.gpu_texture_scaling);

	// Checkboxes: core debug options
	xemu_settings->EnhanceCheckBox(ui->ppuDebug, emu_settings::PPUDebug);
	SubscribeTooltip(ui->ppuDebug, tooltips.settings.ppu_debug);

	xemu_settings->EnhanceCheckBox(ui->spuDebug, emu_settings::SPUDebug);
	SubscribeTooltip(ui->spuDebug, tooltips.settings.spu_debug);

	xemu_settings->EnhanceCheckBox(ui->setDAZandFTZ, emu_settings::SetDAZandFTZ);
	SubscribeTooltip(ui->setDAZandFTZ, tooltips.settings.set_daz_and_ftz);

	xemu_settings->EnhanceCheckBox(ui->accurateGETLLAR, emu_settings::AccurateGETLLAR);
	SubscribeTooltip(ui->accurateGETLLAR, tooltips.settings.accurate_getllar);

	xemu_settings->EnhanceCheckBox(ui->accuratePUTLLUC, emu_settings::AccuratePUTLLUC);
	SubscribeTooltip(ui->accuratePUTLLUC, tooltips.settings.accurate_putlluc);

	xemu_settings->EnhanceCheckBox(ui->hookStFunc, emu_settings::HookStaticFuncs);
	SubscribeTooltip(ui->hookStFunc, tooltips.settings.hook_static_functions);

	// Layout fix for High Dpi
	layout()->setSizeConstraint(QLayout::SetFixedSize);
}

settings_dialog::~settings_dialog()
{
	delete ui;
}

void settings_dialog::EnhanceSlider(emu_settings::SettingsType settings_type, QSlider* slider, QLabel* label, const QString& label_text)
{
	xemu_settings->EnhanceSlider(slider, settings_type);

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
		m_currentSlider = slider;
	});

	connect(slider, &QSlider::sliderReleased, [this]()
	{
		m_currentSlider = nullptr;
	});

	connect(slider, &QSlider::valueChanged, [this, slider, interval](int value)
	{
		if (slider != m_currentSlider)
		{
			return;
		}
		slider->setValue(::rounded_div(value, interval) * interval);
	});
}

void settings_dialog::AddConfigs()
{
	ui->combo_configs->clear();

	ui->combo_configs->addItem(gui::Default);

	for (const QString& entry : xgui_settings->GetConfigEntries())
	{
		if (entry != gui::Default)
		{
			ui->combo_configs->addItem(entry);
		}
	}

	m_currentConfig = xgui_settings->GetValue(gui::m_currentConfig).toString();

	int index = ui->combo_configs->findText(m_currentConfig);
	if (index != -1)
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

	ui->combo_stylesheets->addItem(tr("None"), gui::None);
	ui->combo_stylesheets->addItem(tr("Default (Bright)"), gui::Default);

	for (const QString& entry : xgui_settings->GetStylesheetEntries())
	{
		if (entry != gui::Default)
		{
			ui->combo_stylesheets->addItem(entry, entry);
		}
	}

	m_currentStylesheet = xgui_settings->GetValue(gui::m_currentStylesheet).toString();

	int index = ui->combo_stylesheets->findData(m_currentStylesheet);
	if (index != -1)
	{
		ui->combo_stylesheets->setCurrentIndex(index);
	}
	else
	{
		cfg_log.warning("Trying to set an invalid stylesheets index: %d (%s)", index, sstr(m_currentStylesheet));
	}
}

void settings_dialog::OnBackupCurrentConfig()
{
	QInputDialog* dialog = new QInputDialog(this);
	dialog->setWindowTitle(tr("Choose a unique name"));
	dialog->setLabelText(tr("Configuration Name: "));
	dialog->resize(500, 100);

	while (dialog->exec() != QDialog::Rejected)
	{
		dialog->resize(500, 100);
		QString friendly_name = dialog->textValue();
		if (friendly_name == "")
		{
			QMessageBox::warning(this, tr("Error"), tr("Name cannot be empty"));
			continue;
		}
		if (friendly_name.contains("."))
		{
			QMessageBox::warning(this, tr("Error"), tr("Must choose a name with no '.'"));
			continue;
		}
		if (ui->combo_configs->findText(friendly_name) != -1)
		{
			QMessageBox::warning(this, tr("Error"), tr("Please choose a non-existing name"));
			continue;
		}
		Q_EMIT GuiSettingsSaveRequest();
		xgui_settings->SaveCurrentConfig(friendly_name);
		ui->combo_configs->addItem(friendly_name);
		ui->combo_configs->setCurrentText(friendly_name);
		m_currentConfig = friendly_name;
		break;
	}
}

void settings_dialog::OnApplyConfig()
{
	const QString new_config = ui->combo_configs->currentText();

	if (new_config == m_currentConfig)
	{
		return;
	}

	if (!xgui_settings->ChangeToConfig(new_config))
	{
		const int new_config_idx = ui->combo_configs->currentIndex();
		ui->combo_configs->setCurrentText(m_currentConfig);
		ui->combo_configs->removeItem(new_config_idx);
		return;
	}

	m_currentConfig = new_config;
	Q_EMIT GuiSettingsSyncRequest(true);
}

void settings_dialog::OnApplyStylesheet()
{
	m_currentStylesheet = ui->combo_stylesheets->currentData().toString();
	xgui_settings->SetValue(gui::m_currentStylesheet, m_currentStylesheet);
	Q_EMIT GuiStylesheetRequest(xgui_settings->GetCurrentStylesheetPath());
}

int settings_dialog::exec()
{
	// singleShot Hack to fix following bug:
	// If we use setCurrentIndex now we will miraculously see a resize of the dialog as soon as we
	// switch to the cpu tab after conjuring the settings_dialog with another tab opened first.
	// Weirdly enough this won't happen if we change the tab order so that anything else is at index 0.
	ui->tab_widget_settings->setCurrentIndex(0);
	QTimer::singleShot(0, [=]{ ui->tab_widget_settings->setCurrentIndex(m_tab_Index); });

	// Open a dialog if your config file contained invalid entries
	QTimer::singleShot(10, [this] { xemu_settings->OpenCorrectionDialog(this); });

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

	const int i = ui->tab_widget_settings->currentIndex();
	QLabel* label = m_description_labels[i].first;

	if (event->type() == QEvent::Enter)
	{
		label->setText(m_descriptions[object]);
		return QDialog::eventFilter(object, event);
	}

	QString description = m_description_labels[i].second;

	if (event->type() == QEvent::Leave)
	{
		label->setText(description);
	}

	return QDialog::eventFilter(object, event);
}

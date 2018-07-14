#include <QVBoxLayout>
#include <QButtonGroup>
#include <QPushButton>
#include <QMessageBox>
#include <QInputDialog>
#include <QDesktopServices>
#include <QUrl>
#include <QJsonObject>
#include <QJsonDocument>
#include <QColorDialog>
#include <QSpinBox>
#include <QApplication>
#include <QDesktopWidget>
#include <QTimer>

#include "qt_utils.h"
#include "settings_dialog.h"
#include "ui_settings_dialog.h"

#include "stdafx.h"
#include "Emu/System.h"
#include "Crypto/unself.h"
#include "Utilities/sysinfo.h"

#include <unordered_set>
#include <thread>

#ifdef WITH_DISCORD_RPC
#include "_discord_utils.h"
#endif

inline std::string sstr(const QString& _in) { return _in.toStdString(); }
inline std::string sstr(const QVariant& _in) { return sstr(_in.toString()); }

settings_dialog::settings_dialog(std::shared_ptr<gui_settings> guiSettings, std::shared_ptr<emu_settings> emuSettings, const int& tabIndex, QWidget *parent, const GameInfo* game)
	: QDialog(parent), xgui_settings(guiSettings), xemu_settings(emuSettings), ui(new Ui::settings_dialog), m_tab_Index(tabIndex)
{
	ui->setupUi(this);
	ui->cancelButton->setFocus();
	ui->tab_widget_settings->setUsesScrollButtons(false);
	ui->tab_widget_settings->tabBar()->setObjectName("tab_bar_settings");

	bool showDebugTab = xgui_settings->GetValue(gui::m_showDebugTab).toBool();
	xgui_settings->SetValue(gui::m_showDebugTab, showDebugTab);
	if (!showDebugTab)
	{
		ui->tab_widget_settings->removeTab(8);
	}
	if (game)
	{
		ui->tab_widget_settings->removeTab(7);
	}

	// Add description labels
	SubscribeDescription(ui->description_cpu);
	SubscribeDescription(ui->description_gpu);
	SubscribeDescription(ui->description_audio);
	SubscribeDescription(ui->description_io);
	SubscribeDescription(ui->description_system);
	SubscribeDescription(ui->description_network);
	SubscribeDescription(ui->description_emulator);
	if (!game)
	{
		SubscribeDescription(ui->description_gui);
	}
	SubscribeDescription(ui->description_debug);

	// read tooltips from json
	QFile json_file(":/Json/tooltips.json");
	json_file.open(QIODevice::ReadOnly | QIODevice::Text);
	QJsonObject json_obj = QJsonDocument::fromJson(json_file.readAll()).object();
	json_file.close();

	QJsonObject json_cpu     = json_obj.value("cpu").toObject();
	QJsonObject json_cpu_ppu = json_cpu.value("PPU").toObject();
	QJsonObject json_cpu_spu = json_cpu.value("SPU").toObject();
	QJsonObject json_cpu_cbs = json_cpu.value("checkboxes").toObject();
	QJsonObject json_cpu_cbo = json_cpu.value("comboboxes").toObject();
	QJsonObject json_cpu_lib = json_cpu.value("libraries").toObject();

	QJsonObject json_gpu      = json_obj.value("gpu").toObject();
	QJsonObject json_gpu_cbo  = json_gpu.value("comboboxes").toObject();
	QJsonObject json_gpu_main = json_gpu.value("main").toObject();
	QJsonObject json_gpu_slid = json_gpu.value("sliders").toObject();

	QJsonObject json_audio = json_obj.value("audio").toObject();
	QJsonObject json_input = json_obj.value("input").toObject();
	QJsonObject json_sys   = json_obj.value("system").toObject();
	QJsonObject json_net   = json_obj.value("network").toObject();

	QJsonObject json_emu         = json_obj.value("emulator").toObject();
	QJsonObject json_emu_misc    = json_emu.value("misc").toObject();
	QJsonObject json_emu_overlay = json_emu.value("overlay").toObject();

	QJsonObject json_gui = json_obj.value("gui").toObject();

	QJsonObject json_debug = json_obj.value("debug").toObject();

	if (game)
	{
		xemu_settings->LoadSettings("data/" + game->serial);
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
	connect(ui->okButton, &QAbstractButton::clicked, [=, use_discord_old = m_use_discord, discord_state_old = m_discord_state]
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

	connect(ui->cancelButton, &QAbstractButton::clicked, this, &QWidget::close);

	connect(ui->tab_widget_settings, &QTabWidget::currentChanged, [=]()
	{
		ui->cancelButton->setFocus();
	});

	//     _____ _____  _    _   _______    _
	//    / ____|  __ \| |  | | |__   __|  | |
	//   | |    | |__) | |  | |    | | __ _| |__
	//   | |    |  ___/| |  | |    | |/ _` | '_ \
	//   | |____| |    | |__| |    | | (_| | |_) |
	//    \_____|_|     \____/     |_|\__,_|_.__/

	// Checkboxes

	xemu_settings->EnhanceCheckBox(ui->spuCache, emu_settings::SPUCache);
	SubscribeTooltip(ui->spuCache, json_cpu_cbs["spuCache"].toString());

	xemu_settings->EnhanceCheckBox(ui->enableScheduler, emu_settings::EnableThreadScheduler);
	SubscribeTooltip(ui->enableScheduler, json_cpu_cbs["enableThreadScheduler"].toString());

	xemu_settings->EnhanceCheckBox(ui->lowerSPUThrPrio, emu_settings::LowerSPUThreadPrio);
	SubscribeTooltip(ui->lowerSPUThrPrio, json_cpu_cbs["lowerSPUThrPrio"].toString());

	xemu_settings->EnhanceCheckBox(ui->spuLoopDetection, emu_settings::SPULoopDetection);
	SubscribeTooltip(ui->spuLoopDetection, json_cpu_cbs["spuLoopDetection"].toString());

	// Comboboxes

	xemu_settings->EnhanceComboBox(ui->spuBlockSize, emu_settings::SPUBlockSize);
	SubscribeTooltip(ui->spuBlockSize, json_cpu_cbo["spuBlockSize"].toString());

	xemu_settings->EnhanceComboBox(ui->preferredSPUThreads, emu_settings::PreferredSPUThreads, true);
	SubscribeTooltip(ui->preferredSPUThreads, json_cpu_cbo["preferredSPUThreads"].toString());
	ui->preferredSPUThreads->setItemText(ui->preferredSPUThreads->findData("0"), tr("Auto"));

	if (utils::has_rtm())
	{
		xemu_settings->EnhanceComboBox(ui->enableTSX, emu_settings::EnableTSX);
		SubscribeTooltip(ui->enableTSX, json_cpu_cbo["enableTSX"].toString());

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
		SubscribeTooltip(ui->enableTSX, tr("Unfortunately your cpu model does not support this instruction set."));
	}

	// PPU tool tips
	SubscribeTooltip(ui->ppu_precise, json_cpu_ppu["precise"].toString());
	SubscribeTooltip(ui->ppu_fast, json_cpu_ppu["fast"].toString());
	SubscribeTooltip(ui->ppu_llvm, json_cpu_ppu["LLVM"].toString());

	QButtonGroup *ppuBG = new QButtonGroup(this);
	ppuBG->addButton(ui->ppu_precise, (int)ppu_decoder_type::precise);
	ppuBG->addButton(ui->ppu_fast,    (int)ppu_decoder_type::fast);
	ppuBG->addButton(ui->ppu_llvm,    (int)ppu_decoder_type::llvm);

	{ // PPU Stuff
		QString selectedPPU = qstr(xemu_settings->GetSetting(emu_settings::PPUDecoder));
		QStringList ppu_list = xemu_settings->GetSettingOptions(emu_settings::PPUDecoder);

		for (int i = 0; i < ppu_list.count(); i++)
		{
			if (ppu_list[i] == selectedPPU)
			{
				ppuBG->button(i)->setChecked(true);
			}

			connect(ppuBG->button(i), &QAbstractButton::pressed, [=]()
			{
				xemu_settings->SetSetting(emu_settings::PPUDecoder, sstr(ppu_list[i]));
			});
		}
	}

	// SPU tool tips
	SubscribeTooltip(ui->spu_precise, json_cpu_spu["precise"].toString());
	SubscribeTooltip(ui->spu_fast,    json_cpu_spu["fast"].toString());
	SubscribeTooltip(ui->spu_asmjit,  json_cpu_spu["ASMJIT"].toString());
	SubscribeTooltip(ui->spu_llvm,    json_cpu_spu["LLVM"].toString());

	QButtonGroup *spuBG = new QButtonGroup(this);
	spuBG->addButton(ui->spu_precise, (int)spu_decoder_type::precise);
	spuBG->addButton(ui->spu_fast,    (int)spu_decoder_type::fast);
	spuBG->addButton(ui->spu_asmjit,  (int)spu_decoder_type::asmjit);
	spuBG->addButton(ui->spu_llvm,    (int)spu_decoder_type::llvm);

	{ // Spu stuff
		QString selectedSPU = qstr(xemu_settings->GetSetting(emu_settings::SPUDecoder));
		QStringList spu_list = xemu_settings->GetSettingOptions(emu_settings::SPUDecoder);

		for (int i = 0; i < spu_list.count(); i++)
		{
			if (spu_list[i] == selectedSPU)
			{
				spuBG->button(i)->setChecked(true);
			}

			connect(spuBG->button(i), &QAbstractButton::pressed, [=]()
			{
				xemu_settings->SetSetting(emu_settings::SPUDecoder, sstr(spu_list[i]));
			});
		}
	}

#ifndef LLVM_AVAILABLE
	ui->ppu_llvm->setEnabled(false);
	ui->spu_llvm->setEnabled(false);
#endif

	// lib options tool tips
	SubscribeTooltip(ui->lib_auto, json_cpu_lib["auto"].toString());
	SubscribeTooltip(ui->lib_manu, json_cpu_lib["manual"].toString());
	SubscribeTooltip(ui->lib_both, json_cpu_lib["both"].toString());
	SubscribeTooltip(ui->lib_lv2,  json_cpu_lib["liblv2"].toString());

	// creating this in ui file keeps scrambling the order...
	QButtonGroup *libModeBG = new QButtonGroup(this);
	libModeBG->addButton(ui->lib_auto, (int)lib_loading_type::automatic);
	libModeBG->addButton(ui->lib_manu, (int)lib_loading_type::manual);
	libModeBG->addButton(ui->lib_both, (int)lib_loading_type::both);
	libModeBG->addButton(ui->lib_lv2,  (int)lib_loading_type::liblv2only);

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

			connect(libModeBG->button(i), &QAbstractButton::pressed, [=]()
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
			continue;
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
		if (ind == (int)lib_loading_type::manual || ind == (int)lib_loading_type::both)
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

	//     _____ _____  _    _   _______    _
	//    / ____|  __ \| |  | | |__   __|  | |
	//   | |  __| |__) | |  | |    | | __ _| |__
	//   | | |_ |  ___/| |  | |    | |/ _` | '_ \
	//   | |__| | |    | |__| |    | | (_| | |_) |
	//    \_____|_|     \____/     |_|\__,_|_.__/

	emu_settings::Render_Creator render_creator = xemu_settings.get()->m_render_creator;

	// Comboboxes
	xemu_settings->EnhanceComboBox(ui->renderBox, emu_settings::Renderer);
#ifdef WIN32
	SubscribeTooltip(ui->renderBox, json_gpu_cbo["renderBox"].toString());
	SubscribeTooltip(ui->graphicsAdapterBox, json_gpu_cbo["graphicsAdapterBox"].toString());
#else
	SubscribeTooltip(ui->renderBox, json_gpu_cbo["renderBox_Linux"].toString());
	SubscribeTooltip(ui->graphicsAdapterBox, json_gpu_cbo["graphicsAdapterBox_Linux"].toString());
#endif
	//Change D3D12 to D3D12[DO NOT USE]
	ui->renderBox->setItemText(ui->renderBox->findData("D3D12"), render_creator.name_D3D12);

	xemu_settings->EnhanceComboBox(ui->resBox, emu_settings::Resolution);
	ui->resBox->setItemText(ui->resBox->findData("1280x720"), tr("1280x720 (Recommended)"));
	SubscribeTooltip(ui->resBox, json_gpu_cbo["resBox"].toString());

	xemu_settings->EnhanceComboBox(ui->aspectBox, emu_settings::AspectRatio);
	SubscribeTooltip(ui->aspectBox, json_gpu_cbo["aspectBox"].toString());

	xemu_settings->EnhanceComboBox(ui->frameLimitBox, emu_settings::FrameLimit);
	SubscribeTooltip(ui->frameLimitBox, json_gpu_cbo["frameLimitBox"].toString());

	xemu_settings->EnhanceComboBox(ui->anisotropicFilterOverride, emu_settings::AnisotropicFilterOverride, true);
	SubscribeTooltip(ui->anisotropicFilterOverride, json_gpu_cbo["anisotropicFilterOverride"].toString());
	// only allow values 0,2,4,8,16
	for (int i = ui->anisotropicFilterOverride->count() - 1; i >= 0; i--)
	{
		switch (int val = ui->anisotropicFilterOverride->itemData(i).toInt())
		{
		case 0:
			ui->anisotropicFilterOverride->setItemText(i, tr("Automatic"));
			break;
		case 1:
			ui->anisotropicFilterOverride->setItemText(i, tr("Force Disabled"));
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
	SubscribeTooltip(ui->dumpColor, json_gpu_main["dumpColor"].toString());

	xemu_settings->EnhanceCheckBox(ui->vsync, emu_settings::VSync);
	SubscribeTooltip(ui->vsync, json_gpu_main["vsync"].toString());

	xemu_settings->EnhanceCheckBox(ui->gpuTextureScaling, emu_settings::GPUTextureScaling);
	SubscribeTooltip(ui->gpuTextureScaling, json_gpu_main["gpuTextureScaling"].toString());

	xemu_settings->EnhanceCheckBox(ui->stretchToDisplayArea, emu_settings::StretchToDisplayArea);
	SubscribeTooltip(ui->stretchToDisplayArea, json_gpu_main["stretchToDisplayArea"].toString());

	xemu_settings->EnhanceCheckBox(ui->disableVertexCache, emu_settings::DisableVertexCache);
	SubscribeTooltip(ui->disableVertexCache, json_gpu_main["disableVertexCache"].toString());

	xemu_settings->EnhanceCheckBox(ui->disableAsyncShaders, emu_settings::DisableAsyncShaderCompiler);
	SubscribeTooltip(ui->disableAsyncShaders, json_gpu_main["disableAsyncShaders"].toString());

	xemu_settings->EnhanceCheckBox(ui->scrictModeRendering, emu_settings::StrictRenderingMode);
	SubscribeTooltip(ui->scrictModeRendering, json_gpu_main["scrictModeRendering"].toString());
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

	xemu_settings->EnhanceSlider(ui->resolutionScale, emu_settings::ResolutionScale, true);
	SubscribeTooltip(ui->gb_resolutionScale, json_gpu_slid["resolutionScale"].toString());
	ui->gb_resolutionScale->setEnabled(!ui->scrictModeRendering->isChecked());
	// rename label texts to fit current state of Resolution Scale
	int resolutionScaleDef = stoi(xemu_settings->GetSettingDefault(emu_settings::ResolutionScale));
	auto ScaledResolution = [resolutionScaleDef](int percentage)
	{
		if (percentage == resolutionScaleDef) return QString(tr("100% (Default)"));
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

	xemu_settings->EnhanceSlider(ui->minimumScalableDimension, emu_settings::MinimumScalableDimension, true);
	SubscribeTooltip(ui->gb_minimumScalableDimension, json_gpu_slid["minimumScalableDimension"].toString());
	ui->gb_minimumScalableDimension->setEnabled(!ui->scrictModeRendering->isChecked());
	// rename label texts to fit current state of Minimum Scalable Dimension
	int minimumScalableDimensionDef = stoi(xemu_settings->GetSettingDefault(emu_settings::MinimumScalableDimension));
	auto MinScalableDimension = [minimumScalableDimensionDef](int dim)
	{
		if (dim == minimumScalableDimensionDef) return tr("%1x%1 (Default)").arg(dim);
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
		if (text.isEmpty()) return;

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
					LOG_WARNING(RSX, "%s adapter config empty: setting to default!", sstr(renderer.name));
				}
				else
				{
					LOG_WARNING(RSX, "Last used %s adapter not found: setting to default!", sstr(renderer.name));
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
		if (text.isEmpty()) return;

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

	// Comboboxes

	xemu_settings->EnhanceComboBox(ui->audioOutBox, emu_settings::AudioRenderer);
#ifdef WIN32
	SubscribeTooltip(ui->audioOutBox, json_audio["audioOutBox"].toString());
#else
	SubscribeTooltip(ui->audioOutBox, json_audio["audioOutBox_Linux"].toString());
#endif

	// Checkboxes

	xemu_settings->EnhanceCheckBox(ui->audioDump, emu_settings::DumpToFile);
	SubscribeTooltip(ui->audioDump, json_audio["audioDump"].toString());

	xemu_settings->EnhanceCheckBox(ui->convert, emu_settings::ConvertTo16Bit);
	SubscribeTooltip(ui->convert, json_audio["convert"].toString());

	xemu_settings->EnhanceCheckBox(ui->downmix, emu_settings::DownmixStereo);
	SubscribeTooltip(ui->downmix, json_audio["downmix"].toString());

	//    _____       __   ____    _______    _
	//   |_   _|     / /  / __ \  |__   __|  | |
	//     | |      / /  | |  | |    | | __ _| |__
	//     | |     / /   | |  | |    | |/ _` | '_ \
	//    _| |_   / /    | |__| |    | | (_| | |_) |
	//   |_____| /_/      \____/     |_|\__,_|_.__/

	// Comboboxes

	xemu_settings->EnhanceComboBox(ui->keyboardHandlerBox, emu_settings::KeyboardHandler);
	SubscribeTooltip(ui->keyboardHandlerBox, json_input["keyboardHandlerBox"].toString());

	xemu_settings->EnhanceComboBox(ui->mouseHandlerBox, emu_settings::MouseHandler);
	SubscribeTooltip(ui->mouseHandlerBox, json_input["mouseHandlerBox"].toString());

	xemu_settings->EnhanceComboBox(ui->cameraTypeBox, emu_settings::CameraType);
	SubscribeTooltip(ui->cameraTypeBox, json_input["cameraTypeBox"].toString());

	xemu_settings->EnhanceComboBox(ui->cameraBox, emu_settings::Camera);
	SubscribeTooltip(ui->cameraBox, json_input["cameraBox"].toString());

	xemu_settings->EnhanceComboBox(ui->moveBox, emu_settings::Move);
	SubscribeTooltip(ui->moveBox, json_input["moveBox"].toString());

	//     _____           _                   _______    _
	//    / ____|         | |                 |__   __|  | |
	//   | (___  _   _ ___| |_ ___ _ __ ___      | | __ _| |__
	//    \___ \| | | / __| __/ _ \ '_ ` _ \     | |/ _` | '_ \
	//    ____) | |_| \__ \ ||  __/ | | | | |    | | (_| | |_) |
	//   |_____/ \__, |___/\__\___|_| |_| |_|    |_|\__,_|_.__/
	//            __/ |
	//           |___/

	// Comboboxes

	xemu_settings->EnhanceComboBox(ui->sysLangBox, emu_settings::Language);
	SubscribeTooltip(ui->sysLangBox, json_sys["sysLangBox"].toString());

	// Checkboxes

	xemu_settings->EnhanceCheckBox(ui->enableHostRoot, emu_settings::EnableHostRoot);
	SubscribeTooltip(ui->enableHostRoot, json_sys["enableHostRoot"].toString());

	//    _   _      _                      _      _______    _
	//   | \ | |    | |                    | |    |__   __|  | |
	//   |  \| | ___| |___      _____  _ __| | __    | | __ _| |__
	//   | . ` |/ _ \ __\ \ /\ / / _ \| '__| |/ /    | |/ _` | '_ \
	//   | |\  |  __/ |_ \ V  V / (_) | |  |   <     | | (_| | |_) |
	//   |_| \_|\___|\__| \_/\_/ \___/|_|  |_|\_\    |_|\__,_|_.__/

	// Comboboxes

	xemu_settings->EnhanceComboBox(ui->netStatusBox, emu_settings::ConnectionStatus);
	SubscribeTooltip(ui->netStatusBox, json_net["netStatusBox"].toString());

	//    ______                 _       _               _______    _
	//   |  ____|               | |     | |             |__   __|  | |
	//   | |__   _ __ ___  _   _| | __ _| |_ ___  _ __     | | __ _| |__
	//   |  __| | '_ ` _ \| | | | |/ _` | __/ _ \| '__|    | |/ _` | '_ \
	//   | |____| | | | | | |_| | | (_| | || (_) | |       | | (_| | |_) |
	//   |______|_| |_| |_|\__,_|_|\__,_|\__\___/|_|       |_|\__,_|_.__/

	// Comboboxes

	xemu_settings->EnhanceComboBox(ui->maxLLVMThreads, emu_settings::MaxLLVMThreads, true, true, std::thread::hardware_concurrency());
	SubscribeTooltip(ui->maxLLVMThreads, json_emu_misc["maxLLVMThreads"].toString());
	ui->maxLLVMThreads->setItemText(ui->maxLLVMThreads->findData("0"), tr("All (%1)").arg(std::thread::hardware_concurrency()));

	xemu_settings->EnhanceComboBox(ui->perfOverlayDetailLevel, emu_settings::PerfOverlayDetailLevel);
	SubscribeTooltip(ui->perfOverlayDetailLevel, json_emu_overlay["perfOverlayDetailLevel"].toString());

	xemu_settings->EnhanceComboBox(ui->perfOverlayPosition, emu_settings::PerfOverlayPosition);
	SubscribeTooltip(ui->perfOverlayPosition, json_emu_overlay["perfOverlayPosition"].toString());

	// Checkboxes

	SubscribeTooltip(ui->gs_resizeOnBoot, json_emu_misc["gs_resizeOnBoot"].toString());

	SubscribeTooltip(ui->gs_disableMouse, json_emu_misc["gs_disableMouse"].toString());

	xemu_settings->EnhanceCheckBox(ui->exitOnStop, emu_settings::ExitRPCS3OnFinish);
	SubscribeTooltip(ui->exitOnStop, json_emu_misc["exitOnStop"].toString());

	xemu_settings->EnhanceCheckBox(ui->alwaysStart, emu_settings::StartOnBoot);
	SubscribeTooltip(ui->alwaysStart, json_emu_misc["alwaysStart"].toString());

	xemu_settings->EnhanceCheckBox(ui->startGameFullscreen, emu_settings::StartGameFullscreen);
	SubscribeTooltip(ui->startGameFullscreen, json_emu_misc["startGameFullscreen"].toString());

	xemu_settings->EnhanceCheckBox(ui->showFPSInTitle, emu_settings::ShowFPSInTitle);
	SubscribeTooltip(ui->showFPSInTitle, json_emu_misc["showFPSInTitle"].toString());

	xemu_settings->EnhanceCheckBox(ui->showTrophyPopups, emu_settings::ShowTrophyPopups);
	SubscribeTooltip(ui->showTrophyPopups, json_emu_misc["showTrophyPopups"].toString());

	xemu_settings->EnhanceCheckBox(ui->useNativeInterface, emu_settings::UseNativeInterface);
	SubscribeTooltip(ui->useNativeInterface, json_emu_misc["useNativeInterface"].toString());

	xemu_settings->EnhanceCheckBox(ui->showShaderCompilationHint, emu_settings::ShowShaderCompilationHint);
	SubscribeTooltip(ui->showShaderCompilationHint, json_emu_misc["showShaderCompilationHint"].toString());

	xemu_settings->EnhanceCheckBox(ui->perfOverlayEnabled, emu_settings::PerfOverlayEnabled);
	SubscribeTooltip(ui->perfOverlayEnabled, json_emu_overlay["perfOverlayEnabled"].toString());
	auto EnablePerfOverlayOptions = [this](bool enabled)
	{
		ui->label_detail_level->setEnabled(enabled);
		ui->label_update_interval->setEnabled(enabled);
		ui->label_font_size->setEnabled(enabled);
		ui->perfOverlayDetailLevel->setEnabled(enabled);
		ui->perfOverlayPosition->setEnabled(enabled);
		ui->perfOverlayUpdateInterval->setEnabled(enabled);
		ui->perfOverlayFontSize->setEnabled(enabled);
	};
	EnablePerfOverlayOptions(ui->perfOverlayEnabled->isChecked());
	connect(ui->perfOverlayEnabled, &QCheckBox::clicked, EnablePerfOverlayOptions);

	// Sliders

	xemu_settings->EnhanceSlider(ui->perfOverlayUpdateInterval, emu_settings::PerfOverlayUpdateInterval, true);
	SubscribeTooltip(ui->perfOverlayUpdateInterval, json_emu_overlay["perfOverlayUpdateInterval"].toString());
	ui->label_update_interval->setText(tr("Update Interval: %0 ms").arg(ui->perfOverlayUpdateInterval->value()));
	connect(ui->perfOverlayUpdateInterval, &QSlider::valueChanged, [this](int value)
	{
		ui->label_update_interval->setText(tr("Update Interval: %0 ms").arg(value));
	});

	xemu_settings->EnhanceSlider(ui->perfOverlayFontSize, emu_settings::PerfOverlayFontSize, true);
	SubscribeTooltip(ui->perfOverlayFontSize, json_emu_overlay["perfOverlayFontSize"].toString());
	ui->label_font_size->setText(tr("Font Size: %0 px").arg(ui->perfOverlayFontSize->value()));
	connect(ui->perfOverlayFontSize, &QSlider::valueChanged, [this](int value)
	{
		ui->label_font_size->setText(tr("Font Size: %0 px").arg(value));
	});

	// Global settings (gui_settings)
	if (!game)
	{
		ui->gs_disableMouse->setChecked(xgui_settings->GetValue(gui::gs_disableMouse).toBool());
		connect(ui->gs_disableMouse, &QCheckBox::clicked, [=](bool val)
		{
			xgui_settings->SetValue(gui::gs_disableMouse, val);
		});

		bool enableButtons = xgui_settings->GetValue(gui::gs_resize).toBool();
		ui->gs_resizeOnBoot->setChecked(enableButtons);
		ui->gs_width->setEnabled(enableButtons);
		ui->gs_height->setEnabled(enableButtons);

		QRect screen = QApplication::desktop()->screenGeometry();
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
			ui->gs_width->setValue(std::min(ui->gs_width->value(), QApplication::desktop()->screenGeometry().width()));
			xgui_settings->SetValue(gui::gs_width, ui->gs_width->value());
		});
		connect(ui->gs_height, &QSpinBox::editingFinished, [=]()
		{
			ui->gs_height->setValue(std::min(ui->gs_height->value(), QApplication::desktop()->screenGeometry().height()));
			xgui_settings->SetValue(gui::gs_height, ui->gs_height->value());
		});
	}

	//     _____  _    _  _   _______    _
	//    / ____|| |  | || | |__   __|  | |
	//   | |  __|| |  | || |    | | __ _| |__
	//   | | |_ || |  | || |    | |/ _` | '_ \
	//   | |__| || |__| || |    | | (_| | |_) |
	//    \_____| \____/ |_|    |_|\__,_|_.__/

	// Comboboxes
	SubscribeTooltip(ui->combo_configs, json_gui["configs"].toString());

	SubscribeTooltip(ui->combo_stylesheets, json_gui["stylesheets"].toString());

	// Checkboxes:
	SubscribeTooltip(ui->cb_custom_colors, json_gui["custom_colors"].toString());

	// Checkboxes: gui options
	SubscribeTooltip(ui->cb_show_welcome, json_gui["show_welcome"].toString());

	SubscribeTooltip(ui->useRichPresence, json_gui["useRichPresence"].toString());

	SubscribeTooltip(ui->discordState, json_gui["discordState"].toString());

	if (!game)
	{
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
		};
		AddColoredIcons();

		ui->cb_show_welcome->setChecked(xgui_settings->GetValue(gui::ib_show_welcome).toBool());

		bool enableUIColors = xgui_settings->GetValue(gui::m_enableUIColors).toBool();
		ui->cb_custom_colors->setChecked(enableUIColors);
		ui->pb_gl_icon_color->setEnabled(enableUIColors);

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
			if (m_currentConfig != xgui_settings->GetValue(gui::m_currentConfig).toString())
			{
				OnApplyConfig();
			}
			if (m_currentStylesheet != xgui_settings->GetValue(gui::m_currentStylesheet).toString())
			{
				OnApplyStylesheet();
			}
		};

		connect(ui->okButton, &QAbstractButton::clicked, [=]()
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

		connect(ui->cb_custom_colors, &QCheckBox::clicked, [=](bool val)
		{
			xgui_settings->SetValue(gui::m_enableUIColors, val);
			ui->pb_gl_icon_color->setEnabled(val);
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
	SubscribeTooltip(ui->glLegacyBuffers, json_debug["glLegacyBuffers"].toString());

	xemu_settings->EnhanceCheckBox(ui->forceHighpZ, emu_settings::ForceHighpZ);
	SubscribeTooltip(ui->forceHighpZ, json_debug["forceHighpZ"].toString());

	xemu_settings->EnhanceCheckBox(ui->debugOutput, emu_settings::DebugOutput);
	SubscribeTooltip(ui->debugOutput, json_debug["debugOutput"].toString());

	xemu_settings->EnhanceCheckBox(ui->debugOverlay, emu_settings::DebugOverlay);
	SubscribeTooltip(ui->debugOverlay, json_debug["debugOverlay"].toString());

	xemu_settings->EnhanceCheckBox(ui->logProg, emu_settings::LogShaderPrograms);
	SubscribeTooltip(ui->logProg, json_debug["logProg"].toString());

	xemu_settings->EnhanceCheckBox(ui->readColor, emu_settings::ReadColorBuffers);
	SubscribeTooltip(ui->readColor, json_debug["readColor"].toString());

	xemu_settings->EnhanceCheckBox(ui->dumpDepth, emu_settings::WriteDepthBuffer);
	SubscribeTooltip(ui->dumpDepth, json_debug["dumpDepth"].toString());

	xemu_settings->EnhanceCheckBox(ui->readDepth, emu_settings::ReadDepthBuffer);
	SubscribeTooltip(ui->readDepth, json_debug["readDepth"].toString());

	xemu_settings->EnhanceCheckBox(ui->disableHwOcclusionQueries, emu_settings::DisableOcclusionQueries);
	SubscribeTooltip(ui->disableHwOcclusionQueries, json_debug["disableOcclusionQueries"].toString());

	xemu_settings->EnhanceCheckBox(ui->forceCpuBlitEmulation, emu_settings::ForceCPUBlitEmulation);
	SubscribeTooltip(ui->forceCpuBlitEmulation, json_debug["forceCpuBlitEmulation"].toString());

	xemu_settings->EnhanceCheckBox(ui->disableOnDiskShaderCache, emu_settings::DisableOnDiskShaderCache);
	SubscribeTooltip(ui->disableOnDiskShaderCache, json_debug["disableOnDiskShaderCache"].toString());

	xemu_settings->EnhanceCheckBox(ui->disableVulkanMemAllocator, emu_settings::DisableVulkanMemAllocator);
	SubscribeTooltip(ui->disableVulkanMemAllocator, json_debug["disableVulkanMemAllocator"].toString());

	xemu_settings->EnhanceCheckBox(ui->disableFIFOReordering, emu_settings::DisableFIFOReordering);
	SubscribeTooltip(ui->disableFIFOReordering, json_debug["disableFIFOReordering"].toString());

	// Checkboxes: core debug options
	xemu_settings->EnhanceCheckBox(ui->ppuDebug, emu_settings::PPUDebug);
	SubscribeTooltip(ui->ppuDebug, json_debug["ppuDebug"].toString());

	xemu_settings->EnhanceCheckBox(ui->spuDebug, emu_settings::SPUDebug);
	SubscribeTooltip(ui->spuDebug, json_debug["spuDebug"].toString());

	xemu_settings->EnhanceCheckBox(ui->spuVerification, emu_settings::SPUVerification);
	SubscribeTooltip(ui->spuVerification, json_debug["spuVerification"].toString());

	xemu_settings->EnhanceCheckBox(ui->setDAZandFTZ, emu_settings::SetDAZandFTZ);
	SubscribeTooltip(ui->setDAZandFTZ, json_debug["setDAZandFTZ"].toString());

	xemu_settings->EnhanceCheckBox(ui->accurateGETLLAR, emu_settings::AccurateGETLLAR);
	SubscribeTooltip(ui->accurateGETLLAR, json_debug["accurateGETLLAR"].toString());

	xemu_settings->EnhanceCheckBox(ui->accuratePUTLLUC, emu_settings::AccuratePUTLLUC);
	SubscribeTooltip(ui->accuratePUTLLUC, json_debug["accuratePUTLLUC"].toString());

	xemu_settings->EnhanceCheckBox(ui->hookStFunc, emu_settings::HookStaticFuncs);
	SubscribeTooltip(ui->hookStFunc, json_debug["hookStFunc"].toString());

	//
	// Layout fix for High Dpi
	//
	layout()->setSizeConstraint(QLayout::SetFixedSize);
}

settings_dialog::~settings_dialog()
{
	delete ui;
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

	m_currentConfig = tr("CurrentSettings");

	int index = ui->combo_configs->findText(m_currentConfig);
	if (index != -1)
	{
		ui->combo_configs->setCurrentIndex(index);
	}
	else
	{
		LOG_WARNING(GENERAL, "Trying to set an invalid config index ", index);
	}
}

void settings_dialog::AddStylesheets()
{
	ui->combo_stylesheets->clear();

	ui->combo_stylesheets->addItem("Default (Bright)", gui::Default);

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
		LOG_WARNING(GENERAL, "Trying to set an invalid stylesheets index ", index);
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
		QString friendlyName = dialog->textValue();
		if (friendlyName == "")
		{
			QMessageBox::warning(this, tr("Error"), tr("Name cannot be empty"));
			continue;
		}
		if (friendlyName.contains("."))
		{
			QMessageBox::warning(this, tr("Error"), tr("Must choose a name with no '.'"));
			continue;
		}
		if (ui->combo_configs->findText(friendlyName) != -1)
		{
			QMessageBox::warning(this, tr("Error"), tr("Please choose a non-existing name"));
			continue;
		}
		Q_EMIT GuiSettingsSaveRequest();
		xgui_settings->SaveCurrentConfig(friendlyName);
		ui->combo_configs->addItem(friendlyName);
		ui->combo_configs->setCurrentIndex(ui->combo_configs->findText(friendlyName));
		break;
	}
}

void settings_dialog::OnApplyConfig()
{
	m_currentConfig = ui->combo_configs->currentText();
	xgui_settings->SetValue(gui::m_currentConfig, m_currentConfig);
	xgui_settings->ChangeToConfig(m_currentConfig);
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

	int i = ui->tab_widget_settings->currentIndex();
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

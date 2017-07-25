#include <QVBoxLayout>
#include <QHBoxLayout>
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

#include "settings_dialog.h"

#include "ui_settings_dialog.h"

#include "stdafx.h"
#include "Emu/System.h"
#include "Crypto/unself.h"

#include <unordered_set>

inline std::string sstr(const QString& _in) { return _in.toUtf8().toStdString(); }
inline std::string sstr(const QVariant& _in) { return sstr(_in.toString()); }

settings_dialog::settings_dialog(std::shared_ptr<gui_settings> xSettings, const Render_Creator& r_Creator, const int& tabIndex, QWidget *parent, const GameInfo* game)
	: QDialog(parent), xgui_settings(xSettings), ui(new Ui::settings_dialog), m_tab_Index(tabIndex)
{
	ui->setupUi(this);
	ui->cancelButton->setDefault(true);
	ui->tabWidget->setUsesScrollButtons(false);

	bool showDebugTab = xgui_settings->GetValue(GUI::m_showDebugTab).toBool();
	xgui_settings->SetValue(GUI::m_showDebugTab, showDebugTab);
	if (!showDebugTab)
	{
		ui->tabWidget->removeTab(7);
	}

	// read tooltips from json
	QFile json_file(":/Json/tooltips.json");
	json_file.open(QIODevice::ReadOnly | QIODevice::Text);
	QJsonObject json_obj = QJsonDocument::fromJson(json_file.readAll()).object();
	json_file.close();

	QJsonObject json_cpu = json_obj.value("cpu").toObject();
	QJsonObject json_cpu_ppu = json_cpu.value("PPU").toObject();
	QJsonObject json_cpu_spu = json_cpu.value("SPU").toObject();
	QJsonObject json_cpu_cbs = json_cpu.value("checkboxes").toObject();
	QJsonObject json_cpu_cbo = json_cpu.value("comboboxes").toObject();
	QJsonObject json_cpu_lib = json_cpu.value("libraries").toObject();

	QJsonObject json_gpu = json_obj.value("gpu").toObject();
	QJsonObject json_gpu_cbo = json_gpu.value("comboboxes").toObject();
	QJsonObject json_gpu_main = json_gpu.value("main").toObject();
	QJsonObject json_gpu_deb = json_gpu.value("debug").toObject();

	QJsonObject json_audio = json_obj.value("audio").toObject();
	QJsonObject json_input = json_obj.value("input").toObject();
	QJsonObject json_sys = json_obj.value("system").toObject();
	QJsonObject json_net = json_obj.value("network").toObject();

	QJsonObject json_emu = json_obj.value("emulator").toObject();
	QJsonObject json_emu_gui = json_emu.value("gui").toObject();
	QJsonObject json_emu_misc = json_emu.value("misc").toObject();

	QJsonObject json_debug = json_obj.value("debug").toObject();

	std::shared_ptr<emu_settings> xemu_settings;
	if (game)
	{
		xemu_settings.reset(new emu_settings("data/" + game->serial));
		setWindowTitle(tr("Settings: [") + qstr(game->serial) + "] " + qstr(game->name));
	}
	else
	{
		xemu_settings.reset(new emu_settings(""));
		setWindowTitle(tr("Settings"));
	}

	// Various connects
	connect(ui->okButton, &QAbstractButton::clicked, ui->coreTab, [=]() {
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
		Q_EMIT ToolBarRepaintRequest();
	});
	connect(ui->okButton, &QAbstractButton::clicked, xemu_settings.get(), &emu_settings::SaveSettings);
	connect(ui->okButton, &QAbstractButton::clicked, this, &QDialog::accept);
	connect(ui->cancelButton, &QAbstractButton::clicked, this, &QWidget::close);
	connect(ui->tabWidget, &QTabWidget::currentChanged, [=]() {ui->cancelButton->setFocus(); });

	//     _____ _____  _    _   _______    _     
	//    / ____|  __ \| |  | | |__   __|  | |    
	//   | |    | |__) | |  | |    | | __ _| |__  
	//   | |    |  ___/| |  | |    | |/ _` | '_ \ 
	//   | |____| |    | |__| |    | | (_| | |_) |
	//    \_____|_|     \____/     |_|\__,_|_.__/ 

	// Checkboxes

	xemu_settings->EnhanceCheckBox(ui->hookStFunc, emu_settings::HookStaticFuncs);
	ui->hookStFunc->setToolTip(json_cpu_cbs["hookStFunc"].toString());

	xemu_settings->EnhanceCheckBox(ui->bindSPUThreads, emu_settings::BindSPUThreads);
	ui->bindSPUThreads->setToolTip(json_cpu_cbs["bindSPUThreads"].toString());

	xemu_settings->EnhanceCheckBox(ui->lowerSPUThrPrio, emu_settings::LowerSPUThreadPrio);
	ui->lowerSPUThrPrio->setToolTip(json_cpu_cbs["lowerSPUThrPrio"].toString());

	xemu_settings->EnhanceCheckBox(ui->spuLoopDetection, emu_settings::SPULoopDetection);
	ui->spuLoopDetection->setToolTip(json_cpu_cbs["spuLoopDetection"].toString());

	// Comboboxes

	// TODO implement enhancement for combobox / spinbox with proper range
	//xemu_settings->EnhanceComboBox(ui->preferredSPUThreads, emu_settings::PreferredSPUThreads);
	const QString Auto = tr("Auto");
	ui->preferredSPUThreads->setToolTip(json_cpu_cbo["preferredSPUThreads"].toString());
	for (int i = 0; i <= 6; i++)
	{
		ui->preferredSPUThreads->addItem( i == 0 ? Auto : QString::number(i), QVariant(i) );
	}
	const QString valueOf_PreferredSPUThreads = qstr(xemu_settings->GetSetting(emu_settings::PreferredSPUThreads));
	int index = ui->preferredSPUThreads->findData(valueOf_PreferredSPUThreads == "0" ? Auto : valueOf_PreferredSPUThreads);
	if (index == -1)
	{
		LOG_WARNING(GENERAL, "Current setting not found while creating preferredSPUThreads");
	}
	else
	{
		ui->preferredSPUThreads->setCurrentIndex(index);
	}
	connect(ui->preferredSPUThreads, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged), [=](int index) {
		xemu_settings->SetSetting(emu_settings::PreferredSPUThreads, std::to_string(ui->preferredSPUThreads->itemData(index).toInt()));
	});

	// PPU tool tips
	ui->ppu_precise->setToolTip(json_cpu_ppu["precise"].toString());
	ui->ppu_fast->setToolTip(json_cpu_ppu["fast"].toString());
	ui->ppu_llvm->setToolTip(json_cpu_ppu["LLVM"].toString());

	QButtonGroup *ppuBG = new QButtonGroup(this);
	ppuBG->addButton(ui->ppu_precise, (int)ppu_decoder_type::precise);
	ppuBG->addButton(ui->ppu_fast,    (int)ppu_decoder_type::fast);
	ppuBG->addButton(ui->ppu_llvm,    (int)ppu_decoder_type::llvm);

	{ // PPU Stuff
		QString selectedPPU = qstr(xemu_settings->GetSetting(emu_settings::PPUDecoder));
		QStringList ppu_list = xemu_settings->GetSettingOptions(emu_settings::PPUDecoder);

		for (int i = 0; i < ppu_list.count(); i++)
		{
			ppuBG->button(i)->setText(ppu_list[i]);

			if (ppu_list[i] == selectedPPU)
			{
				ppuBG->button(i)->setChecked(true);
			}

#ifndef LLVM_AVAILABLE
			if (ppu_list[i].toLower().contains("llvm"))
			{
				ppuBG->button(i)->setEnabled(false);
			}
#endif

			connect(ppuBG->button(i), &QAbstractButton::pressed, [=]() {xemu_settings->SetSetting(emu_settings::PPUDecoder, sstr(ppu_list[i])); });
		}
	}

	// SPU tool tips
	ui->spu_precise->setToolTip(json_cpu_spu["precise"].toString());
	ui->spu_fast->setToolTip(json_cpu_spu["fast"].toString());
	ui->spu_asmjit->setToolTip(json_cpu_spu["ASMJIT"].toString());
	ui->spu_llvm->setToolTip(json_cpu_spu["LLVM"].toString());

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
			spuBG->button(i)->setText(spu_list[i]);

			if (spu_list[i] == selectedSPU)
			{
				spuBG->button(i)->setChecked(true);
			}

			connect(spuBG->button(i), &QAbstractButton::pressed, [=]() {xemu_settings->SetSetting(emu_settings::SPUDecoder, sstr(spu_list[i])); });
		}
	}

	// lib options tool tips
	ui->lib_auto->setToolTip(json_cpu_lib["auto"].toString());
	ui->lib_manu->setToolTip(json_cpu_lib["manual"].toString());
	ui->lib_both->setToolTip(json_cpu_lib["both"].toString());
	ui->lib_lv2->setToolTip(json_cpu_lib["liblv2"].toString());

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

			connect(libModeBG->button(i), &QAbstractButton::pressed, [=]() {xemu_settings->SetSetting(emu_settings::LibLoadOptions, sstr(libmode_list[i])); });
		}
	}

	// Sort string vector alphabetically
	static const auto sort_string_vector = [](std::vector<std::string>& vec)
	{
		std::sort(vec.begin(), vec.end(), [](const std::string &str1, const std::string &str2) { return str1 < str2; });
	};

	std::vector<std::string> loadedLibs = xemu_settings->GetLoadedLibraries();

	sort_string_vector(loadedLibs);

	for (auto lib : loadedLibs)
	{
		QListWidgetItem* item = new QListWidgetItem(qstr(lib), ui->lleList);
		item->setFlags(item->flags() | Qt::ItemIsUserCheckable); // set checkable flag
		item->setCheckState(Qt::Checked); // AND initialize check state
		ui->lleList->addItem(item);
	}
	const std::string& lle_dir = Emu.GetLibDir(); // TODO

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

	for (auto lib : lle_module_list_unselected)
	{
		QListWidgetItem* item = new QListWidgetItem(qstr(lib), ui->lleList);
		item->setFlags(item->flags() | Qt::ItemIsUserCheckable); // set checkable flag
		item->setCheckState(Qt::Unchecked); // AND initialize check state
		ui->lleList->addItem(item);
	}

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
		QList<QListWidgetItem*> checked_Libs;
		QList<QListWidgetItem*> unchecked_Libs;

		// create sublists. we need clones to preserve checkstates
		for (int i = 0; i < ui->lleList->count(); ++i)
		{
			if (ui->lleList->item(i)->checkState() == Qt::Checked)
			{
				checked_Libs.append(ui->lleList->item(i)->clone());
			}
			else
			{
				unchecked_Libs.append(ui->lleList->item(i)->clone());
			}
		}

		// sort sublists
		auto qLessThan = [](QListWidgetItem *i1, QListWidgetItem *i2) { return i1->text() < i2->text(); };
		qSort(checked_Libs.begin(), checked_Libs.end(), qLessThan);
		qSort(unchecked_Libs.begin(), unchecked_Libs.end(), qLessThan);

		// refill library list
		ui->lleList->clear();

		for (const auto& lib : checked_Libs)
		{
			ui->lleList->addItem(lib);
		}
		for (const auto& lib : unchecked_Libs)
		{
			ui->lleList->addItem(lib);
		}

		// only show items filtered for search text
		for (int i = 0; i < ui->lleList->count(); i++)
		{
			if (ui->lleList->item(i)->text().contains(searchTerm))
			{
				ui->lleList->setRowHidden(i, false);
			}
			else
			{
				ui->lleList->setRowHidden(i, true);
			}
		}
	};

	// Events
	connect(libModeBG, static_cast<void(QButtonGroup::*)(int)>(&QButtonGroup::buttonClicked), l_OnLibButtonClicked);
	connect(ui->searchBox, &QLineEdit::textChanged, l_OnSearchBoxTextChanged);

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

	// Comboboxes
	ui->graphicsAdapterBox->setToolTip(json_gpu_cbo["graphicsAdapterBox"].toString());

	xemu_settings->EnhanceComboBox(ui->renderBox, emu_settings::Renderer);
	ui->renderBox->setToolTip(json_gpu_cbo["renderBox"].toString());

	xemu_settings->EnhanceComboBox(ui->resBox, emu_settings::Resolution);
	ui->resBox->setToolTip(json_gpu_cbo["resBox"].toString());

	xemu_settings->EnhanceComboBox(ui->aspectBox, emu_settings::AspectRatio);
	ui->aspectBox->setToolTip(json_gpu_cbo["aspectBox"].toString());

	xemu_settings->EnhanceComboBox(ui->frameLimitBox, emu_settings::FrameLimit);
	ui->frameLimitBox->setToolTip(json_gpu_cbo["frameLimitBox"].toString());

	// Checkboxes: main options
	xemu_settings->EnhanceCheckBox(ui->dumpColor, emu_settings::WriteColorBuffers);
	ui->dumpColor->setToolTip(json_gpu_main["dumpColor"].toString());

	xemu_settings->EnhanceCheckBox(ui->vsync, emu_settings::VSync);
	ui->vsync->setToolTip(json_gpu_main["vsync"].toString());

	xemu_settings->EnhanceCheckBox(ui->autoInvalidateCache, emu_settings::AutoInvalidateCache);
	ui->autoInvalidateCache->setToolTip(json_gpu_main["autoInvalidateCache"].toString());

	xemu_settings->EnhanceCheckBox(ui->gpuTextureScaling, emu_settings::GPUTextureScaling);
	ui->gpuTextureScaling->setToolTip(json_gpu_main["gpuTextureScaling"].toString());

	xemu_settings->EnhanceCheckBox(ui->stretchToDisplayArea, emu_settings::StretchToDisplayArea);
	ui->stretchToDisplayArea->setToolTip(json_gpu_main["stretchToDisplayArea"].toString());

	// Graphics Adapter
	QStringList D3D12Adapters = r_Creator.D3D12Adapters;
	QStringList vulkanAdapters = r_Creator.vulkanAdapters;
	bool supportsD3D12 = r_Creator.supportsD3D12;
	bool supportsVulkan = r_Creator.supportsVulkan;
	QString r_D3D12 = r_Creator.render_D3D12;
	QString r_Vulkan = r_Creator.render_Vulkan;
	QString r_OpenGL = r_Creator.render_OpenGL;
	QString old_D3D12;
	QString old_Vulkan;

	if (supportsD3D12)
	{
		old_D3D12 = qstr(xemu_settings->GetSetting(emu_settings::D3D12Adapter));
	}
	else
	{
		// Remove D3D12 option from render combobox
		for (int i = 0; i < ui->renderBox->count(); i++)
		{
			if (ui->renderBox->itemText(i) == r_D3D12)
			{
				ui->renderBox->removeItem(i);
				break;
			}
		}
	}

	if (supportsVulkan)
	{
		old_Vulkan = qstr(xemu_settings->GetSetting(emu_settings::VulkanAdapter));
	}
	else
	{
		// Remove Vulkan option from render combobox
		for (int i = 0; i < ui->renderBox->count(); i++)
		{
			if (ui->renderBox->itemText(i) == r_Vulkan)
			{
				ui->renderBox->removeItem(i);
				break;
			}
		}
	}

	QString oldRender = ui->renderBox->itemText(ui->renderBox->currentIndex());

	auto switchGraphicsAdapter = [=](int index)
	{
		QString render = ui->renderBox->itemText(index);
		m_isD3D12 = render == r_D3D12;
		m_isVulkan = render == r_Vulkan;
		ui->graphicsAdapterBox->setEnabled(m_isD3D12 || m_isVulkan);

		// D3D Adapter
		if (m_isD3D12)
		{
			// Reset other adapters to old config
			if (supportsVulkan)
			{
				xemu_settings->SetSetting(emu_settings::VulkanAdapter, sstr(old_Vulkan));
			}
			// Fill combobox
			ui->graphicsAdapterBox->clear();
			for (const auto& adapter : D3D12Adapters)
			{
				ui->graphicsAdapterBox->addItem(adapter);
			}
			// Reset Adapter to old config
			int idx = ui->graphicsAdapterBox->findText(old_D3D12);
			if (idx == -1)
			{
				idx = 0;
				if (old_D3D12.isEmpty())
				{
					LOG_WARNING(RSX, "%s adapter config empty: setting to default!", sstr(r_D3D12));
				}
				else
				{
					LOG_WARNING(RSX, "Last used %s adapter not found: setting to default!", sstr(r_D3D12));
				}
			}
			ui->graphicsAdapterBox->setCurrentIndex(idx);
			xemu_settings->SetSetting(emu_settings::D3D12Adapter, sstr(ui->graphicsAdapterBox->currentText()));
		}

		// Vulkan Adapter
		else if (m_isVulkan)
		{
			// Reset other adapters to old config
			if (supportsD3D12)
			{
				xemu_settings->SetSetting(emu_settings::D3D12Adapter, sstr(old_D3D12));
			}
			// Fill combobox
			ui->graphicsAdapterBox->clear();
			for (const auto& adapter : vulkanAdapters)
			{
				ui->graphicsAdapterBox->addItem(adapter);
			}
			// Reset Adapter to old config
			int idx = ui->graphicsAdapterBox->findText(old_Vulkan);
			if (idx == -1)
			{
				idx = 0;
				if (old_Vulkan.isEmpty())
				{
					LOG_WARNING(RSX, "%s adapter config empty: setting to default!", sstr(r_Vulkan));
				}
				else
				{
					LOG_WARNING(RSX, "Last used %s adapter not found: setting to default!", sstr(r_Vulkan));
				}
			}
			ui->graphicsAdapterBox->setCurrentIndex(idx);
			xemu_settings->SetSetting(emu_settings::VulkanAdapter, sstr(ui->graphicsAdapterBox->currentText()));
		}

		// Other Adapter
		else
		{
			// Reset Adapters to old config
			if (supportsD3D12)
			{
				xemu_settings->SetSetting(emu_settings::D3D12Adapter, sstr(old_D3D12));
			}
			if (supportsVulkan)
			{
				xemu_settings->SetSetting(emu_settings::VulkanAdapter, sstr(old_Vulkan));
			}

			// Fill combobox with placeholder
			ui->graphicsAdapterBox->clear();
			ui->graphicsAdapterBox->addItem(tr("Not needed for %1 renderer").arg(render));
		}
	};

	auto setAdapter = [=](QString text)
	{
		if (text.isEmpty()) return;

		// don't set adapter if signal was created by switching render
		QString newRender = ui->renderBox->itemText(ui->renderBox->currentIndex());
		if (m_oldRender == newRender)
		{
			if (m_isD3D12 && D3D12Adapters.contains(text))
			{
				xemu_settings->SetSetting(emu_settings::D3D12Adapter, sstr(text));
			}
			else if (m_isVulkan && vulkanAdapters.contains(text))
			{
				xemu_settings->SetSetting(emu_settings::VulkanAdapter, sstr(text));
			}
		}
		else
		{
			m_oldRender = newRender;
		}
	};

	// Init
	setAdapter(ui->graphicsAdapterBox->currentText());
	switchGraphicsAdapter(ui->renderBox->currentIndex());

	// Events
	connect(ui->graphicsAdapterBox, &QComboBox::currentTextChanged, setAdapter);
	connect(ui->renderBox, static_cast<void (QComboBox::*)(int index)>(&QComboBox::currentIndexChanged), switchGraphicsAdapter);

	auto fixGLLegacy = [=](const QString& text) {
		ui->glLegacyBuffers->setEnabled(text == r_OpenGL);
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
	ui->audioOutBox->setToolTip(json_audio["audioOutBox"].toString());

	// Checkboxes

	xemu_settings->EnhanceCheckBox(ui->audioDump, emu_settings::DumpToFile);
	ui->audioDump->setToolTip(json_audio["audioDump"].toString());

	xemu_settings->EnhanceCheckBox(ui->convert, emu_settings::ConvertTo16Bit);
	ui->convert->setToolTip(json_audio["convert"].toString());

	xemu_settings->EnhanceCheckBox(ui->downmix, emu_settings::DownmixStereo);
	ui->downmix->setToolTip(json_audio["downmix"].toString());

	//    _____       __   ____    _______    _     
	//   |_   _|     / /  / __ \  |__   __|  | |    
	//     | |      / /  | |  | |    | | __ _| |__  
	//     | |     / /   | |  | |    | |/ _` | '_ \ 
	//    _| |_   / /    | |__| |    | | (_| | |_) |
	//   |_____| /_/      \____/     |_|\__,_|_.__/ 

	// Comboboxes

	xemu_settings->EnhanceComboBox(ui->padHandlerBox, emu_settings::PadHandler);
	ui->padHandlerBox->setToolTip(json_input["padHandlerBox"].toString());

	xemu_settings->EnhanceComboBox(ui->keyboardHandlerBox, emu_settings::KeyboardHandler);
	ui->keyboardHandlerBox->setToolTip(json_input["keyboardHandlerBox"].toString());

	xemu_settings->EnhanceComboBox(ui->mouseHandlerBox, emu_settings::MouseHandler);
	ui->mouseHandlerBox->setToolTip(json_input["mouseHandlerBox"].toString());

	xemu_settings->EnhanceComboBox(ui->cameraTypeBox, emu_settings::CameraType);
	ui->cameraTypeBox->setToolTip(json_input["cameraTypeBox"].toString());

	xemu_settings->EnhanceComboBox(ui->cameraBox, emu_settings::Camera);
	ui->cameraBox->setToolTip(json_input["cameraBox"].toString());

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
	ui->sysLangBox->setToolTip(json_sys["sysLangBox"].toString());

	// Checkboxes

	xemu_settings->EnhanceCheckBox(ui->enableHostRoot, emu_settings::EnableHostRoot);
	ui->enableHostRoot->setToolTip(json_sys["enableHostRoot"].toString());

	//    _   _      _                      _      _______    _     
	//   | \ | |    | |                    | |    |__   __|  | |    
	//   |  \| | ___| |___      _____  _ __| | __    | | __ _| |__  
	//   | . ` |/ _ \ __\ \ /\ / / _ \| '__| |/ /    | |/ _` | '_ \ 
	//   | |\  |  __/ |_ \ V  V / (_) | |  |   <     | | (_| | |_) |
	//   |_| \_|\___|\__| \_/\_/ \___/|_|  |_|\_\    |_|\__,_|_.__/ 

	// Comboboxes

	xemu_settings->EnhanceComboBox(ui->netStatusBox, emu_settings::ConnectionStatus);
	ui->netStatusBox->setToolTip(json_net["netStatusBox"].toString());

	//    ______                 _       _               _______    _     
	//   |  ____|               | |     | |             |__   __|  | |    
	//   | |__   _ __ ___  _   _| | __ _| |_ ___  _ __     | | __ _| |__  
	//   |  __| | '_ ` _ \| | | | |/ _` | __/ _ \| '__|    | |/ _` | '_ \ 
	//   | |____| | | | | | |_| | | (_| | || (_) | |       | | (_| | |_) |
	//   |______|_| |_| |_|\__,_|_|\__,_|\__\___/|_|       |_|\__,_|_.__/ 

	// Comboboxes

	ui->combo_configs->setToolTip(json_emu_gui["configs"].toString());
	ui->combo_stylesheets->setToolTip(json_emu_gui["stylesheets"].toString());

	// Checkboxes

	ui->cb_show_welcome->setToolTip(json_emu_gui["show_welcome"].toString());

	xemu_settings->EnhanceCheckBox(ui->exitOnStop, emu_settings::ExitRPCS3OnFinish);
	ui->exitOnStop->setToolTip(json_emu_misc["exitOnStop"].toString());

	xemu_settings->EnhanceCheckBox(ui->alwaysStart, emu_settings::StartOnBoot);
	ui->alwaysStart->setToolTip(json_emu_misc["alwaysStart"].toString());

	xemu_settings->EnhanceCheckBox(ui->startGameFullscreen, emu_settings::StartGameFullscreen);
	ui->startGameFullscreen->setToolTip(json_emu_misc["startGameFullscreen"].toString());

	xemu_settings->EnhanceCheckBox(ui->showFPSInTitle, emu_settings::ShowFPSInTitle);
	ui->showFPSInTitle->setToolTip(json_emu_misc["showFPSInTitle"].toString());

	if (game)
	{
		ui->gb_stylesheets->setEnabled(false);
		ui->gb_configs->setEnabled(false);
		ui->gb_settings->setEnabled(false);
	}
	else
	{
		ui->cb_show_welcome->setChecked(xgui_settings->GetValue(GUI::ib_show_welcome).toBool());

		connect(ui->okButton, &QAbstractButton::clicked, [this]() {
			// Only attempt to load a config if changes occurred.
			if (m_startingConfig != xgui_settings->GetValue(GUI::m_currentConfig).toString())
			{
				OnApplyConfig();
			}
			if (m_startingStylesheet != xgui_settings->GetValue(GUI::m_currentStylesheet).toString())
			{
				OnApplyStylesheet();
			}
		});
		connect(ui->pb_reset_default, &QAbstractButton::clicked, [=]() {
			if (QMessageBox::question(this, tr("Reset GUI to default?"), tr("This will include your stylesheet as well. Do you wish to proceed?"),
				QMessageBox::Yes | QMessageBox::No, QMessageBox::No) == QMessageBox::Yes)
			{
				xgui_settings->Reset(true);
				xgui_settings->ChangeToConfig(tr("default"));
				Q_EMIT GuiStylesheetRequest(tr("default"));
				Q_EMIT GuiSettingsSyncRequest();
				AddConfigs();
				AddStylesheets();
			}
		});
		connect(ui->pb_backup_config, &QAbstractButton::clicked, this, &settings_dialog::OnBackupCurrentConfig);
		connect(ui->pb_apply_config, &QAbstractButton::clicked, this, &settings_dialog::OnApplyConfig);
		connect(ui->pb_apply_stylesheet, &QAbstractButton::clicked, this, &settings_dialog::OnApplyStylesheet);
		connect(ui->pb_open_folder, &QAbstractButton::clicked, [=]() {QDesktopServices::openUrl(xgui_settings->GetSettingsDir()); });
		connect(ui->cb_show_welcome, &QCheckBox::clicked, [=](bool val) {xgui_settings->SetValue(GUI::ib_show_welcome, val); });
		auto colorDialog = [&](const GUI_SAVE& color, const QString& title, QPushButton *button){
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
				button->setIcon(gui_settings::colorizedIcon(button->icon(), oldColor, dlg.selectedColor(), true));
			}
		};
		connect(ui->pb_gl_icon_color, &QAbstractButton::clicked, [=]() { colorDialog(GUI::gl_iconColor, tr("Choose gamelist icon color"), ui->pb_gl_icon_color); });
		connect(ui->pb_gl_tool_icon_color, &QAbstractButton::clicked, [=]() { colorDialog(GUI::gl_toolIconColor, tr("Choose gamelist tool icon color"), ui->pb_gl_tool_icon_color); });
		connect(ui->pb_tool_bar_color, &QAbstractButton::clicked, [=]() { colorDialog(GUI::mw_toolBarColor, tr("Choose tool bar color"), ui->pb_tool_bar_color); });
		connect(ui->pb_tool_icon_color, &QAbstractButton::clicked, [=]() { colorDialog(GUI::mw_toolIconColor, tr("Choose tool icon color"), ui->pb_tool_icon_color); });

		// colorize preview icons
		auto addColoredIcon = [&](QPushButton *button, const QColor& color, const QIcon& icon = QIcon(), const QColor& iconColor = QColor()){
			QLabel* text = new QLabel(button->text());
			text->setAlignment(Qt::AlignCenter);
			text->setAttribute(Qt::WA_TransparentForMouseEvents, true);
			if (icon.isNull())
			{
				QPixmap pixmap(100, 100);
				pixmap.fill(color);
				button->setIcon(pixmap);
			}
			else
			{
				button->setIcon(gui_settings::colorizedIcon(icon, iconColor, color));
			}
			button->setText("");
			button->setStyleSheet("text-align:left;");
			button->setLayout(new QGridLayout);
			button->layout()->setContentsMargins(0, 0, 0, 0);
			button->layout()->addWidget(text);
		};
		addColoredIcon(ui->pb_gl_icon_color, xgui_settings->GetValue(GUI::gl_iconColor).value<QColor>());
		addColoredIcon(ui->pb_tool_bar_color, xgui_settings->GetValue(GUI::mw_toolBarColor).value<QColor>());
		addColoredIcon(ui->pb_gl_tool_icon_color, xgui_settings->GetValue(GUI::gl_toolIconColor).value<QColor>(), QIcon(":/Icons/home_blue.png"), GUI::gl_tool_icon_color);
		addColoredIcon(ui->pb_tool_icon_color, xgui_settings->GetValue(GUI::mw_toolIconColor).value<QColor>(), QIcon(":/Icons/stop.png"), GUI::mw_tool_icon_color);

		bool enableButtons = xgui_settings->GetValue(GUI::gs_resize).toBool();
		ui->gs_resizeOnBoot->setChecked(enableButtons);
		ui->gs_width->setEnabled(enableButtons);
		ui->gs_height->setEnabled(enableButtons);

		QRect rec = QApplication::desktop()->screenGeometry();
		int width = xgui_settings->GetValue(GUI::gs_width).toInt();
		int height = xgui_settings->GetValue(GUI::gs_height).toInt();
		const int max_width = rec.width();
		const int max_height = rec.height();
		ui->gs_width->setValue(width < max_width ? width : max_width);
		ui->gs_height->setValue(height < max_height ? height : max_height);

		connect(ui->gs_resizeOnBoot, &QCheckBox::clicked, [=](bool val) {
			xgui_settings->SetValue(GUI::gs_resize, val);
			ui->gs_width->setEnabled(val);
			ui->gs_height->setEnabled(val);
		});
		connect(ui->gs_width, static_cast<void(QSpinBox::*)(int)>(&QSpinBox::valueChanged), [=](int w) {
			int width = QApplication::desktop()->screenGeometry().width();
			w = w > width ? width : w;
			ui->gs_width->setValue(w);
			xgui_settings->SetValue(GUI::gs_width, w);
		});
		connect(ui->gs_height, static_cast<void(QSpinBox::*)(int)>(&QSpinBox::valueChanged), [=](int h) {
			int height = QApplication::desktop()->screenGeometry().height();
			h = h > height ? height : h;
			ui->gs_height->setValue(h);
			xgui_settings->SetValue(GUI::gs_height, h);
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

	// Checkboxes: debug options
	xemu_settings->EnhanceCheckBox(ui->glLegacyBuffers, emu_settings::LegacyBuffers);
	ui->glLegacyBuffers->setToolTip(json_debug["glLegacyBuffers"].toString());

	xemu_settings->EnhanceCheckBox(ui->scrictModeRendering, emu_settings::StrictRenderingMode);
	ui->scrictModeRendering->setToolTip(json_debug["scrictModeRendering"].toString());

	xemu_settings->EnhanceCheckBox(ui->forceHighpZ, emu_settings::ForceHighpZ);
	ui->forceHighpZ->setToolTip(json_debug["forceHighpZ"].toString());

	xemu_settings->EnhanceCheckBox(ui->debugOutput, emu_settings::DebugOutput);
	ui->debugOutput->setToolTip(json_debug["debugOutput"].toString());

	xemu_settings->EnhanceCheckBox(ui->debugOverlay, emu_settings::DebugOverlay);
	ui->debugOverlay->setToolTip(json_debug["debugOverlay"].toString());

	xemu_settings->EnhanceCheckBox(ui->logProg, emu_settings::LogShaderPrograms);
	ui->logProg->setToolTip(json_debug["logProg"].toString());

	xemu_settings->EnhanceCheckBox(ui->readColor, emu_settings::ReadColorBuffers);
	ui->readColor->setToolTip(json_debug["readColor"].toString());

	xemu_settings->EnhanceCheckBox(ui->dumpDepth, emu_settings::WriteDepthBuffer);
	ui->dumpDepth->setToolTip(json_debug["dumpDepth"].toString());

	xemu_settings->EnhanceCheckBox(ui->readDepth, emu_settings::ReadDepthBuffer);
	ui->readDepth->setToolTip(json_debug["readDepth"].toString());

	//
	// Layout fix for High Dpi
	//
	layout()->setSizeConstraint(QLayout::SetFixedSize);
}

void settings_dialog::AddConfigs()
{
	ui->combo_configs->clear();

	ui->combo_configs->addItem(tr("default"));

	for (QString entry : xgui_settings->GetConfigEntries())
	{
		if (entry != tr("default"))
		{
			ui->combo_configs->addItem(entry);
		}
	}

	QString currentSelection = tr("CurrentSettings");
	m_startingConfig = currentSelection;

	int index = ui->combo_configs->findText(currentSelection);
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

	ui->combo_stylesheets->addItem(tr("default"));

	for (QString entry : xgui_settings->GetStylesheetEntries())
	{
		if (entry != tr("default"))
		{
			ui->combo_stylesheets->addItem(entry);
		}
	}

	QString currentSelection = xgui_settings->GetValue(GUI::m_currentStylesheet).toString();
	m_startingStylesheet = currentSelection;

	int index = ui->combo_stylesheets->findText(currentSelection);
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
	QString name = ui->combo_configs->currentText();
	xgui_settings->SetValue(GUI::m_currentConfig, name);
	xgui_settings->ChangeToConfig(name);
	Q_EMIT GuiSettingsSyncRequest();
}

void settings_dialog::OnApplyStylesheet()
{
	xgui_settings->SetValue(GUI::m_currentStylesheet, ui->combo_stylesheets->currentText());
	Q_EMIT GuiStylesheetRequest(xgui_settings->GetCurrentStylesheetPath());
}

int settings_dialog::exec()
{
	show();
	for (int i = 0; i < ui->tabWidget->count(); i++)
	{
		ui->tabWidget->setCurrentIndex(i);
	}
	ui->tabWidget->setCurrentIndex(m_tab_Index);
	return QDialog::exec();
}

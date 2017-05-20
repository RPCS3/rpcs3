#include "core_tab.h"

#include "stdafx.h"
#include "Emu/System.h"
#include "Crypto/unself.h"

#include "rpcs3qt/emu_settings.h"

#include <QCheckBox>
#include <QGroupBox>
#include <QRadioButton>
#include <QButtonGroup>
#include <QVBoxLayout>
#include <QHBoxLayout>

#include <unordered_set>

core_tab::core_tab(std::shared_ptr<emu_settings> settings, QWidget *parent) : QWidget(parent), xemu_settings(settings)
{
	// PPU Decoder
	QGroupBox *ppuDecoder = new QGroupBox(tr("PPU Decoder"));
	QVBoxLayout *ppuVbox = new QVBoxLayout;

	{ // PPU Stuff : I could make a lambda/special getter in emu_settings, but it's only used three times like this, and two times they're done slighly differently (one setting is disabled).
		std::string selectedPPU = settings->GetSetting(emu_settings::PPUDecoder);
		for (QString settingType : settings->GetSettingOptions(emu_settings::PPUDecoder))
		{
			std::string curr = settingType.toStdString();
			QRadioButton* butt = new QRadioButton(tr(curr.c_str()), this);
			if (curr == "Interpreter (precise)")
			{
				butt->setEnabled(false);
			}
			butt->setCheckable(true);
			ppuVbox->addWidget(butt);
			if (settingType.toStdString() == selectedPPU)
			{
				butt->setChecked(true);
			}
			connect(butt, &QAbstractButton::pressed, [=]() {settings->SetSetting(emu_settings::PPUDecoder, curr); });
		}
	}
	ppuDecoder->setLayout(ppuVbox);

	// SPU Decoder
	QGroupBox *spuDecoder = new QGroupBox(tr("SPU Decoder"));
	QVBoxLayout *spuVbox = new QVBoxLayout;

	{ // Spu stuff
		std::string selectedSPU = settings->GetSetting(emu_settings::SPUDecoder);
		for (QString settingType : settings->GetSettingOptions(emu_settings::SPUDecoder))
		{
			std::string curr = settingType.toStdString();
			QRadioButton* butt = new QRadioButton(tr(curr.c_str()), this);
			if (curr == "Recompiler (LLVM)")
			{
				butt->setEnabled(false);
			}
			butt->setCheckable(true);
			spuVbox->addWidget(butt);
			if (settingType.toStdString() == selectedSPU)
			{
				butt->setChecked(true);
			}
			connect(butt, &QAbstractButton::pressed, [=]() {settings->SetSetting(emu_settings::SPUDecoder, curr); });
		}
	}
	spuDecoder->setLayout(spuVbox);

	// Checkboxes
	QCheckBox *hookStFunc = settings->CreateEnhancedCheckBox(emu_settings::HookStaticFuncs, this);
	QCheckBox *bindSPUThreads = settings->CreateEnhancedCheckBox(emu_settings::BindSPUThreads, this);
	QCheckBox *lowerSPUThrPrio = settings->CreateEnhancedCheckBox(emu_settings::LowerSPUThreadPrio, this);

	// Load libraries
	QGroupBox *lle = new QGroupBox(tr("Load libraries"));
	QButtonGroup *libModeBG = new QButtonGroup(this);
	QVBoxLayout *lleVbox = new QVBoxLayout;

	
	{// Handle lib loading options
		std::string selectedLib = settings->GetSetting(emu_settings::LibLoadOptions);
		for (QString settingType : settings->GetSettingOptions(emu_settings::LibLoadOptions))
		{
			std::string curr = settingType.toStdString();
			QRadioButton* butt = new QRadioButton(tr(curr.c_str()), lle);
			butt->setCheckable(true);
			libModeBG->addButton(butt);
			lleVbox->addWidget(butt);
			if (curr == selectedLib)
			{
				butt->setChecked(true);
			}
			connect(butt, &QAbstractButton::pressed, [=]() {settings->SetSetting(emu_settings::LibLoadOptions, curr); });
		}
	}
	lleList = new QListWidget;
	lleList->setSelectionMode(QAbstractItemView::MultiSelection);
	searchBox = new QLineEdit;

	// Sort string vector alphabetically
	static const auto sort_string_vector = [](std::vector<std::string>& vec)
	{
		std::sort(vec.begin(), vec.end(), [](const std::string &str1, const std::string &str2) { return str1 < str2; });
	};

	std::vector<std::string> loadedLibs = xemu_settings->GetLoadedLibraries();

	sort_string_vector(loadedLibs);

	for (auto lib : loadedLibs)
	{
		QListWidgetItem* item = new QListWidgetItem(QString::fromStdString(lib), lleList);
		item->setFlags(item->flags() | Qt::ItemIsUserCheckable); // set checkable flag
		item->setCheckState(Qt::Checked); // AND initialize check state
		lleList->addItem(item);
	}
	const std::string& lle_dir = Emu.GetLibDir(); // TODO

	std::unordered_set<std::string> set(loadedLibs.begin(), loadedLibs.end());
	std::vector<std::string> lle_module_list_unselected;

	for (const auto& prxf : fs::dir(lle_dir))
	{
		// List found unselected modules
		if (prxf.is_directory || (prxf.name.substr(std::max(size_t(3), prxf.name.length()) - 3)) != "prx")
			continue;
		if (verify_npdrm_self_headers(fs::file(lle_dir + prxf.name)) && !set.count(prxf.name))
		{
			lle_module_list_unselected.push_back(prxf.name);
		}

	}

	sort_string_vector(lle_module_list_unselected);

	for (auto lib : lle_module_list_unselected)
	{
		QListWidgetItem* item = new QListWidgetItem(QString::fromStdString(lib), lleList);
		item->setFlags(item->flags() | Qt::ItemIsUserCheckable); // set checkable flag
		item->setCheckState(Qt::Unchecked); // AND initialize check state
		lleList->addItem(item);
	}


	// lleVbox
	lleVbox->addSpacing(5);
	lleVbox->addWidget(lleList);
	lleVbox->addWidget(searchBox);
	lle->setLayout(lleVbox);

	// Main layout
	QVBoxLayout *vbox = new QVBoxLayout;
	vbox->addWidget(ppuDecoder);
	vbox->addWidget(spuDecoder);
	vbox->addWidget(hookStFunc);
	vbox->addWidget(bindSPUThreads);
	vbox->addWidget(lowerSPUThrPrio);
	vbox->addStretch();

	QHBoxLayout *hbox = new QHBoxLayout;
	hbox->addLayout(vbox);
	hbox->addWidget(lle);
	setLayout(hbox);

	// Events
	connect(libModeBG, static_cast<void(QButtonGroup::*)(int)>(&QButtonGroup::buttonClicked), this, &core_tab::OnLibButtonClicked);
	connect(searchBox, &QLineEdit::textChanged, this, &core_tab::OnSearchBoxTextChanged);

	int buttid = libModeBG->checkedId();
	if (buttid != -1)
	{
		OnLibButtonClicked(buttid);
	}
}

void core_tab::SaveSelectedLibraries()
{
	if (shouldSaveLibs)
	{
		std::set<std::string> selectedlle;
		for (int i =0; i<lleList->count(); ++i)
		{
			auto item = lleList->item(i);
			if (item->checkState() != Qt::CheckState::Unchecked)
			{
				std::string lib = item->text().toStdString();
				selectedlle.emplace(lib);
			}
		}

		std::vector<std::string> selected_ls = std::vector<std::string>(selectedlle.begin(), selectedlle.end());
		xemu_settings->SaveSelectedLibraries(selected_ls);
	}
}

void core_tab::OnSearchBoxTextChanged()
{
	QString searchTerm = searchBox->text().toLower();
	auto model = lleList->model();
	auto items = model->match(model->index(0, 0), Qt::DisplayRole, QVariant::fromValue(searchTerm), -1, Qt::MatchContains);

	for (int i = 0; i < lleList->count(); ++i)
	{
		lleList->setRowHidden(i, true);
	}

	for (auto index : items)
	{
		lleList->setRowHidden(index.row(), false);
	}
}

void core_tab::OnLibButtonClicked(int ind)
{
	if (ind == -3 || ind == -4)
	{
		shouldSaveLibs = true;
		searchBox->setEnabled(true);
		lleList->setEnabled(true);
	}
	else
	{
		shouldSaveLibs = false;
		searchBox->setEnabled(false);
		lleList->setEnabled(false);
	}
}

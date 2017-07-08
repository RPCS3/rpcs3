#include <QCheckBox>
#include <QComboBox>
#include <QGroupBox>
#include <QVBoxLayout>
#include <QHBoxLayout>

#include "graphics_tab.h"

inline std::string sstr(const QString& _in) { return _in.toUtf8().toStdString(); }

graphics_tab::graphics_tab(std::shared_ptr<emu_settings> xSettings, Render_Creator r_Creator, QWidget *parent) : QWidget(parent), xemu_settings(xSettings)
{
	// Render
	QGroupBox *render = new QGroupBox(tr("Render"));

	QComboBox *renderBox = xemu_settings->CreateEnhancedComboBox(emu_settings::Renderer, this);

	QVBoxLayout *renderVbox = new QVBoxLayout();
	renderVbox->addWidget(renderBox);
	render->setLayout(renderVbox);

	// Resolution
	QGroupBox *res = new QGroupBox(tr("Resolution"));

	QComboBox* resBox = xemu_settings->CreateEnhancedComboBox(emu_settings::Resolution, this);

	QVBoxLayout *resVbox = new QVBoxLayout();
	resVbox->addWidget(resBox);
	res->setLayout(resVbox);

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
	QGroupBox *graphicsAdapter;
	QComboBox *graphicsAdapterBox;

	if (supportsD3D12)
	{
		old_D3D12 = qstr(xemu_settings->GetSetting(emu_settings::D3D12Adapter));
	}
	else
	{
		// Remove D3D12 option from render combobox
		for (int i = 0; i < renderBox->count(); i++)
		{
			if (renderBox->itemText(i) == r_D3D12)
			{
				renderBox->removeItem(i);
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
		for (int i = 0; i < renderBox->count(); i++)
		{
			if (renderBox->itemText(i) == r_Vulkan)
			{
				renderBox->removeItem(i);
				break;
			}
		}
	}

	if (supportsD3D12 || supportsVulkan)
	{
		graphicsAdapter = new QGroupBox(tr("Select Graphics Device"));
		graphicsAdapterBox = new QComboBox(this);
		QVBoxLayout *graphicsAdapterVbox = new QVBoxLayout();
		graphicsAdapterVbox->addWidget(graphicsAdapterBox);
		graphicsAdapter->setLayout(graphicsAdapterVbox);
		QString oldRender = renderBox->itemText(renderBox->currentIndex());

		auto switchGraphicsAdapter = [=](int index)
		{
			QString render = renderBox->itemText(index);
			m_isD3D12 = render == r_D3D12;
			m_isVulkan = render == r_Vulkan;
			graphicsAdapter->setEnabled(m_isD3D12 || m_isVulkan);

			// D3D Adapter
			if (m_isD3D12)
			{
				// Reset other adapters to old config
				if (supportsVulkan)
				{
					xemu_settings->SetSetting(emu_settings::VulkanAdapter, sstr(old_Vulkan));
				}
				// Fill combobox
				graphicsAdapterBox->clear();
				for (auto adapter : D3D12Adapters)
				{
					graphicsAdapterBox->addItem(adapter);
				}
				// Reset Adapter to old config
				int idx = graphicsAdapterBox->findText(old_D3D12);
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
				graphicsAdapterBox->setCurrentIndex(idx);
				xemu_settings->SetSetting(emu_settings::D3D12Adapter, sstr(graphicsAdapterBox->currentText()));
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
				graphicsAdapterBox->clear();
				for (auto adapter : vulkanAdapters)
				{
					graphicsAdapterBox->addItem(adapter);
				}
				// Reset Adapter to old config
				int idx = graphicsAdapterBox->findText(old_Vulkan);
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
				graphicsAdapterBox->setCurrentIndex(idx);
				xemu_settings->SetSetting(emu_settings::VulkanAdapter, sstr(graphicsAdapterBox->currentText()));
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
				graphicsAdapterBox->clear();
				graphicsAdapterBox->addItem(tr("Not needed for %1 renderer").arg(render));
			}
		};

		auto setAdapter = [=](QString text)
		{
			if (text.isEmpty()) return;

			// don't set adapter if signal was created by switching render
			QString newRender = renderBox->itemText(renderBox->currentIndex());
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
		setAdapter(graphicsAdapterBox->currentText());
		switchGraphicsAdapter(renderBox->currentIndex());

		// Events
		connect(graphicsAdapterBox, &QComboBox::currentTextChanged, setAdapter);
		connect(renderBox, static_cast<void (QComboBox::*)(int index)>(&QComboBox::currentIndexChanged), switchGraphicsAdapter);
	}

	// Aspect ratio
	QGroupBox *aspect = new QGroupBox(tr("Aspect ratio"));

	QComboBox *aspectBox = xemu_settings->CreateEnhancedComboBox(emu_settings::AspectRatio, this);

	QVBoxLayout *aspectVbox = new QVBoxLayout();
	aspectVbox->addWidget(aspectBox);
	aspect->setLayout(aspectVbox);

	// Frame limit
	QGroupBox *frameLimit = new QGroupBox(tr("Frame limit"));

	QComboBox *frameLimitBox = xemu_settings->CreateEnhancedComboBox(emu_settings::FrameLimit, this);

	QVBoxLayout *frameLimitVbox = new QVBoxLayout();
	frameLimitVbox->addWidget(frameLimitBox);
	frameLimit->setLayout(frameLimitVbox);

	// Checkboxes
	QCheckBox *dumpColor = xemu_settings->CreateEnhancedCheckBox(emu_settings::WriteColorBuffers, this);
	QCheckBox *readColor = xemu_settings->CreateEnhancedCheckBox(emu_settings::ReadColorBuffers, this);
	QCheckBox *dumpDepth = xemu_settings->CreateEnhancedCheckBox(emu_settings::WriteDepthBuffer, this);
	QCheckBox *readDepth = xemu_settings->CreateEnhancedCheckBox(emu_settings::ReadDepthBuffer, this);
	QCheckBox *glLegacyBuffers = xemu_settings->CreateEnhancedCheckBox(emu_settings::LegacyBuffers, this);
	QCheckBox *debugOutput = xemu_settings->CreateEnhancedCheckBox(emu_settings::DebugOutput, this);
	QCheckBox *debugOverlay = xemu_settings->CreateEnhancedCheckBox(emu_settings::DebugOverlay, this);
	QCheckBox *logProg = xemu_settings->CreateEnhancedCheckBox(emu_settings::LogShaderPrograms, this);
	QCheckBox *vsync = xemu_settings->CreateEnhancedCheckBox(emu_settings::VSync, this);
	QCheckBox *gpuTextureScaling = xemu_settings->CreateEnhancedCheckBox(emu_settings::GPUTextureScaling, this);
	QCheckBox *stretchToDisplayArea = xemu_settings->CreateEnhancedCheckBox(emu_settings::StretchToDisplayArea, this);
	QCheckBox *forceHighpZ = xemu_settings->CreateEnhancedCheckBox(emu_settings::ForceHighpZ, this);
	QCheckBox *autoInvalidateCache = xemu_settings->CreateEnhancedCheckBox(emu_settings::AutoInvalidateCache, this);
	QCheckBox *scrictModeRendering = xemu_settings->CreateEnhancedCheckBox(emu_settings::StrictRenderingMode, this);

	// Combobox Part
	QHBoxLayout *hbox1 = new QHBoxLayout();
	QVBoxLayout *vbox11 = new QVBoxLayout();
	vbox11->addWidget(render);
	vbox11->addWidget(res);
	if (supportsD3D12 || supportsVulkan) 
	{
		// be careful with layout changes due to render when adding new stuff
		vbox11->addWidget(graphicsAdapter);
	}
	vbox11->addStretch();
	QVBoxLayout *vbox12 = new QVBoxLayout();
	vbox12->addWidget(aspect);
	vbox12->addWidget(frameLimit);
	vbox12->addStretch();
	hbox1->addLayout(vbox11);
	hbox1->addLayout(vbox12);

	// Checkbox Part
	QGroupBox *mainOptions = new QGroupBox(tr("Main Options"));
	QHBoxLayout *hbox2 = new QHBoxLayout();	//main options
	QVBoxLayout *vbox21 = new QVBoxLayout();
	vbox21->addWidget(dumpColor);
	vbox21->addWidget(readColor);
	vbox21->addWidget(dumpDepth);
	vbox21->addWidget(readDepth);
	QVBoxLayout *vbox22 = new QVBoxLayout();
	vbox22->addWidget(vsync);
	vbox22->addWidget(autoInvalidateCache);
	vbox22->addWidget(gpuTextureScaling);
	vbox22->addWidget(stretchToDisplayArea);
	vbox22->addSpacing(20);

	hbox2->addLayout(vbox21);
	hbox2->addLayout(vbox22);

	QGroupBox *debugOptions = new QGroupBox(tr("Debugging Options"));
	QHBoxLayout *hbox3 = new QHBoxLayout();
	QBoxLayout *vbox31 = new QVBoxLayout();
	vbox31->addWidget(glLegacyBuffers);
	vbox31->addWidget(scrictModeRendering);
	vbox31->addWidget(forceHighpZ);
	QVBoxLayout *vbox32 = new QVBoxLayout();
	vbox32->addWidget(debugOutput);
	vbox32->addWidget(debugOverlay);
	vbox32->addWidget(logProg);

	hbox3->addLayout(vbox31);
	hbox3->addLayout(vbox32);

	mainOptions->setLayout(hbox2);
	debugOptions->setLayout(hbox3);

	QVBoxLayout *options_container = new QVBoxLayout();
	options_container->addWidget(mainOptions);
	options_container->addWidget(debugOptions);

	QVBoxLayout *vbox = new QVBoxLayout();
	vbox->addLayout(hbox1);
	vbox->addSpacing(10);
	vbox->addLayout(options_container);
	vbox->addStretch();
	setLayout(vbox);
	
	auto fixGLLegacy = [=](const QString& text) {
		glLegacyBuffers->setEnabled(text == r_OpenGL);
	};

	// Handle connects to disable specific checkboxes that depend on GUI state.
	fixGLLegacy(renderBox->currentText()); // Init
	connect(renderBox, &QComboBox::currentTextChanged, fixGLLegacy);
}

#include <QCheckBox>
#include <QComboBox>
#include <QGroupBox>
#include <QVBoxLayout>
#include <QHBoxLayout>

#ifdef _MSC_VER
#include <Windows.h>
#undef GetHwnd
#include <d3d12.h>
#include <wrl/client.h>
#include <dxgi1_4.h>
#endif

#include "graphicstab.h"

GraphicsTab::GraphicsTab(std::shared_ptr<EmuSettings> xSettings, QWidget *parent) : QWidget(parent), xEmuSettings(xSettings)
{
	// Render
	QGroupBox *render = new QGroupBox(tr("Render"));

	QComboBox *renderBox = xEmuSettings->CreateEnhancedComboBox(EmuSettings::Renderer, this);

	QVBoxLayout *renderVbox = new QVBoxLayout();
	renderVbox->addWidget(renderBox);
	render->setLayout(renderVbox);

	// Resolution
	QGroupBox *res = new QGroupBox(tr("Resolution"));

	QComboBox* resBox = xEmuSettings->CreateEnhancedComboBox(EmuSettings::Resolution, this);

	QVBoxLayout *resVbox = new QVBoxLayout();
	resVbox->addWidget(resBox);
	res->setLayout(resVbox);

	// D3D Adapter
	QGroupBox *d3dAdapter = new QGroupBox(tr("D3D Adapter (DirectX 12 Only)"));

	QComboBox *d3dAdapterBox = new QComboBox(this);

	QVBoxLayout *d3dAdapterVbox = new QVBoxLayout();
	d3dAdapterVbox->addWidget(d3dAdapterBox);
	d3dAdapter->setLayout(d3dAdapterVbox);

	// Aspect ratio
	QGroupBox *aspect = new QGroupBox(tr("Aspect ratio"));

	QComboBox *aspectBox = xEmuSettings->CreateEnhancedComboBox(EmuSettings::AspectRatio, this);

	QVBoxLayout *aspectVbox = new QVBoxLayout();
	aspectVbox->addWidget(aspectBox);
	aspect->setLayout(aspectVbox);

	// Frame limit
	QGroupBox *frameLimit = new QGroupBox(tr("Frame limit"));

	QComboBox *frameLimitBox = xEmuSettings->CreateEnhancedComboBox(EmuSettings::FrameLimit, this);

	QVBoxLayout *frameLimitVbox = new QVBoxLayout();
	frameLimitVbox->addWidget(frameLimitBox);
	frameLimit->setLayout(frameLimitVbox);

	// Checkboxes
	QCheckBox *dumpColor = xEmuSettings->CreateEnhancedCheckBox(EmuSettings::WriteColorBuffers, this);
	QCheckBox *readColor = xEmuSettings->CreateEnhancedCheckBox(EmuSettings::ReadColorBuffers, this);
	QCheckBox *dumpDepth = xEmuSettings->CreateEnhancedCheckBox(EmuSettings::WriteDepthBuffers, this);
	QCheckBox *readDepth = xEmuSettings->CreateEnhancedCheckBox(EmuSettings::ReadDepthBuffers, this);
	QCheckBox *glLegacyBuffers = xEmuSettings->CreateEnhancedCheckBox(EmuSettings::LegacyBuffers, this);
	QCheckBox *debugOutput = xEmuSettings->CreateEnhancedCheckBox(EmuSettings::DebugOutput, this);
	QCheckBox *debugOverlay = xEmuSettings->CreateEnhancedCheckBox(EmuSettings::DebugOverlay, this);
	QCheckBox *logProg = xEmuSettings->CreateEnhancedCheckBox(EmuSettings::LogShaderPrograms, this);
	QCheckBox *vsync = xEmuSettings->CreateEnhancedCheckBox(EmuSettings::VSync, this);
	QCheckBox *gpuTextureScaling = xEmuSettings->CreateEnhancedCheckBox(EmuSettings::GPUTextureScaling, this);

	QHBoxLayout *hbox1 = new QHBoxLayout();
	QVBoxLayout *vbox11 = new QVBoxLayout();
	vbox11->addWidget(render);
	vbox11->addWidget(res);
	vbox11->addWidget(d3dAdapter);
	vbox11->addStretch();
	QVBoxLayout *vbox12 = new QVBoxLayout();
	vbox12->addWidget(aspect);
	vbox12->addWidget(frameLimit);
	vbox12->addStretch();
	hbox1->addLayout(vbox11);
	hbox1->addLayout(vbox12);

	QHBoxLayout *hbox2 = new QHBoxLayout();
	QVBoxLayout *vbox21 = new QVBoxLayout();
	vbox21->addWidget(dumpColor);
	vbox21->addWidget(readColor);
	vbox21->addWidget(dumpDepth);
	vbox21->addWidget(readDepth);
	vbox21->addWidget(glLegacyBuffers);
	QVBoxLayout *vbox22 = new QVBoxLayout();
	vbox22->addWidget(debugOutput);
	vbox22->addWidget(debugOverlay);
	vbox22->addWidget(logProg);
	vbox22->addWidget(vsync);
	vbox22->addWidget(gpuTextureScaling);
	hbox2->addLayout(vbox21);
	hbox2->addLayout(vbox22);

	QVBoxLayout *vbox = new QVBoxLayout();
	vbox->addLayout(hbox1);
	vbox->addSpacing(10);
	vbox->addLayout(hbox2);
	vbox->addStretch();
	setLayout(vbox);

#ifdef _MSC_VER
	Microsoft::WRL::ComPtr<IDXGIFactory4> dxgi_factory;

	if (SUCCEEDED(CreateDXGIFactory(IID_PPV_ARGS(&dxgi_factory))))
	{
		Microsoft::WRL::ComPtr<IDXGIAdapter> adapter;

		for (UINT id = 0; dxgi_factory->EnumAdapters(id, adapter.ReleaseAndGetAddressOf()) != DXGI_ERROR_NOT_FOUND; id++)
		{
			DXGI_ADAPTER_DESC desc;
			adapter->GetDesc(&desc);
			d3dAdapterBox->addItem(QString::fromWCharArray(desc.Description));
		}

		QString adapterTxt = QString::fromStdString(xEmuSettings->GetSetting(EmuSettings::D3D12Adapter));
		int index = d3dAdapterBox->findText(adapterTxt);
		if (index == -1)
		{
			LOG_WARNING(GENERAL, "Can't find the selected D3D adapter in dialog");
		}
		else
		{
			d3dAdapterBox->setCurrentIndex(index);
		}
		connect(d3dAdapterBox, &QComboBox::currentTextChanged, [=](QString text) {
			xEmuSettings->SetSetting(EmuSettings::D3D12Adapter, text.toStdString());
		});
	}
	else
#endif
	{
		d3dAdapter->setEnabled(false);
	}
}

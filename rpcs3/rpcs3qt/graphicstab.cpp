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
	bool supportsD3D12 = false;
	QStringList D3D12Adapters;

// check for dx12 adapters
#ifdef _MSC_VER
	Microsoft::WRL::ComPtr<IDXGIFactory4> dxgi_factory;
	supportsD3D12 = SUCCEEDED(CreateDXGIFactory(IID_PPV_ARGS(&dxgi_factory)));
	if (supportsD3D12)
	{
		supportsD3D12 = false;
		IDXGIAdapter1* pAdapter = nullptr;
		for (UINT adapterIndex = 0; DXGI_ERROR_NOT_FOUND != dxgi_factory->EnumAdapters1(adapterIndex, &pAdapter); ++adapterIndex)
		{
			HMODULE D3D12Module = verify("d3d12.dll", LoadLibrary(L"d3d12.dll"));
			PFN_D3D12_CREATE_DEVICE wrapD3D12CreateDevice = (PFN_D3D12_CREATE_DEVICE)GetProcAddress(D3D12Module, "D3D12CreateDevice");
			if (SUCCEEDED(wrapD3D12CreateDevice(pAdapter, D3D_FEATURE_LEVEL_11_0, _uuidof(ID3D12Device), nullptr)))
			{
				DXGI_ADAPTER_DESC desc;
				pAdapter->GetDesc(&desc);
				D3D12Adapters.append(QString::fromWCharArray(desc.Description));
				supportsD3D12 = true;
			}
		}
	}
#endif

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
	QGroupBox *d3dAdapter;
	QComboBox *d3dAdapterBox;
	if (supportsD3D12)
	{
		d3dAdapter = new QGroupBox(tr("D3D Adapter (DirectX 12 Only)"));
		d3dAdapterBox = new QComboBox(this);

		QVBoxLayout *d3dAdapterVbox = new QVBoxLayout();
		d3dAdapterVbox->addWidget(d3dAdapterBox);
		d3dAdapter->setLayout(d3dAdapterVbox);
		auto enableAdapter = [=](int index)
		{
			if (renderBox->itemText(index) == "D3D12") d3dAdapter->setEnabled(true);
			else d3dAdapter->setEnabled(false);
		};
		enableAdapter(renderBox->currentIndex());
		connect(renderBox, static_cast<void (QComboBox::*)(int index)>(&QComboBox::currentIndexChanged), d3dAdapter, enableAdapter);
	}

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
	// be careful with layout changes due to D3D12 when adding new stuff
	if (supportsD3D12) vbox11->addWidget(d3dAdapter);
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

	if (supportsD3D12) // Fill D3D12 adapter combobox
	{
		for (auto adapter : D3D12Adapters)
		{
			d3dAdapterBox->addItem(adapter);
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
	else // Remove D3D12 option from render combobox
	{
		for (int i = 0; i < renderBox->count(); i++)
		{
			if (renderBox->itemText(i) == "D3D12")
			{
				renderBox->removeItem(i);
				break;
			}
		}
	}
}

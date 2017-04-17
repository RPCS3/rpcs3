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

GraphicsTab::GraphicsTab(QWidget *parent) : QWidget(parent)
{
	// Render
	QGroupBox *render = new QGroupBox(tr("Render"));

	QComboBox *renderBox = new QComboBox;
	renderBox->addItem(tr("Null"));
	renderBox->addItem(tr("Opengl"));
#ifdef _MSC_VER
	renderBox->addItem(tr("D3D12"));
#endif // _MSC_VER
#ifdef _WIN32
	renderBox->addItem(tr("Vulkan"));
#endif // _WIN32

	QVBoxLayout *renderVbox = new QVBoxLayout;
	renderVbox->addWidget(renderBox);
	render->setLayout(renderVbox);

	// Resolution
	QGroupBox *res = new QGroupBox(tr("Resolution"));

	QComboBox *resBox = new QComboBox;
	resBox->addItem(tr("1920x1080"));
	resBox->addItem(tr("1280x720"));
	resBox->addItem(tr("720x480"));
	resBox->addItem(tr("720x576"));
	resBox->addItem(tr("1600x1080"));
	resBox->addItem(tr("1440x1080"));
	resBox->addItem(tr("1280x1080"));
	resBox->addItem(tr("960x1080"));

	QVBoxLayout *resVbox = new QVBoxLayout;
	resVbox->addWidget(resBox);
	res->setLayout(resVbox);

	// D3D Adapter
	QGroupBox *d3dAdapter = new QGroupBox(tr("D3D Adapter (DirectX 12 Only)"));

	QComboBox *d3dAdapterBox = new QComboBox;

	QVBoxLayout *d3dAdapterVbox = new QVBoxLayout;
	d3dAdapterVbox->addWidget(d3dAdapterBox);
	d3dAdapter->setLayout(d3dAdapterVbox);

	// Aspect ratio
	QGroupBox *aspect = new QGroupBox(tr("Aspect ratio"));

	QComboBox *aspectBox = new QComboBox;
	aspectBox->addItem(tr("4x3"));
	aspectBox->addItem(tr("16x9"));

	QVBoxLayout *aspectVbox = new QVBoxLayout;
	aspectVbox->addWidget(aspectBox);
	aspect->setLayout(aspectVbox);

	// Frame limit
	QGroupBox *frameLimit = new QGroupBox(tr("Frame limit"));

	QComboBox *frameLimitBox = new QComboBox;
	frameLimitBox->addItem(tr("Off"));
	frameLimitBox->addItem(tr("59.94"));
	frameLimitBox->addItem(tr("50"));
	frameLimitBox->addItem(tr("60"));
	frameLimitBox->addItem(tr("30"));
	frameLimitBox->addItem(tr("Auto"));

	QVBoxLayout *frameLimitVbox = new QVBoxLayout;
	frameLimitVbox->addWidget(frameLimitBox);
	frameLimit->setLayout(frameLimitVbox);

	// Checkboxes
	QCheckBox *dumpColor = new QCheckBox(tr("Write Color Buffers"));
	QCheckBox *readColor = new QCheckBox(tr("Read Color Buffers"));
	QCheckBox *dumpDepth = new QCheckBox(tr("Write Depth Buffer"));
	QCheckBox *readDepth = new QCheckBox(tr("Read Depth Buffer"));
	QCheckBox *glLegacyBuffers = new QCheckBox(tr("Use Legacy OpenGL Buffers"));
	QCheckBox *debugOutput = new QCheckBox(tr("Debug Output"));
	QCheckBox *debugOverlay = new QCheckBox(tr("Debug Overlay"));
	QCheckBox *logProg = new QCheckBox(tr("Log Shader Programs"));
	QCheckBox *vsync = new QCheckBox(tr("VSync"));
	QCheckBox *gpuTextureScaling = new QCheckBox(tr("Use GPU Texture Scaling"));

	QHBoxLayout *hbox1 = new QHBoxLayout;
	QVBoxLayout *vbox11 = new QVBoxLayout;
	vbox11->addWidget(render);
	vbox11->addWidget(res);
	vbox11->addWidget(d3dAdapter);
	vbox11->addStretch();
	QVBoxLayout *vbox12 = new QVBoxLayout;
	vbox12->addWidget(aspect);
	vbox12->addWidget(frameLimit);
	vbox12->addStretch();
	hbox1->addLayout(vbox11);
	hbox1->addLayout(vbox12);

	QHBoxLayout *hbox2 = new QHBoxLayout;
	QVBoxLayout *vbox21 = new QVBoxLayout;
	vbox21->addWidget(dumpColor);
	vbox21->addWidget(readColor);
	vbox21->addWidget(dumpDepth);
	vbox21->addWidget(readDepth);
	vbox21->addWidget(glLegacyBuffers);
	QVBoxLayout *vbox22 = new QVBoxLayout;
	vbox22->addWidget(debugOutput);
	vbox22->addWidget(debugOverlay);
	vbox22->addWidget(logProg);
	vbox22->addWidget(vsync);
	vbox22->addWidget(gpuTextureScaling);
	hbox2->addLayout(vbox21);
	hbox2->addLayout(vbox22);

	QVBoxLayout *vbox = new QVBoxLayout;
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

		//pads.emplace_back(std::make_unique<combobox_pad>(cfg_location{ "Video", "D3D12", "Adapter" }, cbox_gs_d3d_adapter));
	}
	else
#endif
	{
		d3dAdapter->setEnabled(false);
	}
}

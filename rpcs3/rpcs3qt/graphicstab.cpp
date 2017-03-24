#ifdef QT_UI

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
	resBox->addItem(tr("1600x1080"));
	resBox->addItem(tr("1440x1080"));
	resBox->addItem(tr("1280x1080"));
	resBox->addItem(tr("960x1080"));

	QVBoxLayout *resVbox = new QVBoxLayout;
	resVbox->addWidget(resBox);
	res->setLayout(resVbox);

	// D3D Adapter
	QGroupBox *d3dAdapter = new QGroupBox(tr("D3D Adapter"));

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
	QCheckBox *logProg = new QCheckBox(tr("Log shader programs"));
	QCheckBox *vsync = new QCheckBox(tr("VSync"));

	// Main layout
	QVBoxLayout *vbox1 = new QVBoxLayout;
	vbox1->addWidget(render);
	vbox1->addWidget(res);
	vbox1->addWidget(d3dAdapter);
	vbox1->addWidget(aspect);
	vbox1->addWidget(frameLimit);

	QVBoxLayout *vbox2 = new QVBoxLayout;
	vbox2->addWidget(dumpColor);
	vbox2->addWidget(readColor);
	vbox2->addWidget(dumpDepth);
	vbox2->addWidget(readDepth);
	vbox2->addWidget(glLegacyBuffers);
	vbox2->addWidget(debugOutput);
	vbox2->addWidget(debugOverlay);
	vbox2->addWidget(logProg);
	vbox2->addWidget(vsync);
	vbox2->addStretch();

	QHBoxLayout *hbox = new QHBoxLayout;
	hbox->addLayout(vbox1);
	hbox->addLayout(vbox2);
	setLayout(hbox);

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

#endif // QT_UI

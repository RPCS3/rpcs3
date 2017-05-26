#ifndef EMU_SETTINGS_H
#define EMU_SETTINGS_H

#include "Utilities/File.h"
#include "Utilities/Log.h"

#include "yaml-cpp/yaml.h"

#include <QCheckBox>
#include <QStringList>
#include <QMap>
#include <QObject>
#include <QComboBox>

#ifdef _MSC_VER
#include <Windows.h>
#undef GetHwnd
#include <d3d12.h>
#include <wrl/client.h>
#include <dxgi1_4.h>
#endif

#ifdef _WIN32
#include "Emu/RSX/VK/VKHelpers.h"
#endif

inline QString qstr(const std::string& _in) { return QString::fromUtf8(_in.data(), _in.size()); }

struct Render_Creator
{
	bool supportsD3D12 = false;
	bool supportsVulkan = true;
	QStringList D3D12Adapters;
	QStringList vulkanAdapters;
	QString render_Vulkan = QObject::tr("Vulkan");
	QString render_D3D12 = QObject::tr("D3D12");
	QString render_OpenGL = QObject::tr("OpenGL");

	Render_Creator()
	{
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

		// check for vulkan adapters
#ifdef _WIN32
		vk::context device_enum_context;
		device_enum_context.createInstance("RPCS3");
		device_enum_context.makeCurrentInstance(1);
		std::vector<vk::physical_device>& gpus = device_enum_context.enumerateDevices();
		device_enum_context.close();
		if (gpus.size() > 0)
		{
			for (auto& gpu : gpus)
			{
				vulkanAdapters.append(qstr(gpu.name()));
			}
		}
		else
		{
			supportsVulkan = false;
		}
#endif
	}
};

// Node location
using cfg_location = std::vector<const char*>;

class emu_settings : public QObject
{
	/** A settings class for Emulator specific settings.  This class is a refactored version of the wx version.  It is much nicer
	*
	*/
	Q_OBJECT
public:
	enum SettingsType {
		// Core
		PPUDecoder,
		SPUDecoder,
		LibLoadOptions,
		HookStaticFuncs,
		BindSPUThreads,
		LowerSPUThreadPrio,

		// Graphics
		Renderer,
		Resolution,
		AspectRatio,
		FrameLimit,
		LogShaderPrograms,
		WriteDepthBuffer,
		WriteColorBuffers,
		ReadColorBuffers,
		ReadDepthBuffer,
		VSync,
		DebugOutput,
		DebugOverlay,
		LegacyBuffers,
		GPUTextureScaling,
		D3D12Adapter,
		VulkanAdapter,

		// Audio
		AudioRenderer,
		DumpToFile,
		ConvertTo16Bit,
		DownmixStereo,

		// Input / Output
		PadHandler,
		KeyboardHandler,
		MouseHandler,
		Camera,
		CameraType,

		// Misc
		ExitRPCS3OnFinish,
		StartOnBoot,
		StartGameFullscreen,

		// Network
		ConnectionStatus,

		// Language
		Language,
		EnableHostRoot,
	};

	/** Creates a settings object which reads in the config.yml file at rpcs3/bin/%path%/config.yml 
	* Settings are only written when SaveSettings is called.
	*/
	emu_settings(const std::string& path);
	~emu_settings();

	/** Returns a combo box of that setting type that is bound to the parent. */
	QComboBox* CreateEnhancedComboBox(SettingsType type, QWidget* parent = nullptr);

	/** Returns a check button that is connected to the target settings type, bound to the life of parent*/
	QCheckBox* CreateEnhancedCheckBox(SettingsType target, QWidget* parent = nullptr);

	std::vector<std::string> GetLoadedLibraries();
	void SaveSelectedLibraries(const std::vector<std::string>& libs);

	/** Returns the valid options for a given setting.*/
	QStringList GetSettingOptions(SettingsType type) const;

	/** Returns the value of the setting type.*/
	std::string GetSetting(SettingsType type) const;

	/** Sets the setting type to a given value.*/
	void SetSetting(SettingsType type, const std::string& val);
public slots:
/** Writes the unsaved settings to file.  Used in settings dialog on accept.*/
	void SaveSettings();
private:
	/** A helper map that keeps track of where a given setting type is located*/
	const QMap<SettingsType, cfg_location> SettingsLoc = {
		// Core Tab
		{ PPUDecoder,		{ "Core", "PPU Decoder"}},
		{ SPUDecoder,		{ "Core", "SPU Decoder"}},
		{ LibLoadOptions,	{ "Core", "Lib Loader"}},
		{ HookStaticFuncs,	{ "Core", "Hook static functions"}},
		{ BindSPUThreads,	{ "Core", "Bind SPU threads to secondary cores"}},
		{ LowerSPUThreadPrio, { "Core", "Lower SPU thread priority"}},

		// Graphics Tab
		{ Renderer,			{ "Video", "Renderer"}},
		{ Resolution,		{ "Video", "Resolution"}},
		{ AspectRatio,		{ "Video", "Aspect ratio"}},
		{ FrameLimit,		{ "Video", "Frame limit"}},
		{ LogShaderPrograms,{ "Video", "Log shader programs"}},
		{ WriteDepthBuffer, { "Video", "Write Depth Buffer"}},
		{ WriteColorBuffers,{ "Video", "Write Color Buffers"}},
		{ ReadColorBuffers, { "Video", "Read Color Buffers"}},
		{ ReadDepthBuffer,	{ "Video", "Read Depth Buffer"}},
		{ VSync,			{ "Video", "VSync"}},
		{ DebugOutput,		{ "Video", "Debug output"}},
		{ DebugOverlay,		{ "Video", "Debug overlay"}},
		{ LegacyBuffers,	{ "Video", "Use Legacy OpenGL Buffers (Debug)"}},
		{ GPUTextureScaling,{ "Video", "Use GPU texture scaling"}},
		{ D3D12Adapter,		{ "Video", "D3D12", "Adapter"}},
		{ VulkanAdapter,		{ "Video", "Vulkan", "Adapter"}},

		// Audio
		{ AudioRenderer,	{ "Audio", "Renderer"}},
		{ DumpToFile,		{ "Audio", "Dump to file"}},
		{ ConvertTo16Bit,	{ "Audio", "Convert to 16 bit"}},
		{ DownmixStereo,	{ "Audio", "Downmix to Stereo"}},

		// Input / Output
		{ PadHandler,		{ "Input/Output", "Pad"}},
		{ KeyboardHandler,	{ "Input/Output", "Keyboard"}},
		{ MouseHandler,		{ "Input/Output", "Mouse"}},
		{ Camera,			{ "Input/Output", "Camera"}},
		{ CameraType,		{ "Input/Output", "Camera type"}},

		// Misc
		{ExitRPCS3OnFinish,	{ "Miscellaneous", "Exit RPCS3 when process finishes" }},
		{StartOnBoot,		{ "Miscellaneous", "Always start after boot" }},
		{StartGameFullscreen, { "Miscellaneous", "Start Games in Fullscreen Mode"}},

		// Networking
		{ConnectionStatus,	{ "Net", "Connection status"}},

		// System
		{Language,			{ "System", "Language"}},
		{EnableHostRoot,	{ "VFS", "Enable /host_root/"}},

	};

	YAML::Node currentSettings; // The current settings as a YAML node.
	fs::file config; //! File to read/write the config settings.
};

#endif

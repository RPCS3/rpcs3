#pragma once

#include "Utilities/File.h"
#include "Utilities/Log.h"

#include "yaml-cpp/yaml.h"

#include <QCheckBox>
#include <QStringList>
#include <QMap>
#include <QObject>
#include <QComboBox>
#include <QSpinBox>

constexpr auto qstr = QString::fromStdString;

// Node location
using cfg_location = std::vector<const char*>;

class emu_settings : public QObject
{
	/** A settings class for Emulator specific settings.  This class is a refactored version of the wx version.  It is much nicer
	*
	*/
	Q_OBJECT
public:
	enum SettingsType
	{
		// Core
		PPUDecoder,
		SPUDecoder,
		LibLoadOptions,
		HookStaticFuncs,
		EnableThreadScheduler,
		LowerSPUThreadPrio,
		SPULoopDetection,
		PreferredSPUThreads,
		PPUDebug,
		SPUDebug,
		MaxLLVMThreads,
		EnableTSX,
		AccurateGETLLAR,
		AccuratePUTLLUC,
		AccurateXFloat,
		SetDAZandFTZ,
		SPUBlockSize,
		SPUCache,
		DebugConsoleMode,
		SilenceAllLogs,
		MaxSPURSThreads,
		SleepTimersAccuracy,
		ClocksScale,

		// Graphics
		Renderer,
		Resolution,
		AspectRatio,
		FrameLimit,
		MSAA,
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
		StretchToDisplayArea,
		VulkanAdapter,
		ForceHighpZ,
		StrictRenderingMode,
		DisableVertexCache,
		DisableOcclusionQueries,
		DisableFIFOReordering,
		StrictTextureFlushing,
		AnisotropicFilterOverride,
		ResolutionScale,
		MinimumScalableDimension,
		ForceCPUBlitEmulation,
		DisableOnDiskShaderCache,
		DisableVulkanMemAllocator,
		DisableAsyncShaderCompiler,
		MultithreadedRSX,
		VBlankRate,
		RelaxedZCULL,
		DriverWakeUpDelay,

		// Performance Overlay
		PerfOverlayEnabled,
		PerfOverlayFramerateGraphEnabled,
		PerfOverlayFrametimeGraphEnabled,
		PerfOverlayDetailLevel,
		PerfOverlayPosition,
		PerfOverlayUpdateInterval,
		PerfOverlayFontSize,
		PerfOverlayOpacity,
		PerfOverlayMarginX,
		PerfOverlayMarginY,
		PerfOverlayCenterX,
		PerfOverlayCenterY,

		// Shader Loading Dialog
		ShaderLoadBgEnabled,
		ShaderLoadBgDarkening,
		ShaderLoadBgBlur,

		// Audio
		AudioRenderer,
		DumpToFile,
		ConvertTo16Bit,
		DownmixStereo,
		MasterVolume,
		EnableBuffering,
		AudioBufferDuration,
		EnableTimeStretching,
		TimeStretchingThreshold,
		MicrophoneType,
		MicrophoneDevices,

		// Input / Output
		PadHandler,
		KeyboardHandler,
		MouseHandler,
		Camera,
		CameraType,
		Move,

		// Misc
		ExitRPCS3OnFinish,
		StartOnBoot,
		StartGameFullscreen,
		PreventDisplaySleep,
		ShowFPSInTitle,
		ShowTrophyPopups,
		ShowWelcomeScreen,
		UseNativeInterface,
		ShowShaderCompilationHint,

		// Network
		ConnectionStatus,

		// System
		Language,
		KeyboardType,
		EnterButtonAssignment,
		EnableHostRoot,
		LimitCacheSize,
		MaximumCacheSize,

		// Virtual File System
		emulatorLocation,
		dev_hdd0Location,
		dev_hdd1Location,
		dev_flashLocation,
		dev_usb000Location,
	};

	struct Render_Info
	{
		QString name;
		QString old_adapter;
		QStringList adapters;
		SettingsType type = VulkanAdapter;
		bool supported = true;
		bool has_adapters = true;
		bool has_msaa = false;

		Render_Info() = default;
		explicit Render_Info(QString name) : name(std::move(name)), has_adapters(false) {}
		Render_Info(QString name, QStringList adapters, bool supported, SettingsType type, bool has_msaa)
			: name(std::move(name)), adapters(std::move(adapters)), supported(supported), type(type), has_msaa(has_msaa) {}
	};

	struct Render_Creator
	{
		bool supportsVulkan = false;
		QStringList vulkanAdapters;
		QString name_Null = tr("Disable Video Output");
		QString name_Vulkan = tr("Vulkan");
		QString name_OpenGL = tr("OpenGL");
		Render_Info Vulkan;
		Render_Info OpenGL;
		Render_Info NullRender;
		std::vector<Render_Info*> renderers;

		Render_Creator();
	};

	struct Microphone_Creator
	{
		QStringList microphones_list;
		QString mic_none = tr("None");
		std::array<std::string, 4> sel_list;
		std::string SetDevice(u32 num, QString& text);
		void ParseDevices(std::string list);
		void RefreshList();
		Microphone_Creator();
	};

	std::set<SettingsType> m_broken_types; // list of broken settings

	/** Creates a settings object which reads in the config.yml file at rpcs3/bin/%path%/config.yml
	* Settings are only written when SaveSettings is called.
	*/
	emu_settings();
	~emu_settings();

	/** Connects a combo box with the target settings type*/
	void EnhanceComboBox(QComboBox* combobox, SettingsType type, bool is_ranged = false, bool use_max = false, int max = 0, bool sorted = false);

	/** Connects a check box with the target settings type*/
	void EnhanceCheckBox(QCheckBox* checkbox, SettingsType type);

	/** Connects a slider with the target settings type*/
	void EnhanceSlider(QSlider* slider, SettingsType type);

	/** Connects an integer spin box with the target settings type*/
	void EnhanceSpinBox(QSpinBox* slider, SettingsType type, const QString& prefix = "", const QString& suffix = "");

	/** Connects a double spin box with the target settings type*/
	void EnhanceDoubleSpinBox(QDoubleSpinBox* slider, SettingsType type, const QString& prefix = "", const QString& suffix = "");

	std::vector<std::string> GetLoadedLibraries();
	void SaveSelectedLibraries(const std::vector<std::string>& libs);

	/** Returns the valid options for a given setting.*/
	QStringList GetSettingOptions(SettingsType type) const;

	/** Returns the string for a given setting.*/
	std::string GetSettingName(SettingsType type) const;

	/** Returns the default value of the setting type.*/
	std::string GetSettingDefault(SettingsType type) const;

	/** Returns the value of the setting type.*/
	std::string GetSetting(SettingsType type) const;

	/** Sets the setting type to a given value.*/
	void SetSetting(SettingsType type, const std::string& val);

	/** Gets all the renderer info for gpu settings.*/
	Render_Creator m_render_creator;

	/** Gets a list of all the microphones available.*/
	Microphone_Creator m_microphone_creator;

	/** Loads the settings from path.*/
	void LoadSettings(const std::string& title_id = "");

	/** Fixes all registered invalid settings after asking the user for permission.*/
	void OpenCorrectionDialog(QWidget* parent = Q_NULLPTR);

public Q_SLOTS:
	/** Writes the unsaved settings to file.  Used in settings dialog on accept.*/
	void SaveSettings();
private:
	/** A helper map that keeps track of where a given setting type is located*/
	const QMap<SettingsType, cfg_location> SettingsLoc =
	{
		// Core Tab
		{ PPUDecoder,               { "Core", "PPU Decoder"}},
		{ SPUDecoder,               { "Core", "SPU Decoder"}},
		{ LibLoadOptions,           { "Core", "Lib Loader"}},
		{ HookStaticFuncs,          { "Core", "Hook static functions"}},
		{ EnableThreadScheduler,    { "Core", "Enable thread scheduler"}},
		{ LowerSPUThreadPrio,       { "Core", "Lower SPU thread priority"}},
		{ SPULoopDetection,         { "Core", "SPU loop detection"}},
		{ PreferredSPUThreads,      { "Core", "Preferred SPU Threads"}},
		{ PPUDebug,                 { "Core", "PPU Debug"}},
		{ SPUDebug,                 { "Core", "SPU Debug"}},
		{ MaxLLVMThreads,           { "Core", "Max LLVM Compile Threads"}},
		{ EnableTSX,                { "Core", "Enable TSX"}},
		{ AccurateGETLLAR,          { "Core", "Accurate GETLLAR"}},
		{ AccuratePUTLLUC,          { "Core", "Accurate PUTLLUC"}},
		{ AccurateXFloat,           { "Core", "Accurate xfloat"}},
		{ SetDAZandFTZ,             { "Core", "Set DAZ and FTZ"}},
		{ SPUBlockSize,             { "Core", "SPU Block Size"}},
		{ SPUCache,                 { "Core", "SPU Cache"}},
		{ DebugConsoleMode,         { "Core", "Debug Console Mode"}},
		{ MaxSPURSThreads,          { "Core", "Max SPURS Threads"}},
		{ SleepTimersAccuracy,      { "Core", "Sleep Timers Accuracy"}},
		{ ClocksScale,              { "Core", "Clocks scale"}},

		// Graphics Tab
		{ Renderer,                   { "Video", "Renderer"}},
		{ Resolution,                 { "Video", "Resolution"}},
		{ AspectRatio,                { "Video", "Aspect ratio"}},
		{ FrameLimit,                 { "Video", "Frame limit"}},
		{ MSAA,                       { "Video", "MSAA"}},
		{ LogShaderPrograms,          { "Video", "Log shader programs"}},
		{ WriteDepthBuffer,           { "Video", "Write Depth Buffer"}},
		{ WriteColorBuffers,          { "Video", "Write Color Buffers"}},
		{ ReadColorBuffers,           { "Video", "Read Color Buffers"}},
		{ ReadDepthBuffer,            { "Video", "Read Depth Buffer"}},
		{ VSync,                      { "Video", "VSync"}},
		{ DebugOutput,                { "Video", "Debug output"}},
		{ DebugOverlay,               { "Video", "Debug overlay"}},
		{ LegacyBuffers,              { "Video", "Use Legacy OpenGL Buffers"}},
		{ GPUTextureScaling,          { "Video", "Use GPU texture scaling"}},
		{ StretchToDisplayArea,       { "Video", "Stretch To Display Area"}},
		{ ForceHighpZ,                { "Video", "Force High Precision Z buffer"}},
		{ StrictRenderingMode,        { "Video", "Strict Rendering Mode"}},
		{ DisableVertexCache,         { "Video", "Disable Vertex Cache"}},
		{ DisableOcclusionQueries,    { "Video", "Disable ZCull Occlusion Queries"}},
		{ DisableFIFOReordering,      { "Video", "Disable FIFO Reordering"}},
		{ StrictTextureFlushing,      { "Video", "Strict Texture Flushing"}},
		{ ForceCPUBlitEmulation,      { "Video", "Force CPU Blit"}},
		{ DisableOnDiskShaderCache,   { "Video", "Disable On-Disk Shader Cache"}},
		{ DisableVulkanMemAllocator,  { "Video", "Disable Vulkan Memory Allocator"}},
		{ DisableAsyncShaderCompiler, { "Video", "Disable Asynchronous Shader Compiler"}},
		{ MultithreadedRSX,           { "Video", "Multithreaded RSX"}},
		{ RelaxedZCULL,               { "Video", "Relaxed ZCULL Sync"}},
		{ AnisotropicFilterOverride,  { "Video", "Anisotropic Filter Override"}},
		{ ResolutionScale,            { "Video", "Resolution Scale"}},
		{ MinimumScalableDimension,   { "Video", "Minimum Scalable Dimension"}},
		{ VulkanAdapter,              { "Video", "Vulkan", "Adapter"}},
		{ VBlankRate,                 { "Video", "Vblank Rate"}},
		{ DriverWakeUpDelay,          { "Video", "Driver Wake-Up Delay"}},

		// Performance Overlay
		{ PerfOverlayEnabled,               { "Video", "Performance Overlay", "Enabled" } },
		{ PerfOverlayFramerateGraphEnabled, { "Video", "Performance Overlay", "Enable Framerate Graph" } },
		{ PerfOverlayFrametimeGraphEnabled, { "Video", "Performance Overlay", "Enable Frametime Graph" } },
		{ PerfOverlayDetailLevel,           { "Video", "Performance Overlay", "Detail level" } },
		{ PerfOverlayPosition,              { "Video", "Performance Overlay", "Position" } },
		{ PerfOverlayUpdateInterval,        { "Video", "Performance Overlay", "Metrics update interval (ms)" } },
		{ PerfOverlayFontSize,              { "Video", "Performance Overlay", "Font size (px)" } },
		{ PerfOverlayOpacity,               { "Video", "Performance Overlay", "Opacity (%)" } },
		{ PerfOverlayMarginX,               { "Video", "Performance Overlay", "Horizontal Margin (px)" } },
		{ PerfOverlayMarginY,               { "Video", "Performance Overlay", "Vertical Margin (px)" } },
		{ PerfOverlayCenterX,               { "Video", "Performance Overlay", "Center Horizontally" } },
		{ PerfOverlayCenterY,               { "Video", "Performance Overlay", "Center Vertically" } },

		// Shader Loading Dialog
		{ ShaderLoadBgEnabled,      { "Video", "Shader Loading Dialog", "Allow custom background" } },
		{ ShaderLoadBgDarkening,    { "Video", "Shader Loading Dialog", "Darkening effect strength" } },
		{ ShaderLoadBgBlur,         { "Video", "Shader Loading Dialog", "Blur effect strength" } },

		// Audio
		{ AudioRenderer,           { "Audio", "Renderer"}},
		{ DumpToFile,              { "Audio", "Dump to file"}},
		{ ConvertTo16Bit,          { "Audio", "Convert to 16 bit"}},
		{ DownmixStereo,           { "Audio", "Downmix to Stereo"}},
		{ MasterVolume,            { "Audio", "Master Volume"}},
		{ EnableBuffering,         { "Audio", "Enable Buffering"}},
		{ AudioBufferDuration,     { "Audio", "Desired Audio Buffer Duration"}},
		{ EnableTimeStretching,    { "Audio", "Enable Time Stretching"}},
		{ TimeStretchingThreshold, { "Audio", "Time Stretching Threshold"}},
		{ MicrophoneType,          { "Audio", "Microphone Type" }},
		{ MicrophoneDevices,       { "Audio", "Microphone Devices" }},

		// Input / Output
		{ PadHandler,      { "Input/Output", "Pad"}},
		{ KeyboardHandler, { "Input/Output", "Keyboard"}},
		{ MouseHandler,    { "Input/Output", "Mouse"}},
		{ Camera,          { "Input/Output", "Camera"}},
		{ CameraType,      { "Input/Output", "Camera type"}},
		{ Move,            { "Input/Output", "Move" }},

		// Misc
		{ ExitRPCS3OnFinish,         { "Miscellaneous", "Exit RPCS3 when process finishes" }},
		{ StartOnBoot,               { "Miscellaneous", "Automatically start games after boot" }},
		{ StartGameFullscreen,       { "Miscellaneous", "Start games in fullscreen mode"}},
		{ PreventDisplaySleep,       { "Miscellaneous", "Prevent display sleep while running games"}},
		{ ShowFPSInTitle,            { "Miscellaneous", "Show FPS counter in window title"}},
		{ ShowTrophyPopups,          { "Miscellaneous", "Show trophy popups"}},
		{ ShowWelcomeScreen,         { "Miscellaneous", "Show Welcome Screen"}},
		{ UseNativeInterface,        { "Miscellaneous", "Use native user interface"}},
		{ ShowShaderCompilationHint, { "Miscellaneous", "Show shader compilation hint"}},
		{ SilenceAllLogs,            { "Miscellaneous", "Silence All Logs" }},

		// Networking
		{ ConnectionStatus, { "Net", "Connection status"}},

		// System
		{ Language,              { "System", "Language"}},
		{ KeyboardType,          { "System", "Keyboard Type"} },
		{ EnterButtonAssignment, { "System", "Enter button assignment"}},
		{ EnableHostRoot,        { "VFS", "Enable /host_root/"}},
		{ LimitCacheSize,        { "VFS", "Limit disk cache size"}},
		{ MaximumCacheSize,      { "VFS", "Disk cache maximum size (MB)"}},

		// Virtual File System
		{ emulatorLocation,   { "VFS", "$(EmulatorDir)"}},
		{ dev_hdd0Location,   { "VFS", "/dev_hdd0/" }},
		{ dev_hdd1Location,   { "VFS", "/dev_hdd1/" }},
		{ dev_flashLocation,  { "VFS", "/dev_flash/"}},
		{ dev_usb000Location, { "VFS", "/dev_usb000/"}},
	};

	YAML::Node m_defaultSettings; // The default settings as a YAML node.
	YAML::Node m_currentSettings; // The current settings as a YAML node.
	std::string m_title_id;
};

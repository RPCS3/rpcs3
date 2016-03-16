#include "stdafx.h"
#include "stdafx_gui.h"

#include "Emu/System.h"
#include "Emu/state.h"
#include "Emu/SysCalls/Modules/cellVideoOut.h"
#include "SettingsDialog.h"

#ifdef _WIN32
#include <windows.h>
#include <iphlpapi.h>
#pragma comment(lib, "iphlpapi.lib")

#undef GetHwnd
#ifdef _MSC_VER
#include <d3d12.h>
#include <wrl/client.h>
#include <dxgi1_4.h>
#endif
#else
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netdb.h>
#include <ifaddrs.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#endif

std::vector<std::string> GetAdapters()
{
	std::vector<std::string> adapters;
#ifdef _WIN32
	PIP_ADAPTER_INFO pAdapterInfo;
	pAdapterInfo = (IP_ADAPTER_INFO*)malloc(sizeof(IP_ADAPTER_INFO));
	ULONG buflen = sizeof(IP_ADAPTER_INFO);

	if (GetAdaptersInfo(pAdapterInfo, &buflen) == ERROR_BUFFER_OVERFLOW)
	{
		free(pAdapterInfo);
		pAdapterInfo = (IP_ADAPTER_INFO*)malloc(buflen);
	}

	if (GetAdaptersInfo(pAdapterInfo, &buflen) == NO_ERROR)
	{
		PIP_ADAPTER_INFO pAdapter = pAdapterInfo;
		while (pAdapter)
		{
			adapters.emplace_back(pAdapter->Description);
			pAdapter = pAdapter->Next;
		}
	}
	else
	{
		LOG_ERROR(HLE, "Call to GetAdaptersInfo failed.");
	}
#else
	struct ifaddrs *ifaddr, *ifa;
	int family, s, n;
	char host[NI_MAXHOST];

	if (getifaddrs(&ifaddr) == -1)
	{
		LOG_ERROR(HLE, "Call to getifaddrs returned negative.");
	}

	for (ifa = ifaddr, n = 0; ifa != NULL; ifa = ifa->ifa_next, n++)
	{
		if (ifa->ifa_addr == NULL)
		{
			continue;
		}

		family = ifa->ifa_addr->sa_family;

		if (family == AF_INET || family == AF_INET6)
		{
			adapters.emplace_back(ifa->ifa_name);
		}
	}

	freeifaddrs(ifaddr);
#endif

	return adapters;
}


SettingsDialog::SettingsDialog(wxWindow *parent, rpcs3::config_t* cfg)
	: wxDialog(parent, wxID_ANY, "Settings", wxDefaultPosition)
{
	const bool was_running = Emu.Pause();
	if (was_running || Emu.IsReady()) cfg = &rpcs3::state.config;

	static const u32 width = 458;
	static const u32 height = 400;

	// Settings panels
	wxNotebook* nb_config = new wxNotebook(this, wxID_ANY, wxPoint(6, 6), wxSize(width, height));
	wxPanel* p_system = new wxPanel(nb_config, wxID_ANY);
	wxPanel* p_core = new wxPanel(nb_config, wxID_ANY);
	wxPanel* p_graphics = new wxPanel(nb_config, wxID_ANY);
	wxPanel* p_audio = new wxPanel(nb_config, wxID_ANY);
	wxPanel* p_io = new wxPanel(nb_config, wxID_ANY);
	wxPanel* p_misc = new wxPanel(nb_config, wxID_ANY);
	wxPanel* p_networking = new wxPanel(nb_config, wxID_ANY);

	nb_config->AddPage(p_core, wxT("Core"));
	nb_config->AddPage(p_graphics, wxT("Graphics"));
	nb_config->AddPage(p_audio, wxT("Audio"));
	nb_config->AddPage(p_io, wxT("Input / Output"));
	nb_config->AddPage(p_misc, wxT("Miscellaneous"));
	nb_config->AddPage(p_networking, wxT("Networking"));
	nb_config->AddPage(p_system, wxT("System"));

	wxBoxSizer* s_subpanel_core = new wxBoxSizer(wxHORIZONTAL);
	wxBoxSizer* s_subpanel_core1 = new wxBoxSizer(wxVERTICAL);
	wxBoxSizer* s_subpanel_core2 = new wxBoxSizer(wxVERTICAL);
	wxBoxSizer* s_subpanel_graphics = new wxBoxSizer(wxHORIZONTAL);
	wxBoxSizer* s_subpanel_graphics1 = new wxBoxSizer(wxVERTICAL);
	wxBoxSizer* s_subpanel_graphics2 = new wxBoxSizer(wxVERTICAL);
	wxBoxSizer* s_subpanel_audio = new wxBoxSizer(wxVERTICAL);
	wxBoxSizer* s_subpanel_io = new wxBoxSizer(wxHORIZONTAL);
	wxBoxSizer* s_subpanel_io1 = new wxBoxSizer(wxVERTICAL);
	wxBoxSizer* s_subpanel_io2 = new wxBoxSizer(wxVERTICAL);

	wxBoxSizer* s_subpanel_system = new wxBoxSizer(wxVERTICAL);
	wxBoxSizer* s_subpanel_misc = new wxBoxSizer(wxVERTICAL);
	wxBoxSizer* s_subpanel_networking = new wxBoxSizer(wxVERTICAL);

	// Core settings
	wxStaticBoxSizer* s_round_llvm = new wxStaticBoxSizer(wxVERTICAL, p_core, _("LLVM config"));
	wxStaticBoxSizer* s_round_llvm_range = new wxStaticBoxSizer(wxHORIZONTAL, p_core, _("Excluded block range"));
	wxStaticBoxSizer* s_round_llvm_threshold = new wxStaticBoxSizer(wxHORIZONTAL, p_core, _("Compilation threshold"));

	// Graphics
	wxStaticBoxSizer* s_round_gs_render = new wxStaticBoxSizer(wxVERTICAL, p_graphics, _("Render"));
	wxStaticBoxSizer* s_round_gs_d3d_adaptater = new wxStaticBoxSizer(wxVERTICAL, p_graphics, _("D3D Adaptater"));
	wxStaticBoxSizer* s_round_gs_res = new wxStaticBoxSizer(wxVERTICAL, p_graphics, _("Resolution"));
	wxStaticBoxSizer* s_round_gs_aspect = new wxStaticBoxSizer(wxVERTICAL, p_graphics, _("Aspect ratio"));
	wxStaticBoxSizer* s_round_gs_frame_limit = new wxStaticBoxSizer(wxVERTICAL, p_graphics, _("Frame limit"));

	// Input / Output
	wxStaticBoxSizer* s_round_io_pad_handler = new wxStaticBoxSizer(wxVERTICAL, p_io, _("Pad Handler"));
	wxStaticBoxSizer* s_round_io_keyboard_handler = new wxStaticBoxSizer(wxVERTICAL, p_io, _("Keyboard Handler"));
	wxStaticBoxSizer* s_round_io_mouse_handler = new wxStaticBoxSizer(wxVERTICAL, p_io, _("Mouse Handler"));
	wxStaticBoxSizer* s_round_io_camera = new wxStaticBoxSizer(wxVERTICAL, p_io, _("Camera"));
	wxStaticBoxSizer* s_round_io_camera_type = new wxStaticBoxSizer(wxVERTICAL, p_io, _("Camera type"));

	// Audio
	wxStaticBoxSizer* s_round_audio_out = new wxStaticBoxSizer(wxVERTICAL, p_audio, _("Audio Out"));

	// Miscellaneous
	wxStaticBoxSizer* s_round_hle_log_lvl = new wxStaticBoxSizer(wxVERTICAL, p_misc, _("Log Level"));

	// Networking
	wxStaticBoxSizer* s_round_net_status = new wxStaticBoxSizer(wxVERTICAL, p_networking, _("Connection status"));
	wxStaticBoxSizer* s_round_net_interface = new wxStaticBoxSizer(wxVERTICAL, p_networking, _("Network adapter"));

	// System
	wxStaticBoxSizer* s_round_sys_lang = new wxStaticBoxSizer(wxVERTICAL, p_system, _("Language"));


	wxRadioBox* rbox_ppu_decoder;
	wxRadioBox* rbox_spu_decoder;
	wxComboBox* cbox_gs_render = new wxComboBox(p_graphics, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(150, -1), 0, NULL, wxCB_READONLY);
	wxComboBox* cbox_gs_d3d_adaptater = new wxComboBox(p_graphics, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(150, -1), 0, NULL, wxCB_READONLY);
	wxComboBox* cbox_gs_resolution = new wxComboBox(p_graphics, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(150, -1), 0, NULL, wxCB_READONLY);
	wxComboBox* cbox_gs_aspect = new wxComboBox(p_graphics, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(150, -1), 0, NULL, wxCB_READONLY);
	wxComboBox* cbox_gs_frame_limit = new wxComboBox(p_graphics, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(150, -1), 0, NULL, wxCB_READONLY);
	wxComboBox* cbox_pad_handler = new wxComboBox(p_io, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(150, -1), 0, NULL, wxCB_READONLY);;
	wxComboBox* cbox_keyboard_handler = new wxComboBox(p_io, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(150, -1), 0, NULL, wxCB_READONLY);
	wxComboBox* cbox_mouse_handler = new wxComboBox(p_io, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(150, -1), 0, NULL, wxCB_READONLY);
	wxComboBox* cbox_camera = new wxComboBox(p_io, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(150, -1), 0, NULL, wxCB_READONLY);
	wxComboBox* cbox_camera_type = new wxComboBox(p_io, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(150, -1), 0, NULL, wxCB_READONLY);
	wxComboBox* cbox_audio_out = new wxComboBox(p_audio, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(150, -1), 0, NULL, wxCB_READONLY);
	wxComboBox* cbox_hle_loglvl = new wxComboBox(p_misc, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(150, -1), 0, NULL, wxCB_READONLY);
	wxComboBox* cbox_net_status = new wxComboBox(p_networking, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0, NULL, wxCB_READONLY);
	wxComboBox* cbox_net_interface = new wxComboBox(p_networking, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0, NULL, wxCB_READONLY);
	wxComboBox* cbox_sys_lang = new wxComboBox(p_system, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0, NULL, wxCB_READONLY);

	wxCheckBox* chbox_core_llvm_exclud = new wxCheckBox(p_core, wxID_ANY, "Compiled blocks exclusion");
	wxCheckBox* chbox_core_hook_stfunc = new wxCheckBox(p_core, wxID_ANY, "Hook static functions");
	wxCheckBox* chbox_core_load_liblv2 = new wxCheckBox(p_core, wxID_ANY, "Load liblv2.sprx");
	wxCheckBox* chbox_gs_log_prog = new wxCheckBox(p_graphics, wxID_ANY, "Log shader programs");
	wxCheckBox* chbox_gs_dump_depth = new wxCheckBox(p_graphics, wxID_ANY, "Write Depth Buffer");
	wxCheckBox* chbox_gs_dump_color = new wxCheckBox(p_graphics, wxID_ANY, "Write Color Buffers");
	wxCheckBox* chbox_gs_read_color = new wxCheckBox(p_graphics, wxID_ANY, "Read Color Buffers");
	wxCheckBox* chbox_gs_read_depth = new wxCheckBox(p_graphics, wxID_ANY, "Read Depth Buffer");
	wxCheckBox* chbox_gs_vsync = new wxCheckBox(p_graphics, wxID_ANY, "VSync");
	wxCheckBox* chbox_gs_debug_output = new wxCheckBox(p_graphics, wxID_ANY, "Debug Output");
	wxCheckBox* chbox_gs_3dmonitor = new wxCheckBox(p_graphics, wxID_ANY, "3D Monitor");
	wxCheckBox* chbox_gs_overlay = new wxCheckBox(p_graphics, wxID_ANY, "Debug overlay");
	wxCheckBox* chbox_audio_dump = new wxCheckBox(p_audio, wxID_ANY, "Dump to file");
	wxCheckBox* chbox_audio_conv = new wxCheckBox(p_audio, wxID_ANY, "Convert to 16 bit");
	wxCheckBox* chbox_rsx_logging = new wxCheckBox(p_misc, wxID_ANY, "RSX Logging");
	wxCheckBox* chbox_hle_exitonstop = new wxCheckBox(p_misc, wxID_ANY, "Exit RPCS3 when process finishes");
	wxCheckBox* chbox_hle_always_start = new wxCheckBox(p_misc, wxID_ANY, "Always start after boot");
	wxCheckBox* chbox_hle_use_default_ini = new wxCheckBox(p_misc, wxID_ANY, "Use default configuration");

	wxTextCtrl* txt_dbg_range_min = new wxTextCtrl(p_core, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(55, 20));
	wxTextCtrl* txt_dbg_range_max = new wxTextCtrl(p_core, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(55, 20));
	wxTextCtrl* txt_llvm_threshold = new wxTextCtrl(p_core, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(55, 20));

	//Auto Pause
	wxCheckBox* chbox_dbg_ap_systemcall = new wxCheckBox(p_misc, wxID_ANY, "Auto Pause at System Call");
	wxCheckBox* chbox_dbg_ap_functioncall = new wxCheckBox(p_misc, wxID_ANY, "Auto Pause at Function Call");

	//Custom EmulationDir
	wxCheckBox* chbox_emulationdir_enable = new wxCheckBox(p_system, wxID_ANY, "Use path below as EmulationDir. (Restart required)");
	wxTextCtrl* txt_emulationdir_path = new wxTextCtrl(p_system, wxID_ANY, fs::get_executable_dir());


	wxArrayString ppu_decoder_modes;
	ppu_decoder_modes.Add("Interpreter");
	ppu_decoder_modes.Add("Interpreter 2");
	ppu_decoder_modes.Add("Recompiler (LLVM)");
	rbox_ppu_decoder = new wxRadioBox(p_core, wxID_ANY, "PPU Decoder", wxDefaultPosition, wxSize(215, -1), ppu_decoder_modes, 1);

#if !defined(LLVM_AVAILABLE)
	rbox_ppu_decoder->Enable(2, false);
#endif

	wxArrayString spu_decoder_modes;
	spu_decoder_modes.Add("Interpreter (precise)");
	spu_decoder_modes.Add("Interpreter (fast)");
	spu_decoder_modes.Add("Recompiler (ASMJIT)");
	rbox_spu_decoder = new wxRadioBox(p_core, wxID_ANY, "SPU Decoder", wxDefaultPosition, wxSize(215, -1), spu_decoder_modes, 1);

	cbox_gs_render->Append("Null");
	cbox_gs_render->Append("OpenGL");
	cbox_gs_render->Append("Vulkan");

#ifdef _MSC_VER
	Microsoft::WRL::ComPtr<IDXGIFactory4> dxgiFactory;
	Microsoft::WRL::ComPtr<IDXGIAdapter> adapter;

	if (SUCCEEDED(CreateDXGIFactory(IID_PPV_ARGS(&dxgiFactory))))
	{
		cbox_gs_render->Append("DirectX 12");

		for (uint id = 0; dxgiFactory->EnumAdapters(id, adapter.GetAddressOf()) != DXGI_ERROR_NOT_FOUND; id++)
		{
			DXGI_ADAPTER_DESC adapterDesc;
			adapter->GetDesc(&adapterDesc);
			cbox_gs_d3d_adaptater->Append(adapterDesc.Description);
		}
	}
	else
	{
		cbox_gs_d3d_adaptater->Enable(false);
		chbox_gs_overlay->Enable(false);
	}

#endif

	for (int i = 1; i < WXSIZEOF(ResolutionTable); ++i)
	{
		cbox_gs_resolution->Append(wxString::Format("%dx%d", ResolutionTable[i].width.value(), ResolutionTable[i].height.value()));
	}

	cbox_gs_aspect->Append("4:3");
	cbox_gs_aspect->Append("16:9");

	for (auto item : { "Off", "50", "59.94", "30", "60", "Auto" })
		cbox_gs_frame_limit->Append(item);

	cbox_pad_handler->Append("Null");
	cbox_pad_handler->Append("Windows");
#ifdef _MSC_VER
	cbox_pad_handler->Append("XInput");
#endif

	//cbox_pad_handler->Append("DirectInput");

	cbox_keyboard_handler->Append("Null");
	cbox_keyboard_handler->Append("Windows");
	//cbox_keyboard_handler->Append("DirectInput");

	cbox_mouse_handler->Append("Null");
	cbox_mouse_handler->Append("Windows");
	//cbox_mouse_handler->Append("DirectInput");

	cbox_audio_out->Append("Null");
	cbox_audio_out->Append("OpenAL");
#ifdef _MSC_VER
	cbox_audio_out->Append("XAudio2");
#endif

	cbox_camera->Append("Null");
	cbox_camera->Append("Connected");

	cbox_camera_type->Append("Unknown");
	cbox_camera_type->Append("EyeToy");
	cbox_camera_type->Append("PlayStation Eye");
	cbox_camera_type->Append("USB Video Class 1.1");

	cbox_hle_loglvl->Append("Nothing");
	cbox_hle_loglvl->Append("Fatal");
	cbox_hle_loglvl->Append("Error");
	cbox_hle_loglvl->Append("TODO");
	cbox_hle_loglvl->Append("Success");
	cbox_hle_loglvl->Append("Warning");
	cbox_hle_loglvl->Append("Notice");
	cbox_hle_loglvl->Append("All");

	cbox_net_status->Append("IP Obtained");
	cbox_net_status->Append("Obtaining IP");
	cbox_net_status->Append("Connecting");
	cbox_net_status->Append("Disconnected");

	for(const auto& adapterName : GetAdapters())
		cbox_net_interface->Append(adapterName);

	static wxString s_langs[] =
	{
		"Japanese", "English (US)", "French", "Spanish", "German",
		"Italian", "Dutch", "Portuguese (PT)", "Russian",
		"Korean", "Chinese (Trad.)", "Chinese (Simp.)", "Finnish",
		"Swedish", "Danish", "Norwegian", "Polish", "English (UK)"
	};

	for (const auto& lang : s_langs)
		cbox_sys_lang->Append(lang);

	chbox_core_llvm_exclud->SetValue(cfg->core.llvm.exclusion_range.value());
	chbox_gs_log_prog->SetValue(rpcs3::config.rsx.log_programs.value());
	chbox_gs_dump_depth->SetValue(cfg->rsx.opengl.write_depth_buffer.value());
	chbox_gs_dump_color->SetValue(cfg->rsx.opengl.write_color_buffers.value());
	chbox_gs_read_color->SetValue(cfg->rsx.opengl.read_color_buffers.value());
	chbox_gs_read_depth->SetValue(cfg->rsx.opengl.read_depth_buffer.value());
	chbox_gs_vsync->SetValue(rpcs3::config.rsx.vsync.value());
	chbox_gs_debug_output->SetValue(cfg->rsx.d3d12.debug_output.value());
	chbox_gs_3dmonitor->SetValue(rpcs3::config.rsx._3dtv.value());
	chbox_gs_overlay->SetValue(cfg->rsx.d3d12.overlay.value());
	chbox_audio_dump->SetValue(rpcs3::config.audio.dump_to_file.value());
	chbox_audio_conv->SetValue(rpcs3::config.audio.convert_to_u16.value());
	chbox_rsx_logging->SetValue(rpcs3::config.misc.log.rsx_logging.value());
	chbox_hle_exitonstop->SetValue(rpcs3::config.misc.exit_on_stop.value());
	chbox_hle_always_start->SetValue(rpcs3::config.misc.always_start.value());
	chbox_hle_use_default_ini->SetValue(rpcs3::config.misc.use_default_ini.value());
	chbox_core_hook_stfunc->SetValue(cfg->core.hook_st_func.value());
	chbox_core_load_liblv2->SetValue(cfg->core.load_liblv2.value());

	//Auto Pause related
	chbox_dbg_ap_systemcall->SetValue(rpcs3::config.misc.debug.auto_pause_syscall.value());
	chbox_dbg_ap_functioncall->SetValue(rpcs3::config.misc.debug.auto_pause_func_call.value());

	//Custom EmulationDir
	chbox_emulationdir_enable->SetValue(rpcs3::config.system.emulation_dir_path_enable.value());
	txt_emulationdir_path->SetValue(rpcs3::config.system.emulation_dir_path.value());

	rbox_ppu_decoder->SetSelection((int)cfg->core.ppu_decoder.value());
	txt_dbg_range_min->SetValue(cfg->core.llvm.min_id.string_value());
	txt_dbg_range_max->SetValue(cfg->core.llvm.max_id.string_value());
	txt_llvm_threshold->SetValue(cfg->core.llvm.threshold.string_value());
	rbox_spu_decoder->SetSelection((int)cfg->core.spu_decoder.value());
	cbox_gs_render->SetSelection((int)cfg->rsx.renderer.value());
	cbox_gs_d3d_adaptater->SetSelection(cfg->rsx.d3d12.adaptater.value());
	cbox_gs_resolution->SetSelection(ResolutionIdToNum((int)cfg->rsx.resolution.value()) - 1);
	cbox_gs_aspect->SetSelection((int)cfg->rsx.aspect_ratio.value() - 1);
	cbox_gs_frame_limit->SetSelection((int)cfg->rsx.frame_limit.value());
	cbox_pad_handler->SetSelection((int)cfg->io.pad_handler_mode.value());
	cbox_keyboard_handler->SetSelection((int)cfg->io.keyboard_handler_mode.value());
	cbox_mouse_handler->SetSelection((int)cfg->io.mouse_handler_mode.value());
	cbox_audio_out->SetSelection((int)cfg->audio.out.value());
	cbox_camera->SetSelection((int)cfg->io.camera.value());
	cbox_camera_type->SetSelection((int)cfg->io.camera_type.value());
	cbox_hle_loglvl->SetSelection((int)rpcs3::config.misc.log.level.value());
	cbox_net_status->SetSelection((int)rpcs3::config.misc.net.status.value());
	cbox_net_interface->SetSelection((int)rpcs3::config.misc.net._interface.value());
	cbox_sys_lang->SetSelection((int)rpcs3::config.system.language.value());

	// Core
	s_round_llvm->Add(chbox_core_llvm_exclud, wxSizerFlags().Border(wxALL, 5).Expand());
	s_round_llvm_range->Add(txt_dbg_range_min, wxSizerFlags().Border(wxALL, 5).Expand());
	s_round_llvm_range->Add(txt_dbg_range_max, wxSizerFlags().Border(wxALL, 5).Expand());
	s_round_llvm->Add(s_round_llvm_range, wxSizerFlags().Border(wxALL, 5).Expand());
	s_round_llvm_threshold->Add(txt_llvm_threshold, wxSizerFlags().Border(wxALL, 5).Expand());
	s_round_llvm->Add(s_round_llvm_threshold, wxSizerFlags().Border(wxALL, 5).Expand());

	// Rendering
	s_round_gs_render->Add(cbox_gs_render, wxSizerFlags().Border(wxALL, 5).Expand());
	s_round_gs_d3d_adaptater->Add(cbox_gs_d3d_adaptater, wxSizerFlags().Border(wxALL, 5).Expand());
	s_round_gs_res->Add(cbox_gs_resolution, wxSizerFlags().Border(wxALL, 5).Expand());
	s_round_gs_aspect->Add(cbox_gs_aspect, wxSizerFlags().Border(wxALL, 5).Expand());
	s_round_gs_frame_limit->Add(cbox_gs_frame_limit, wxSizerFlags().Border(wxALL, 5).Expand());

	// Input/Output
	s_round_io_pad_handler->Add(cbox_pad_handler, wxSizerFlags().Border(wxALL, 5).Expand());
	s_round_io_keyboard_handler->Add(cbox_keyboard_handler, wxSizerFlags().Border(wxALL, 5).Expand());
	s_round_io_mouse_handler->Add(cbox_mouse_handler, wxSizerFlags().Border(wxALL, 5).Expand());
	s_round_io_camera->Add(cbox_camera, wxSizerFlags().Border(wxALL, 5).Expand());
	s_round_io_camera_type->Add(cbox_camera_type, wxSizerFlags().Border(wxALL, 5).Expand());

	s_round_audio_out->Add(cbox_audio_out, wxSizerFlags().Border(wxALL, 5).Expand());

	// Miscellaneous
	s_round_hle_log_lvl->Add(cbox_hle_loglvl, wxSizerFlags().Border(wxALL, 5).Expand());

	// Networking
	s_round_net_status->Add(cbox_net_status, wxSizerFlags().Border(wxALL, 5).Expand());
	s_round_net_interface->Add(cbox_net_interface, wxSizerFlags().Border(wxALL, 5).Expand());

	s_round_sys_lang->Add(cbox_sys_lang, wxSizerFlags().Border(wxALL, 5).Expand());

	// Core
	s_subpanel_core1->Add(rbox_ppu_decoder, wxSizerFlags().Border(wxALL, 5).Expand());
	s_subpanel_core2->Add(rbox_spu_decoder, wxSizerFlags().Border(wxALL, 5).Expand());
	s_subpanel_core1->Add(s_round_llvm, wxSizerFlags().Border(wxALL, 5).Expand());
	s_subpanel_core1->Add(chbox_core_hook_stfunc, wxSizerFlags().Border(wxALL, 5).Expand());
	s_subpanel_core1->Add(chbox_core_load_liblv2, wxSizerFlags().Border(wxALL, 5).Expand());
	s_subpanel_core->Add(s_subpanel_core1);
	s_subpanel_core->Add(s_subpanel_core2);

	// Graphics
	s_subpanel_graphics1->Add(s_round_gs_render, wxSizerFlags().Border(wxALL, 5).Expand());
	s_subpanel_graphics1->Add(s_round_gs_res, wxSizerFlags().Border(wxALL, 5).Expand());
	s_subpanel_graphics1->Add(s_round_gs_d3d_adaptater, wxSizerFlags().Border(wxALL, 5).Expand());
	s_subpanel_graphics1->Add(chbox_gs_dump_color, wxSizerFlags().Border(wxALL, 5).Expand());
	s_subpanel_graphics1->Add(chbox_gs_read_color, wxSizerFlags().Border(wxALL, 5).Expand());
	s_subpanel_graphics1->Add(chbox_gs_dump_depth, wxSizerFlags().Border(wxALL, 5).Expand());
	s_subpanel_graphics1->Add(chbox_gs_read_depth, wxSizerFlags().Border(wxALL, 5).Expand());
	s_subpanel_graphics1->Add(chbox_gs_vsync, wxSizerFlags().Border(wxALL, 5).Expand());
	s_subpanel_graphics2->Add(s_round_gs_aspect, wxSizerFlags().Border(wxALL, 5).Expand());
	s_subpanel_graphics2->Add(s_round_gs_frame_limit, wxSizerFlags().Border(wxALL, 5).Expand());
	s_subpanel_graphics2->AddSpacer(68);
	s_subpanel_graphics2->Add(chbox_gs_debug_output, wxSizerFlags().Border(wxALL, 5).Expand());
	s_subpanel_graphics2->Add(chbox_gs_3dmonitor, wxSizerFlags().Border(wxALL, 5).Expand());
	s_subpanel_graphics2->Add(chbox_gs_overlay, wxSizerFlags().Border(wxALL, 5).Expand());
	s_subpanel_graphics2->Add(chbox_gs_log_prog, wxSizerFlags().Border(wxALL, 5).Expand());
	s_subpanel_graphics->Add(s_subpanel_graphics1);
	s_subpanel_graphics->Add(s_subpanel_graphics2);

	// Input - Output
	s_subpanel_io1->Add(s_round_io_pad_handler, wxSizerFlags().Border(wxALL, 5).Expand());
	s_subpanel_io1->Add(s_round_io_keyboard_handler, wxSizerFlags().Border(wxALL, 5).Expand());
	s_subpanel_io1->Add(s_round_io_mouse_handler, wxSizerFlags().Border(wxALL, 5).Expand());
	s_subpanel_io2->Add(s_round_io_camera, wxSizerFlags().Border(wxALL, 5).Expand());
	s_subpanel_io2->Add(s_round_io_camera_type, wxSizerFlags().Border(wxALL, 5).Expand());
	s_subpanel_io->Add(s_subpanel_io1);
	s_subpanel_io->Add(s_subpanel_io2);

	// Audio
	s_subpanel_audio->Add(s_round_audio_out, wxSizerFlags().Border(wxALL, 5).Expand());
	s_subpanel_audio->Add(chbox_audio_dump, wxSizerFlags().Border(wxALL, 5).Expand());
	s_subpanel_audio->Add(chbox_audio_conv, wxSizerFlags().Border(wxALL, 5).Expand());

	// Miscellaneous
	s_subpanel_misc->Add(s_round_hle_log_lvl, wxSizerFlags().Border(wxALL, 5).Expand());
	s_subpanel_misc->Add(chbox_rsx_logging, wxSizerFlags().Border(wxALL, 5).Expand());
	s_subpanel_misc->Add(chbox_hle_exitonstop, wxSizerFlags().Border(wxALL, 5).Expand());
	s_subpanel_misc->Add(chbox_hle_always_start, wxSizerFlags().Border(wxALL, 5).Expand());
	s_subpanel_misc->Add(chbox_hle_use_default_ini, wxSizerFlags().Border(wxALL, 5).Expand());

	// Auto Pause
	s_subpanel_misc->Add(chbox_dbg_ap_systemcall, wxSizerFlags().Border(wxALL, 5).Expand());
	s_subpanel_misc->Add(chbox_dbg_ap_functioncall, wxSizerFlags().Border(wxALL, 5).Expand());

	// Networking
	s_subpanel_networking->Add(s_round_net_status, wxSizerFlags().Border(wxALL, 5).Expand());
	s_subpanel_networking->Add(s_round_net_interface, wxSizerFlags().Border(wxALL, 5).Expand());

	// System
	s_subpanel_system->Add(s_round_sys_lang, wxSizerFlags().Border(wxALL, 5).Expand());

	// Custom EmulationDir
	s_subpanel_system->Add(chbox_emulationdir_enable, wxSizerFlags().Border(wxALL, 5).Expand());
	s_subpanel_system->Add(txt_emulationdir_path, wxSizerFlags().Border(wxALL, 5).Expand());

	// Buttons
	wxBoxSizer* s_b_panel(new wxBoxSizer(wxHORIZONTAL));
	s_b_panel->Add(new wxButton(this, wxID_OK), wxSizerFlags().Border(wxALL, 5).Bottom());
	s_b_panel->Add(new wxButton(this, wxID_CANCEL), wxSizerFlags().Border(wxALL, 5).Bottom());

	// Resize panels 
	SetSizerAndFit(s_subpanel_core, false);
	SetSizerAndFit(s_subpanel_graphics, false);
	SetSizerAndFit(s_subpanel_io, false);
	SetSizerAndFit(s_subpanel_audio, false);
	SetSizerAndFit(s_subpanel_misc, false);
	SetSizerAndFit(s_subpanel_networking, false);
	SetSizerAndFit(s_subpanel_system, false);
	SetSizerAndFit(s_b_panel, false);

	SetSize(width + 26, height + 80);

	if (ShowModal() == wxID_OK)
	{
		long llvmthreshold;
		long minllvmid, maxllvmid;
		txt_dbg_range_min->GetValue().ToLong(&minllvmid);
		txt_dbg_range_max->GetValue().ToLong(&maxllvmid);
		txt_llvm_threshold->GetValue().ToLong(&llvmthreshold);

		// individual settings
		cfg->core.ppu_decoder = rbox_ppu_decoder->GetSelection();
		cfg->core.llvm.exclusion_range = chbox_core_llvm_exclud->GetValue();
		cfg->core.llvm.min_id = minllvmid;
		cfg->core.llvm.max_id = maxllvmid;
		cfg->core.llvm.threshold = llvmthreshold;
		cfg->core.spu_decoder = rbox_spu_decoder->GetSelection();
		cfg->core.hook_st_func = chbox_core_hook_stfunc->GetValue();
		cfg->core.load_liblv2 = chbox_core_load_liblv2->GetValue();

		// Translates renderer string to enum class for config.h
		if (cbox_gs_render->GetString(cbox_gs_render->GetSelection()) == "Null")
			cfg->rsx.renderer = rsx_renderer_type::Null;
		if (cbox_gs_render->GetString(cbox_gs_render->GetSelection()) == "OpenGL")
			cfg->rsx.renderer = rsx_renderer_type::OpenGL;
		if (cbox_gs_render->GetString(cbox_gs_render->GetSelection()) == "Vulkan")
			cfg->rsx.renderer = rsx_renderer_type::Vulkan;
		if (cbox_gs_render->GetString(cbox_gs_render->GetSelection()) == "DirectX 12")
			cfg->rsx.renderer = rsx_renderer_type::DX12;
		
		cfg->rsx.d3d12.adaptater = cbox_gs_d3d_adaptater->GetSelection();
		cfg->rsx.resolution = ResolutionNumToId(cbox_gs_resolution->GetSelection() + 1);
		cfg->rsx.aspect_ratio = cbox_gs_aspect->GetSelection() + 1;
		cfg->rsx.frame_limit = cbox_gs_frame_limit->GetSelection();
		cfg->rsx.opengl.write_depth_buffer = chbox_gs_dump_depth->GetValue();
		cfg->rsx.opengl.write_color_buffers = chbox_gs_dump_color->GetValue();
		cfg->rsx.opengl.read_color_buffers = chbox_gs_read_color->GetValue();
		cfg->rsx.opengl.read_depth_buffer = chbox_gs_read_depth->GetValue();

		cfg->audio.out = cbox_audio_out->GetSelection();

		cfg->io.pad_handler_mode = cbox_pad_handler->GetSelection();
		cfg->io.keyboard_handler_mode = cbox_keyboard_handler->GetSelection();
		cfg->io.mouse_handler_mode = cbox_mouse_handler->GetSelection();
		cfg->io.camera = cbox_camera->GetSelection();
		cfg->io.camera_type = cbox_camera_type->GetSelection();

		
		// global settings
		rpcs3::config.rsx.log_programs = chbox_gs_log_prog->GetValue();
		rpcs3::config.rsx.vsync = chbox_gs_vsync->GetValue();
		rpcs3::config.rsx._3dtv = chbox_gs_3dmonitor->GetValue();
		rpcs3::config.rsx.d3d12.debug_output = chbox_gs_debug_output->GetValue();
		rpcs3::config.rsx.d3d12.overlay = chbox_gs_overlay->GetValue();
		rpcs3::config.audio.dump_to_file = chbox_audio_dump->GetValue();
		rpcs3::config.audio.convert_to_u16 = chbox_audio_conv->GetValue();
		rpcs3::config.misc.log.level = cbox_hle_loglvl->GetSelection();
		rpcs3::config.misc.log.rsx_logging = chbox_rsx_logging->GetValue();
		rpcs3::config.misc.net.status = cbox_net_status->GetSelection();
		rpcs3::config.misc.net._interface = cbox_net_interface->GetSelection();
		rpcs3::config.misc.debug.auto_pause_syscall = chbox_dbg_ap_systemcall->GetValue();
		rpcs3::config.misc.debug.auto_pause_func_call = chbox_dbg_ap_functioncall->GetValue();
		rpcs3::config.misc.always_start = chbox_hle_always_start->GetValue();
		rpcs3::config.misc.exit_on_stop = chbox_hle_exitonstop->GetValue();
		rpcs3::config.misc.use_default_ini = chbox_hle_use_default_ini->GetValue();
		rpcs3::config.system.language = cbox_sys_lang->GetSelection();
		rpcs3::config.system.emulation_dir_path_enable = chbox_emulationdir_enable->GetValue();
		rpcs3::config.system.emulation_dir_path = txt_emulationdir_path->GetValue().ToStdString();
		rpcs3::config.save();

		cfg->save();
	}

	if (was_running) Emu.Resume();
}

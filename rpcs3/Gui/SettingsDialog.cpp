#include "stdafx_gui.h"

#include "Ini.h"
#include "Emu/System.h"
#include "Emu/SysCalls/Modules/cellVideoOut.h"
#include "SettingsDialog.h"
#include "Utilities/Log.h"

SettingsDialog::SettingsDialog(wxWindow *parent)
	: wxDialog(parent, wxID_ANY, "Settings", wxDefaultPosition)
{
	bool paused = false;

	if (Emu.IsRunning())
	{
		Emu.Pause();
		paused = true;
	}

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
	wxCheckBox* chbox_gs_read_color = new wxCheckBox(p_graphics, wxID_ANY, "Read Color Buffer");
	wxCheckBox* chbox_gs_vsync = new wxCheckBox(p_graphics, wxID_ANY, "VSync");
	wxCheckBox* chbox_gs_debug_output = new wxCheckBox(p_graphics, wxID_ANY, "Debug Output");
	wxCheckBox* chbox_gs_3dmonitor = new wxCheckBox(p_graphics, wxID_ANY, "3D Monitor");
	wxCheckBox* chbox_gs_overlay = new wxCheckBox(p_graphics, wxID_ANY, "Debug overlay");
	wxCheckBox* chbox_audio_dump = new wxCheckBox(p_audio, wxID_ANY, "Dump to file");
	wxCheckBox* chbox_audio_conv = new wxCheckBox(p_audio, wxID_ANY, "Convert to 16 bit");
	wxCheckBox* chbox_hle_logging = new wxCheckBox(p_misc, wxID_ANY, "Log everything");
	wxCheckBox* chbox_rsx_logging = new wxCheckBox(p_misc, wxID_ANY, "RSX Logging");
	wxCheckBox* chbox_hle_savetty = new wxCheckBox(p_misc, wxID_ANY, "Save TTY output to file");
	wxCheckBox* chbox_hle_exitonstop = new wxCheckBox(p_misc, wxID_ANY, "Exit RPCS3 when process finishes");
	wxCheckBox* chbox_hle_always_start = new wxCheckBox(p_misc, wxID_ANY, "Always start after boot");

	wxTextCtrl* txt_dbg_range_min = new wxTextCtrl(p_core, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(55, 20));
	wxTextCtrl* txt_dbg_range_max = new wxTextCtrl(p_core, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(55, 20));
	wxTextCtrl* txt_llvm_threshold = new wxTextCtrl(p_core, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(55, 20));

	//Auto Pause
	wxCheckBox* chbox_dbg_ap_systemcall = new wxCheckBox(p_misc, wxID_ANY, "Auto Pause at System Call");
	wxCheckBox* chbox_dbg_ap_functioncall = new wxCheckBox(p_misc, wxID_ANY, "Auto Pause at Function Call");

	//Custom EmulationDir
	wxCheckBox* chbox_emulationdir_enable = new wxCheckBox(p_system, wxID_ANY, "Use Path Below as EmulationDir ? (Need Restart)");
	wxTextCtrl* txt_emulationdir_path = new wxTextCtrl(p_system, wxID_ANY, Emu.GetEmulatorPath());


	wxArrayString ppu_decoder_modes;
	ppu_decoder_modes.Add("Interpreter");
	ppu_decoder_modes.Add("Interpreter 2");
	ppu_decoder_modes.Add("Recompiler (LLVM)");
	rbox_ppu_decoder = new wxRadioBox(p_core, wxID_ANY, "PPU Decoder", wxDefaultPosition, wxSize(215, -1), ppu_decoder_modes, 1);
#if !defined(LLVM_AVAILABLE)
	rbox_ppu_decoder->Enable(2, false);
#endif

	wxArrayString spu_decoder_modes;
	spu_decoder_modes.Add("Interpreter");
	spu_decoder_modes.Add("Interpreter 2");
	spu_decoder_modes.Add("Recompiler (ASMJIT)");
	rbox_spu_decoder = new wxRadioBox(p_core, wxID_ANY, "SPU Decoder", wxDefaultPosition, wxSize(215, -1), spu_decoder_modes, 1);

	cbox_gs_render->Append("Null");
	cbox_gs_render->Append("OpenGL");
#if defined(DX12_SUPPORT)
	cbox_gs_render->Append("DirectX 12");
#endif

	cbox_gs_d3d_adaptater->Append("WARP");
	cbox_gs_d3d_adaptater->Append("default");
	cbox_gs_d3d_adaptater->Append("renderer 0");
	cbox_gs_d3d_adaptater->Append("renderer 1");
	cbox_gs_d3d_adaptater->Append("renderer 2");

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
#if defined (_WIN32)
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
#if defined (_WIN32)
	cbox_audio_out->Append("XAudio2");
#endif

	cbox_camera->Append("Null");
	cbox_camera->Append("Connected");

	cbox_camera_type->Append("Unknown");
	cbox_camera_type->Append("EyeToy");
	cbox_camera_type->Append("PlayStation Eye");
	cbox_camera_type->Append("USB Video Class 1.1");

	cbox_hle_loglvl->Append("All");
	cbox_hle_loglvl->Append("Warnings");
	cbox_hle_loglvl->Append("Success");
	cbox_hle_loglvl->Append("Errors");
	cbox_hle_loglvl->Append("Nothing");

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

	// Get values from .ini
	chbox_core_llvm_exclud->SetValue(Ini.LLVMExclusionRange.GetValue());
	chbox_gs_log_prog->SetValue(Ini.GSLogPrograms.GetValue());
	chbox_gs_dump_depth->SetValue(Ini.GSDumpDepthBuffer.GetValue());
	chbox_gs_dump_color->SetValue(Ini.GSDumpColorBuffers.GetValue());
	chbox_gs_read_color->SetValue(Ini.GSReadColorBuffer.GetValue());
	chbox_gs_vsync->SetValue(Ini.GSVSyncEnable.GetValue());
	chbox_gs_debug_output->SetValue(Ini.GSDebugOutputEnable.GetValue());
	chbox_gs_3dmonitor->SetValue(Ini.GS3DTV.GetValue());
	chbox_gs_overlay->SetValue(Ini.GSOverlay.GetValue());
	chbox_audio_dump->SetValue(Ini.AudioDumpToFile.GetValue());
	chbox_audio_conv->SetValue(Ini.AudioConvertToU16.GetValue());
	chbox_hle_logging->SetValue(Ini.HLELogging.GetValue());
	chbox_rsx_logging->SetValue(Ini.RSXLogging.GetValue());
	chbox_hle_savetty->SetValue(Ini.HLESaveTTY.GetValue());
	chbox_hle_exitonstop->SetValue(Ini.HLEExitOnStop.GetValue());
	chbox_hle_always_start->SetValue(Ini.HLEAlwaysStart.GetValue());
	chbox_core_hook_stfunc->SetValue(Ini.HookStFunc.GetValue());
	chbox_core_load_liblv2->SetValue(Ini.LoadLibLv2.GetValue());

	//Auto Pause related
	chbox_dbg_ap_systemcall->SetValue(Ini.DBGAutoPauseSystemCall.GetValue());
	chbox_dbg_ap_functioncall->SetValue(Ini.DBGAutoPauseFunctionCall.GetValue());

	//Custom EmulationDir
	chbox_emulationdir_enable->SetValue(Ini.SysEmulationDirPathEnable.GetValue());
	txt_emulationdir_path->SetValue(Ini.SysEmulationDirPath.GetValue());

	rbox_ppu_decoder->SetSelection(Ini.CPUDecoderMode.GetValue() ? Ini.CPUDecoderMode.GetValue() : 0);
	txt_dbg_range_min->SetValue(std::to_string(Ini.LLVMMinId.GetValue()));
	txt_dbg_range_max->SetValue(std::to_string(Ini.LLVMMaxId.GetValue()));
	txt_llvm_threshold->SetValue(std::to_string(Ini.LLVMThreshold.GetValue()));
	rbox_spu_decoder->SetSelection(Ini.SPUDecoderMode.GetValue() ? Ini.SPUDecoderMode.GetValue() : 0);
	cbox_gs_render->SetSelection(Ini.GSRenderMode.GetValue());
	cbox_gs_d3d_adaptater->SetSelection(Ini.GSD3DAdaptater.GetValue());
	cbox_gs_resolution->SetSelection(ResolutionIdToNum(Ini.GSResolution.GetValue()) - 1);
	cbox_gs_aspect->SetSelection(Ini.GSAspectRatio.GetValue() - 1);
	cbox_gs_frame_limit->SetSelection(Ini.GSFrameLimit.GetValue());
	cbox_pad_handler->SetSelection(Ini.PadHandlerMode.GetValue());
	cbox_keyboard_handler->SetSelection(Ini.KeyboardHandlerMode.GetValue());
	cbox_mouse_handler->SetSelection(Ini.MouseHandlerMode.GetValue());
	cbox_audio_out->SetSelection(Ini.AudioOutMode.GetValue());
	cbox_camera->SetSelection(Ini.Camera.GetValue());
	cbox_camera_type->SetSelection(Ini.CameraType.GetValue());
	cbox_hle_loglvl->SetSelection(Ini.HLELogLvl.GetValue());
	cbox_net_status->SetSelection(Ini.NETStatus.GetValue());
	cbox_net_interface->SetSelection(Ini.NETInterface.GetValue());
	cbox_sys_lang->SetSelection(Ini.SysLanguage.GetValue());

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
	s_subpanel_graphics1->Add(chbox_gs_dump_depth, wxSizerFlags().Border(wxALL, 5).Expand());
	s_subpanel_graphics1->Add(chbox_gs_dump_color, wxSizerFlags().Border(wxALL, 5).Expand());
	s_subpanel_graphics1->Add(chbox_gs_read_color, wxSizerFlags().Border(wxALL, 5).Expand());
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
	s_subpanel_misc->Add(chbox_hle_logging, wxSizerFlags().Border(wxALL, 5).Expand());
	s_subpanel_misc->Add(chbox_rsx_logging, wxSizerFlags().Border(wxALL, 5).Expand());
	s_subpanel_misc->Add(chbox_hle_savetty, wxSizerFlags().Border(wxALL, 5).Expand());
	s_subpanel_misc->Add(chbox_hle_exitonstop, wxSizerFlags().Border(wxALL, 5).Expand());
	s_subpanel_misc->Add(chbox_hle_always_start, wxSizerFlags().Border(wxALL, 5).Expand());

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
		Ini.CPUDecoderMode.SetValue(rbox_ppu_decoder->GetSelection());
		long minllvmid, maxllvmid;
		txt_dbg_range_min->GetValue().ToLong(&minllvmid);
		txt_dbg_range_max->GetValue().ToLong(&maxllvmid);
		Ini.LLVMExclusionRange.SetValue(chbox_core_llvm_exclud->GetValue());
		Ini.LLVMMinId.SetValue(minllvmid);
		Ini.LLVMMaxId.SetValue(maxllvmid);
		long llvmthreshold;
		txt_llvm_threshold->GetValue().ToLong(&llvmthreshold);
		Ini.LLVMThreshold.SetValue(llvmthreshold);
		Ini.SPUDecoderMode.SetValue(rbox_spu_decoder->GetSelection());
		Ini.HookStFunc.SetValue(chbox_core_hook_stfunc->GetValue());
		Ini.LoadLibLv2.SetValue(chbox_core_load_liblv2->GetValue());
		Ini.GSRenderMode.SetValue(cbox_gs_render->GetSelection());
		Ini.GSD3DAdaptater.SetValue(cbox_gs_d3d_adaptater->GetSelection());
		Ini.GSResolution.SetValue(ResolutionNumToId(cbox_gs_resolution->GetSelection() + 1));
		Ini.GSAspectRatio.SetValue(cbox_gs_aspect->GetSelection() + 1);
		Ini.GSFrameLimit.SetValue(cbox_gs_frame_limit->GetSelection());
		Ini.GSLogPrograms.SetValue(chbox_gs_log_prog->GetValue());
		Ini.GSDumpDepthBuffer.SetValue(chbox_gs_dump_depth->GetValue());
		Ini.GSDumpColorBuffers.SetValue(chbox_gs_dump_color->GetValue());
		Ini.GSReadColorBuffer.SetValue(chbox_gs_read_color->GetValue());
		Ini.GSVSyncEnable.SetValue(chbox_gs_vsync->GetValue());
		Ini.GSDebugOutputEnable.SetValue(chbox_gs_debug_output->GetValue());
		Ini.GS3DTV.SetValue(chbox_gs_3dmonitor->GetValue());
		Ini.GSOverlay.SetValue(chbox_gs_overlay->GetValue());
		Ini.PadHandlerMode.SetValue(cbox_pad_handler->GetSelection());
		Ini.KeyboardHandlerMode.SetValue(cbox_keyboard_handler->GetSelection());
		Ini.MouseHandlerMode.SetValue(cbox_mouse_handler->GetSelection());
		Ini.AudioOutMode.SetValue(cbox_audio_out->GetSelection());
		Ini.AudioDumpToFile.SetValue(chbox_audio_dump->GetValue());
		Ini.AudioConvertToU16.SetValue(chbox_audio_conv->GetValue());
		Ini.Camera.SetValue(cbox_camera->GetSelection());
		Ini.CameraType.SetValue(cbox_camera_type->GetSelection());
		Ini.HLELogging.SetValue(chbox_hle_logging->GetValue());
		Ini.RSXLogging.SetValue(chbox_rsx_logging->GetValue());
		Ini.HLESaveTTY.SetValue(chbox_hle_savetty->GetValue());
		Ini.HLEExitOnStop.SetValue(chbox_hle_exitonstop->GetValue());
		Ini.HLELogLvl.SetValue(cbox_hle_loglvl->GetSelection());
		Ini.NETStatus.SetValue(cbox_net_status->GetSelection());
		Ini.NETInterface.SetValue(cbox_net_interface->GetSelection());
		Ini.SysLanguage.SetValue(cbox_sys_lang->GetSelection());
		Ini.HLEAlwaysStart.SetValue(chbox_hle_always_start->GetValue());

		//Auto Pause
		Ini.DBGAutoPauseFunctionCall.SetValue(chbox_dbg_ap_functioncall->GetValue());
		Ini.DBGAutoPauseSystemCall.SetValue(chbox_dbg_ap_systemcall->GetValue());

		//Custom EmulationDir
		Ini.SysEmulationDirPathEnable.SetValue(chbox_emulationdir_enable->GetValue());
		Ini.SysEmulationDirPath.SetValue(txt_emulationdir_path->GetValue().ToStdString());

		Ini.Save();
	}

	if (paused) Emu.Resume();
}
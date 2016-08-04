#include "stdafx.h"
#include "stdafx_gui.h"
#include "Utilities/Config.h"
#include "Loader/ELF.h"
#include "Emu/System.h"

#ifdef _MSC_VER
#include <Windows.h>
#undef GetHwnd
#include <d3d12.h>
#include <wrl/client.h>
#include <dxgi1_4.h>
#endif

#include "SettingsDialog.h"

#include <set>
#include <unordered_set>
#include <algorithm>

static const int default_width = 550;
static const int default_height = 610;

SettingsDialog::SettingsDialog(wxWindow* parent,
							   Mode mode,
							   const wxString& title)
: wxDialog(parent, wxID_ANY, title, wxDefaultPosition, wxSize(default_width, default_height), wxDEFAULT_DIALOG_STYLE)
{
	BuildDialog(mode);
	AddRenderers();
	AddRendererSettings();
	AddAudioBackends();
	AddInputHandlers();
	AddCameras();
	AddConnectionStats();
	AddSystemLanguages();


	if (mode == Global)
	{

	}

	else
	{

	}

	if (ShowModal() == wxID_OK)
	{
	}
}

void SettingsDialog::OnModuleListItemToggled(wxCommandEvent& event)
{
}

void SettingsDialog::OnSearchBoxTextChanged(wxCommandEvent& event)
{
}

void SettingsDialog::OnCBoxRendererChanged(wxCommandEvent& event)
{
	cb_gpu_selector->Clear();

	if (event.GetString() == "Null")
	{
		l_gpu_selector->SetLabelText("GPU Selector");
		cb_gpu_selector->Enable(false);
	}

	else if (event.GetString() == "OpenGL")
	{
		l_gpu_selector->SetLabelText("GPU Selector");
		cb_gpu_selector->Enable(false);
	}

#ifdef _WIN32
	else if (event.GetString() == "D3D12")
	{
		l_gpu_selector->SetLabelText("D3D Adapter");
		if (gpus_d3d.empty())
		{
			cb_gpu_selector->Enable(false);
		}

		else
		{
			cb_gpu_selector->Enable(true);
			cb_gpu_selector->Append(gpus_d3d);
			cb_gpu_selector->SetSelection(0);
		}
	}

	else if (event.GetString() == "Vulkan")
	{
		l_gpu_selector->SetLabelText("Vulkan Adapter");
		if (gpus_vulkan.empty())
		{
			cb_gpu_selector->Enable(false);
		}

		else
		{
			cb_gpu_selector->Enable(true);
			cb_gpu_selector->Append(gpus_vulkan);
			cb_gpu_selector->SetSelection(0);
		}
	}
#endif

	else
	{
		l_gpu_selector->SetLabelText("GPU Selector");
		cb_gpu_selector->Enable(false);
	}
}

void SettingsDialog::AddRenderers()
{
#ifdef _WIN32
	GetD3DAdapters();
	GetVulkanAdapters();
#endif
}

void SettingsDialog::AddRendererSettings()
{
}

void SettingsDialog::AddAudioBackends()
{
}

void SettingsDialog::AddInputHandlers()
{
}

void SettingsDialog::AddCameras()
{
}

void SettingsDialog::AddConnectionStats()
{
}

void SettingsDialog::AddSystemLanguages()
{
}

#ifdef _WIN32
void SettingsDialog::GetD3DAdapters()
{
	Microsoft::WRL::ComPtr<IDXGIFactory4> dxgi_factory;

	if (SUCCEEDED(CreateDXGIFactory(IID_PPV_ARGS(&dxgi_factory))))
	{
		Microsoft::WRL::ComPtr<IDXGIAdapter> adapter;

		for (UINT id = 0; dxgi_factory->EnumAdapters(id, adapter.ReleaseAndGetAddressOf()) != DXGI_ERROR_NOT_FOUND; id++)
		{
			DXGI_ADAPTER_DESC desc;
			adapter->GetDesc(&desc);
			gpus_d3d.push_back(desc.Description);
		}
	}
}

void SettingsDialog::GetVulkanAdapters()
{
	// ???
}
#endif

void SettingsDialog::BuildDialog(Mode mode)
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );

	// Create Layout Sizer
	s_dialog = new wxBoxSizer(wxVERTICAL);
	s_core = new wxBoxSizer(wxHORIZONTAL);
	s_core_left = new wxBoxSizer(wxVERTICAL);
	s_core_ppu_decoder = new wxBoxSizer(wxVERTICAL);
	s_core_spu_decoder = new wxBoxSizer(wxVERTICAL);
	s_core_ckb = new wxBoxSizer(wxVERTICAL);
	s_core_right = new wxBoxSizer(wxVERTICAL);
	s_graphics = new wxBoxSizer(wxVERTICAL);
	s_graphics_cb = new wxGridSizer(0, 2, 0, 0);
	s_graphics_renderer = new wxBoxSizer(wxVERTICAL);
	s_graphics_aspect_ratio = new wxBoxSizer(wxVERTICAL);
	s_graphics_resolution = new wxBoxSizer(wxVERTICAL);
	s_graphics_frame_limit = new wxBoxSizer(wxVERTICAL);
	s_graphics_ckb = new wxGridSizer(4, 2, 0, 0);
	s_graphics_gpu_selector = new wxBoxSizer(wxVERTICAL);
	s_audio = new wxBoxSizer(wxVERTICAL);
	s_audio_cb = new wxGridSizer(0, 2, 0, 0);
	s_audio_audio_out = new wxBoxSizer(wxVERTICAL);
	s_audio_ckb = new wxGridSizer(0, 1, 0, 0);

	if (mode == Global)
	{
		s_io = new wxBoxSizer(wxVERTICAL);
		s_io_cb = new wxGridSizer(0, 2, 0, 0);
		s_io_pad_handler = new wxBoxSizer(wxVERTICAL);
		s_io_camera = new wxBoxSizer(wxVERTICAL);
		s_io_keyboard_handler = new wxBoxSizer(wxVERTICAL);
		s_io_camera_type = new wxBoxSizer(wxVERTICAL);
		s_io_mouse_handler = new wxBoxSizer(wxVERTICAL);
		s_misc = new wxBoxSizer(wxVERTICAL);
		s_misc_ckb = new wxGridSizer(0, 1, 0, 0);
		s_networking = new wxBoxSizer(wxVERTICAL);
		s_networking_cb = new wxGridSizer(0, 2, 0, 0);
		s_networking_connection_status = new wxBoxSizer(wxVERTICAL);
		s_system = new wxBoxSizer(wxVERTICAL);
		s_system_ckb = new wxGridSizer(0, 2, 0, 0);
		s_system_cb = new wxGridSizer(0, 2, 0, 0);
		s_system_language = new wxBoxSizer(wxVERTICAL);
	}

	s_button_box = new wxBoxSizer(wxHORIZONTAL);

	wxSize ui_elem_min_size = wxSize(default_width / 2, wxDefaultSize.GetY());
	wxSize ui_elem_core_min_size = wxSize(ui_elem_min_size.GetX() / 1.5 + 10, ui_elem_min_size.GetY());

	// Helper macros
#define CreateLabel(UI_PANEL, UI_NAME, NAME) \
	l_##UI_NAME = new wxStaticText(p_##UI_PANEL, wxID_ANY, wxT(NAME), wxDefaultPosition, wxDefaultSize, 0); \
	l_##UI_NAME->Wrap(-1) \

#define CreateCheckBox(UI_PANEL, UI_NAME, NAME) \
	ckb_##UI_NAME = new wxCheckBox(p_##UI_PANEL, wxID_ANY, wxT(NAME), wxDefaultPosition, wxDefaultSize, 0); \
	ckb_##UI_NAME->SetMinSize(ui_elem_min_size); \
	s_##UI_PANEL##_ckb->Add(ckb_##UI_NAME, 0, wxALL | wxEXPAND, 5)

#define CreateCheckBoxCore(UI_PANEL, UI_NAME, NAME) \
	CreateCheckBox(UI_PANEL, UI_NAME, NAME); \
	ckb_##UI_NAME->SetMinSize(ui_elem_core_min_size)

#define CreateComboBox(UI_PANEL, UI_NAME, NAME) \
	l_##UI_NAME = new wxStaticText(p_##UI_PANEL, wxID_ANY, wxT(NAME), wxDefaultPosition, wxDefaultSize, 0); \
	l_##UI_NAME->Wrap(-1); \
	cb_##UI_NAME = new wxChoice(p_##UI_PANEL, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0, NULL, 0); \
	cb_##UI_NAME->SetMinSize(ui_elem_min_size); \
	s_##UI_PANEL##_##UI_NAME->Add(l_##UI_NAME, 0, wxALL | wxEXPAND, 5); \
	s_##UI_PANEL##_##UI_NAME->Add(cb_##UI_NAME, 0, wxALL | wxEXPAND, 5); \
	s_##UI_PANEL##_cb->Add(s_##UI_PANEL##_##UI_NAME, 1, wxEXPAND, 5)

#define CreateRadioBox(UI_PANEL, UI_NAME, NUM_CHOICES, CHOICES) \
	radiobox_##UI_NAME = new wxRadioBox(p_##UI_PANEL, wxID_ANY, wxT(NAME), \
		wxDefaultPosition, wxDefaultSize, \
		NUM_CHOICES, CHOICES, 1, wxRA_SPECIFY_COLS); \
	radiobox_##UI_NAME->SetSelection(0)

#define FinalizePanelLayout(UI_PANEL, NAME, DEFAULT_PAGE) \
	p_##UI_PANEL->SetSizer(s_##UI_PANEL); \
	p_##UI_PANEL->Layout(); \
	s_##UI_PANEL->Fit(p_##UI_PANEL); \
	nb_config->AddPage(p_##UI_PANEL, wxT(NAME), DEFAULT_PAGE);

	//=============================================================================================//
	// Panels                                                                                      //
	//=============================================================================================//

	// NOTE: the pages needs to be added last, otherwise the layout doesn't work

	nb_config = new wxNotebook(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0);
	p_core = new wxPanel(nb_config, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL);
	p_graphics = new wxPanel(nb_config, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL);
	p_audio = new wxPanel(nb_config, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL);
	p_io = new wxPanel(nb_config, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL);
	p_misc = new wxPanel(nb_config, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL);
	p_networking = new wxPanel(nb_config, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL);
	p_system = new wxPanel(nb_config, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL);

	//=============================================================================================//
	// Core                                                                                        //
	//=============================================================================================//

	wxString radiobox_ppu_decoderChoices[] = {
		wxT("Interpreter (precise)"),
		wxT("Interpreter (fast)"),
		wxT("Recompiler (LLVM)")
	};
	int radiobox_ppu_decoderNChoices = sizeof(radiobox_ppu_decoderChoices) / sizeof(wxString);
	radiobox_ppu_decoder = new wxRadioBox(p_core, wxID_ANY, wxT("PPU Decoder"),
										  wxDefaultPosition, wxDefaultSize,
										  radiobox_ppu_decoderNChoices, radiobox_ppu_decoderChoices,
										  1, wxRA_SPECIFY_COLS);
	radiobox_ppu_decoder->SetSelection(0);

	wxString radiobox_spu_decoderChoices[] = {
		wxT("Interpreter (precise)"),
		wxT("Interpreter (fast)"),
		wxT("Recompiler (ASMJIT)"),
		wxT("Recompiler (LLVM)")
	};
	int radiobox_spu_decoderNChoices = sizeof(radiobox_spu_decoderChoices) / sizeof(wxString);
	radiobox_spu_decoder = new wxRadioBox(p_core, wxID_ANY, wxT("SPU Decoder"),
										  wxDefaultPosition, wxDefaultSize,
										  radiobox_spu_decoderNChoices, radiobox_spu_decoderChoices,
										  1, wxRA_SPECIFY_COLS);
	radiobox_spu_decoder->SetSelection(0);

	CreateCheckBoxCore(core, hook_static_functions, "Hook static functions");
	CreateCheckBoxCore(core, load_liblv2_sprx_only, "Load liblv2.sprx only");

	CreateLabel(core, load_libraries, "Load libraries");
	clb_lle_module_list = new wxCheckListBox(p_core, wxID_ANY, wxDefaultPosition, wxDefaultSize, {}, 0);
	clb_lle_module_list->Bind(wxEVT_CHECKLISTBOX, &SettingsDialog::OnModuleListItemToggled, this);
	lle_search_box = new wxTextCtrl(p_core, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0);
	lle_search_box->Bind(wxEVT_TEXT, &SettingsDialog::OnSearchBoxTextChanged, this);

	// setup additional layout elements
	s_core_right->Add(l_load_libraries, 0, wxALL, 5);
	s_core_ppu_decoder->Add(radiobox_ppu_decoder, 1, wxALL | wxEXPAND, 5);
	s_core_left->Add(s_core_ppu_decoder, 2, wxEXPAND, 5);
	s_core_spu_decoder->Add(radiobox_spu_decoder, 1, wxALL | wxEXPAND, 5);
	s_core_left->Add(s_core_spu_decoder, 2, wxEXPAND, 5);
	s_core_left->Add(s_core_ckb, 2, wxEXPAND, 5);
	s_core_left->Add(0, 0, 1, wxEXPAND, 5);
	s_core->Add(s_core_left, 1, wxEXPAND, 5);
	s_core_right->Add(clb_lle_module_list, 5, wxALL | wxEXPAND, 5);
	s_core_right->Add(lle_search_box, 0, wxALL | wxEXPAND, 5);
	s_core->Add(s_core_right, 2, wxEXPAND, 5);

	FinalizePanelLayout(core, "Core", true);

	//=============================================================================================//
	// Graphics                                                                                    //
	//=============================================================================================//

	CreateComboBox(graphics, renderer, "Renderer");
	cb_renderer->Bind(wxEVT_CHOICE, &SettingsDialog::OnCBoxRendererChanged, this);
	CreateComboBox(graphics, aspect_ratio, "Aspect ratio");
	CreateComboBox(graphics, resolution, "Resolution");
	CreateComboBox(graphics, frame_limit, "Frame limit");
	CreateComboBox(graphics, gpu_selector, "GPU Selector");

	CreateCheckBox(graphics, write_color_buffers, "Write Color Buffers");
	CreateCheckBox(graphics, write_depth_buffer, "Write Depth Buffer");
	CreateCheckBox(graphics, read_color_buffers, "Read Color Buffers");
	CreateCheckBox(graphics, read_depth_buffer, "Read Depth Buffer");
	CreateCheckBox(graphics, debug_output, "Debug Output");
	CreateCheckBox(graphics, debug_overlay, "Debug overlay");
	CreateCheckBox(graphics, log_shader_programs, "Log shader programs");

	// setup additional layout elements
	s_graphics->Add(s_graphics_cb, 0, 0, 5);
	s_graphics->Add(s_graphics_ckb, 0, 0, 5);

	FinalizePanelLayout(graphics, "Graphics", false);

	//=============================================================================================//
	// Audio                                                                                       //
	//=============================================================================================//

	CreateComboBox(audio, audio_out, "Audio Out");

	CreateCheckBox(audio, dump_to_file, "Dump to file");
	CreateCheckBox(audio, convert_to_16bit, "Convert to 16 bit");

	// setup additional layout elements
	s_audio->Add(s_audio_cb, 0, 0, 5);
	s_audio->Add(s_audio_ckb, 0, 0, 5);

	FinalizePanelLayout(audio, "Audio", false);

	//=============================================================================================//
	// Input / Output                                                                              //
	//=============================================================================================//

	if (mode == Global) { // BEGIN GLOBAL MODE

	CreateComboBox(io, pad_handler, "Pad Handler");
	CreateComboBox(io, camera, "Camera");
	CreateComboBox(io, keyboard_handler, "Keyboard Handler");
	CreateComboBox(io, camera_type, "Camera type");
	CreateComboBox(io, mouse_handler, "Mouse Handler");

	// setup additional layout elements
	s_io->Add(s_io_cb, 0, 0, 5);

	FinalizePanelLayout(io, "Input / Output", false);

	//=============================================================================================//
	// Misc                                                                                        //
	//=============================================================================================//

	CreateCheckBox(misc, exit_on_process_finish, "Exit RPCS3 when process finishes");
	CreateCheckBox(misc, start_after_boot, "Always start after boot");
	CreateCheckBox(misc, auto_pause_syscall, "Auto Pause at System Call");
	CreateCheckBox(misc, auto_pause_funccall, "Auto Pause at Function Call");

	// setup additional layout elements
	s_misc->Add(s_misc_ckb, 0, 0, 5);

	FinalizePanelLayout(misc, "Misc", false);

	//=============================================================================================//
	// Networking                                                                                  //
	//=============================================================================================//

	CreateComboBox(networking, connection_status, "Connection status");

	// setup additional layout elements
	s_networking->Add(s_networking_cb, 0, 0, 5);

	FinalizePanelLayout(networking, "Networking", false);

	//=============================================================================================//
	// System                                                                                      //
	//=============================================================================================//

	CreateCheckBox(system, enable_host_root, "Enable /host_root/");

	CreateComboBox(system, language, "Language");

	// setup additional layout elements
	s_system->Add(s_system_ckb, 0, 0, 5);
	s_system->Add(s_system_cb, 0, 0, 5);

	FinalizePanelLayout(system, "System", false);

	//=============================================================================================//
	// Finalize Dialog                                                                             //
	//=============================================================================================//

	} // END GLOBAL MODE

	s_dialog->Add(nb_config, 5, wxEXPAND | wxALL, 5);

	btn_ok = new wxButton(this, wxID_OK, wxT("OK"), wxDefaultPosition, wxDefaultSize, 0);
	s_button_box->Add(btn_ok, 1, wxALIGN_BOTTOM | wxALL, 5);

	btn_cancel = new wxButton(this, wxID_CANCEL, wxT("Cancel"), wxDefaultPosition, wxDefaultSize, 0);
	s_button_box->Add(btn_cancel, 1, wxALIGN_BOTTOM | wxALL, 5);

	s_button_box->Add(0, 0, 5, wxALIGN_BOTTOM | wxEXPAND, 5);

	s_dialog->Add(s_button_box, 1, wxEXPAND, 5);

	SetSizer(s_dialog);
	Layout();

	Centre(wxBOTH);

	SetMinSize(wxSize(default_width, default_height));
}

SettingsDialog::~SettingsDialog()
{
	lle_module_list.clear();

#ifdef _WIN32
	gpus_d3d.Clear();
	gpus_vulkan.Clear();
#endif
}

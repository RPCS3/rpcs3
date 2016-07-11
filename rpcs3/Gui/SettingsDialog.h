#pragma once

class SettingsDialog : public wxDialog
{
	using module_map = std::map<std::string, bool>;
	using gpu_list = wxArrayString;

public:
	enum Mode
	{
		Global,
		Game
	};

	SettingsDialog(wxWindow* parent,
				   Mode mode = Global,
				   const wxString& title = wxT("Settings"));
	~SettingsDialog();

private:
	void OnModuleListItemToggled(wxCommandEvent& event);
	void OnSearchBoxTextChanged(wxCommandEvent& event);
	void OnCBoxRendererChanged(wxCommandEvent& event);

	void AddRenderers();
	void AddRendererSettings();
	void AddAudioBackends();
	void AddInputHandlers();
	void AddCameras();
	void AddConnectionStats();
	void AddSystemLanguages();

#ifdef _WIN32
	void GetD3DAdapters();
	void GetVulkanAdapters(); // TODO: Linux Vulkan support
#endif

	module_map lle_module_list;

#ifdef _WIN32
	gpu_list gpus_d3d;
	gpu_list gpus_vulkan;
#endif

	void BuildDialog(Mode mode);
	wxNotebook* nb_config;
	wxPanel* p_core;
	wxRadioBox* radiobox_ppu_decoder;
	wxRadioBox* radiobox_spu_decoder;
	wxCheckBox* ckb_hook_static_functions;
	wxCheckBox* ckb_load_liblv2_sprx_only;
	wxStaticText* l_load_libraries;
	wxCheckListBox* clb_lle_module_list;
	wxTextCtrl* lle_search_box;
	wxPanel* p_graphics;
	wxStaticText* l_renderer;
	wxChoice* cb_renderer;
	wxStaticText* l_aspect_ratio;
	wxChoice* cb_aspect_ratio;
	wxStaticText* l_resolution;
	wxChoice* cb_resolution;
	wxStaticText* l_frame_limit;
	wxChoice* cb_frame_limit;
	wxStaticText* l_gpu_selector;
	wxChoice* cb_gpu_selector;
	wxCheckBox* ckb_write_color_buffers;
	wxCheckBox* ckb_debug_output;
	wxCheckBox* ckb_read_color_buffers;
	wxCheckBox* ckb_read_depth_buffer;
	wxCheckBox* ckb_debug_overlay;
	wxCheckBox* ckb_write_depth_buffer;
	wxCheckBox* ckb_log_shader_programs;
	wxPanel* p_audio;
	wxStaticText* l_audio_out;
	wxChoice* cb_audio_out;
	wxCheckBox* ckb_dump_to_file;
	wxCheckBox* ckb_convert_to_16bit;
	wxPanel* p_io;
	wxStaticText* l_pad_handler;
	wxChoice* cb_pad_handler;
	wxStaticText* l_camera;
	wxChoice* cb_camera;
	wxStaticText* l_keyboard_handler;
	wxChoice* cb_keyboard_handler;
	wxStaticText* l_camera_type;
	wxChoice* cb_camera_type;
	wxStaticText* l_mouse_handler;
	wxChoice* cb_mouse_handler;
	wxPanel* p_misc;
	wxCheckBox* ckb_exit_on_process_finish;
	wxCheckBox* ckb_start_after_boot;
	wxCheckBox* ckb_auto_pause_syscall;
	wxCheckBox* ckb_auto_pause_funccall;
	wxPanel* p_networking;
	wxStaticText* l_connection_status;
	wxChoice* cb_connection_status;
	wxPanel* p_system;
	wxCheckBox* ckb_enable_host_root;
	wxStaticText* l_language;
	wxChoice* cb_language;
	wxButton* btn_ok;
	wxButton* btn_cancel;

	// layout
	wxBoxSizer* s_dialog;
	wxBoxSizer* s_core;
	wxBoxSizer* s_core_left;
	wxBoxSizer* s_core_ppu_decoder;
	wxBoxSizer* s_core_spu_decoder;
	wxBoxSizer* s_core_ckb;
	wxBoxSizer* s_core_right;
	wxBoxSizer* s_graphics;
	wxGridSizer* s_graphics_cb;
	wxBoxSizer* s_graphics_renderer;
	wxBoxSizer* s_graphics_aspect_ratio;
	wxBoxSizer* s_graphics_resolution;
	wxBoxSizer* s_graphics_frame_limit;
	wxGridSizer* s_graphics_ckb;
	wxBoxSizer* s_graphics_gpu_selector;
	wxBoxSizer* s_audio;
	wxGridSizer* s_audio_cb;
	wxBoxSizer* s_audio_audio_out;
	wxGridSizer* s_audio_ckb ;
	wxBoxSizer* s_io;
	wxGridSizer* s_io_cb;
	wxBoxSizer* s_io_pad_handler;
	wxBoxSizer* s_io_camera;
	wxBoxSizer* s_io_keyboard_handler;
	wxBoxSizer* s_io_camera_type;
	wxBoxSizer* s_io_mouse_handler;
	wxBoxSizer* s_misc;
	wxGridSizer* s_misc_ckb;
	wxBoxSizer* s_networking;
	wxGridSizer* s_networking_cb;
	wxBoxSizer* s_networking_connection_status;
	wxBoxSizer* s_system;
	wxGridSizer* s_system_ckb;
	wxGridSizer* s_system_cb;
	wxBoxSizer* s_system_language;
	wxBoxSizer* s_button_box;
};

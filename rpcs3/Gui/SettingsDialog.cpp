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

// Node location
using cfg_location = std::vector<const char*>;

extern std::string g_cfg_defaults;

static YAML::Node loaded;
static YAML::Node saved;

// Emit sorted YAML
static never_inline void emit(YAML::Emitter& out, const YAML::Node& node)
{
	// TODO
	out << node;
}

// Incrementally load YAML
static never_inline void operator +=(YAML::Node& left, const YAML::Node& node)
{
	if (node && !node.IsNull())
	{
		if (node.IsMap())
		{
			for (const auto& pair : node)
			{
				if (pair.first.IsScalar())
				{
					auto&& lhs = left[pair.first.Scalar()];
					lhs += pair.second;
				}
				else
				{
					// Exotic case (TODO: probably doesn't work)
					auto&& lhs = left[YAML::Clone(pair.first)];
					lhs += pair.second;
				}
			}
		}
		else if (node.IsScalar() || node.IsSequence())
		{
			// Scalars and sequences are replaced completely, but this may change in future.
			// This logic may be overwritten by custom demands of every specific cfg:: node.
			left = node;
		}
	}
}

// Connects wx gui element to the config node; abstract base class
struct cfg_adapter
{
	cfg_location location;

	cfg_adapter(cfg_location&& _loc)
		: location(std::move(_loc))
	{
	}

	static cfg::entry_base& get_cfg(cfg::entry_base& root, cfg_location::const_iterator begin, cfg_location::const_iterator end)
	{
		return begin == end ? root : get_cfg(root[*begin], begin + 1, end);
	}

	static YAML::Node get_node(YAML::Node node, cfg_location::const_iterator begin, cfg_location::const_iterator end)
	{
		return begin == end ? node : get_node(node[*begin], begin + 1, end); // TODO
	}

	cfg::entry_base& get_cfg() const
	{
		return get_cfg(cfg::root, location.cbegin(), location.cend());
	}

	YAML::Node get_node(YAML::Node root) const
	{
		return get_node(root, location.cbegin(), location.cend());
	}

	virtual void save() = 0;
};

struct radiobox_pad_helper
{
	cfg_location location;
	wxArrayString values;

	radiobox_pad_helper(cfg_location&& _loc)
		: location(std::move(_loc))
	{
		for (const auto& v : cfg_adapter::get_cfg(cfg::root, location.cbegin(), location.cend()).to_list())
		{
			values.Add(fmt::FromUTF8(v));
		}
	}

	operator const wxArrayString&() const
	{
		return values;
	}
};

struct radiobox_pad : cfg_adapter
{
	wxRadioBox*& ptr;

	radiobox_pad(radiobox_pad_helper&& helper, wxRadioBox*& ptr)
		: cfg_adapter(std::move(helper.location))
		, ptr(ptr)
	{
		const auto& value = get_node(loaded).Scalar();
		const auto& values = get_cfg().to_list();

		for (int i = 0; i < values.size(); i++)
		{
			if (value == values[i])
			{
				ptr->SetSelection(i);
				return;
			}
		}
	}

	void save() override
	{
		get_node(saved) = get_cfg().to_list()[ptr->GetSelection()];
	}
};

struct combobox_pad : cfg_adapter
{
	wxComboBox*& ptr;

	combobox_pad(cfg_location&& _loc, wxComboBox*& ptr)
		: cfg_adapter(std::move(_loc))
		, ptr(ptr)
	{
		for (const auto& v : get_cfg().to_list())
		{
			ptr->Append(fmt::FromUTF8(v));
		}

		ptr->SetValue(fmt::FromUTF8(get_node(loaded).Scalar()));
	}

	void save() override
	{
		get_node(saved) = fmt::ToUTF8(ptr->GetValue());
	}
};

struct checkbox_pad : cfg_adapter
{
	wxCheckBox*& ptr;

	checkbox_pad(cfg_location&& _loc, wxCheckBox*& ptr)
		: cfg_adapter(std::move(_loc))
		, ptr(ptr)
	{
		ptr->SetValue(get_node(loaded).as<bool>(false));
	}

	void save() override
	{
		get_node(saved) = ptr->GetValue() ? "true" : "false";
	}
};

struct textctrl_pad : cfg_adapter
{
	wxTextCtrl*& ptr;

	textctrl_pad(cfg_location&& _loc, wxTextCtrl*& ptr)
		: cfg_adapter(std::move(_loc))
		, ptr(ptr)
	{
		ptr->SetValue(fmt::FromUTF8(get_node(loaded).Scalar()));
	}

	void save() override
	{
		get_node(saved) = fmt::ToUTF8(ptr->GetValue());
	}
};


SettingsDialog::SettingsDialog(wxWindow* parent)
	: wxDialog(parent, wxID_ANY, "Settings", wxDefaultPosition)
{
	// Load default config
	loaded = YAML::Load(g_cfg_defaults);

	// Incrementally load config.yml
	const fs::file config(fs::get_config_dir() + "/config.yml", fs::read + fs::write + fs::create);
	loaded += YAML::Load(config.to_string());

	std::vector<std::unique_ptr<cfg_adapter>> pads;

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

	nb_config->AddPage(p_core, "Core");
	nb_config->AddPage(p_graphics, "Graphics");
	nb_config->AddPage(p_audio, "Audio");
	nb_config->AddPage(p_io, "Input / Output");
	nb_config->AddPage(p_misc, "Misc");
	nb_config->AddPage(p_networking, "Networking");
	nb_config->AddPage(p_system, "System");

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

	// Core
	wxStaticBoxSizer* s_round_core_lle = new wxStaticBoxSizer(wxVERTICAL, p_core, "Load libraries");
	wxCheckListBox* chbox_list_core_lle = new wxCheckListBox(p_core, wxID_ANY, wxDefaultPosition, wxDefaultSize, {}, wxLB_EXTENDED);

	// Graphics
	wxStaticBoxSizer* s_round_gs_render = new wxStaticBoxSizer(wxVERTICAL, p_graphics, "Render");
	wxStaticBoxSizer* s_round_gs_d3d_adapter = new wxStaticBoxSizer(wxVERTICAL, p_graphics, "D3D Adapter");
	wxStaticBoxSizer* s_round_gs_res = new wxStaticBoxSizer(wxVERTICAL, p_graphics, "Resolution");
	wxStaticBoxSizer* s_round_gs_aspect = new wxStaticBoxSizer(wxVERTICAL, p_graphics, "Aspect ratio");
	wxStaticBoxSizer* s_round_gs_frame_limit = new wxStaticBoxSizer(wxVERTICAL, p_graphics, "Frame limit");

	// Input / Output
	wxStaticBoxSizer* s_round_io_pad_handler = new wxStaticBoxSizer(wxVERTICAL, p_io, "Pad Handler");
	wxStaticBoxSizer* s_round_io_keyboard_handler = new wxStaticBoxSizer(wxVERTICAL, p_io, "Keyboard Handler");
	wxStaticBoxSizer* s_round_io_mouse_handler = new wxStaticBoxSizer(wxVERTICAL, p_io, "Mouse Handler");
	wxStaticBoxSizer* s_round_io_camera = new wxStaticBoxSizer(wxVERTICAL, p_io, "Camera");
	wxStaticBoxSizer* s_round_io_camera_type = new wxStaticBoxSizer(wxVERTICAL, p_io, "Camera type");

	// Audio
	wxStaticBoxSizer* s_round_audio_out = new wxStaticBoxSizer(wxVERTICAL, p_audio, "Audio Out");

	// Networking
	wxStaticBoxSizer* s_round_net_status = new wxStaticBoxSizer(wxVERTICAL, p_networking, "Connection status");

	// System
	wxStaticBoxSizer* s_round_sys_lang = new wxStaticBoxSizer(wxVERTICAL, p_system, "Language");


	wxRadioBox* rbox_ppu_decoder;
	wxRadioBox* rbox_spu_decoder;
	wxComboBox* cbox_gs_render = new wxComboBox(p_graphics, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(150, -1), 0, NULL, wxCB_READONLY);
	wxComboBox* cbox_gs_d3d_adapter = new wxComboBox(p_graphics, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(150, -1), 0, NULL, wxCB_READONLY);
	wxComboBox* cbox_gs_resolution = new wxComboBox(p_graphics, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(150, -1), 0, NULL, wxCB_READONLY);
	wxComboBox* cbox_gs_aspect = new wxComboBox(p_graphics, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(150, -1), 0, NULL, wxCB_READONLY);
	wxComboBox* cbox_gs_frame_limit = new wxComboBox(p_graphics, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(150, -1), 0, NULL, wxCB_READONLY);
	wxComboBox* cbox_pad_handler = new wxComboBox(p_io, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(150, -1), 0, NULL, wxCB_READONLY);;
	wxComboBox* cbox_keyboard_handler = new wxComboBox(p_io, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(150, -1), 0, NULL, wxCB_READONLY);
	wxComboBox* cbox_mouse_handler = new wxComboBox(p_io, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(150, -1), 0, NULL, wxCB_READONLY);
	wxComboBox* cbox_camera = new wxComboBox(p_io, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(150, -1), 0, NULL, wxCB_READONLY);
	wxComboBox* cbox_camera_type = new wxComboBox(p_io, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(150, -1), 0, NULL, wxCB_READONLY);
	wxComboBox* cbox_audio_out = new wxComboBox(p_audio, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(150, -1), 0, NULL, wxCB_READONLY);
	wxComboBox* cbox_net_status = new wxComboBox(p_networking, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0, NULL, wxCB_READONLY);
	wxComboBox* cbox_sys_lang = new wxComboBox(p_system, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0, NULL, wxCB_READONLY);

	wxCheckBox* chbox_core_hook_stfunc = new wxCheckBox(p_core, wxID_ANY, "Hook static functions");
	wxCheckBox* chbox_core_load_liblv2 = new wxCheckBox(p_core, wxID_ANY, "Load liblv2.sprx only");
	wxCheckBox* chbox_vfs_enable_host_root = new wxCheckBox(p_system, wxID_ANY, "Enable /host_root/");
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
	wxCheckBox* chbox_hle_exitonstop = new wxCheckBox(p_misc, wxID_ANY, "Exit RPCS3 when process finishes");
	wxCheckBox* chbox_hle_always_start = new wxCheckBox(p_misc, wxID_ANY, "Always start after boot");
	wxCheckBox* chbox_dbg_ap_systemcall = new wxCheckBox(p_misc, wxID_ANY, "Auto Pause at System Call");
	wxCheckBox* chbox_dbg_ap_functioncall = new wxCheckBox(p_misc, wxID_ANY, "Auto Pause at Function Call");

	std::vector<std::string> lle_module_list;
	{
		auto&& data = loaded["Core"]["Load libraries"].as<std::vector<std::string>, std::initializer_list<std::string>>({});

		// List selected modules first
		for (const auto& unk : data)
		{
			chbox_list_core_lle->Check(chbox_list_core_lle->Append(unk));
			lle_module_list.push_back(unk);
		}

		const std::string& lle_dir = vfs::get("/dev_flash/sys/external/"); // TODO

		std::unordered_set<std::string> set(data.begin(), data.end());

		for (const auto& prxf : fs::dir(lle_dir))
		{
			// List found unselected modules
			if (!prxf.is_directory && ppu_prx_loader(fs::file(lle_dir + prxf.name)) == elf_error::ok && !set.count(prxf.name))
			{
				chbox_list_core_lle->Check(chbox_list_core_lle->Append(prxf.name), false);
				lle_module_list.push_back(prxf.name);
			}
		}
	}

	radiobox_pad_helper ppu_decoder_modes({ "Core", "PPU Decoder" });
	rbox_ppu_decoder = new wxRadioBox(p_core, wxID_ANY, "PPU Decoder", wxDefaultPosition, wxSize(-1, -1), ppu_decoder_modes, 1);
	pads.emplace_back(std::make_unique<radiobox_pad>(std::move(ppu_decoder_modes), rbox_ppu_decoder));

	radiobox_pad_helper spu_decoder_modes({ "Core", "SPU Decoder" });
	rbox_spu_decoder = new wxRadioBox(p_core, wxID_ANY, "SPU Decoder", wxDefaultPosition, wxSize(-1, -1), spu_decoder_modes, 1);
	pads.emplace_back(std::make_unique<radiobox_pad>(std::move(spu_decoder_modes), rbox_spu_decoder));
	rbox_spu_decoder->Enable(3, false); // TODO

	pads.emplace_back(std::make_unique<checkbox_pad>(cfg_location{ "Core", "Hook static functions" }, chbox_core_hook_stfunc));
	pads.emplace_back(std::make_unique<checkbox_pad>(cfg_location{ "Core", "Load liblv2.sprx only" }, chbox_core_load_liblv2));

	pads.emplace_back(std::make_unique<checkbox_pad>(cfg_location{ "VFS", "Enable /host_root/" }, chbox_vfs_enable_host_root));

	pads.emplace_back(std::make_unique<combobox_pad>(cfg_location{ "Video", "Renderer" }, cbox_gs_render));
	pads.emplace_back(std::make_unique<combobox_pad>(cfg_location{ "Video", "Resolution" }, cbox_gs_resolution));
	pads.emplace_back(std::make_unique<combobox_pad>(cfg_location{ "Video", "Aspect ratio" }, cbox_gs_aspect));
	pads.emplace_back(std::make_unique<combobox_pad>(cfg_location{ "Video", "Frame limit" }, cbox_gs_frame_limit));
	pads.emplace_back(std::make_unique<checkbox_pad>(cfg_location{ "Video", "Log shader programs" }, chbox_gs_log_prog));
	pads.emplace_back(std::make_unique<checkbox_pad>(cfg_location{ "Video", "Write Depth Buffer" }, chbox_gs_dump_depth));
	pads.emplace_back(std::make_unique<checkbox_pad>(cfg_location{ "Video", "Write Color Buffers" }, chbox_gs_dump_color));
	pads.emplace_back(std::make_unique<checkbox_pad>(cfg_location{ "Video", "Read Color Buffers" }, chbox_gs_read_color));
	pads.emplace_back(std::make_unique<checkbox_pad>(cfg_location{ "Video", "Read Depth Buffer" }, chbox_gs_read_depth));
	pads.emplace_back(std::make_unique<checkbox_pad>(cfg_location{ "Video", "VSync" }, chbox_gs_vsync));
	pads.emplace_back(std::make_unique<checkbox_pad>(cfg_location{ "Video", "Debug output" }, chbox_gs_debug_output));
	pads.emplace_back(std::make_unique<checkbox_pad>(cfg_location{ "Video", "3D Monitor" }, chbox_gs_3dmonitor));
	pads.emplace_back(std::make_unique<checkbox_pad>(cfg_location{ "Video", "Debug overlay" }, chbox_gs_overlay));
	
	pads.emplace_back(std::make_unique<combobox_pad>(cfg_location{ "Audio", "Renderer" }, cbox_audio_out));
	pads.emplace_back(std::make_unique<checkbox_pad>(cfg_location{ "Audio", "Dump to file" }, chbox_audio_dump));
	pads.emplace_back(std::make_unique<checkbox_pad>(cfg_location{ "Audio", "Convert to 16 bit" }, chbox_audio_conv));

	pads.emplace_back(std::make_unique<combobox_pad>(cfg_location{ "Input/Output", "Pad" }, cbox_pad_handler));
	pads.emplace_back(std::make_unique<combobox_pad>(cfg_location{ "Input/Output", "Keyboard" }, cbox_keyboard_handler));
	pads.emplace_back(std::make_unique<combobox_pad>(cfg_location{ "Input/Output", "Mouse" }, cbox_mouse_handler));
	pads.emplace_back(std::make_unique<combobox_pad>(cfg_location{ "Input/Output", "Camera" }, cbox_camera));
	pads.emplace_back(std::make_unique<combobox_pad>(cfg_location{ "Input/Output", "Camera type" }, cbox_camera_type));

	pads.emplace_back(std::make_unique<combobox_pad>(cfg_location{ "Net", "Connection status" }, cbox_net_status));

	pads.emplace_back(std::make_unique<combobox_pad>(cfg_location{ "System", "Language" }, cbox_sys_lang));

	pads.emplace_back(std::make_unique<checkbox_pad>(cfg_location{ "Miscellaneous", "Exit RPCS3 when process finishes" }, chbox_hle_exitonstop));
	pads.emplace_back(std::make_unique<checkbox_pad>(cfg_location{ "Miscellaneous", "Always start after boot" }, chbox_hle_always_start));
	pads.emplace_back(std::make_unique<checkbox_pad>(cfg_location{ "Miscellaneous", "Auto Pause at System Call" }, chbox_dbg_ap_systemcall));
	pads.emplace_back(std::make_unique<checkbox_pad>(cfg_location{ "Miscellaneous", "Auto Pause at Function Call" }, chbox_dbg_ap_functioncall));

#ifdef _MSC_VER
	Microsoft::WRL::ComPtr<IDXGIFactory4> dxgi_factory;

	if (SUCCEEDED(CreateDXGIFactory(IID_PPV_ARGS(&dxgi_factory))))
	{
		Microsoft::WRL::ComPtr<IDXGIAdapter> adapter;

		for (UINT id = 0; dxgi_factory->EnumAdapters(id, adapter.ReleaseAndGetAddressOf()) != DXGI_ERROR_NOT_FOUND; id++)
		{
			DXGI_ADAPTER_DESC desc;
			adapter->GetDesc(&desc);
			cbox_gs_d3d_adapter->Append(desc.Description);
		}

		pads.emplace_back(std::make_unique<combobox_pad>(cfg_location{ "Video", "D3D12", "Adapter" }, cbox_gs_d3d_adapter));
	}
	else
#endif
	{
		cbox_gs_d3d_adapter->Enable(false);
	}

	// Core
	s_round_core_lle->Add(chbox_list_core_lle, wxSizerFlags().Border(wxALL, 5).Expand());

	// Rendering
	s_round_gs_render->Add(cbox_gs_render, wxSizerFlags().Border(wxALL, 5).Expand());
	s_round_gs_d3d_adapter->Add(cbox_gs_d3d_adapter, wxSizerFlags().Border(wxALL, 5).Expand());
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

	// Networking
	s_round_net_status->Add(cbox_net_status, wxSizerFlags().Border(wxALL, 5).Expand());

	// System
	s_round_sys_lang->Add(cbox_sys_lang, wxSizerFlags().Border(wxALL, 5).Expand());

	// Core
	s_subpanel_core1->Add(rbox_ppu_decoder, wxSizerFlags().Border(wxALL, 5).Expand());
	s_subpanel_core1->Add(rbox_spu_decoder, wxSizerFlags().Border(wxALL, 5).Expand());
	s_subpanel_core1->Add(chbox_core_hook_stfunc, wxSizerFlags().Border(wxALL, 5).Expand());
	s_subpanel_core1->Add(chbox_core_load_liblv2, wxSizerFlags().Border(wxALL, 5).Expand());
	s_subpanel_core2->Add(s_round_core_lle, wxSizerFlags().Border(wxALL, 5).Expand());
	s_subpanel_core->Add(s_subpanel_core1);
	s_subpanel_core->Add(s_subpanel_core2);

	// Graphics
	s_subpanel_graphics1->Add(s_round_gs_render, wxSizerFlags().Border(wxALL, 5).Expand());
	s_subpanel_graphics1->Add(s_round_gs_res, wxSizerFlags().Border(wxALL, 5).Expand());
	s_subpanel_graphics1->Add(s_round_gs_d3d_adapter, wxSizerFlags().Border(wxALL, 5).Expand());
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
	s_subpanel_misc->Add(chbox_hle_exitonstop, wxSizerFlags().Border(wxALL, 5).Expand());
	s_subpanel_misc->Add(chbox_hle_always_start, wxSizerFlags().Border(wxALL, 5).Expand());

	// Auto Pause
	s_subpanel_misc->Add(chbox_dbg_ap_systemcall, wxSizerFlags().Border(wxALL, 5).Expand());
	s_subpanel_misc->Add(chbox_dbg_ap_functioncall, wxSizerFlags().Border(wxALL, 5).Expand());

	// Networking
	s_subpanel_networking->Add(s_round_net_status, wxSizerFlags().Border(wxALL, 5).Expand());

	// System
	s_subpanel_system->Add(chbox_vfs_enable_host_root, wxSizerFlags().Border(wxALL, 5).Expand());
	s_subpanel_system->Add(s_round_sys_lang, wxSizerFlags().Border(wxALL, 5).Expand());

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
		std::set<std::string> lle_selected;

		for (auto i = 0; i < lle_module_list.size(); i++)
		{
			if (chbox_list_core_lle->IsChecked(i))
			{
				lle_selected.emplace(lle_module_list[i]);
			}
		}

		saved.reset();
		saved["Core"]["Load libraries"] = std::vector<std::string>(lle_selected.begin(), lle_selected.end());

		for (auto& pad : pads)
		{
			pad->save();
		}

		loaded += saved;
		YAML::Emitter out;
		emit(out, loaded);

		// Save config
		config.seek(0);
		config.trunc(0);
		config.write(out.c_str(), out.size());
	}
}

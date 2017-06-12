#include "stdafx.h"
#include "stdafx_gui.h"
#include "Utilities/Config.h"
#include "Crypto/unself.h"
#include "Loader/ELF.h"
#include "Emu/System.h"

#ifdef _MSC_VER
#include <Windows.h>
#undef GetHwnd
#include <d3d12.h>
#include <wrl/client.h>
#include <dxgi1_4.h>
#endif

#include "Emu/RSX/VK/VKHelpers.h"
#include "SettingsDialog.h"

#include <set>
#include <unordered_set>
#include <algorithm>

// Node location
using cfg_location = std::vector<const char*>;

extern std::string g_cfg_defaults;

static YAML::Node loaded;
static YAML::Node saved;

// Emit sorted YAML
static NEVER_INLINE void emit(YAML::Emitter& out, const YAML::Node& node)
{
	// TODO
	out << node;
}

// Incrementally load YAML
static NEVER_INLINE void operator +=(YAML::Node& left, const YAML::Node& node)
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

	static cfg::_base& get_cfg(cfg::_base& root, const std::string& name)
	{
		if (root.get_type() == cfg::type::node)
		{
			for (const auto& pair : static_cast<cfg::node&>(root).get_nodes())
			{
				if (pair.first == name)
				{
					return *pair.second;
				}
			}
		}

		fmt::throw_exception("Node not found: %s", name);
	}

	static cfg::_base& get_cfg(cfg::_base& root, cfg_location::const_iterator begin, cfg_location::const_iterator end)
	{
		return begin == end ? root : get_cfg(get_cfg(root, *begin), begin + 1, end);
	}

	static YAML::Node get_node(YAML::Node node, cfg_location::const_iterator begin, cfg_location::const_iterator end)
	{
		return begin == end ? node : get_node(node[*begin], begin + 1, end); // TODO
	}

	cfg::_base& get_cfg() const
	{
		return get_cfg(g_cfg, location.cbegin(), location.cend());
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
		for (const auto& v : cfg_adapter::get_cfg(g_cfg, location.cbegin(), location.cend()).to_list())
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


SettingsDialog::SettingsDialog(wxWindow* parent, const std::string& path)
	: wxDialog(parent, wxID_ANY, "Settings", wxDefaultPosition)
{
	// Load default config
	loaded = YAML::Load(g_cfg_defaults);

	// Create config path if necessary
	fs::create_path(fs::get_config_dir() + path);

	// Incrementally load config.yml
	const fs::file config(fs::get_config_dir() + path + "/config.yml", fs::read + fs::write + fs::create);

	if (config.size() == 0 && !path.empty()) // First time
	{
		const fs::file configexisted(fs::get_config_dir() + "/config.yml", fs::read + fs::write + fs::create);
		loaded += YAML::Load(configexisted.to_string());
	}
	else
	{
		loaded += YAML::Load(config.to_string());
	}

	std::vector<std::unique_ptr<cfg_adapter>> pads;

	static const u32 width  = 512;
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
	chbox_list_core_lle = new wxCheckListBox(p_core, wxID_ANY, wxDefaultPosition, wxSize(-1,-1), {}, wxLB_EXTENDED);
	chbox_list_core_lle->Bind(wxEVT_CHECKLISTBOX, &SettingsDialog::OnModuleListItemToggled, this);
	s_module_search_box = new wxTextCtrl(p_core, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, {});
	s_module_search_box->Bind(wxEVT_TEXT, &SettingsDialog::OnSearchBoxTextChanged, this);

	// Graphics
	wxStaticBoxSizer* s_round_gs_render = new wxStaticBoxSizer(wxVERTICAL, p_graphics, "Render");
	wxStaticBoxSizer* s_round_gs_d3d_adapter = new wxStaticBoxSizer(wxVERTICAL, p_graphics, "D3D Adapter (DirectX 12 Only)");
	wxStaticBoxSizer* s_round_gs_vk_adapter = new wxStaticBoxSizer(wxVERTICAL, p_graphics, "Vulkan Adapter");
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
	wxComboBox* cbox_gs_vk_adapter = new wxComboBox(p_graphics, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(150, -1), 0, NULL, wxCB_READONLY);
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
	wxCheckBox* chbox_core_bind_spu_threads = new wxCheckBox(p_core, wxID_ANY, "Bind SPU threads to secondary cores");
	wxCheckBox* chbox_core_lower_spu_priority = new wxCheckBox(p_core, wxID_ANY, "Lower SPU thread priority");
	wxCheckBox* chbox_vfs_enable_host_root = new wxCheckBox(p_system, wxID_ANY, "Enable /host_root/");
	wxCheckBox* chbox_gs_log_prog = new wxCheckBox(p_graphics, wxID_ANY, "Log Shader Programs");
	wxCheckBox* chbox_gs_dump_depth = new wxCheckBox(p_graphics, wxID_ANY, "Write Depth Buffer");
	wxCheckBox* chbox_gs_dump_color = new wxCheckBox(p_graphics, wxID_ANY, "Write Color Buffers");
	wxCheckBox* chbox_gs_read_color = new wxCheckBox(p_graphics, wxID_ANY, "Read Color Buffers");
	wxCheckBox* chbox_gs_read_depth = new wxCheckBox(p_graphics, wxID_ANY, "Read Depth Buffer");
	wxCheckBox* chbox_gs_vsync = new wxCheckBox(p_graphics, wxID_ANY, "VSync");
	wxCheckBox* chbox_gs_debug_output = new wxCheckBox(p_graphics, wxID_ANY, "Debug Output");
	wxCheckBox* chbox_gs_overlay = new wxCheckBox(p_graphics, wxID_ANY, "Debug Overlay");
	wxCheckBox* chbox_gs_gl_legacy_buffers = new wxCheckBox(p_graphics, wxID_ANY, "Use Legacy OpenGL Buffers");
	wxCheckBox* chbox_gs_gpu_texture_scaling = new wxCheckBox(p_graphics, wxID_ANY, "Use GPU texture scaling");
	wxCheckBox* chbox_audio_dump = new wxCheckBox(p_audio, wxID_ANY, "Dump to file");
	wxCheckBox* chbox_audio_conv = new wxCheckBox(p_audio, wxID_ANY, "Convert to 16 bit");
	wxCheckBox* chbox_audio_dnmx = new wxCheckBox(p_audio, wxID_ANY, "Downmix to Stereo");
	wxCheckBox* chbox_hle_exitonstop = new wxCheckBox(p_misc, wxID_ANY, "Exit RPCS3 when process finishes");
	wxCheckBox* chbox_hle_always_start = new wxCheckBox(p_misc, wxID_ANY, "Always start after boot");
	wxCheckBox* chbox_dbg_ap_systemcall = new wxCheckBox(p_misc, wxID_ANY, "Auto Pause at System Call");
	wxCheckBox* chbox_dbg_ap_functioncall = new wxCheckBox(p_misc, wxID_ANY, "Auto Pause at Function Call");

	{
		// Sort string vector alphabetically
		static const auto sort_string_vector = [](std::vector<std::string>& vec)
		{
			std::sort(vec.begin(), vec.end(), [](const std::string &str1, const std::string &str2) { return str1 < str2; });
		};

		auto&& data = loaded["Core"]["Load libraries"].as<std::vector<std::string>, std::initializer_list<std::string>>({});
		sort_string_vector(data);

		// List selected modules first
		for (const auto& unk : data)
		{
			lle_module_list.insert(lle_module_list.end(), std::pair<std::string, bool>(unk, true));
			chbox_list_core_lle->Check(chbox_list_core_lle->Append(unk));
		}

		const std::string& lle_dir = Emu.GetLibDir(); // TODO

		std::unordered_set<std::string> set(data.begin(), data.end());
		std::vector<std::string> lle_module_list_unselected;

		for (const auto& prxf : fs::dir(lle_dir))
		{
			// List found unselected modules
			if (prxf.is_directory || (prxf.name.substr(std::max(size_t(3), prxf.name.length()) - 3)) != "prx")
				continue;
			if (verify_npdrm_self_headers(fs::file(lle_dir + prxf.name)) && !set.count(prxf.name))
			{
				lle_module_list_unselected.push_back(prxf.name);
			}

		}

		sort_string_vector(lle_module_list_unselected);

		for (const auto& prxf : lle_module_list_unselected)
		{
			lle_module_list.insert(lle_module_list.end(), std::pair<std::string, bool>(prxf, false));
			chbox_list_core_lle->Check(chbox_list_core_lle->Append(prxf), false);
		}

		lle_module_list_unselected.clear();
	}

	radiobox_pad_helper ppu_decoder_modes({ "Core", "PPU Decoder" });
	rbox_ppu_decoder = new wxRadioBox(p_core, wxID_ANY, "PPU Decoder", wxDefaultPosition, wxSize(-1, -1), ppu_decoder_modes, 1);
	pads.emplace_back(std::make_unique<radiobox_pad>(std::move(ppu_decoder_modes), rbox_ppu_decoder));
	rbox_ppu_decoder->Enable(0, false); // TODO

	radiobox_pad_helper spu_decoder_modes({ "Core", "SPU Decoder" });
	rbox_spu_decoder = new wxRadioBox(p_core, wxID_ANY, "SPU Decoder", wxDefaultPosition, wxSize(-1, -1), spu_decoder_modes, 1);
	pads.emplace_back(std::make_unique<radiobox_pad>(std::move(spu_decoder_modes), rbox_spu_decoder));
	rbox_spu_decoder->Enable(3, false); // TODO

	radiobox_pad_helper lib_loader_modes({ "Core", "Lib Loader" });
	rbox_lib_loader = new wxRadioBox(p_core, wxID_ANY, "Library Loading Mode", wxDefaultPosition, wxSize(-1, -1), lib_loader_modes, 1);
	pads.emplace_back(std::make_unique<radiobox_pad>(std::move(lib_loader_modes), rbox_lib_loader));
	rbox_lib_loader->Bind(wxEVT_COMMAND_RADIOBOX_SELECTED, &SettingsDialog::OnLibLoaderToggled, this);
	EnableModuleList(rbox_lib_loader->GetSelection());

	pads.emplace_back(std::make_unique<checkbox_pad>(cfg_location{ "Core", "Hook static functions" }, chbox_core_hook_stfunc));
	pads.emplace_back(std::make_unique<checkbox_pad>(cfg_location{ "Core", "Bind SPU threads to secondary cores" }, chbox_core_bind_spu_threads));
	pads.emplace_back(std::make_unique<checkbox_pad>(cfg_location{ "Core", "Lower SPU thread priority" }, chbox_core_lower_spu_priority));
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
	pads.emplace_back(std::make_unique<checkbox_pad>(cfg_location{ "Video", "Debug overlay" }, chbox_gs_overlay));
	pads.emplace_back(std::make_unique<checkbox_pad>(cfg_location{ "Video", "Use Legacy OpenGL Buffers (Debug)" }, chbox_gs_gl_legacy_buffers));
	pads.emplace_back(std::make_unique<checkbox_pad>(cfg_location{ "Video", "Use GPU texture scaling" }, chbox_gs_gpu_texture_scaling));

	pads.emplace_back(std::make_unique<combobox_pad>(cfg_location{ "Audio", "Renderer" }, cbox_audio_out));
	pads.emplace_back(std::make_unique<checkbox_pad>(cfg_location{ "Audio", "Dump to file" }, chbox_audio_dump));
	pads.emplace_back(std::make_unique<checkbox_pad>(cfg_location{ "Audio", "Convert to 16 bit" }, chbox_audio_conv));
	pads.emplace_back(std::make_unique<checkbox_pad>(cfg_location{ "Audio", "Downmix to Stereo" }, chbox_audio_dnmx));

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
	{
		// Removes D3D12 from Render list when the system doesn't support it
		cbox_gs_render->Delete(cbox_gs_render->FindString("D3D12"));
		cbox_gs_d3d_adapter->Enable(false);
	}
#else
	cbox_gs_d3d_adapter->Enable(false);
#endif

	//TODO: This is very slow. Only init once
	bool vulkan_supported = false;

	vk::context device_enum_context;
	u32 instance_handle = device_enum_context.createInstance("RPCS3", true);
	
	if (instance_handle > 0)
	{
		device_enum_context.makeCurrentInstance(instance_handle);
		std::vector<vk::physical_device>& gpus = device_enum_context.enumerateDevices();
		
		if (gpus.size() > 0)
		{
			//A device with vulkan support found. Init data
			vulkan_supported = true;

			for (auto& gpu : gpus)
			{
				cbox_gs_vk_adapter->Append(gpu.name());
			}

			pads.emplace_back(std::make_unique<combobox_pad>(cfg_location{ "Video", "Vulkan", "Adapter" }, cbox_gs_vk_adapter));
		}
	}
	
	if (!vulkan_supported)
	{
		// Removes Vulkan from Render list when the system doesn't support it
		cbox_gs_render->Delete(cbox_gs_render->FindString("Vulkan"));
		cbox_gs_vk_adapter->Enable(false);
	}

	device_enum_context.close();

	// Rendering
	s_round_gs_render->Add(cbox_gs_render, wxSizerFlags().Border(wxALL, 5).Expand());
	s_round_gs_d3d_adapter->Add(cbox_gs_d3d_adapter, wxSizerFlags().Border(wxALL, 5).Expand());
	s_round_gs_vk_adapter->Add(cbox_gs_vk_adapter, wxSizerFlags().Border(wxALL, 5).Expand());
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
	s_round_core_lle->Add(chbox_list_core_lle, wxSizerFlags().Border(wxALL, 4).Expand());
	s_round_core_lle->Add(s_module_search_box, wxSizerFlags().Border(wxALL, 4).Expand());
	s_subpanel_core1->Add(rbox_ppu_decoder, wxSizerFlags().Border(wxALL, 5).Expand());
	s_subpanel_core1->Add(rbox_spu_decoder, wxSizerFlags().Border(wxALL, 5).Expand());
	s_subpanel_core1->Add(chbox_core_hook_stfunc, wxSizerFlags().Border(wxALL, 5).Expand());
	s_subpanel_core1->Add(chbox_core_bind_spu_threads, wxSizerFlags().Border(wxALL, 5).Expand());
	s_subpanel_core1->Add(chbox_core_lower_spu_priority, wxSizerFlags().Border(wxALL, 5).Expand());
	s_subpanel_core2->Add(rbox_lib_loader, wxSizerFlags().Border(wxALL, 5).Expand());
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
	s_subpanel_graphics1->Add(chbox_gs_gl_legacy_buffers, wxSizerFlags().Border(wxALL, 5).Expand());
	s_subpanel_graphics2->Add(s_round_gs_aspect, wxSizerFlags().Border(wxALL, 5).Expand());
	s_subpanel_graphics2->Add(s_round_gs_frame_limit, wxSizerFlags().Border(wxALL, 5).Expand());
	s_subpanel_graphics2->Add(s_round_gs_vk_adapter, wxSizerFlags().Border(wxALL, 5).Expand());
	s_subpanel_graphics2->Add(chbox_gs_debug_output, wxSizerFlags().Border(wxALL, 5).Expand());
	s_subpanel_graphics2->Add(chbox_gs_overlay, wxSizerFlags().Border(wxALL, 5).Expand());
	s_subpanel_graphics2->Add(chbox_gs_log_prog, wxSizerFlags().Border(wxALL, 5).Expand());
	s_subpanel_graphics2->Add(chbox_gs_vsync, wxSizerFlags().Border(wxALL, 5).Expand());
	s_subpanel_graphics2->Add(chbox_gs_gpu_texture_scaling, wxSizerFlags().Border(wxALL, 5).Expand());
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
	s_subpanel_audio->Add(chbox_audio_dnmx, wxSizerFlags().Border(wxALL, 5).Expand());

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

		for (auto& i : lle_module_list)
		{
			if (i.second) // selected
			{
				lle_selected.emplace(i.first);
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

void SettingsDialog::OnModuleListItemToggled(wxCommandEvent &event)
{
	lle_module_list[fmt::ToUTF8(event.GetString())] = chbox_list_core_lle->IsChecked(event.GetSelection());
}

void SettingsDialog::OnSearchBoxTextChanged(wxCommandEvent &event)
{
	// helper to preserve alphabetically order while inserting items
	unsigned int item_index = 0;

	if (event.GetString().IsEmpty())
	{
		for (auto& i : lle_module_list)
		{
			if (i.second)
			{
				chbox_list_core_lle->Check(chbox_list_core_lle->Insert(i.first, item_index));
				item_index++;
			}

			else
			{
				chbox_list_core_lle->Check(chbox_list_core_lle->Insert(i.first, chbox_list_core_lle->GetCount()), false);
			}
		}
	}

	chbox_list_core_lle->Clear();

	wxString search_term = event.GetString().Lower();
	item_index = 0;

	for (auto& i : lle_module_list)
	{
		if (fmt::FromUTF8(i.first).Find(search_term) != wxString::npos)
		{
			if (i.second)
			{
				chbox_list_core_lle->Check(chbox_list_core_lle->Insert(i.first, item_index));
				item_index++;
			}

			else
			{
				chbox_list_core_lle->Check(chbox_list_core_lle->Insert(i.first, chbox_list_core_lle->GetCount()), false);
			}
		}
	}
}

void SettingsDialog::OnLibLoaderToggled(wxCommandEvent& event)
{
	EnableModuleList(event.GetSelection());
}

// Enables or disables the modul list interaction
void SettingsDialog::EnableModuleList(int selection)
{
	if (selection == 1 || selection == 2)
	{
		chbox_list_core_lle->Enable();
		s_module_search_box->Enable();
	}
	else
	{
		chbox_list_core_lle->Disable();
		s_module_search_box->Disable();
	}
}


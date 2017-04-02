#include "bin_patch.h"
#include "yaml-cpp/yaml.h"
#include "File.h"
#include "Config.h"

template <>
void fmt_class_string<patch_type>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](patch_type value)
	{
		switch (value)
		{
		case patch_type::byte: return "byte";
		case patch_type::le16: return "le16";
		case patch_type::le32: return "le32";
		case patch_type::le64: return "le64";
		case patch_type::be16: return "be16";
		case patch_type::be32: return "be32";
		case patch_type::be64: return "be64";
		}

		return unknown;
	});
}

void patch_engine::append(const std::string& patch)
{
	if (fs::file f{patch})
	{
		auto root = YAML::Load(f.to_string());

		for (auto pair : root)
		{
			auto& name = pair.first.Scalar();
			auto& data = m_map[name];
			
			for (auto patch : pair.second)
			{
				u64 type64 = 0;
				cfg::try_to_enum_value(&type64, &fmt_class_string<patch_type>::format, patch[0].Scalar());

				struct patch info;
				info.type   = static_cast<patch_type>(type64);
				info.offset = patch[1].as<u32>();
				info.value  = patch[2].as<u64>();
				data.emplace_back(info);
			}
		}
	}
}

void patch_engine::apply(const std::string& name, u8* dst) const
{
	const auto found = m_map.find(name);

	if (found == m_map.cend())
	{
		return;
	}

	// Apply modifications sequentially
	for (const auto& p : found->second)
	{
		auto ptr = dst + p.offset;

		switch (p.type)
		{
		case patch_type::byte:
		{
			*ptr = static_cast<u8>(p.value);
			break;
		}
		case patch_type::le16:
		{
			*reinterpret_cast<le_t<u16, 1>*>(ptr) = static_cast<u16>(p.value);
			break;
		}
		case patch_type::le32:
		{
			*reinterpret_cast<le_t<u32, 1>*>(ptr) = static_cast<u32>(p.value);
			break;
		}
		case patch_type::le64:
		{
			*reinterpret_cast<le_t<u64, 1>*>(ptr) = static_cast<u64>(p.value);
			break;
		}
		case patch_type::be16:
		{
			*reinterpret_cast<be_t<u16, 1>*>(ptr) = static_cast<u16>(p.value);
			break;
		}
		case patch_type::be32:
		{
			*reinterpret_cast<be_t<u32, 1>*>(ptr) = static_cast<u32>(p.value);
			break;
		}
		case patch_type::be64:
		{
			*reinterpret_cast<be_t<u64, 1>*>(ptr) = static_cast<u64>(p.value);
			break;
		}
		}
	}
}

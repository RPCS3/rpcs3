#include "stdafx.h"
#include "cheat_info.h"
#include "Emu/System.h"
#include "Emu/IdManager.h"
#include "Emu/NP/np_handler.h"
#include "Emu/CPU/CPUThread.h"
#include "Emu/Memory/vm.h"

#include <charconv>

LOG_CHANNEL(log_cheat, "Cheat");

template <>
void fmt_class_string<cheat_type>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](cheat_type value)
		{
			switch (value)
			{
			case cheat_type::normal: return "Normal";
			case cheat_type::constant: return "Constant";
			default: return unknown;
			}
		});
}

template <>
void fmt_class_string<cheat_inst>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](cheat_inst value)
		{
			switch (value)
			{
			case cheat_inst::write_bytes: return "write_bytes";
			case cheat_inst::or_bytes: return "or_bytes";
			case cheat_inst::and_bytes: return "and_bytes";
			case cheat_inst::xor_bytes: return "xor_bytes";
			case cheat_inst::write_text: return "write_text";
			case cheat_inst::write_float: return "write_float";
			case cheat_inst::write_condensed: return "write_condensed";
			case cheat_inst::read_pointer: return "read_pointer";
			case cheat_inst::copy: return "copy";
			case cheat_inst::paste: return "paste";
			case cheat_inst::find_replace: return "find_replace";
			case cheat_inst::compare_cond: return "compared_cond";
			case cheat_inst::and_compare_cond: return "and_compare_cond";
			case cheat_inst::copy_bytes: return "copy_bytes";
			default: return unknown;
			}
		});
}

cheat_engine g_cheat_engine;

cheat_executor::cheat_executor(std::string_view game_name, std::string_view cheat_name, const cheat_entry& entry, std::unordered_map<std::string, std::string> var_choices)
	: m_game_name(game_name), m_cheat_name(cheat_name), m_entry(entry), m_var_choices(var_choices)
{
}

std::pair<const std::string&, const std::string&> cheat_executor::get_name() const
{
	return {m_game_name, m_cheat_name};
}

bool cheat_executor::is(std::string_view game_name, std::string_view cheat_name) const
{
	return (m_game_name == game_name && m_cheat_name == cheat_name);
}

bool cheat_executor::operator<(const cheat_executor& rhs) const
{
	return m_game_name < rhs.m_game_name || m_cheat_name < rhs.m_cheat_name;
}

std::string cheat_executor::parse_value(const std::string& to_parse) const
{
	std::string variable;
	std::string final_string;

	auto check_variable = [&]()
	{
		if (!variable.empty())
		{
			final_string += m_var_choices.at(variable);
			variable.clear();
		}
	};

	for (auto it = to_parse.begin(); it != to_parse.end(); it++)
	{
		if (*it == 'Z')
		{
			variable += 'Z';
			continue;
		}

		check_variable();
		final_string += *it;
	}

	check_variable();

	return final_string;
}

std::optional<u32> cheat_executor::parse_value_to_u32(const std::string& to_parse) const
{
	const std::string final_str = parse_value(to_parse);
	u32 result;
	if (std::from_chars(final_str.data(), final_str.data() + final_str.size(), result, 16).ec != std::errc())
	{
		log_cheat.warning("Couldn't parse %s into an u32!", final_str);
		return std::nullopt;
	}
	return result;
}

std::optional<float> cheat_executor::parse_value_to_float(const std::string& to_parse) const
{
	const std::string final_str = parse_value(to_parse);
	char* str_end               = nullptr;
	float result                = std::strtof(final_str.data(), &str_end);

	if (str_end != (final_str.data() + final_str.size()))
	{
		return std::nullopt;
	}

	// Fails on FreeBSD and MacOS which don't support FP from_chars
	// if (std::from_chars(final_str.data(), final_str.data() + final_str.size(), result).ec != std::errc())
	// {
	// 	log_cheat.warning("Couldn't parse %s into a float!", final_str);
	// 	return std::nullopt;
	// }

	return result;
}

std::optional<std::vector<u8>> cheat_executor::parse_value_to_vector(const std::string& to_parse) const
{
	std::string final_str = parse_value(to_parse);
	std::vector<u8> result;
	if (final_str.size() % 2 != 0)
	{
		log_cheat.warning("Added a leading 0 to %s!", final_str);
		final_str = "0" + final_str;
	}

	for (size_t i = 0; i < final_str.size(); i += 2)
	{
		u8 val;
		if (std::from_chars(final_str.data() + i, final_str.data() + i + 2, val, 16).ec != std::errc())
		{
			log_cheat.error("Failed to parse %s into a vector of bytes!", final_str);
			return std::nullopt;
		}
		result.push_back(val);
	}
	return result;
}

bool cheat_executor::valid_range(u32 addr, u32 size)
{
	const u32 begin    = addr - (addr % 4096);
	const u32 end_addr = addr + size;
	const u32 end      = end_addr - (end_addr % 4096);

	for (u32 index = begin; index <= end; index += 4096)
	{
		if (!vm::check_addr(index))
		{
			return false;
		}
	}

	return true;
}

bool cheat_executor::execute(bool pause) const
{
	if (Emu.IsStopped())
	{
		log_cheat.error("Attempted to execute a cheat while the emulator was stopped!");
		return false;
	}

	if (g_fxo->get<named_thread<np::np_handler>>().get_psn_status() == SCE_NP_MANAGER_STATUS_ONLINE)
	{
		return true;
	}

	log_cheat.notice("Applying cheat '%s':'%s'", m_game_name, m_cheat_name);

	auto apply_cheat = [&]
	{
		u32 ptr_addr = 0;
		usz skip     = 0;
		std::vector<u8> copy_buf;

		for (const auto& code : m_entry.codes)
		{
			u32 addr = 0;

			if (skip)
			{
				skip--;
				continue;
			}

			if (ptr_addr)
			{
				addr     = ptr_addr;
				ptr_addr = 0;
			}
			else
			{
				auto opt_addr = parse_value_to_u32(code.addr);
				if (!opt_addr)
				{
					return;
				}
				addr = *opt_addr;
			}

			auto memory_op = [](cheat_inst type, u8* dest, u8* src, u32 size)
			{
				switch (type)
				{
				case cheat_inst::write_bytes:
					memcpy(dest, src, size);
					break;
				case cheat_inst::or_bytes:
					for (u32 i = 0; i < size; i++)
					{
						*(dest + i) |= src[i];
					}
					break;
				case cheat_inst::and_bytes:
					for (u32 i = 0; i < size; i++)
					{
						*(dest + i) &= src[i];
					}
					break;
				case cheat_inst::xor_bytes:
					for (u32 i = 0; i < size; i++)
					{
						*(dest + i) ^= src[i];
					}
					break;
				default:
					log_cheat.error("Unexpected code type in memory_op()");
					break;
				}
			};

			switch (code.type)
			{
			case cheat_inst::write_bytes:
			case cheat_inst::or_bytes:
			case cheat_inst::and_bytes:
			case cheat_inst::xor_bytes:
			{
				auto data = parse_value_to_vector(code.value);
				if (!data)
				{
					return;
				}
				if (!valid_range(addr, data->size()))
				{
					log_cheat.error("Error validating range for %s", code.type);
					return;
				}

				memory_op(code.type, vm::get_super_ptr(addr), data->data(), data->size());
				break;
			}
			case cheat_inst::write_text:
			{
				if (!valid_range(addr, code.value.size()))
				{
					log_cheat.error("Error validating range for write_text");
					return;
				}
				memcpy(vm::get_super_ptr(addr), code.value.c_str(), std::strlen(code.value.c_str()));
				break;
			}
			case cheat_inst::write_float:
			{
				auto data = parse_value_to_float(code.value);
				if (!data)
				{
					return;
				}
				if (!valid_range(addr, sizeof(float)))
				{
					log_cheat.error("Error validating range for write_float");
					return;
				}
				*vm::get_super_ptr<be_t<float>>(addr) = *data;
			}
			case cheat_inst::write_condensed:
			{
				auto data          = parse_value_to_vector(code.value);
				auto opt_increment = parse_value_to_u32(code.opt1);
				auto opt_count     = parse_value_to_u32(code.opt2);

				if (!data || !opt_increment || !opt_count)
				{
					return;
				}
				auto increment = *opt_increment;
				auto count     = *opt_count;

				if (!valid_range(addr, (increment * count) + data->size()))
				{
					log_cheat.error("Error validating range for write_condensed");
					return;
				}

				u8* ptr = vm::get_super_ptr(addr);

				for (u32 i = 0; i < count; i++)
				{
					memcpy(ptr + (count * increment), data->data(), data->size());
				}
				break;
			}
			case cheat_inst::read_pointer:
			{
				auto increment = parse_value_to_u32(code.value);
				if (!increment)
				{
					return;
				}

				if (!valid_range(addr, sizeof(u32)))
				{
					log_cheat.error("Error validating range for read_pointer");
					return;
				}

				ptr_addr = (*vm::get_super_ptr<be_t<u32>>(addr)) + *increment;
				break;
			}
			case cheat_inst::copy:
			{
				auto size = parse_value_to_u32(code.value);
				if (!size)
				{
					return;
				}

				if (!valid_range(addr, *size))
				{
					log_cheat.error("Error validating range for copy");
					return;
				}

				copy_buf.resize(*size);
				memcpy(copy_buf.data(), vm::get_super_ptr(addr), *size);
				break;
			}
			case cheat_inst::paste:
			{
				auto size = parse_value_to_u32(code.value);
				if (!size)
				{
					return;
				}

				if (!valid_range(addr, *size))
				{
					log_cheat.error("Error validating range for paste");
					return;
				}

				if (*size > copy_buf.size())
				{
					log_cheat.error("copy_buf buffer is too small for paste");
					return;
				}

				memcpy(vm::get_super_ptr(addr), copy_buf.data(), *size);
				break;
			}
			case cheat_inst::find_replace:
			{
				auto end_addr   = parse_value_to_u32(code.value);
				auto to_find    = parse_value_to_vector(code.opt1);
				auto to_replace = parse_value_to_vector(code.opt2);
				if (!end_addr || !to_find || !to_replace)
				{
					return;
				}

				if (*end_addr < addr)
				{
					std::swap(*end_addr, addr);
				}

				for (u32 i = addr; i < *end_addr; i++)
				{
					if (!valid_range(i, to_find->size()))
					{
						continue;
					}
					if (memcmp(vm::get_super_ptr(i), to_find->data(), to_find->size()) == 0)
					{
						if (!valid_range(i, to_replace->size()))
						{
							log_cheat.error("Failed to validate range for the replacement in find_replace");
							continue;
						}
						memcpy(vm::get_super_ptr(i), to_replace->data(), to_replace->size());
						break;
					}
				}
				break;
			}
			case cheat_inst::compare_cond:
			{
				auto to_skip = parse_value_to_u32(code.opt1);
				auto data    = parse_value_to_vector(code.value);
				if (!to_skip || !data)
				{
					return;
				}

				if (!valid_range(addr, data->size()))
				{
					log_cheat.error("Failed to validate range for compared_cond");
					return;
				}

				if (memcmp(vm::get_super_ptr(addr), data->data(), data->size()) != 0)
				{
					skip = *to_skip;
				}

				break;
			}
			case cheat_inst::and_compare_cond:
			{
				auto to_skip = parse_value_to_u32(code.opt1);
				auto data    = parse_value_to_vector(code.value);
				if (!to_skip || !data)
				{
					return;
				}

				if (!valid_range(addr, data->size()))
				{
					log_cheat.error("Failed to validate range for compared_cond");
					return;
				}

				u8* ptr = vm::get_super_ptr(addr);

				bool matched = true;

				for (usz i = 0; i < data->size(); i++)
				{
					if ((ptr[i] & (*data)[i]) != (*data)[i])
					{
						matched = false;
						break;
					}
				}

				if (!matched)
				{
					skip = *to_skip;
				}

				break;
			}
			case cheat_inst::copy_bytes:
			{
				auto dest_addr = parse_value_to_u32(code.value);
				auto num_bytes = parse_value_to_u32(code.opt1);
				if (!dest_addr || !num_bytes)
				{
					return;
				}

				if (!valid_range(addr, *num_bytes) || !valid_range(*dest_addr, *num_bytes))
				{
					log_cheat.error("Failed to validate range for copy_bytes");
					return;
				}

				memcpy(vm::get_super_ptr(addr), vm::get_super_ptr(*dest_addr), *num_bytes);
				break;
			}
			default:
				log_cheat.error("Encountered invalid cheat_inst during execution!");
				return;
			}
		}
	};

	if (pause)
	{
		cpu_thread::suspend_all(nullptr, {}, apply_cheat);
	}
	else
	{
		apply_cheat();
	}

	return true;
}

const std::set<cheat_executor>& cheat_engine::get_active_constant_cheats() const
{
	return m_constant_cheats;
}

const std::set<cheat_executor>& cheat_engine::get_queued_cheats() const
{
	return m_queued_cheats;
}

void cheat_engine::clear()
{
	std::lock_guard lock_constant(m_mutex_constant);
	std::lock_guard lock_queued(m_mutex_queued);
	m_constant_cheats.clear();
	m_queued_cheats.clear();
}

bool cheat_engine::activate_cheat(const std::string& game_name, const std::string& cheat_name, cheat_entry entry, std::unordered_map<std::string, std::string> var_choices)
{
	cheat_executor ce(game_name, cheat_name, entry, var_choices);

	if (entry.type == cheat_type::constant)
	{
		std::lock_guard lock(m_mutex_constant);
		return m_constant_cheats.insert(std::move(ce)).second;
	}

	if (Emu.IsStopped())
	{
		std::lock_guard lock(m_mutex_queued);
		return m_queued_cheats.insert(std::move(ce)).second;
	}

	return ce.execute();
}

bool cheat_engine::deactivate_cheat(const std::string& game_name, const std::string& cheat_name)
{
	{
		std::lock_guard lock(m_mutex_constant);

		for (auto it = m_constant_cheats.begin(); it != m_constant_cheats.end(); it++)
		{
			if (it->is(game_name, cheat_name))
			{
				m_constant_cheats.erase(it);
				return true;
			}
		}
	}

	{
		std::lock_guard lock(m_mutex_queued);

		for (auto it = m_queued_cheats.begin(); it != m_queued_cheats.end(); it++)
		{
			if (it->is(game_name, cheat_name))
			{
				m_queued_cheats.erase(it);
				return true;
			}
		}
	}

	return false;
}

void cheat_engine::apply_queued_cheats()
{
	for (const auto& cheat : m_queued_cheats)
	{
		cheat.execute(false);
	}
}

void cheat_engine::operator()()
{
	// TODO: constant cheats
	// while (thread_ctrl::state() != thread_state::aborting && !Emu.IsStopped())
	// {

	// }
}

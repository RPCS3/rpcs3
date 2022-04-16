#pragma once

#include "util/types.hpp"
#include "Utilities/mutex.h"

#include <string>
#include <set>
#include <optional>
#include <unordered_map>

enum class cheat_inst : u8
{
	write_bytes,
	or_bytes,
	and_bytes,
	xor_bytes,

	write_text,
	write_float,
	write_condensed,

	read_pointer,

	copy,
	paste,
	find_replace,
	compare_cond,
	and_compare_cond,
	copy_bytes,
};

enum class cheat_type : u8
{
	normal,
	constant,
};

struct cheat_code
{
	cheat_inst type{};
	std::string addr, value, opt1, opt2;
};

struct cheat_entry
{
	cheat_type type{};
	std::string author;
	std::vector<cheat_code> codes;
	std::map<std::string, std::map<std::string, std::string>> variables;
	std::vector<std::string> comments;
};

class cheat_executor
{
public:
	cheat_executor(std::string_view game_name, std::string_view cheat_name, const cheat_entry& entry, std::unordered_map<std::string, std::string> var_choices);
	bool is(std::string_view game_name, std::string_view cheat_name) const;
	bool operator<(const cheat_executor& rhs) const;
	bool execute(bool pause = true) const;

	std::pair<const std::string&, const std::string&> get_name() const;

private:
	std::string parse_value(const std::string& to_parse) const;
	std::optional<u32> parse_value_to_u32(const std::string& to_parse) const;
	std::optional<float> parse_value_to_float(const std::string& to_parse) const;
	std::optional<std::vector<u8>> parse_value_to_vector(const std::string& to_parse) const;

	static bool valid_range(u32 addr, u32 size);

private:
	std::string m_game_name;
	std::string m_cheat_name;
	cheat_entry m_entry{};
	std::unordered_map<std::string, std::string> m_var_choices;
};

class cheat_engine
{
public:
	cheat_engine()  = default;
	~cheat_engine() = default;

	const std::set<cheat_executor>& get_active_constant_cheats() const;
	const std::set<cheat_executor>& get_queued_cheats() const;

	void clear();

	bool activate_cheat(const std::string& game_name, const std::string& cheat_name, cheat_entry entry, std::unordered_map<std::string, std::string> var_choices);
	bool deactivate_cheat(const std::string& game_name, const std::string& cheat_name);

	void apply_queued_cheats();

	void operator()();

public:
	static constexpr std::string_view thread_name = "Cheat Thread";

private:
	shared_mutex m_mutex_constant, m_mutex_queued;
	std::set<cheat_executor> m_constant_cheats, m_queued_cheats;
};

extern cheat_engine g_cheat_engine;

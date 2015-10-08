#include "stdafx.h"
#include "config_context.h"
#include "convert.h"
#include "StrFmt.h"
#include <iostream>
#include <fstream>

void config_context_t::group::init()
{
	m_cfg->m_groups[full_name()] = this;
}

config_context_t::group::group(config_context_t* cfg, const std::string& name)
	: m_cfg(cfg)
	, m_name(name)
	, m_parent(nullptr)
{
	init();
}

config_context_t::group::group(group* parent, const std::string& name)
	: m_cfg(parent->m_cfg)
	, m_name(name)
	, m_parent(parent)
{
	init();
}

void config_context_t::group::set_parent(config_context_t* cfg)
{
	m_cfg = cfg;
	init();
}

std::string config_context_t::group::name() const
{
	return m_name;
}

std::string config_context_t::group::full_name() const
{
	if (m_parent)
		return m_parent->full_name() + "/" + m_name;

	return m_name;
}

void config_context_t::assign(const config_context_t& rhs)
{
	for (auto &rhs_g : rhs.m_groups)
	{
		auto g = m_groups.at(rhs_g.first);

		for (auto rhs_e : rhs_g.second->entries)
		{
			g->entries[rhs_e.first]->value_from(rhs_e.second);
		}
	}
}

void config_context_t::path(const std::string &new_path)
{
	m_path = new_path;
}

std::string config_context_t::path() const
{
	return m_path;
}

void config_context_t::load()
{
	set_defaults();

	uint line_index = 0;
	std::string line;
	group *current_group = nullptr;

	if (auto &stream = std::ifstream{ m_path })
	{
		while (std::getline(stream, line))
		{
			++line_index;
			line = fmt::trim(line);

			if (line.empty())
				continue;

			if (line[0] == '[')
			{
				std::string group_name = line.substr(1, line.length() - 2);

				auto found = m_groups.find(group_name);

				if (found == m_groups.end())
				{
					std::cerr << m_path << " " << line_index << ": group '" << group_name << "' not exists. ignored" << std::endl;
					current_group = nullptr;
					continue;
				}

				current_group = found->second;
				continue;
			}

			if (current_group == nullptr)
			{
				std::cerr << m_path << " " << line_index << ": line '" << line << "' ignored, no group." << std::endl;
				continue;
			}

			auto name_value = fmt::split(line, { "=" });
			switch (name_value.size())
			{
			case 1: current_group->entries[name_value[0]]->string_value(""); break;

			default:
				std::cerr << m_path << " " << line_index << ": line '" << line << "' has more than one symbol '='. used only first" << std::endl;
			case 2: current_group->entries[name_value[0]]->string_value(name_value[1]); break;

			}

		}
	}
}

void config_context_t::save()
{
	if (auto &stream = std::ofstream{ m_path })
	{
		for (auto &g : m_groups)
		{
			stream << "[" + g.first + "]" << std::endl;

			for (auto &e : g.second->entries)
			{
				stream << e.first << "=" << e.second->string_value() << std::endl;
			}

			stream << std::endl;
		}
	}
}

void config_context_t::set_defaults()
{
	for (auto &g : m_groups)
	{
		for (auto &e : g.second->entries)
		{
			e.second->to_default();
		}
	}
}
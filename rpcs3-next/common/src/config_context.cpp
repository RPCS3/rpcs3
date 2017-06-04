#include "config_context.h"
#include "fmt.h"
#include <iostream>
#include <sstream>

namespace common
{
	void config_context::group::init()
	{
		m_cfg->m_groups[full_name()] = this;
	}

	config_context::group::group(config_context* cfg, const std::string& name)
		: m_parent(nullptr)
		, m_cfg(cfg)
		, m_name(name)
	{
		init();
	}

	config_context::group::group(group* parent, const std::string& name)
		: m_parent(parent)
		, m_cfg(parent->m_cfg)
		, m_name(name)
	{
		init();
	}

	void config_context::group::set_parent(config_context* cfg)
	{
		m_cfg = cfg;
		init();
	}

	std::string config_context::group::name() const
	{
		return m_name;
	}

	std::string config_context::group::full_name() const
	{
		if (m_parent)
			return m_parent->full_name() + "/" + m_name;

		return m_name;
	}

	void config_context::assign(const config_context& rhs)
	{
		for (auto &rhs_g : rhs.m_groups)
		{
			auto g = m_groups.at(rhs_g.first);

			for (auto rhs_e : rhs_g.second->entries)
			{
				if (auto entry = g->entries[rhs_e.first])
					entry->value_from(rhs_e.second);
				else
					g->add_entry(rhs_e.first, rhs_e.second->string_value());
			}
		}
	}

	void config_context::deserialize(std::istream& stream)
	{
		set_defaults();

		uint line_index = 0;
		std::string line;
		group *current_group = nullptr;

		while (std::getline(stream, line))
		{
			++line_index;
			line = fmt::trim(line);

			if (line.empty())
				continue;

			if (line.front() == '[' && line.back() == ']')
			{
				std::string group_name = line.substr(1, line.length() - 2);

				auto found = m_groups.find(group_name);

				if (found == m_groups.end())
				{
					std::cerr << line_index << ": group '" << group_name << "' not exists. ignored" << std::endl;
					current_group = nullptr;
					continue;
				}

				current_group = found->second;
				continue;
			}

			if (current_group == nullptr)
			{
				std::cerr << line_index << ": line '" << line << "' ignored, no group." << std::endl;
				continue;
			}

			auto name_value = fmt::split(line, { "=" });
			switch (name_value.size())
			{
			case 1:
			{
				if (auto entry = current_group->entries[fmt::trim(name_value[0])])
					entry->string_value({});

				else
					current_group->add_entry(fmt::trim(name_value[0]), std::string{});
			}
			break;

			default:
				std::cerr << line_index << ": line '" << line << "' has more than one symbol '='. used only first" << std::endl;
			case 2:
			{
				if (auto entry = current_group->entries[fmt::trim(name_value[0])])
					entry->string_value(fmt::trim(name_value[1]));
				else
					current_group->add_entry(fmt::trim(name_value[0]), fmt::trim(name_value[1]));
			}
			break;
			}
		}
	}

	void config_context::serialize(std::ostream& stream) const
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

	void config_context::set_defaults()
	{
		for (auto &g : m_groups)
		{
			for (auto &e : g.second->entries)
			{
				e.second->to_default();
			}
		}
	}

	std::string config_context::to_string() const
	{
		std::ostringstream result;

		serialize(result);

		return result.str();
	}

	void config_context::add_group(const std::string& name)
	{
		new group(this, name);
	}

	config_context::group& config_context::get_group(const std::string& name)
	{
		if (!m_groups[name])
			add_group(name);

		return *m_groups[name];
	}
}
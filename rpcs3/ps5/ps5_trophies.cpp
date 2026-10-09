// PS5: the launcher's Trophies tab's reading (ps5_trophies.h).

#include "stdafx.h"
#include "ps5_trophies.h"

#include "Emu/System.h"
#include "Emu/system_utils.hpp"
#include "Loader/TROPUSR.h"
#include "Utilities/File.h"
#include "Utilities/StrUtil.h"

#include <algorithm>
#include <ctime>

LOG_CHANNEL(trophies_log, "Trophies");

namespace ps5_trophies
{
	namespace
	{
		// Each grade's points, which the PS3 counts a set's progress in
		constexpr u32 c_points[4] = {180, 90, 30, 15};

		grade grade_of(const std::string& ttype)
		{
			switch (ttype.empty() ? '\0' : ttype[0])
			{
			case 'P': return grade::platinum;
			case 'G': return grade::gold;
			case 'S': return grade::silver;
			case 'B': return grade::bronze;
			default: return grade::unknown;
			}
		}

		// Spaces and line breaks run together, as the Trophy Manager shows them
		std::string simplified(const std::string& text)
		{
			std::string out;
			bool space = false;
			for (const char c : text)
			{
				if (c == ' ' || c == '\t' || c == '\r' || c == '\n')
				{
					space = !out.empty();
					continue;
				}
				if (space)
				{
					out += ' ';
					space = false;
				}
				out += c;
			}
			return out;
		}

		// TROPUSR.DAT's earned states and times, by trophy, in the file's order
		// (as TROPUSRLoader reads them, without its rewriting a file it cannot)
		struct earned_state
		{
			bool earned = false;
			u64 tick = 0;
		};

		std::vector<earned_state> read_tropusr(const std::string& path)
		{
			std::vector<earned_state> states;
			fs::file file(path);
			if (!file)
			{
				return states;
			}

			TROPUSRHeader header{};
			if (!file.read(header) || header.magic != 0x818F54ADu || header.tables_count > 64u)
			{
				trophies_log.warning("%s is not a TROPUSR.DAT", path);
				return states;
			}

			std::vector<TROPUSRTableHeader> tables;
			file.seek(0x30);
			if (!file.read(tables, header.tables_count))
			{
				return states;
			}

			for (const TROPUSRTableHeader& table : tables)
			{
				if (table.type != 6u || table.entries_count > 4096u)
				{
					continue;
				}
				std::vector<TROPUSREntry6> entries;
				file.seek(table.offset);
				if (!file.read(entries, table.entries_count))
				{
					trophies_log.warning("%s: its earned table is cut short", path);
					return {};
				}
				for (const TROPUSREntry6& entry : entries)
				{
					states.push_back({entry.trophy_state != 0u, entry.timestamp2});
				}
			}
			return states;
		}

		bool read_set(const std::string& path, const std::string& name, game& out)
		{
			fs::file config(path + "TROPCONF.SFM");
			if (!config)
			{
				return false;
			}

			trophy_xml_document doc{};
			if (!doc.Read(config.to_string()))
			{
				trophies_log.warning("%s: TROPCONF.SFM is not readable XML", name);
				return false;
			}
			const std::shared_ptr<rXmlNode> base = doc.GetRoot();
			if (!base)
			{
				return false;
			}

			// The add-ons' names, for the trophies that belong to one
			std::vector<std::pair<std::string, std::string>> groups;
			for (std::shared_ptr<rXmlNode> n = base->GetChildren(); n; n = n->GetNext())
			{
				const std::string tag = n->GetName();
				if (tag == "title-name")
				{
					out.name = simplified(n->GetNodeContent());
				}
				else if (tag == "group")
				{
					for (std::shared_ptr<rXmlNode> child = n->GetChildren(); child; child = child->GetNext())
					{
						if (child->GetName() == "name")
						{
							groups.emplace_back(n->GetAttribute("id"), simplified(child->GetNodeContent()));
						}
					}
				}
			}

			for (std::shared_ptr<rXmlNode> n = base->GetChildren(); n; n = n->GetNext())
			{
				if (n->GetName() != "trophy")
				{
					continue;
				}

				trophy t;
				t.id = static_cast<u32>(std::max(0, std::atoi(n->GetAttribute("id").c_str())));
				t.kind = grade_of(n->GetAttribute("ttype"));
				const std::string hidden = n->GetAttribute("hidden");
				t.hidden = !hidden.empty() && hidden[0] == 'y';
				for (std::shared_ptr<rXmlNode> child = n->GetChildren(); child; child = child->GetNext())
				{
					const std::string tag = child->GetName();
					if (tag == "name")
					{
						t.name = simplified(child->GetNodeContent());
					}
					else if (tag == "detail")
					{
						t.detail = simplified(child->GetNodeContent());
					}
				}
				if (const std::string gid = n->GetAttribute("gid"); !gid.empty())
				{
					const auto found = std::find_if(groups.begin(), groups.end(), [&](const auto& g) { return g.first == gid; });
					t.group = found != groups.end() ? found->second : "Add-on " + gid;
				}
				t.icon_path = fmt::format("%sTROP%03d.PNG", path, t.id);
				out.trophies.push_back(std::move(t));
			}

			if (out.trophies.empty())
			{
				return false;
			}

			std::sort(out.trophies.begin(), out.trophies.end(), [](const trophy& a, const trophy& b) { return a.id < b.id; });

			// TROPUSR.DAT holds the trophies in their ids' order, as RPCS3 writes it
			const std::vector<earned_state> states = read_tropusr(path + "TROPUSR.DAT");
			u32 points = 0, earned_points = 0;
			for (trophy& t : out.trophies)
			{
				if (t.id < states.size())
				{
					t.earned = states[t.id].earned;
					t.earned_tick = t.earned ? states[t.id].tick : 0;
				}
				const usz g = static_cast<usz>(t.kind);
				if (g < 4)
				{
					out.count[g]++;
					points += c_points[g];
					if (t.earned)
					{
						out.earned_count[g]++;
						earned_points += c_points[g];
					}
				}
				if (t.earned)
				{
					out.earned++;
					out.last_tick = std::max(out.last_tick, t.earned_tick);
				}
			}
			out.progress = points ? static_cast<f32>(earned_points) / points : static_cast<f32>(out.earned) / out.trophies.size();
			if (out.name.empty())
			{
				out.name = name;
			}
			return true;
		}
	}

	std::string folder()
	{
		const std::string user = Emu.GetUsr().empty() ? std::string("00000001") : Emu.GetUsr();
		return rpcs3::utils::get_hdd0_dir() + "home/" + user + "/trophy/";
	}

	std::vector<game> read_all()
	{
		std::vector<game> games;
		const std::string dir = folder();
		fs::dir listing{dir};
		if (!listing)
		{
			trophies_log.notice("No trophy folder (%s)", dir);
			return games;
		}

		for (const fs::dir_entry& entry : listing)
		{
			if (!entry.is_directory || entry.name == "." || entry.name == "..")
			{
				continue;
			}
			game g;
			g.folder = entry.name;
			g.path = dir + entry.name + "/";
			if (fs::is_file(g.path + "ICON0.PNG"))
			{
				g.icon_path = g.path + "ICON0.PNG";
			}
			if (read_set(g.path, entry.name, g))
			{
				games.push_back(std::move(g));
			}
		}

		std::sort(games.begin(), games.end(), [](const game& a, const game& b)
		{
			if (a.last_tick != b.last_tick)
			{
				return a.last_tick > b.last_tick;
			}
			return a.name < b.name;
		});
		return games;
	}

	std::string date_text(u64 tick)
	{
		if (!tick)
		{
			return {};
		}

		// From 0001-01-01 to 1970-01-01: 719162 days
		const s64 seconds = static_cast<s64>(tick / 1'000'000) - 719162ll * 86400;
		const std::time_t when = static_cast<std::time_t>(seconds);
		std::tm parts{};
		if (!localtime_r(&when, &parts) && !gmtime_r(&when, &parts))
		{
			return {};
		}
		static constexpr const char* months[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
		return fmt::format("%d %s %d", parts.tm_mday, months[std::clamp(parts.tm_mon, 0, 11)], parts.tm_year + 1900);
	}

	const char* grade_text(grade kind)
	{
		switch (kind)
		{
		case grade::platinum: return "Platinum";
		case grade::gold: return "Gold";
		case grade::silver: return "Silver";
		case grade::bronze: return "Bronze";
		default: return "Trophy";
		}
	}
}

#include "stdafx.h"
#include "title.h"
#include "rpcs3_version.h"

namespace rpcs3
{
	std::string get_formatted_title(const title_format_data& title_data)
	{
		// Get version by substringing VersionNumber-buildnumber-commithash to get just the part before the dash
		std::string version = rpcs3::get_version().to_string();
		const auto last_minus = version.find_last_of('-');

		// Add branch and commit hash to version on frame unless it's master.
		if (rpcs3::get_branch() != "master"sv && rpcs3::get_branch() != "HEAD"sv)
		{
			version = version.substr(0, ~last_minus ? last_minus + 9 : last_minus);
			version += '-';
			version += rpcs3::get_branch();
		}
		else
		{
			version = version.substr(0, last_minus);
		}

		// Parse title format string
		std::string title_string;

		for (std::size_t i = 0; i < title_data.format.size();)
		{
			const char c1 = title_data.format[i];

			if (c1 == '\0')
			{
				break;
			}

			switch (c1)
			{
			case '%':
			{
				const char c2 = title_data.format[i + 1];

				if (c2 == '\0')
				{
					title_string += '%';
					i++;
					continue;
				}

				switch (c2)
				{
				case '%':
				{
					title_string += '%';
					break;
				}
				case 'T':
				{
					title_string += title_data.title;
					break;
				}
				case 't':
				{
					title_string += title_data.title_id;
					break;
				}
				case 'R':
				{
					fmt::append(title_string, "%s", title_data.renderer);
					break;
				}
				case 'V':
				{
					title_string += version;
					break;
				}
				case 'F':
				{
					fmt::append(title_string, "%.2f", title_data.fps);
					break;
				}
				default:
				{
					title_string += '%';
					title_string += c2;
					break;
				}
				}

				i += 2;
				break;
			}
			default:
			{
				title_string += c1;
				i += 1;
				break;
			}
			}
		}

		return title_string;
	}
}

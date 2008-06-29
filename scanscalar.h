#pragma once

#include <string>

namespace YAML
{
	struct WhitespaceInfo {
		WhitespaceInfo();

		void SetChompers(char ch);
		void AddBlank(char ch);
		void AddBreak(const std::string& line);
		std::string Join(bool lastline = false);

		bool leadingBlanks;
		bool fold;
		std::string whitespace, leadingBreaks, trailingBreaks;
		int chomp, increment;
	};
}

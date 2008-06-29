#pragma once

#include <string>
#include "regex.h"
#include "stream.h"

namespace YAML
{
	void GetBlockIndentation(Stream& INPUT, int& indent, std::string& breaks, int topIndent);
	std::string ScanScalar(Stream& INPUT, RegEx end, bool eatEnd, int indent, char escape, bool fold, bool eatLeadingWhitespace, bool trimTrailingSpaces, int chomp);

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

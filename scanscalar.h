#pragma once

#include <string>
#include "regex.h"
#include "stream.h"

namespace YAML
{
	enum CHOMP { STRIP = -1, CLIP, KEEP };

	struct ScanScalarInfo {
		ScanScalarInfo(): eatEnd(false), indent(0), eatLeadingWhitespace(0), escape(0), fold(false), trimTrailingSpaces(0), chomp(CLIP) {}

		RegEx end;                      // what condition ends this scalar?
		bool eatEnd;                    // should we eat that condition when we see it?
		int indent;                     // what level of indentation should be eaten and ignored?
		bool eatLeadingWhitespace;      // should we continue eating this delicious indentation after 'indent' spaces?
		char escape;                    // what character do we escape on (i.e., slash or single quote) (0 for none)
		bool fold;                      // do we fold line ends?
		bool trimTrailingSpaces;        // do we remove all trailing spaces (at the very end)
		CHOMP chomp;                    // do we strip, clip, or keep trailing newlines (at the very end)
		                                //   Note: strip means kill all, clip means keep at most one, keep means keep all
	};

	void GetBlockIndentation(Stream& INPUT, int& indent, std::string& breaks, int topIndent);
	std::string ScanScalar(Stream& INPUT, ScanScalarInfo info);

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

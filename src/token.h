#pragma once

#ifndef TOKEN_H_62B23520_7C8E_11DE_8A39_0800200C9A66
#define TOKEN_H_62B23520_7C8E_11DE_8A39_0800200C9A66


#include "mark.h"
#include <ios>
#include <string>
#include <vector>

namespace YAML
{
	enum TOKEN_STATUS { TS_VALID, TS_INVALID, TS_UNVERIFIED };
	enum TOKEN_TYPE {
		TT_DIRECTIVE,
		TT_DOC_START,
		TT_DOC_END,
		TT_BLOCK_SEQ_START,
		TT_BLOCK_MAP_START,
		TT_BLOCK_END,
		TT_BLOCK_ENTRY,
		TT_FLOW_SEQ_START,
		TT_FLOW_MAP_START,
		TT_FLOW_SEQ_END,
		TT_FLOW_MAP_END,
		TT_FLOW_ENTRY,
		TT_KEY,
		TT_VALUE,
		TT_ANCHOR,
		TT_ALIAS,
		TT_TAG,
		TT_SCALAR
	};

	const std::string TokenNames[] = {
		"DIRECTIVE",
		"DOC_START",
		"DOC_END",
		"BLOCK_SEQ_START",
		"BLOCK_MAP_START",
		"BLOCK_END",
		"BLOCK_ENTRY",
		"FLOW_SEQ_START",
		"FLOW_MAP_START",
		"FLOW_SEQ_END",
		"FLOW_MAP_END",
		"FLOW_ENTRY",
		"KEY",
		"VALUE",
		"ANCHOR",
		"ALIAS",
		"TAG",
		"SCALAR"
	};

	struct Token {
		Token(TOKEN_TYPE type_, const Mark& mark_): status(TS_VALID), type(type_), mark(mark_) {}

		friend std::ostream& operator << (std::ostream& out, const Token& token) {
			out << TokenNames[token.type] << std::string(": ") << token.value;
			for(std::size_t i=0;i<token.params.size();i++)
				out << std::string(" ") << token.params[i];
			return out;
		}

		TOKEN_STATUS status;
		TOKEN_TYPE type;
		Mark mark;
		std::string value;
		std::vector <std::string> params;
	};
}

#endif // TOKEN_H_62B23520_7C8E_11DE_8A39_0800200C9A66

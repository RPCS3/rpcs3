#pragma once

#ifndef TAG_H_62B23520_7C8E_11DE_8A39_0800200C9A66
#define TAG_H_62B23520_7C8E_11DE_8A39_0800200C9A66

#include <string>

namespace YAML
{
	struct Token;
	struct ParserState;

	struct Tag {
		enum TYPE {
			VERBATIM, PRIMARY_HANDLE, SECONDARY_HANDLE, NAMED_HANDLE, NON_SPECIFIC
		};
		
		Tag(const Token& token);
		const std::string Translate(const ParserState& state);
		
		TYPE type;
		std::string handle, value;
	};
}

#endif // TAG_H_62B23520_7C8E_11DE_8A39_0800200C9A66

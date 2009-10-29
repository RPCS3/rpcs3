#include "tag.h"
#include "token.h"
#include "parserstate.h"
#include <cassert>

namespace YAML
{
	Tag::Tag(const Token& token): type(static_cast<TYPE>(token.data))
	{
		switch(type) {
			case VERBATIM:
				value = token.value;
				break;
			case PRIMARY_HANDLE:
				value = token.value;
				break;
			case SECONDARY_HANDLE:
				value = token.value;
				break;
			case NAMED_HANDLE:
				handle = token.value;
				value = token.params[0];
				break;
			case NON_SPECIFIC:
				break;
			default:
				assert(false);
		}
	}

	const std::string Tag::Translate(const ParserState& state)
	{
		switch(type) {
			case VERBATIM:
				return value;
			case PRIMARY_HANDLE:
				return state.TranslateTagHandle("!") + value;
			case SECONDARY_HANDLE:
				return state.TranslateTagHandle("!!") + value;
			case NAMED_HANDLE:
				return state.TranslateTagHandle("!" + handle + "!") + value;
			case NON_SPECIFIC:
				// TODO:
				return "!";
			default:
				assert(false);
		}
	}
}


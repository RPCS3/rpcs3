#pragma once

#include <ios>

namespace YAML
{
	struct Token {
		Token(): isValid(true), isPossible(true) {}
		virtual ~Token() {}
		virtual void Write(std::ostream& out) const {}

		friend std::ostream& operator << (std::ostream& out, const Token& token) { token.Write(out); return out; }
		bool isValid, isPossible;
	};

	struct StreamStartToken: public Token {};
	struct StreamEndToken: public Token {};
	struct DocumentStartToken: public Token {};
	struct DocumentEndToken: public Token {};

	struct BlockSeqStartToken: public Token {};
	struct BlockMapStartToken: public Token {};
	struct BlockEndToken: public Token {};
	struct BlockEntryToken: public Token {};

	struct FlowSeqStartToken: public Token {};
	struct FlowMapStartToken: public Token {};
	struct FlowSeqEndToken: public Token {};
	struct FlowMapEndToken: public Token {};
	struct FlowEntryToken: public Token {};

	struct KeyToken: public Token {};
	struct ValueToken: public Token {};

	struct ScalarToken: public Token {
		std::string value;
		virtual void Write(std::ostream& out) const { out << value; }
	};

	struct PlainScalarToken: public ScalarToken {};
	struct QuotedScalarToken: public ScalarToken {};
	struct BlockScalarToken: public ScalarToken {};
}

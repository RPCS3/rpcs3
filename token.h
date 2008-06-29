#pragma once

#include <ios>

namespace YAML
{
	enum TOKEN_STATUS { TS_VALID, TS_INVALID, TS_UNVERIFIED };

	struct Token {
		Token(): status(TS_VALID) {}
		virtual ~Token() {}
		virtual void Write(std::ostream& out) const {}

		friend std::ostream& operator << (std::ostream& out, const Token& token) { token.Write(out); return out; }
		TOKEN_STATUS status;
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
	struct AnchorToken: public Token {
		bool alias;
		std::string value;

		virtual void Write(std::ostream& out) const { out << (alias ? '*' : '&') << value; }
	};

	struct ScalarToken: public Token {
		std::string value;
		virtual void Write(std::ostream& out) const { out << value; }
	};

	struct PlainScalarToken: public ScalarToken {};
	struct QuotedScalarToken: public ScalarToken {
		bool single;
		virtual void Write(std::ostream& out) const { out << (single ? '\'' : '\"') << value << (single ? '\'' : '\"'); }
	};

	struct BlockScalarToken: public ScalarToken {};
}

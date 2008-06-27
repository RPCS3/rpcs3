#pragma once

namespace YAML
{
	class Token { public: virtual ~Token() {} };

	class StreamStartToken: public Token {};
	class StreamEndToken: public Token {};
	class DocumentStartToken: public Token {};
	class DocumentEndToken: public Token {};

	class BlockSeqStartToken: public Token {};
	class BlockMapStartToken: public Token {};
	class BlockEndToken: public Token {};
	class BlockEntryToken: public Token {};

	class FlowSeqStartToken: public Token {};
	class FlowMapStartToken: public Token {};
	class FlowSeqEndToken: public Token {};
	class FlowMapEndToken: public Token {};
	class FlowEntryToken: public Token {};

	class KeyToken: public Token {};
	class ValueToken: public Token {};

	class PlainScalarToken: public Token {
	public:
		void SetValue(const std::string& value) { m_value = value; }
	protected:
		std::string m_value;
	};
}

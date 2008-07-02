#pragma once

#include "content.h"
#include <vector>

namespace YAML
{
	class Node;

	class Sequence: public Content
	{
	public:
		Sequence();
		virtual ~Sequence();

		void Clear();
		virtual bool GetBegin(std::vector <Node *>::const_iterator& it);
		virtual bool GetEnd(std::vector <Node *>::const_iterator& it);

		virtual void Parse(Scanner *pScanner, const ParserState& state);
		virtual void Write(std::ostream& out, int indent);

	private:
		void ParseBlock(Scanner *pScanner, const ParserState& state);
		void ParseImplicit(Scanner *pScanner, const ParserState& state);
		void ParseFlow(Scanner *pScanner, const ParserState& state);

	protected:
		std::vector <Node *> m_data;
	};
}

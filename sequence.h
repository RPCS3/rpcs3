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
		virtual void Parse(Scanner *pScanner);
		virtual void Write(std::ostream& out, int indent);

	private:
		void ParseBlock(Scanner *pScanner);
		void ParseImplicit(Scanner *pScanner);
		void ParseFlow(Scanner *pScanner);

	protected:
		std::vector <Node *> m_data;
	};
}

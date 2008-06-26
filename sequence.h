#pragma once

#include "content.h"
#include <vector>

namespace YAML
{
	class Node;
	class Parser;

	class Sequence: public Content
	{
	public:
		Sequence(Parser *pParser);
		virtual ~Sequence();

		void Read(Parser *pParser);

	protected:
		std::vector <Node *> m_data;
	};
}

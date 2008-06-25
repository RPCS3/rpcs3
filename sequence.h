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

	protected:
		std::vector <Node *> m_data;
	};
}

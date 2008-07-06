#pragma once

namespace YAML
{
	class Node;

	struct ltnode {
		bool operator()(const Node *pNode1, const Node *pNode2) const;
	};
}

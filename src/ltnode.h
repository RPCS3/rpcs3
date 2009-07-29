#pragma once

#ifndef LTNODE_H_62B23520_7C8E_11DE_8A39_0800200C9A66
#define LTNODE_H_62B23520_7C8E_11DE_8A39_0800200C9A66


namespace YAML
{
	class Node;

	struct ltnode {
		bool operator()(const Node *pNode1, const Node *pNode2) const;
	};
}

#endif // LTNODE_H_62B23520_7C8E_11DE_8A39_0800200C9A66

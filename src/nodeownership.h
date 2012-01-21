#ifndef NODE_OWNERSHIP_H_62B23520_7C8E_11DE_8A39_0800200C9A66
#define NODE_OWNERSHIP_H_62B23520_7C8E_11DE_8A39_0800200C9A66

#if defined(_MSC_VER) || (defined(__GNUC__) && (__GNUC__ == 3 && __GNUC_MINOR__ >= 4) || (__GNUC__ >= 4)) // GCC supports "pragma once" correctly since 3.4
#pragma once
#endif


#include "yaml-cpp/noncopyable.h"
#include "ptr_vector.h"
#include <set>

namespace YAML
{
	class Node;
	
	class NodeOwnership: private noncopyable
	{
	public:
		explicit NodeOwnership(NodeOwnership *pOwner = 0);
		~NodeOwnership();
		
		Node& Create() { return m_pOwner->_Create(); }
		void MarkAsAliased(const Node& node) { m_pOwner->_MarkAsAliased(node); }
		bool IsAliased(const Node& node) const { return m_pOwner->_IsAliased(node); }
		
	private:
		Node& _Create();
		void _MarkAsAliased(const Node& node);
		bool _IsAliased(const Node& node) const;
		
	private:
		ptr_vector<Node> m_nodes;
		std::set<const Node *> m_aliasedNodes;
		NodeOwnership *m_pOwner;
	};
}

#endif // NODE_OWNERSHIP_H_62B23520_7C8E_11DE_8A39_0800200C9A66

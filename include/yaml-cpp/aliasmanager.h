#pragma once

#ifndef ALIASMANAGER_H_62B23520_7C8E_11DE_8A39_0800200C9A66
#define ALIASMANAGER_H_62B23520_7C8E_11DE_8A39_0800200C9A66

#include "yaml-cpp/anchor.h"
#include <map>

namespace YAML
{
	class Node;
	
	class AliasManager
	{
	public:
		AliasManager();
		
		void RegisterReference(const Node& node);
		
		const Node *LookupReference(const Node& node) const;
		anchor_t LookupAnchor(const Node& node) const;
		
	private:
		const Node *_LookupReference(const Node& oldIdentity) const;
		anchor_t _CreateNewAnchor();
		
	private:
		typedef std::map<const Node*, const Node*> NodeByNode;
		NodeByNode m_newIdentityByOldIdentity;
		
		typedef std::map<const Node*, anchor_t> AnchorByIdentity;
		AnchorByIdentity m_anchorByIdentity;
		
		anchor_t m_curAnchor;
	};
}

#endif // ALIASMANAGER_H_62B23520_7C8E_11DE_8A39_0800200C9A66

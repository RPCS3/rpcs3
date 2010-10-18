#include "aliasmanager.h"
#include "node.h"
#include <cassert>
#include <sstream>

namespace YAML
{
	AliasManager::AliasManager(): m_curAnchor(0)
	{
	}

	void AliasManager::RegisterReference(const Node& node)
	{
		const Node *pIdentity = node.Identity();
		m_newIdentityByOldIdentity.insert(std::make_pair(pIdentity, &node));
		m_anchorByIdentity.insert(std::make_pair(&node, _CreateNewAnchor()));
	}
	
	const Node *AliasManager::LookupReference(const Node& node) const
	{
		const Node *pIdentity = node.Identity();
		return _LookupReference(*pIdentity);
	}
	
	anchor_t AliasManager::LookupAnchor(const Node& node) const
	{
		AnchorByIdentity::const_iterator it = m_anchorByIdentity.find(&node);
		if(it == m_anchorByIdentity.end())
			assert(false); // TODO: throw
		return it->second;
	}

	const Node *AliasManager::_LookupReference(const Node& oldIdentity) const
	{
		NodeByNode::const_iterator it = m_newIdentityByOldIdentity.find(&oldIdentity);
		if(it == m_newIdentityByOldIdentity.end())
			return 0;
		return it->second;
	}
	
	anchor_t AliasManager::_CreateNewAnchor()
	{
		return ++m_curAnchor;
	}
}

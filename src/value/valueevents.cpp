#include "valueevents.h"
#include "yaml-cpp/value.h"
#include "yaml-cpp/eventhandler.h"
#include "yaml-cpp/mark.h"

namespace YAML
{
	void ValueEvents::AliasManager::RegisterReference(const detail::node& node)
	{
		m_anchorByIdentity.insert(std::make_pair(node.ref(), _CreateNewAnchor()));
	}
	
	anchor_t ValueEvents::AliasManager::LookupAnchor(const detail::node& node) const
	{
		AnchorByIdentity::const_iterator it = m_anchorByIdentity.find(node.ref());
		if(it == m_anchorByIdentity.end())
			return 0;
		return it->second;
	}

	ValueEvents::ValueEvents(const Value& value): m_pMemory(value.m_pMemory), m_root(*value.m_pNode)
	{
		Setup(m_root);
	}

	void ValueEvents::Setup(const detail::node& node)
	{
		int& refCount = m_refCount[node.ref()];
		refCount++;
		if(refCount > 1)
			return;
		
		if(node.type() == ValueType::Sequence) {
			for(detail::const_node_iterator it=node.begin();it!=node.end();++it)
				Setup(**it);
		} else if(node.type() == ValueType::Map) {
			for(detail::const_node_iterator it=node.begin();it!=node.end();++it) {
				Setup(*it->first);
				Setup(*it->second);
			}
		}
	}

	void ValueEvents::Emit(EventHandler& handler)
	{
		AliasManager am;
		
		handler.OnDocumentStart(Mark());
		Emit(m_root, handler, am);
		handler.OnDocumentEnd();
	}

	void ValueEvents::Emit(const detail::node& node, EventHandler& handler, AliasManager& am) const
	{
		anchor_t anchor = NullAnchor;
		if(IsAliased(node)) {
			anchor = am.LookupAnchor(node);
			if(anchor) {
				handler.OnAlias(Mark(), anchor);
				return;
			}
			
			am.RegisterReference(node);
			anchor = am.LookupAnchor(node);
		}
		
		switch(node.type()) {
			case ValueType::Undefined:
				break;
			case ValueType::Null:
				handler.OnNull(Mark(), anchor);
				break;
			case ValueType::Scalar:
				handler.OnScalar(Mark(), "", anchor, node.scalar());
				break;
			case ValueType::Sequence:
				handler.OnSequenceStart(Mark(), "", anchor);
				for(detail::const_node_iterator it=node.begin();it!=node.end();++it)
					Emit(**it, handler, am);
				handler.OnSequenceEnd();
				break;
			case ValueType::Map:
				handler.OnMapStart(Mark(), "", anchor);
				for(detail::const_node_iterator it=node.begin();it!=node.end();++it) {
					Emit(*it->first, handler, am);
					Emit(*it->second, handler, am);
				}
				handler.OnMapEnd();
				break;
		}
	}

	bool ValueEvents::IsAliased(const detail::node& node) const
	{
		RefCount::const_iterator it = m_refCount.find(node.ref());
		return it != m_refCount.end() && it->second > 1;
	}
}

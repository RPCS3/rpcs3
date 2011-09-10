#include "valueevents.h"
#include "yaml-cpp/value.h"

namespace YAML
{
	ValueEvents::ValueEvents(const Value& value): m_pMemory(value.m_pMemory), m_root(*value.m_pNode)
	{
		Visit(m_root);
	}

	void ValueEvents::Visit(const detail::node& node)
	{
		int& refCount = m_refCount[node.ref()];
		refCount++;
		if(refCount > 1)
			return;
		
		if(node.type() == ValueType::Sequence) {
			for(detail::const_node_iterator it=node.begin();it!=node.end();++it)
				Visit(**it);
		} else if(node.type() == ValueType::Map) {
			for(detail::const_node_iterator it=node.begin();it!=node.end();++it) {
				Visit(*it->first);
				Visit(*it->second);
			}
		}
	}
}

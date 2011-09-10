#include "valueevents.h"

namespace YAML
{
	ValueEvents::ValueEvents(const Value& value): m_pMemory(value.m_pMemory), m_root(*value.m_pNode)
	{
		Visit(m_root);
	}

	void Visit(detail::node& node)
	{
		int& refCount = m_refCount[node.ref()];
		refCount++;
		if(refCount > 1)
			return;
		
		if(node.type() == ValueType::Sequence) {
			
		} else if(node.type() == ValueType::Map) {
		}
	}
}

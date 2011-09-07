#include "yaml-cpp/value/detail/memory.h"
#include "yaml-cpp/value/detail/node.h"

namespace YAML
{
	namespace detail
	{
		void memory_holder::merge(memory_holder& rhs)
		{
			if(m_pMemory == rhs.m_pMemory)
				return;
			
			m_pMemory->merge(*rhs.m_pMemory);
			rhs.m_pMemory = m_pMemory;
		}
		
		shared_node memory::create_node()
		{
			shared_node_ref pRef(new node_ref);
			m_nodes.insert(pRef);
			return shared_node(new node(pRef));
		}
		
		void memory::merge(const memory& rhs)
		{
			m_nodes.insert(rhs.m_nodes.begin(), rhs.m_nodes.end());
		}
	}
}

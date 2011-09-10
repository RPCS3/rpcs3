#include "yaml-cpp/node/detail/node_data.h"
#include "yaml-cpp/node/detail/memory.h"
#include "yaml-cpp/node/detail/node.h"
#include <sstream>
#include <stdexcept>

namespace YAML
{
	namespace detail
	{
		std::string node_data::empty_scalar;

		node_data::node_data(): m_isDefined(false), m_type(NodeType::Null)
		{
		}

		void node_data::set_type(NodeType::value type)
		{
			if(type == NodeType::Undefined) {
				m_type = type;
				m_isDefined = false;
				return;
			}
			

			m_isDefined = true;
			if(type == m_type)
				return;
			
			m_type = type;
			
			switch(m_type) {
				case NodeType::Null:
					break;
				case NodeType::Scalar:
					m_scalar.clear();
					break;
				case NodeType::Sequence:
					m_sequence.clear();
					break;
				case NodeType::Map:
					m_map.clear();
					break;
				case NodeType::Undefined:
					assert(false);
					break;
			}
		}

		void node_data::set_null()
		{
			m_isDefined = true;
			m_type = NodeType::Null;
		}
		
		void node_data::set_scalar(const std::string& scalar)
		{
			m_isDefined = true;
			m_type = NodeType::Scalar;
			m_scalar = scalar;
		}

		// size/iterator
		std::size_t node_data::size() const
		{
			return 0;
		}
		
		const_node_iterator node_data::begin() const
		{
			switch(m_type) {
				case NodeType::Sequence: return const_node_iterator(m_sequence.begin());
				case NodeType::Map: return const_node_iterator(m_map.begin());
				default: return const_node_iterator();
			}
		}
		
		node_iterator node_data::begin()
		{
			switch(m_type) {
				case NodeType::Sequence: return node_iterator(m_sequence.begin());
				case NodeType::Map: return node_iterator(m_map.begin());
				default: return node_iterator();
			}
		}
		
		const_node_iterator node_data::end() const
		{
			switch(m_type) {
				case NodeType::Sequence: return const_node_iterator(m_sequence.end());
				case NodeType::Map: return const_node_iterator(m_map.end());
				default: return const_node_iterator();
			}
		}
		
		node_iterator node_data::end()
		{
			switch(m_type) {
				case NodeType::Sequence: return node_iterator(m_sequence.end());
				case NodeType::Map: return node_iterator(m_map.end());
				default: return node_iterator();
			}
		}

		// sequence
		void node_data::append(node& node, shared_memory_holder /* pMemory */)
		{
			if(m_type != NodeType::Sequence)
				throw std::runtime_error("Can't append to a non-sequence node");
			
			m_sequence.push_back(&node);
		}

		void node_data::insert(node& key, node& value, shared_memory_holder /* pMemory */)
		{
			if(m_type != NodeType::Map)
				throw std::runtime_error("Can't insert into a non-map node");

			m_map.push_back(kv_pair(&key, &value));
		}

		// indexing
		node& node_data::get(node& key, shared_memory_holder pMemory) const
		{
			if(m_type != NodeType::Map)
				return pMemory->create_node();
			
			for(node_map::const_iterator it=m_map.begin();it!=m_map.end();++it) {
				if(it->first->is(key))
					return *it->second;
			}
			
			return pMemory->create_node();
		}
		
		node& node_data::get(node& key, shared_memory_holder pMemory)
		{
			switch(m_type) {
				case NodeType::Undefined:
				case NodeType::Null:
				case NodeType::Scalar:
					m_type = NodeType::Map;
					m_map.clear();
					break;
				case NodeType::Sequence:
					convert_sequence_to_map(pMemory);
					break;
				case NodeType::Map:
					break;
			}

			for(node_map::const_iterator it=m_map.begin();it!=m_map.end();++it) {
				if(it->first->is(key))
					return *it->second;
			}
			
			node& value = pMemory->create_node();
			m_map.push_back(kv_pair(&key, &value));
			return value;
		}
		
		bool node_data::remove(node& key, shared_memory_holder /* pMemory */)
		{
			if(m_type != NodeType::Map)
				return false;

			for(node_map::iterator it=m_map.begin();it!=m_map.end();++it) {
				if(it->first->is(key)) {
					m_map.erase(it);
					return true;
				}
			}
			
			return false;
		}

		void node_data::convert_sequence_to_map(shared_memory_holder pMemory)
		{
			assert(m_type == NodeType::Sequence);
			
			m_map.clear();
			for(std::size_t i=0;i<m_sequence.size();i++) {
				std::stringstream stream;
				stream << i;

				node& key = pMemory->create_node();
				key.set_scalar(stream.str());
				m_map.push_back(kv_pair(&key, m_sequence[i]));
			}
			
			m_sequence.clear();
			m_type = NodeType::Map;
		}
	}
}

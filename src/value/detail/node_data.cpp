#include "yaml-cpp/value/detail/node_data.h"
#include "yaml-cpp/value/detail/memory.h"
#include "yaml-cpp/value/detail/node.h"
#include <sstream>
#include <stdexcept>

namespace YAML
{
	namespace detail
	{
		std::string node_data::empty_scalar;

		node_data::node_data(): m_isDefined(false), m_type(ValueType::Null)
		{
		}

		void node_data::set_type(ValueType::value type)
		{
			if(type == ValueType::Undefined) {
				m_type = type;
				m_isDefined = false;
				return;
			}
			

			m_isDefined = true;
			if(type == m_type)
				return;
			
			m_type = type;
			
			switch(m_type) {
				case ValueType::Null:
					break;
				case ValueType::Scalar:
					m_scalar.clear();
					break;
				case ValueType::Sequence:
					m_sequence.clear();
					break;
				case ValueType::Map:
					m_map.clear();
					break;
				case ValueType::Undefined:
					assert(false);
					break;
			}
		}

		void node_data::set_null()
		{
			m_isDefined = true;
			m_type = ValueType::Null;
		}
		
		void node_data::set_scalar(const std::string& scalar)
		{
			m_isDefined = true;
			m_type = ValueType::Scalar;
			m_scalar = scalar;
		}

		void node_data::append(node& node, shared_memory_holder /* pMemory */)
		{
			if(m_type != ValueType::Sequence)
				throw std::runtime_error("Can't append to a non-sequence node");
			
			m_sequence.push_back(&node);
		}

		// indexing
		node& node_data::get(node& key, shared_memory_holder pMemory) const
		{
			if(m_type != ValueType::Map)
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
				case ValueType::Undefined:
				case ValueType::Null:
				case ValueType::Scalar:
					m_type = ValueType::Map;
					m_map.clear();
					break;
				case ValueType::Sequence:
					convert_sequence_to_map(pMemory);
					break;
				case ValueType::Map:
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
			if(m_type != ValueType::Map)
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
			assert(m_type == ValueType::Sequence);
			
			m_map.clear();
			for(std::size_t i=0;i<m_sequence.size();i++) {
				std::stringstream stream;
				stream << i;

				node& key = pMemory->create_node();
				key.set_scalar(stream.str());
				m_map.push_back(kv_pair(&key, m_sequence[i]));
			}
			
			m_sequence.clear();
			m_type = ValueType::Map;
		}
	}
}

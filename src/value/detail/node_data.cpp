#include "yaml-cpp/value/detail/node_data.h"
#include "yaml-cpp/value/detail/node.h"
#include <sstream>

namespace YAML
{
	namespace detail
	{
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

		// indexing
		shared_node node_data::operator[](shared_node pKey) const
		{
			if(m_type != ValueType::Map)
				return shared_node(new node);
			
			for(node_map::const_iterator it=m_map.begin();it!=m_map.end();++it) {
				if(it->first == pKey)
					return it->second;
			}
			
			return shared_node(new node);
		}
		
		shared_node node_data::operator[](shared_node pKey)
		{
			switch(m_type) {
				case ValueType::Undefined:
				case ValueType::Null:
				case ValueType::Scalar:
					m_type = ValueType::Map;
					m_map.clear();
					break;
				case ValueType::Sequence:
					convert_sequence_to_map();
					break;
				case ValueType::Map:
					break;
			}

			for(node_map::const_iterator it=m_map.begin();it!=m_map.end();++it) {
				if(it->first == pKey)
					return it->second;
			}
			
			shared_node pValue(new node);
			m_map.push_back(kv_pair(pKey, pValue));
			return pValue;
		}
		
		bool node_data::remove(shared_node pKey)
		{
			if(m_type != ValueType::Map)
				return false;

			for(node_map::iterator it=m_map.begin();it!=m_map.end();++it) {
				if(it->first == pKey) {
					m_map.erase(it);
					return true;
				}
			}
			
			return false;
		}

		void node_data::convert_sequence_to_map()
		{
			assert(m_type == ValueType::Sequence);
			
			m_map.clear();
			for(std::size_t i=0;i<m_sequence.size();i++) {
				std::stringstream stream;
				stream << i;

				shared_node pKey(new node);
				pKey->set_scalar(stream.str());
				m_map.push_back(kv_pair(pKey, m_sequence[i]));
			}
			
			m_sequence.clear();
			m_type = ValueType::Map;
		}
	}
}

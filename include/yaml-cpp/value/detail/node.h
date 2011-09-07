#ifndef VALUE_DETAIL_NODE_H_62B23520_7C8E_11DE_8A39_0800200C9A66
#define VALUE_DETAIL_NODE_H_62B23520_7C8E_11DE_8A39_0800200C9A66

#if defined(_MSC_VER) || (defined(__GNUC__) && (__GNUC__ == 3 && __GNUC_MINOR__ >= 4) || (__GNUC__ >= 4)) // GCC supports "pragma once" correctly since 3.4
#pragma once
#endif


#include "yaml-cpp/dll.h"
#include "yaml-cpp/value/ptr.h"
#include "yaml-cpp/value/detail/node_data.h"

namespace YAML
{
	namespace detail
	{
		class node
		{
		public:
			node();
			
			void assign_data(const node& rhs);
			void set_scalar(const std::string& data);
			
		private:
			shared_node_data m_pData;
		};
		
		inline node::node()
		{
		}
		
		inline void node::assign_data(const node& rhs)
		{
			m_pData = rhs.m_pData;
		}

		inline void node::set_scalar(const std::string& data)
		{
			m_pData.reset(new node_data(data));
		}
	}
}

#endif // VALUE_DETAIL_NODE_H_62B23520_7C8E_11DE_8A39_0800200C9A66

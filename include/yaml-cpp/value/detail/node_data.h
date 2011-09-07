#ifndef VALUE_DETAIL_NODE_DATA_H_62B23520_7C8E_11DE_8A39_0800200C9A66
#define VALUE_DETAIL_NODE_DATA_H_62B23520_7C8E_11DE_8A39_0800200C9A66

#if defined(_MSC_VER) || (defined(__GNUC__) && (__GNUC__ == 3 && __GNUC_MINOR__ >= 4) || (__GNUC__ >= 4)) // GCC supports "pragma once" correctly since 3.4
#pragma once
#endif


#include "yaml-cpp/dll.h"
#include "yaml-cpp/value/ptr.h"

namespace YAML
{
	namespace detail
	{
		class node_data
		{
		public:
			explicit node_data(const std::string& data) {}
		};
	}
}

#endif // VALUE_DETAIL_NODE_DATA_H_62B23520_7C8E_11DE_8A39_0800200C9A66

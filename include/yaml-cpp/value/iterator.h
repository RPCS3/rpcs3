#ifndef VALUE_ITERATOR_H_62B23520_7C8E_11DE_8A39_0800200C9A66
#define VALUE_ITERATOR_H_62B23520_7C8E_11DE_8A39_0800200C9A66

#if defined(_MSC_VER) || (defined(__GNUC__) && (__GNUC__ == 3 && __GNUC_MINOR__ >= 4) || (__GNUC__ >= 4)) // GCC supports "pragma once" correctly since 3.4
#pragma once
#endif


#include "yaml-cpp/dll.h"
#include "yaml-cpp/value/detail/iterator.h"

namespace YAML
{
	class iterator: public detail::iterator_base<detail::iterator_value, detail::node_seq_iterator, detail::node_map_iterator> {};
	class const_iterator: public detail::iterator_base<const detail::iterator_value, detail::node_seq_const_iterator, detail::node_map_const_iterator> {};
}

#endif // VALUE_ITERATOR_H_62B23520_7C8E_11DE_8A39_0800200C9A66

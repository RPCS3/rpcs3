#ifndef VALUE_DETAIL_ITERATOR_FWD_H_62B23520_7C8E_11DE_8A39_0800200C9A66
#define VALUE_DETAIL_ITERATOR_FWD_H_62B23520_7C8E_11DE_8A39_0800200C9A66

#if defined(_MSC_VER) || (defined(__GNUC__) && (__GNUC__ == 3 && __GNUC_MINOR__ >= 4) || (__GNUC__ >= 4)) // GCC supports "pragma once" correctly since 3.4
#pragma once
#endif


#include "yaml-cpp/dll.h"
#include <list>
#include <utility>
#include <vector>

namespace YAML
{
	class node;

	namespace detail {
		struct iterator_value;
		typedef std::vector<node *> node_seq;
		typedef std::pair<node *, node *> kv_pair;
		typedef std::list<kv_pair> node_map;
		
		typedef node_seq::iterator node_seq_iterator;
		typedef node_seq::const_iterator node_seq_const_iterator;
		
		typedef node_map::iterator node_map_iterator;
		typedef node_map::const_iterator node_map_const_iterator;
		
		template<typename V, typename SeqIter, typename MapIter> class iterator_base;
	}
	
	typedef detail::iterator_base<detail::iterator_value, detail::node_seq_iterator, detail::node_map_iterator> iterator;
	typedef detail::iterator_base<const detail::iterator_value, detail::node_seq_const_iterator, detail::node_map_const_iterator> const_iterator;
}

#endif // VALUE_DETAIL_ITERATOR_FWD_H_62B23520_7C8E_11DE_8A39_0800200C9A66

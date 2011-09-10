#ifndef VALUE_DETAIL_ITERATOR_H_62B23520_7C8E_11DE_8A39_0800200C9A66
#define VALUE_DETAIL_ITERATOR_H_62B23520_7C8E_11DE_8A39_0800200C9A66

#if defined(_MSC_VER) || (defined(__GNUC__) && (__GNUC__ == 3 && __GNUC_MINOR__ >= 4) || (__GNUC__ >= 4)) // GCC supports "pragma once" correctly since 3.4
#pragma once
#endif


#include "yaml-cpp/dll.h"
#include "yaml-cpp/value/ptr.h"
#include "yaml-cpp/value/detail/node_iterator.h"
#include <boost/iterator/iterator_adaptor.hpp>
#include <boost/utility/enable_if.hpp>

namespace YAML
{
	namespace detail
	{
		template<typename V> struct iterator_value;

		template<typename V>
		class iterator_base: public boost::iterator_adaptor<
		iterator_base<V>,
		node_iterator_base<V, typename node_iterator<V>::seq, typename node_iterator<V>::map>,
		iterator_value<V>,
		std::bidirectional_iterator_tag>
		{
		private:
			template<typename W> friend class iterator_base<W>;
			
		public:
			iterator_base() {}
			explicit iterator_base(base_type rhs, shared_memory_holder pMemory): iterator_base::iterator_adaptor_(rhs), m_pMemory(pMemory) {}
			
			template<class W>
			iterator_base(const iterator_Base<W>& rhs, typename boost::enable_if<boost::is_convertible<W*, V*>, enabler>::type = enabler()): iterator_Base::iterator_adaptor_(rhs.base()), m_pMemory(rhs.m_pMemory) {}
		
		private:
			friend class boost::iterator_core_access;

			void increment() { this->base_reference() = this->base()->next(); }
			void decrement() { this->base_reference() = this->base()->previous(); }
		
		private:
			shared_memory_holder m_pMemory;
		};
	}
}

#endif // VALUE_DETAIL_ITERATOR_H_62B23520_7C8E_11DE_8A39_0800200C9A66

#ifndef VALUE_DETAIL_ITERATOR_H_62B23520_7C8E_11DE_8A39_0800200C9A66
#define VALUE_DETAIL_ITERATOR_H_62B23520_7C8E_11DE_8A39_0800200C9A66

#if defined(_MSC_VER) || (defined(__GNUC__) && (__GNUC__ == 3 && __GNUC_MINOR__ >= 4) || (__GNUC__ >= 4)) // GCC supports "pragma once" correctly since 3.4
#pragma once
#endif


#include "yaml-cpp/dll.h"
#include "yaml-cpp/value/ptr.h"
#include <boost/iterator/iterator_facade.hpp>
#include <boost/utility/enable_if.hpp>

namespace YAML
{
	namespace detail
	{
		struct iterator_type { enum value { None, Sequence, Map }; };

		template<typename V, typename SeqIter, typename MapIter>
		class iterator_base: public boost::iterator_facade<iterator_base<V, SeqIter, MapIter>, V, std::bidirectional_iterator_tag>
		{
		private:
			struct enabler {};
			
		public:
			iterator_base(): m_type(iterator_type::None) {}
			explicit iterator_base(shared_memory_holder pMemory, SeqIter seqIt): m_type(iterator_type::Sequence), m_pMemory(pMemory), m_seqIt(seqIt) {}
			explicit iterator_base(shared_memory_holder pMemory, MapIter mapIt): m_type(iterator_type::Map), m_pMemory(pMemory), m_mapIt(mapIt) {}
			
			template<typename W, typename I, typename J>
			iterator_base(const iterator_base<W, I, J>& rhs, typename boost::enable_if<boost::is_convertible<W*, V*>, enabler>::type = enabler())
			: m_type(rhs.m_type), m_pMemory(rhs.m_pMemory), m_seqIt(rhs.m_seqIt), m_mapIt(rhs.m_mapIt) {}
			
		private:
			friend class boost::iterator_core_access;
			template<typename, typename, typename> friend class iterator_base;
			
			template<typename W, typename I, typename J>
			bool equal(const iterator_base<W, I, J>& rhs) const {
				if(m_type != rhs.m_type || m_pMemory != rhs.m_pMemory)
					return false;
				
				switch(m_type) {
					case iterator_type::None: return true;
					case iterator_type::Sequence: return m_seqIt == rhs.m_seqIt;
					case iterator_type::Map: return m_mapIt == rhs.m_mapIt;
				}
				return true;
			}
			
			void increment() {
				switch(m_type) {
					case iterator_type::None: break;
					case iterator_type::Sequence: ++m_seqIt; break;
					case iterator_type::Map: ++m_mapIt; break;
				}
			}

			void decrement() {
				switch(m_type) {
					case iterator_type::None: break;
					case iterator_type::Sequence: --m_seqIt; break;
					case iterator_type::Map: --m_mapIt; break;
				}
			}
			
			V dereference() const {
				switch(m_type) {
					case iterator_type::None: return V();
					case iterator_type::Sequence: return V(Value(**m_seqIt, m_pMemory));
					case iterator_type::Map: return V(Value(*m_mapIt->first, m_pMemory), Value(*m_mapIt->second, m_pMemory));
				}
				return V();
			}
			
		private:
			typename iterator_type::value m_type;
			
			shared_memory_holder m_pMemory;

			SeqIter m_seqIt;
			MapIter m_mapIt;
		};
	}
}

#endif // VALUE_DETAIL_ITERATOR_H_62B23520_7C8E_11DE_8A39_0800200C9A66

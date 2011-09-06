#ifndef NODEUTIL_H_62B23520_7C8E_11DE_8A39_0800200C9A66
#define NODEUTIL_H_62B23520_7C8E_11DE_8A39_0800200C9A66

#if defined(_MSC_VER) || (defined(__GNUC__) && (__GNUC__ == 3 && __GNUC_MINOR__ >= 4) || (__GNUC__ >= 4)) // GCC supports "pragma once" correctly since 3.4
#pragma once
#endif


namespace YAML
{
	template <typename T, typename U>
	struct is_same_type {
		enum { value = false };
	};
	
	template <typename T>
	struct is_same_type<T, T> {
		enum { value = true };
	};
	
	template <typename T, bool check>
	struct is_index_type_with_check {
		enum { value = false };
	};
	
	template <> struct is_index_type_with_check<std::size_t, false> { enum { value = true }; };

#define MAKE_INDEX_TYPE(Type) \
	template <> struct is_index_type_with_check<Type, is_same_type<Type, std::size_t>::value> { enum { value = true }; }
	
	MAKE_INDEX_TYPE(int);
	MAKE_INDEX_TYPE(unsigned);
	MAKE_INDEX_TYPE(short);
	MAKE_INDEX_TYPE(unsigned short);
	MAKE_INDEX_TYPE(long);
	MAKE_INDEX_TYPE(unsigned long);

#undef MAKE_INDEX_TYPE
	
	template <typename T>
	struct is_index_type: public is_index_type_with_check<T, false> {};
	
	// messing around with template stuff to get the right overload for operator [] for a sequence
	template <typename T, bool b>
	struct _FindFromNodeAtIndex {
		const Node *pRet;
		_FindFromNodeAtIndex(const Node&, const T&): pRet(0) {}
	};

	template <typename T>
	struct _FindFromNodeAtIndex<T, true> {
		const Node *pRet;
		_FindFromNodeAtIndex(const Node& node, const T& key): pRet(node.FindAtIndex(static_cast<std::size_t>(key))) {}
	};

	template <typename T>
	inline const Node *FindFromNodeAtIndex(const Node& node, const T& key) {
		return _FindFromNodeAtIndex<T, is_index_type<T>::value>(node, key).pRet;
	}
}

#endif // NODEUTIL_H_62B23520_7C8E_11DE_8A39_0800200C9A66

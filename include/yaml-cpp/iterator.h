#ifndef ITERATOR_H_62B23520_7C8E_11DE_8A39_0800200C9A66
#define ITERATOR_H_62B23520_7C8E_11DE_8A39_0800200C9A66

#if defined(_MSC_VER) || (defined(__GNUC__) && (__GNUC__ == 3 && __GNUC_MINOR__ >= 4) || (__GNUC__ >= 4)) // GCC supports "pragma once" correctly since 3.4
#pragma once
#endif

#include "yaml-cpp/dll.h"
#include <memory>

namespace YAML
{
	class Node;
	struct IterPriv;

	class YAML_CPP_API Iterator
	{
	public:
		Iterator();
		Iterator(std::auto_ptr<IterPriv> pData);
		Iterator(const Iterator& rhs);
		~Iterator();

		Iterator& operator = (const Iterator& rhs);
		Iterator& operator ++ ();
		Iterator operator ++ (int);
		const Node& operator * () const;
		const Node *operator -> () const;
		const Node& first() const;
		const Node& second() const;

		friend YAML_CPP_API bool operator == (const Iterator& it, const Iterator& jt);
		friend YAML_CPP_API bool operator != (const Iterator& it, const Iterator& jt);

	private:
		std::auto_ptr<IterPriv> m_pData;
	};
}

#endif // ITERATOR_H_62B23520_7C8E_11DE_8A39_0800200C9A66

#pragma once

#ifndef ITERATOR_H_62B23520_7C8E_11DE_8A39_0800200C9A66
#define ITERATOR_H_62B23520_7C8E_11DE_8A39_0800200C9A66


namespace YAML
{
	class Node;
	struct IterPriv;

	class Iterator
	{
	public:
		Iterator();
		Iterator(IterPriv *pData);
		Iterator(const Iterator& rhs);
		~Iterator();

		Iterator& operator = (const Iterator& rhs);
		Iterator& operator ++ ();
		Iterator operator ++ (int);
		const Node& operator * () const;
		const Node *operator -> () const;
		const Node& first() const;
		const Node& second() const;

		friend bool operator == (const Iterator& it, const Iterator& jt);
		friend bool operator != (const Iterator& it, const Iterator& jt);

	private:
		IterPriv *m_pData;
	};
}

#endif // ITERATOR_H_62B23520_7C8E_11DE_8A39_0800200C9A66

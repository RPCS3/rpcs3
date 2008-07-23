#pragma once

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

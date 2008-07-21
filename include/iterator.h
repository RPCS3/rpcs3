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

		friend bool operator == (const Iterator& it, const Iterator& jt);
		friend bool operator != (const Iterator& it, const Iterator& jt);
		Iterator& operator = (const Iterator& rhs);
		Iterator& operator ++ ();
		Iterator operator ++ (int);
		const Node& operator * ();
		const Node *operator -> ();
		const Node& first();
		const Node& second();

	private:
		IterPriv *m_pData;
	};
}

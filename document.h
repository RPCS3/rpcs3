#pragma once

namespace YAML
{
	class Node;

	class Document
	{
	public:
		Document();
		~Document();

		void Clear();

	private:
		Node *m_pRoot;
	};
}

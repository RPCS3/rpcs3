#pragma once

#include <string>

namespace YAML
{
	class Node;

	class Document
	{
	public:
		Document();
		Document(const std::string& fileName);
		~Document();

		void Clear();
		void Load(const std::string& fileName);

	private:
		Node *m_pRoot;
	};
}

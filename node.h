#pragma once

#include <string>
#include <ios>

namespace YAML
{
	const std::string StrTag = "!!str";
	const std::string SeqTag = "!!seq";
	const std::string MapTag = "!!map";

	class Content;

	class Node
	{
	public:
		Node();
		~Node();

		void Clear();

	private:
		std::string m_tag;
		Content *m_pContent;
	};
}

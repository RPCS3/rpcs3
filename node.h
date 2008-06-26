#pragma once

#include <string>
#include <ios>

namespace YAML
{
	const std::string StrTag = "!!str";
	const std::string SeqTag = "!!seq";
	const std::string MapTag = "!!map";

	class Content;
	class Parser;

	class Node
	{
	public:
		Node();
		~Node();

		void Clear();
		void Read(Parser *pParser, const std::string& token);

	private:
		std::string m_tag;
		Content *m_pContent;
	};
}

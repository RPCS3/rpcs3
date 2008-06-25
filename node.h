#pragma once

#include <string>

namespace YAML
{
	const std::string Str = "!!str";
	const std::string Seq = "!!seq";
	const std::string Map = "!!map";

	class Node
	{
	public:
		Node();
		~Node();

	private:
		std::string m_tag;
	};
}

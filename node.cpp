#include "node.h"
#include "content.h"

namespace YAML
{
	Node::Node(): m_pContent(0)
	{
	}

	Node::~Node()
	{
		Clear();
	}

	void Node::Clear()
	{
		delete m_pContent;
		m_pContent = 0;
	}

	void Node::Read(std::istream& in)
	{
	}
}

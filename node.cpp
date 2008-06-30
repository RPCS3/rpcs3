#include "node.h"
#include "content.h"
#include "parser.h"
#include "scalar.h"
#include "sequence.h"

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
}

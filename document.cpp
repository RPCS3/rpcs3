#include "document.h"
#include "node.h"

namespace YAML
{
	Document::Document(): m_pRoot(0)
	{
	}

	Document::~Document()
	{
		Clear();
	}

	void Document::Clear()
	{
		delete m_pRoot;
		m_pRoot = 0;
	}
}

#include "document.h"
#include "node.h"
#include <fstream>

namespace YAML
{
	Document::Document(): m_pRoot(0)
	{
	}

	Document::Document(const std::string& fileName): m_pRoot(0)
	{
		Load(fileName);
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

	void Document::Load(const std::string& fileName)
	{
		Clear();

		std::ifstream fin(fileName.c_str());
		m_pRoot = new Node;
		m_pRoot->Read(fin);
	}
}

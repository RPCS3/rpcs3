#include "reader.h"
#include "scanner.h"
#include "parser.h"

namespace YAML
{
	Reader::Reader(std::istream& in): m_pParser(0)
	{
		m_pParser = new Parser(in);
	}

	Reader::~Reader()
	{
		delete m_pParser;
	}

	void Reader::GetNextDocument(Document& document)
	{
		m_pParser->GetNextDocument(document);
	}
}

#include "document.h"
#include "node.h"
#include "token.h"
#include "scanner.h"

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

	void Document::Parse(Scanner *pScanner, const ParserState& state)
	{
		Clear();

		// we better have some tokens in the queue
		if(!pScanner->PeekNextToken())
			return;

		// first eat doc start (optional)
		if(pScanner->PeekNextToken()->type == TT_DOC_START)
			pScanner->EatNextToken();

		// now create our root node and parse it
		m_pRoot = new Node;
		m_pRoot->Parse(pScanner, state);

		// and finally eat any doc ends we see
		while(pScanner->PeekNextToken() && pScanner->PeekNextToken()->type == TT_DOC_END)
			pScanner->EatNextToken();
	}

	const Node& Document::GetRoot() const
	{
		if(!m_pRoot)
			throw;

		return *m_pRoot;
	}

	std::ostream& operator << (std::ostream& out, const Document& doc)
	{
		out << "---\n";
		if(!doc.m_pRoot) {
			out << "{empty node}\n";
			return out;
		}

		doc.m_pRoot->Write(out, 0);
		return out;
	}
}

#include "node.h"
#include "token.h"
#include "scanner.h"
#include "content.h"
#include "parser.h"
#include "scalar.h"
#include "sequence.h"
#include "map.h"

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

	void Node::Parse(Scanner *pScanner)
	{
		ParseHeader(pScanner);

		// now split based on what kind of node we should be
		Token *pToken = pScanner->PeekNextToken();
		if(pToken->type == TT_DOC_END)
			return;

		switch(pToken->type) {
			case TT_SCALAR:
				m_pContent = new Scalar;
				m_pContent->Parse(pScanner);
				break;
			case TT_FLOW_SEQ_START:
			case TT_BLOCK_SEQ_START:
			case TT_BLOCK_ENTRY:
				m_pContent = new Sequence;
				m_pContent->Parse(pScanner);
				break;
			case TT_FLOW_MAP_START:
			case TT_BLOCK_MAP_START:
				m_pContent = new Map;
				m_pContent->Parse(pScanner);
		}
	}

	// ParseHeader
	// . Grabs any tag, alias, or anchor tokens and deals with them.
	void Node::ParseHeader(Scanner *pScanner)
	{
		while(1) {
			Token *pToken = pScanner->PeekNextToken();
			if(!pToken || pToken->type != TT_TAG || pToken->type != TT_ANCHOR || pToken->type != TT_ALIAS)
				break;

			pScanner->PopNextToken();
			switch(pToken->type) {
				case TT_TAG:
					break;
				case TT_ANCHOR:
					break;
				case TT_ALIAS:
					break;
			}
			delete pToken;
		}
	}

	void Node::Write(std::ostream& out, int indent)
	{
		if(!m_pContent) {
			for(int i=0;i<indent;i++)
				out << "  ";
			out << "{no content}\n";
		} else
			m_pContent->Write(out, indent);
	}
}

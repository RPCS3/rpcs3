#include "sequence.h"
#include "node.h"
#include "scanner.h"
#include "token.h"

namespace YAML
{
	Sequence::Sequence()
	{

	}

	Sequence::~Sequence()
	{
		Clear();
	}

	void Sequence::Clear()
	{
		for(unsigned i=0;i<m_data.size();i++)
			delete m_data[i];
		m_data.clear();
	}

	void Sequence::Parse(Scanner *pScanner, const ParserState& state)
	{
		Clear();

		// split based on start token
		Token *pToken = pScanner->PeekNextToken();

		switch(pToken->type) {
			case TT_BLOCK_SEQ_START: ParseBlock(pScanner, state); break;
			case TT_BLOCK_ENTRY: ParseImplicit(pScanner, state); break;
			case TT_FLOW_SEQ_START: ParseFlow(pScanner, state); break;
		}
	}

	void Sequence::ParseBlock(Scanner *pScanner, const ParserState& state)
	{
		// eat start token
		pScanner->EatNextToken();

		while(1) {
			Token *pToken = pScanner->PeekNextToken();
			if(!pToken)
				break;  // TODO: throw?

			if(pToken->type != TT_BLOCK_ENTRY && pToken->type != TT_BLOCK_END)
				break;  // TODO: throw?

			pScanner->PopNextToken();
			if(pToken->type == TT_BLOCK_END)
				break;

			Node *pNode = new Node;
			m_data.push_back(pNode);
			pNode->Parse(pScanner, state);
		}
	}

	void Sequence::ParseImplicit(Scanner *pScanner, const ParserState& state)
	{
		while(1) {
			Token *pToken = pScanner->PeekNextToken();
			// we're actually *allowed* to have no tokens at some point
			if(!pToken)
				break;

			// and we end at anything other than a block entry
			if(pToken->type != TT_BLOCK_ENTRY)
				break;

			pScanner->PopNextToken();

			Node *pNode = new Node;
			m_data.push_back(pNode);
			pNode->Parse(pScanner, state);
		}
	}

	void Sequence::ParseFlow(Scanner *pScanner, const ParserState& state)
	{
		// eat start token
		pScanner->EatNextToken();

		while(1) {
			Token *pToken = pScanner->PeekNextToken();
			if(!pToken)
				break;  // TODO: throw?

			// first check for end
			if(pToken->type == TT_FLOW_SEQ_END) {
				pScanner->PopNextToken();
				break;
			}

			// then read the node
			Node *pNode = new Node;
			m_data.push_back(pNode);
			pNode->Parse(pScanner, state);

			// now eat the separator (or could be a sequence end, which we ignore - but if it's neither, then it's a bad node)
			pToken = pScanner->PeekNextToken();
			if(pToken->type == TT_FLOW_ENTRY)
				pScanner->EatNextToken();
			else if(pToken->type != TT_FLOW_SEQ_END)
				break;  // TODO: throw?
		}
	}

	void Sequence::Write(std::ostream& out, int indent)
	{
		for(int i=0;i<indent;i++)
			out << "  ";
		out << "{sequence}\n";
		for(unsigned i=0;i<m_data.size();i++)
			m_data[i]->Write(out, indent + 1);
	}
}

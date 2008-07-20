#include "crt.h"
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

	bool Sequence::GetBegin(std::vector <Node *>::const_iterator& it) const
	{
		it = m_data.begin();
		return true;
	}

	bool Sequence::GetEnd(std::vector <Node *>::const_iterator& it) const
	{
		it = m_data.end();
		return true;
	}

	Node *Sequence::GetNode(unsigned i) const
	{
		if(i < m_data.size())
			return m_data[i];
		return 0;
	}

	unsigned Sequence::GetSize() const
	{
		return m_data.size();
	}

	void Sequence::Parse(Scanner *pScanner, const ParserState& state)
	{
		Clear();

		// split based on start token
		Token& token = pScanner->PeekToken();

		switch(token.type) {
			case TT_BLOCK_SEQ_START: ParseBlock(pScanner, state); break;
			case TT_BLOCK_ENTRY: ParseImplicit(pScanner, state); break;
			case TT_FLOW_SEQ_START: ParseFlow(pScanner, state); break;
		}
	}

	void Sequence::ParseBlock(Scanner *pScanner, const ParserState& state)
	{
		// eat start token
		pScanner->PopToken();

		while(1) {
			if(pScanner->IsEmpty())
				throw ParserException(-1, -1, ErrorMsg::END_OF_SEQ);

			Token token = pScanner->PeekToken();
			if(token.type != TT_BLOCK_ENTRY && token.type != TT_BLOCK_END)
				throw ParserException(token.line, token.column, ErrorMsg::END_OF_SEQ);

			pScanner->PopToken();
			if(token.type == TT_BLOCK_END)
				break;

			Node *pNode = new Node;
			m_data.push_back(pNode);
			pNode->Parse(pScanner, state);
		}
	}

	void Sequence::ParseImplicit(Scanner *pScanner, const ParserState& state)
	{
		while(1) {
			// we're actually *allowed* to have no tokens at some point
			if(pScanner->IsEmpty())
				break;

			// and we end at anything other than a block entry
			Token& token = pScanner->PeekToken();
			if(token.type != TT_BLOCK_ENTRY)
				break;

			pScanner->PopToken();

			Node *pNode = new Node;
			m_data.push_back(pNode);
			pNode->Parse(pScanner, state);
		}
	}

	void Sequence::ParseFlow(Scanner *pScanner, const ParserState& state)
	{
		// eat start token
		pScanner->PopToken();

		while(1) {
			if(pScanner->IsEmpty())
				throw ParserException(-1, -1, ErrorMsg::END_OF_SEQ_FLOW);

			// first check for end
			if(pScanner->PeekToken().type == TT_FLOW_SEQ_END) {
				pScanner->PopToken();
				break;
			}

			// then read the node
			Node *pNode = new Node;
			m_data.push_back(pNode);
			pNode->Parse(pScanner, state);

			// now eat the separator (or could be a sequence end, which we ignore - but if it's neither, then it's a bad node)
			Token& token = pScanner->PeekToken();
			if(token.type == TT_FLOW_ENTRY)
				pScanner->PopToken();
			else if(token.type != TT_FLOW_SEQ_END)
				throw ParserException(token.line, token.column, ErrorMsg::END_OF_SEQ_FLOW);
		}
	}

	void Sequence::Write(std::ostream& out, int indent, bool startedLine, bool onlyOneCharOnLine)
	{
		if(startedLine && !onlyOneCharOnLine)
			out << std::endl;

		for(unsigned i=0;i<m_data.size();i++) {
			if((startedLine && !onlyOneCharOnLine) || i > 0) {
				for(int j=0;j<indent;j++)
					out  << "  ";
			}

			out << "- ";
			m_data[i]->Write(out, indent + 1, true, i > 0 || !startedLine || onlyOneCharOnLine);
		}

		if(m_data.empty())
			out << std::endl;
	}

	int Sequence::Compare(Content *pContent)
	{
		return -pContent->Compare(this);
	}

	int Sequence::Compare(Sequence *pSeq)
	{
		unsigned n = m_data.size(), m = pSeq->m_data.size();
		if(n < m)
			return -1;
		else if(n > m)
			return 1;

		for(unsigned i=0;i<n;i++) {
			int cmp = m_data[i]->Compare(*pSeq->m_data[i]);
			if(cmp != 0)
				return cmp;
		}

		return 0;
	}
}

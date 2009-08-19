#include "crt.h"
#include "sequence.h"
#include "node.h"
#include "scanner.h"
#include "token.h"
#include "emitter.h"
#include <stdexcept>

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
		for(std::size_t i=0;i<m_data.size();i++)
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

	Node *Sequence::GetNode(std::size_t i) const
	{
		if(i < m_data.size())
			return m_data[i];
		return 0;
	}

	std::size_t Sequence::GetSize() const
	{
		return m_data.size();
	}

	void Sequence::Parse(Scanner *pScanner, const ParserState& state)
	{
		Clear();

		// split based on start token
		switch(pScanner->peek().type) {
			case TT_BLOCK_SEQ_START: ParseBlock(pScanner, state); break;
			case TT_BLOCK_ENTRY: ParseImplicit(pScanner, state); break;
			case TT_FLOW_SEQ_START: ParseFlow(pScanner, state); break;
			default: break;
		}
	}

	void Sequence::ParseBlock(Scanner *pScanner, const ParserState& state)
	{
		// eat start token
		pScanner->pop();

		while(1) {
			if(pScanner->empty())
				throw ParserException(Mark::null(), ErrorMsg::END_OF_SEQ);

			Token token = pScanner->peek();
			if(token.type != TT_BLOCK_ENTRY && token.type != TT_BLOCK_END)
				throw ParserException(token.mark, ErrorMsg::END_OF_SEQ);

			pScanner->pop();
			if(token.type == TT_BLOCK_END)
				break;

			Node *pNode = new Node;
			m_data.push_back(pNode);
			
			// check for null
			if(!pScanner->empty()) {
				const Token& token = pScanner->peek();
				if(token.type == TT_BLOCK_ENTRY || token.type == TT_BLOCK_END)
					continue;
			}
			
			pNode->Parse(pScanner, state);
		}
	}

	void Sequence::ParseImplicit(Scanner *pScanner, const ParserState& state)
	{
		while(1) {
			// we're actually *allowed* to have no tokens at some point
			if(pScanner->empty())
				break;

			// and we end at anything other than a block entry
			Token& token = pScanner->peek();
			if(token.type != TT_BLOCK_ENTRY)
				break;

			pScanner->pop();

			Node *pNode = new Node;
			m_data.push_back(pNode);
			pNode->Parse(pScanner, state);
		}
	}

	void Sequence::ParseFlow(Scanner *pScanner, const ParserState& state)
	{
		// eat start token
		pScanner->pop();

		while(1) {
			if(pScanner->empty())
				throw ParserException(Mark::null(), ErrorMsg::END_OF_SEQ_FLOW);

			// first check for end
			if(pScanner->peek().type == TT_FLOW_SEQ_END) {
				pScanner->pop();
				break;
			}

			// then read the node
			Node *pNode = new Node;
			m_data.push_back(pNode);
			pNode->Parse(pScanner, state);

			// now eat the separator (or could be a sequence end, which we ignore - but if it's neither, then it's a bad node)
			Token& token = pScanner->peek();
			if(token.type == TT_FLOW_ENTRY)
				pScanner->pop();
			else if(token.type != TT_FLOW_SEQ_END)
				throw ParserException(token.mark, ErrorMsg::END_OF_SEQ_FLOW);
		}
	}

	void Sequence::Write(Emitter& out) const
	{
		out << BeginSeq;
		for(std::size_t i=0;i<m_data.size();i++)
			out << *m_data[i];
		out << EndSeq;
	}

	int Sequence::Compare(Content *pContent)
	{
		return -pContent->Compare(this);
	}

	int Sequence::Compare(Sequence *pSeq)
	{
		std::size_t n = m_data.size(), m = pSeq->m_data.size();
		if(n < m)
			return -1;
		else if(n > m)
			return 1;

		for(std::size_t i=0;i<n;i++) {
			int cmp = m_data[i]->Compare(*pSeq->m_data[i]);
			if(cmp != 0)
				return cmp;
		}

		return 0;
	}
}

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

	Sequence::Sequence(const std::vector<Node *>& data)
	{
		for(std::size_t i=0;i<data.size();i++)
			m_data.push_back(data[i]->Clone().release());
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

	Content *Sequence::Clone() const
	{
		return new Sequence(m_data);
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

	void Sequence::Parse(Scanner *pScanner, ParserState& state)
	{
		Clear();

		// split based on start token
		switch(pScanner->peek().type) {
			case Token::BLOCK_SEQ_START: ParseBlock(pScanner, state); break;
			case Token::FLOW_SEQ_START: ParseFlow(pScanner, state); break;
			default: break;
		}
	}

	void Sequence::ParseBlock(Scanner *pScanner, ParserState& state)
	{
		// eat start token
		pScanner->pop();
		state.PushCollectionType(ParserState::BLOCK_SEQ);

		while(1) {
			if(pScanner->empty())
				throw ParserException(Mark::null(), ErrorMsg::END_OF_SEQ);

			Token token = pScanner->peek();
			if(token.type != Token::BLOCK_ENTRY && token.type != Token::BLOCK_SEQ_END)
				throw ParserException(token.mark, ErrorMsg::END_OF_SEQ);

			pScanner->pop();
			if(token.type == Token::BLOCK_SEQ_END)
				break;

			Node *pNode = new Node;
			m_data.push_back(pNode);
			
			// check for null
			if(!pScanner->empty()) {
				const Token& token = pScanner->peek();
				if(token.type == Token::BLOCK_ENTRY || token.type == Token::BLOCK_SEQ_END)
					continue;
			}
			
			pNode->Parse(pScanner, state);
		}

		state.PopCollectionType(ParserState::BLOCK_SEQ);
	}

	void Sequence::ParseFlow(Scanner *pScanner, ParserState& state)
	{
		// eat start token
		pScanner->pop();
		state.PushCollectionType(ParserState::FLOW_SEQ);

		while(1) {
			if(pScanner->empty())
				throw ParserException(Mark::null(), ErrorMsg::END_OF_SEQ_FLOW);

			// first check for end
			if(pScanner->peek().type == Token::FLOW_SEQ_END) {
				pScanner->pop();
				break;
			}

			// then read the node
			Node *pNode = new Node;
			m_data.push_back(pNode);
			pNode->Parse(pScanner, state);

			// now eat the separator (or could be a sequence end, which we ignore - but if it's neither, then it's a bad node)
			Token& token = pScanner->peek();
			if(token.type == Token::FLOW_ENTRY)
				pScanner->pop();
			else if(token.type != Token::FLOW_SEQ_END)
				throw ParserException(token.mark, ErrorMsg::END_OF_SEQ_FLOW);
		}

		state.PopCollectionType(ParserState::FLOW_SEQ);
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

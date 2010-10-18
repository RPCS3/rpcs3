#include "sequence.h"
#include "eventhandler.h"
#include "node.h"
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

	void Sequence::Append(std::auto_ptr<Node> pNode)
	{
		m_data.push_back(pNode.release());
	}
	
	void Sequence::EmitEvents(AliasManager& am, EventHandler& eventHandler, const Mark& mark, const std::string& tag, anchor_t anchor) const
	{
		eventHandler.OnSequenceStart(mark, tag, anchor);
		for(std::size_t i=0;i<m_data.size();i++)
			m_data[i]->EmitEvents(am, eventHandler);
		eventHandler.OnSequenceEnd();
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

#include "sequence.h"
#include "node.h"
#include "parser.h"

namespace YAML
{
	Sequence::Sequence(Parser *pParser)
	{
		Read(pParser);
	}

	Sequence::~Sequence()
	{
		for(unsigned i=0;i<m_data.size();i++)
			delete m_data[i];
	}

	void Sequence::Read(Parser *pParser)
	{
		do {
			Node *pNode = pParser->ReadNextNode();
			m_data.push_back(pNode);
		} while(pParser->SeqContinues());
	}
}

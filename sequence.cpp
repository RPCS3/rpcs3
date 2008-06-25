#include "sequence.h"
#include "node.h"

namespace YAML
{
	Sequence::Sequence()
	{
	}

	Sequence::~Sequence()
	{
		for(unsigned i=0;i<m_data.size();i++)
			delete m_data[i];
	}
}

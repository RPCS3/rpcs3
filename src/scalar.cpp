#include "scalar.h"
#include "eventhandler.h"

namespace YAML
{
	Scalar::Scalar()
	{
	}

	Scalar::~Scalar()
	{
	}

	void Scalar::EmitEvents(AliasManager&, EventHandler& eventHandler, const Mark& mark, const std::string& tag, anchor_t anchor) const
	{
		eventHandler.OnScalar(mark, tag, anchor, m_data);
	}

	int Scalar::Compare(Content *pContent)
	{
		return -pContent->Compare(this);
	}

	int Scalar::Compare(Scalar *pScalar)
	{
		if(m_data < pScalar->m_data)
			return -1;
		else if(m_data > pScalar->m_data)
			return 1;
		else
			return 0;
	}
}


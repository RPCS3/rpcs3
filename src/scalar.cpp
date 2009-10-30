#include "scalar.h"
#include "scanner.h"
#include "token.h"
#include "exceptions.h"
#include "node.h"
#include "emitter.h"

namespace YAML
{
	Scalar::Scalar()
	{
	}

	Scalar::Scalar(const std::string& data): m_data(data)
	{
	}

	Scalar::~Scalar()
	{
	}

	Content *Scalar::Clone() const
	{
		return new Scalar(m_data);
	}

	void Scalar::Parse(Scanner *pScanner, ParserState& /*state*/)
	{
		Token& token = pScanner->peek();
		m_data = token.value;
		pScanner->pop();
	}

	void Scalar::Write(Emitter& out) const
	{
		out << m_data;
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


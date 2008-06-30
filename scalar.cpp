#include "scalar.h"
#include "scanner.h"
#include "token.h"

namespace YAML
{
	Scalar::Scalar()
	{
	}

	Scalar::~Scalar()
	{
	}

	void Scalar::Parse(Scanner *pScanner)
	{
		Token *pToken = pScanner->GetNextToken();
		m_data = pToken->value;
		delete pToken;
	}

	void Scalar::Write(std::ostream& out, int indent)
	{
		for(int i=0;i<indent;i++)
			out << "  ";
		out << "{scalar}\n";
		for(int i=0;i<indent;i++)
			out << "  ";
		out << m_data << std::endl;
	}
}

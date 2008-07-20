#include "crt.h"
#include "scalar.h"
#include "scanner.h"
#include "token.h"
#include "exceptions.h"
#include <sstream>

namespace YAML
{
	Scalar::Scalar()
	{
	}

	Scalar::~Scalar()
	{
	}

	void Scalar::Parse(Scanner *pScanner, const ParserState& state)
	{
		Token& token = pScanner->PeekToken();
		m_data = token.value;
		pScanner->PopToken();
	}

	void Scalar::Write(std::ostream& out, int indent, bool startedLine, bool onlyOneCharOnLine)
	{
		out << "\"";
		for(unsigned i=0;i<m_data.size();i++) {
			switch(m_data[i]) {
				case '\\': out << "\\\\"; break;
				case '\t': out << "\\t"; break;
				case '\n': out << "\\n"; break;
				case '\r': out << "\\r"; break;
				default: out << m_data[i]; break;
			}
		}
		out << "\"\n";
	}

	void Scalar::Read(std::string& s)
	{
		s = m_data;
	}

	void Scalar::Read(int& i)
	{
		std::stringstream data(m_data);
		data >> i;
		if(!data)
			throw InvalidScalar();
	}

	void Scalar::Read(unsigned& u)
	{
		std::stringstream data(m_data);
		data >> u;
		if(!data)
			throw InvalidScalar();
	}

	void Scalar::Read(long& l)
	{
		std::stringstream data(m_data);
		data >> l;
		if(!data)
			throw InvalidScalar();
	}
	
	void Scalar::Read(float& f)
	{
		std::stringstream data(m_data);
		data >> f;
		if(!data)
			throw InvalidScalar();
	}
	
	void Scalar::Read(double& d)
	{
		std::stringstream data(m_data);
		data >> d;
		if(!data)
			throw InvalidScalar();
	}

	void Scalar::Read(char& c)
	{
		std::stringstream data(m_data);
		data >> c;
		if(!data)
			throw InvalidScalar();
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

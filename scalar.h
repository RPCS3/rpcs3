#pragma once

#include "content.h"
#include <string>

namespace YAML
{
	class Scalar: public Content
	{
	public:
		Scalar();
		virtual ~Scalar();

		virtual void Parse(Scanner *pScanner, const ParserState& state);
		virtual void Write(std::ostream& out, int indent);

		// extraction
		virtual void Read(std::string& s);
		virtual void Read(int& i);
		virtual void Read(unsigned& u);
		virtual void Read(long& l);
		virtual void Read(float& f);
		virtual void Read(double& d);
		virtual void Read(char& c);

	protected:
		std::string m_data;
	};
}

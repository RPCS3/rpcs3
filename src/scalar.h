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
		virtual void Write(std::ostream& out, int indent, bool startedLine, bool onlyOneCharOnLine);

		virtual CONTENT_TYPE GetType() const;

		// extraction
		virtual void Read(std::string& s);
		virtual void Read(int& i);
		virtual void Read(unsigned& u);
		virtual void Read(long& l);
		virtual void Read(float& f);
		virtual void Read(double& d);
		virtual void Read(char& c);

		// ordering
		virtual int Compare(Content *pContent);
		virtual int Compare(Scalar *pScalar);
		virtual int Compare(Sequence *pSeq) { return -1; }
		virtual int Compare(Map *pMap) { return -1; }

	protected:
		std::string m_data;
	};
}

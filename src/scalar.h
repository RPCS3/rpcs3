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

		virtual bool IsScalar() const { return true; }

		// extraction
		virtual bool Read(std::string& s) const;
		virtual bool Read(int& i) const;
		virtual bool Read(unsigned& u) const;
		virtual bool Read(long& l) const;
		virtual bool Read(float& f) const;
		virtual bool Read(double& d) const;
		virtual bool Read(char& c) const;

		// ordering
		virtual int Compare(Content *pContent);
		virtual int Compare(Scalar *pScalar);
		virtual int Compare(Sequence *pSeq) { return -1; }
		virtual int Compare(Map *pMap) { return -1; }

	protected:
		std::string m_data;
	};
}

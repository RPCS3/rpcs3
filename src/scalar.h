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
		virtual bool GetScalar(std::string& scalar) const {
			scalar = m_data;
			return true;
		}

		// ordering
		virtual int Compare(Content *pContent);
		virtual int Compare(Scalar *pScalar);
		virtual int Compare(Sequence *) { return -1; }
		virtual int Compare(Map *) { return -1; }

	protected:
		std::string m_data;
	};
}

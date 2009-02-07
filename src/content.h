#pragma once

#include <ios>
#include <vector>
#include <map>
#include "parserstate.h"
#include "exceptions.h"
#include "ltnode.h"

namespace YAML
{
	class Scanner;
	class Parser;
	class Node;
	class Scalar;
	class Sequence;
	class Map;

	class Content
	{
	public:
		Content();
		virtual ~Content();

		virtual void Parse(Scanner *pScanner, const ParserState& state) = 0;
		virtual void Write(std::ostream& out, int indent, bool startedLine, bool onlyOneCharOnLine) = 0;

		virtual bool GetBegin(std::vector <Node *>::const_iterator&) const { return false; }
		virtual bool GetBegin(std::map <Node *, Node *, ltnode>::const_iterator&) const { return false; }
		virtual bool GetEnd(std::vector <Node *>::const_iterator&) const { return false; }
		virtual bool GetEnd(std::map <Node *, Node *, ltnode>::const_iterator&) const { return false; }
		virtual Node *GetNode(unsigned) const { return 0; }
		virtual unsigned GetSize() const { return 0; }
		virtual bool IsScalar() const { return false; }
		virtual bool IsMap() const { return false; }
		virtual bool IsSequence() const { return false; }

		// extraction
		virtual bool Read(std::string&) const { return false; }
		virtual bool Read(int&) const { return false; }
		virtual bool Read(unsigned&) const { return false; }
		virtual bool Read(long&) const { return false; }
		virtual bool Read(float&) const { return false; }
		virtual bool Read(double&) const { return false; }
		virtual bool Read(char&) const { return false; }
		virtual bool Read(bool&) const { return false; }

		// ordering
		virtual int Compare(Content *) { return 0; }
		virtual int Compare(Scalar *) { return 0; }
		virtual int Compare(Sequence *) { return 0; }
		virtual int Compare(Map *) { return 0; }

	protected:
	};
}

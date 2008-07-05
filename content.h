#pragma once

#include <ios>
#include <vector>
#include <map>
#include "parserstate.h"
#include "exceptions.h"

namespace YAML
{
	class Scanner;
	class Parser;
	class Node;

	class Content
	{
	public:
		Content();
		virtual ~Content();

		virtual void Parse(Scanner *pScanner, const ParserState& state) = 0;
		virtual void Write(std::ostream& out, int indent, bool startedLine, bool onlyOneCharOnLine) = 0;

		virtual bool GetBegin(std::vector <Node *>::const_iterator& it) const { return false; }
		virtual bool GetBegin(std::map <Node *, Node *>::const_iterator& it) const { return false; }
		virtual bool GetEnd(std::vector <Node *>::const_iterator& it) const { return false; }
		virtual bool GetEnd(std::map <Node *, Node *>::const_iterator& it) const { return false; }
		virtual Node *GetNode(unsigned i) const { return 0; }
		virtual unsigned GetSize() const { return 0; }

		// extraction
		virtual void Read(std::string& s) { throw InvalidScalar(); }
		virtual void Read(int& i) { throw InvalidScalar(); }
		virtual void Read(unsigned& u) { throw InvalidScalar(); }
		virtual void Read(long& l) { throw InvalidScalar(); }
		virtual void Read(float& f) { throw InvalidScalar(); }
		virtual void Read(double& d) { throw InvalidScalar(); }
		virtual void Read(char& c) { throw InvalidScalar(); }

	protected:
	};
}

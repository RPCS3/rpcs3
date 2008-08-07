#pragma once

#include "content.h"
#include <vector>

namespace YAML
{
	class Node;

	class Sequence: public Content
	{
	public:
		Sequence();
		virtual ~Sequence();

		void Clear();
		virtual bool GetBegin(std::vector <Node *>::const_iterator& it) const;
		virtual bool GetEnd(std::vector <Node *>::const_iterator& it) const;
		virtual Node *GetNode(unsigned i) const;
		virtual unsigned GetSize() const;

		virtual void Parse(Scanner *pScanner, const ParserState& state);
		virtual void Write(std::ostream& out, int indent, bool startedLine, bool onlyOneCharOnLine);

		virtual bool IsSequence() const { return true; }

		// ordering
		virtual int Compare(Content *pContent);
		virtual int Compare(Scalar *pScalar) { return 1; }
		virtual int Compare(Sequence *pSeq);
		virtual int Compare(Map *pMap) { return -1; }

	private:
		void ParseBlock(Scanner *pScanner, const ParserState& state);
		void ParseImplicit(Scanner *pScanner, const ParserState& state);
		void ParseFlow(Scanner *pScanner, const ParserState& state);

	protected:
		std::vector <Node *> m_data;
	};
}

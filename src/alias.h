#pragma once

#include "content.h"

namespace YAML
{
	class Alias : public Content
	{
	public:
		Alias(Content *pNodeContent);

		virtual void Parse(Scanner* pScanner, const ParserState& state);
		virtual void Write(std::ostream& out, int indent, bool startedLine, bool onlyOneCharOnLine);

		virtual bool GetBegin(std::vector <Node *>::const_iterator&) const;
		virtual bool GetBegin(std::map <Node *, Node *, ltnode>::const_iterator&) const;
		virtual bool GetEnd(std::vector <Node *>::const_iterator&) const;
		virtual bool GetEnd(std::map <Node *, Node *, ltnode>::const_iterator&) const;
		virtual Node* GetNode(unsigned) const;
		virtual unsigned GetSize() const;
		virtual bool IsScalar() const;
		virtual bool IsMap() const;
		virtual bool IsSequence() const;

		virtual bool GetScalar(std::string& s) const;

		virtual int Compare(Content *);
		virtual int Compare(Scalar *);
		virtual int Compare(Sequence *);
		virtual int Compare(Map *);

	private:
		Content* m_pRef;
	};
}

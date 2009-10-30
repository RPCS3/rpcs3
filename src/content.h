#pragma once

#ifndef CONTENT_H_62B23520_7C8E_11DE_8A39_0800200C9A66
#define CONTENT_H_62B23520_7C8E_11DE_8A39_0800200C9A66


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
	class Emitter;

	class Content
	{
	public:
		Content() {}
		virtual ~Content() {}
		
		virtual Content *Clone() const = 0;

		virtual void Parse(Scanner *pScanner, ParserState& state) = 0;
		virtual void Write(Emitter& out) const = 0;

		virtual bool GetBegin(std::vector <Node *>::const_iterator&) const { return false; }
		virtual bool GetBegin(std::map <Node *, Node *, ltnode>::const_iterator&) const { return false; }
		virtual bool GetEnd(std::vector <Node *>::const_iterator&) const { return false; }
		virtual bool GetEnd(std::map <Node *, Node *, ltnode>::const_iterator&) const { return false; }
		virtual Node *GetNode(std::size_t) const { return 0; }
		virtual std::size_t GetSize() const { return 0; }
		virtual bool IsScalar() const { return false; }
		virtual bool IsMap() const { return false; }
		virtual bool IsSequence() const { return false; }

		// extraction
		virtual bool GetScalar(std::string&) const { return false; }

		// ordering
		virtual int Compare(Content *) { return 0; }
		virtual int Compare(Scalar *) { return 0; }
		virtual int Compare(Sequence *) { return 0; }
		virtual int Compare(Map *) { return 0; }

	protected:
	};
}

#endif // CONTENT_H_62B23520_7C8E_11DE_8A39_0800200C9A66

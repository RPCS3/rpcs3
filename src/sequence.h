#pragma once

#ifndef SEQUENCE_H_62B23520_7C8E_11DE_8A39_0800200C9A66
#define SEQUENCE_H_62B23520_7C8E_11DE_8A39_0800200C9A66


#include "content.h"
#include <vector>

namespace YAML
{
	class Node;

	class Sequence: public Content
	{
	public:
		Sequence();
		Sequence(const std::vector<Node *>& data);
		virtual ~Sequence();

		void Clear();
		virtual Content *Clone() const;

		virtual bool GetBegin(std::vector <Node *>::const_iterator& it) const;
		virtual bool GetEnd(std::vector <Node *>::const_iterator& it) const;
		virtual Node *GetNode(std::size_t i) const;
		virtual std::size_t GetSize() const;

		virtual void Parse(Scanner *pScanner, ParserState& state);
		virtual void Write(Emitter& out) const;

		virtual bool IsSequence() const { return true; }

		// ordering
		virtual int Compare(Content *pContent);
		virtual int Compare(Scalar *) { return 1; }
		virtual int Compare(Sequence *pSeq);
		virtual int Compare(Map *) { return -1; }

	private:
		void ParseBlock(Scanner *pScanner, ParserState& state);
		void ParseFlow(Scanner *pScanner, ParserState& state);

	protected:
		std::vector <Node *> m_data;
	};
}

#endif // SEQUENCE_H_62B23520_7C8E_11DE_8A39_0800200C9A66

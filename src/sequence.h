#ifndef SEQUENCE_H_62B23520_7C8E_11DE_8A39_0800200C9A66
#define SEQUENCE_H_62B23520_7C8E_11DE_8A39_0800200C9A66

#if !defined(__GNUC__) || (__GNUC__ == 3 && __GNUC_MINOR__ >= 4) || (__GNUC__ >= 4) // GCC supports "pragma once" correctly since 3.4
#pragma once
#endif


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
		virtual Node *GetNode(std::size_t i) const;
		virtual std::size_t GetSize() const;

		virtual void Append(std::auto_ptr<Node> pNode);
		virtual void EmitEvents(AliasManager& am, EventHandler& eventHandler, const Mark& mark, const std::string& tag, anchor_t anchor) const;

		virtual bool IsSequence() const { return true; }

		// ordering
		virtual int Compare(Content *pContent);
		virtual int Compare(Scalar *) { return 1; }
		virtual int Compare(Sequence *pSeq);
		virtual int Compare(Map *) { return -1; }

	protected:
		std::vector <Node *> m_data;
	};
}

#endif // SEQUENCE_H_62B23520_7C8E_11DE_8A39_0800200C9A66

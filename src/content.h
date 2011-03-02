#ifndef CONTENT_H_62B23520_7C8E_11DE_8A39_0800200C9A66
#define CONTENT_H_62B23520_7C8E_11DE_8A39_0800200C9A66

#if !defined(__GNUC__) || (__GNUC__ == 3 && __GNUC_MINOR__ >= 4) || (__GNUC__ >= 4) // GCC supports "pragma once" correctly since 3.4
#pragma once
#endif


#include "yaml-cpp/anchor.h"
#include "yaml-cpp/exceptions.h"
#include "ltnode.h"
#include <map>
#include <memory>
#include <vector>

namespace YAML
{
	struct Mark;
	struct NodeProperties;
	class AliasManager;
	class EventHandler;
	class Map;
	class Node;
	class Scalar;
	class Scanner;
	class Sequence;

	class Content
	{
	public:
		Content() {}
		virtual ~Content() {}

		virtual bool GetBegin(std::vector <Node *>::const_iterator&) const { return false; }
		virtual bool GetBegin(std::map <Node *, Node *, ltnode>::const_iterator&) const { return false; }
		virtual bool GetEnd(std::vector <Node *>::const_iterator&) const { return false; }
		virtual bool GetEnd(std::map <Node *, Node *, ltnode>::const_iterator&) const { return false; }
		virtual Node *GetNode(std::size_t) const { return 0; }
		virtual std::size_t GetSize() const { return 0; }
		virtual bool IsScalar() const { return false; }
		virtual bool IsMap() const { return false; }
		virtual bool IsSequence() const { return false; }
		
		virtual void SetData(const std::string& data);
		virtual void Append(std::auto_ptr<Node> pNode);
		virtual void Insert(std::auto_ptr<Node> pKey, std::auto_ptr<Node> pValue);
		virtual void EmitEvents(AliasManager& am, EventHandler& eventHandler, const Mark& mark, const std::string& tag, anchor_t anchor) const = 0;

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

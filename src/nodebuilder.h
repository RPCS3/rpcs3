#pragma once

#ifndef NODEBUILDER_H_62B23520_7C8E_11DE_8A39_0800200C9A66
#define NODEBUILDER_H_62B23520_7C8E_11DE_8A39_0800200C9A66

#include "yaml-cpp/eventhandler.h"
#include "ptr_stack.h"
#include <map>
#include <memory>
#include <stack>

namespace YAML
{
	class Node;
	
	class NodeBuilder: public EventHandler
	{
	public:
		explicit NodeBuilder(Node& root);
		virtual ~NodeBuilder();

		virtual void OnDocumentStart(const Mark& mark);
		virtual void OnDocumentEnd();
		
		virtual void OnNull(const std::string& tag, anchor_t anchor);
		virtual void OnAlias(const Mark& mark, anchor_t anchor);
		virtual void OnScalar(const Mark& mark, const std::string& tag, anchor_t anchor, const std::string& value);
		
		virtual void OnSequenceStart(const Mark& mark, const std::string& tag, anchor_t anchor);
		virtual void OnSequenceEnd();
		
		virtual void OnMapStart(const Mark& mark, const std::string& tag, anchor_t anchor);
		virtual void OnMapEnd();
		
	private:
		Node& Push(anchor_t anchor);
		Node& Push();
		Node& Top();
		void Pop();

		void Insert(std::auto_ptr<Node> pNode);
		void RegisterAnchor(anchor_t anchor, const Node& node);
		
	private:
		Node& m_root;
		bool m_initializedRoot;
		bool m_finished;
		
		ptr_stack<Node> m_stack;
		ptr_stack<Node> m_pendingKeys;
		std::stack<bool> m_didPushKey;

		typedef std::vector<const Node *> Anchors;
		Anchors m_anchors;
	};
}

#endif // NODEBUILDER_H_62B23520_7C8E_11DE_8A39_0800200C9A66


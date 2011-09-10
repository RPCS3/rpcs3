#include "nodebuilder.h"
#include "yaml-cpp/mark.h"
#include "yaml-cpp/node.h"
#include <cassert>

namespace YAML
{
	NodeBuilder::NodeBuilder(Node& root): m_root(root), m_initializedRoot(false), m_finished(false)
	{
		m_root.Clear();
		m_anchors.push_back(0); // since the anchors start at 1
	}
	
	NodeBuilder::~NodeBuilder()
	{
	}

	void NodeBuilder::OnDocumentStart(const Mark&)
	{
	}

	void NodeBuilder::OnDocumentEnd()
	{
		assert(m_finished);
	}

	void NodeBuilder::OnNull(const Mark& mark, anchor_t anchor)
	{
		Node& node = Push(anchor);
		node.Init(NodeType::Null, mark, "");
		Pop();
	}

	void NodeBuilder::OnAlias(const Mark& /*mark*/, anchor_t anchor)
	{
		Node& node = *m_anchors[anchor];
		Insert(node);
		node.MarkAsAliased();
	}

	void NodeBuilder::OnScalar(const Mark& mark, const std::string& tag, anchor_t anchor, const std::string& value)
	{
		Node& node = Push(anchor);
		node.Init(NodeType::Scalar, mark, tag);
		node.SetScalarData(value);
		Pop();
	}

	void NodeBuilder::OnSequenceStart(const Mark& mark, const std::string& tag, anchor_t anchor)
	{
		Node& node = Push(anchor);
		node.Init(NodeType::Sequence, mark, tag);
	}

	void NodeBuilder::OnSequenceEnd()
	{
		Pop();
	}

	void NodeBuilder::OnMapStart(const Mark& mark, const std::string& tag, anchor_t anchor)
	{
		Node& node = Push(anchor);
		node.Init(NodeType::Map, mark, tag);
		m_didPushKey.push(false);
	}

	void NodeBuilder::OnMapEnd()
	{
		m_didPushKey.pop();
		Pop();
	}
	
	Node& NodeBuilder::Push(anchor_t anchor)
	{
		Node& node = Push();
		RegisterAnchor(anchor, node);
		return node;
	}
	
	Node& NodeBuilder::Push()
	{
		if(!m_initializedRoot) {
			m_initializedRoot = true;
			return m_root;
		}
		
		Node& node = m_root.CreateNode();
		m_stack.push(&node);
		return node;
	}
	
	Node& NodeBuilder::Top()
	{
		return m_stack.empty() ? m_root : *m_stack.top();
	}
	
	void NodeBuilder::Pop()
	{
		assert(!m_finished);
		if(m_stack.empty()) {
			m_finished = true;
			return;
		}
		
		Node& node = *m_stack.top();
		m_stack.pop();
		Insert(node);
	}
	
	void NodeBuilder::Insert(Node& node)
	{
		Node& curTop = Top();
		switch(curTop.Type()) {
			case NodeType::Null:
			case NodeType::Scalar:
				assert(false);
				break;
			case NodeType::Sequence:
				curTop.Append(node);
				break;
			case NodeType::Map:
				assert(!m_didPushKey.empty());
				if(m_didPushKey.top()) {
					assert(!m_pendingKeys.empty());

					Node& key = *m_pendingKeys.top();
					m_pendingKeys.pop();
					curTop.Insert(key, node);
					m_didPushKey.top() = false;
				} else {
					m_pendingKeys.push(&node);
					m_didPushKey.top() = true;
				}
				break;
		}
	}

	void NodeBuilder::RegisterAnchor(anchor_t anchor, Node& node)
	{
		if(anchor) {
			assert(anchor == m_anchors.size());
			m_anchors.push_back(&node);
		}
	}
}

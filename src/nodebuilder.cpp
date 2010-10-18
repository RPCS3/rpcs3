#include "nodebuilder.h"
#include "mark.h"
#include "node.h"
#include "nodeproperties.h"
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

	void NodeBuilder::OnNull(const std::string& tag, anchor_t anchor)
	{
		Node& node = Push(anchor);
		node.InitNull(tag);
		Pop();
	}

	void NodeBuilder::OnAlias(const Mark& mark, anchor_t anchor)
	{
		Node& node = Push();
		node.InitAlias(mark, *m_anchors[anchor]);
		Pop();
	}

	void NodeBuilder::OnScalar(const Mark& mark, const std::string& tag, anchor_t anchor, const std::string& value)
	{
		Node& node = Push(anchor);
		node.Init(CT_SCALAR, mark, tag);
		node.SetData(value);
		Pop();
	}

	void NodeBuilder::OnSequenceStart(const Mark& mark, const std::string& tag, anchor_t anchor)
	{
		Node& node = Push(anchor);
		node.Init(CT_SEQUENCE, mark, tag);
	}

	void NodeBuilder::OnSequenceEnd()
	{
		Pop();
	}

	void NodeBuilder::OnMapStart(const Mark& mark, const std::string& tag, anchor_t anchor)
	{
		Node& node = Push(anchor);
		node.Init(CT_MAP, mark, tag);
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
		
		std::auto_ptr<Node> pNode(new Node);
		m_stack.push(pNode);
		return m_stack.top();
	}
	
	Node& NodeBuilder::Top()
	{
		return m_stack.empty() ? m_root : m_stack.top();
	}
	
	void NodeBuilder::Pop()
	{
		assert(!m_finished);
		if(m_stack.empty()) {
			m_finished = true;
			return;
		}
		
		std::auto_ptr<Node> pNode = m_stack.pop();
		Insert(pNode);
	}
	
	void NodeBuilder::Insert(std::auto_ptr<Node> pNode)
	{
		Node& node = Top();
		switch(node.GetType()) {
			case CT_SEQUENCE:
				node.Append(pNode);
				break;
			case CT_MAP:
				assert(!m_didPushKey.empty());
				if(m_didPushKey.top()) {
					assert(!m_pendingKeys.empty());

					std::auto_ptr<Node> pKey = m_pendingKeys.pop();
					node.Insert(pKey, pNode);
					m_didPushKey.top() = false;
				} else {
					m_pendingKeys.push(pNode);
					m_didPushKey.top() = true;
				}
				break;
			default:
				assert(false);
		}
	}

	void NodeBuilder::RegisterAnchor(anchor_t anchor, const Node& node)
	{
		if(anchor) {
			assert(anchor == m_anchors.size());
			m_anchors.push_back(&node);
		}
	}
}

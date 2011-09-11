#include "yaml-cpp/old-api/node.h"
#include "yaml-cpp/aliasmanager.h"
#include "yaml-cpp/emitfromevents.h"
#include "yaml-cpp/emitter.h"
#include "yaml-cpp/eventhandler.h"
#include "old-api/iterpriv.h"
#include "old-api/nodebuilder.h"
#include "old-api/nodeownership.h"
#include "scanner.h"
#include "tag.h"
#include "token.h"
#include <cassert>
#include <stdexcept>

namespace YAML
{
	bool ltnode::operator()(const Node *pNode1, const Node *pNode2) const {
		return *pNode1 < *pNode2;
	}

	Node::Node(): m_pOwnership(new NodeOwnership), m_type(NodeType::Null)
	{
	}

	Node::Node(NodeOwnership& owner): m_pOwnership(new NodeOwnership(&owner)), m_type(NodeType::Null)
	{
	}

	Node::~Node()
	{
		Clear();
	}

	void Node::Clear()
	{
		m_pOwnership.reset(new NodeOwnership);
		m_type = NodeType::Null;
		m_tag.clear();
		m_scalarData.clear();
		m_seqData.clear();
		m_mapData.clear();
	}
	
	bool Node::IsAliased() const
	{
		return m_pOwnership->IsAliased(*this);
	}

	Node& Node::CreateNode()
	{
		return m_pOwnership->Create();
	}

	std::auto_ptr<Node> Node::Clone() const
	{
		std::auto_ptr<Node> pNode(new Node);
		NodeBuilder nodeBuilder(*pNode);
		EmitEvents(nodeBuilder);
		return pNode;
	}

	void Node::EmitEvents(EventHandler& eventHandler) const
	{
		eventHandler.OnDocumentStart(m_mark);
		AliasManager am;
		EmitEvents(am, eventHandler);
		eventHandler.OnDocumentEnd();
	}

	void Node::EmitEvents(AliasManager& am, EventHandler& eventHandler) const
	{
		anchor_t anchor = NullAnchor;
		if(IsAliased()) {
			anchor = am.LookupAnchor(*this);
			if(anchor) {
				eventHandler.OnAlias(m_mark, anchor);
				return;
			}
			
			am.RegisterReference(*this);
			anchor = am.LookupAnchor(*this);
		}
		
		switch(m_type) {
			case NodeType::Null:
				eventHandler.OnNull(m_mark, anchor);
				break;
			case NodeType::Scalar:
				eventHandler.OnScalar(m_mark, m_tag, anchor, m_scalarData);
				break;
			case NodeType::Sequence:
				eventHandler.OnSequenceStart(m_mark, m_tag, anchor);
				for(std::size_t i=0;i<m_seqData.size();i++)
					m_seqData[i]->EmitEvents(am, eventHandler);
				eventHandler.OnSequenceEnd();
				break;
			case NodeType::Map:
				eventHandler.OnMapStart(m_mark, m_tag, anchor);
				for(node_map::const_iterator it=m_mapData.begin();it!=m_mapData.end();++it) {
					it->first->EmitEvents(am, eventHandler);
					it->second->EmitEvents(am, eventHandler);
				}
				eventHandler.OnMapEnd();
				break;
		}
	}

	void Node::Init(NodeType::value type, const Mark& mark, const std::string& tag)
	{
		Clear();
		m_mark = mark;
		m_type = type;
		m_tag = tag;
	}

	void Node::MarkAsAliased()
	{
		m_pOwnership->MarkAsAliased(*this);
	}
	
	void Node::SetScalarData(const std::string& data)
	{
		assert(m_type == NodeType::Scalar); // TODO: throw?
		m_scalarData = data;
	}

	void Node::Append(Node& node)
	{
		assert(m_type == NodeType::Sequence); // TODO: throw?
		m_seqData.push_back(&node);
	}
	
	void Node::Insert(Node& key, Node& value)
	{
		assert(m_type == NodeType::Map); // TODO: throw?
		m_mapData[&key] = &value;
	}

	// begin
	// Returns an iterator to the beginning of this (sequence or map).
	Iterator Node::begin() const
	{
		switch(m_type) {
			case NodeType::Null:
			case NodeType::Scalar:
				return Iterator();
			case NodeType::Sequence:
				return Iterator(std::auto_ptr<IterPriv>(new IterPriv(m_seqData.begin())));
			case NodeType::Map:
				return Iterator(std::auto_ptr<IterPriv>(new IterPriv(m_mapData.begin())));
		}
		
		assert(false);
		return Iterator();
	}

	// end
	// . Returns an iterator to the end of this (sequence or map).
	Iterator Node::end() const
	{
		switch(m_type) {
			case NodeType::Null:
			case NodeType::Scalar:
				return Iterator();
			case NodeType::Sequence:
				return Iterator(std::auto_ptr<IterPriv>(new IterPriv(m_seqData.end())));
			case NodeType::Map:
				return Iterator(std::auto_ptr<IterPriv>(new IterPriv(m_mapData.end())));
		}
		
		assert(false);
		return Iterator();
	}

	// size
	// . Returns the size of a sequence or map node
	// . Otherwise, returns zero.
	std::size_t Node::size() const
	{
		switch(m_type) {
			case NodeType::Null:
			case NodeType::Scalar:
				return 0;
			case NodeType::Sequence:
				return m_seqData.size();
			case NodeType::Map:
				return m_mapData.size();
		}
		
		assert(false);
		return 0;
	}

	const Node *Node::FindAtIndex(std::size_t i) const
	{
		if(m_type == NodeType::Sequence)
			return m_seqData[i];
		return 0;
	}

	bool Node::GetScalar(std::string& s) const
	{
		switch(m_type) {
			case NodeType::Null:
				s = "~";
				return true;
			case NodeType::Scalar:
				s = m_scalarData;
				return true;
			case NodeType::Sequence:
			case NodeType::Map:
				return false;
		}
		
		assert(false);
		return false;
	}

	Emitter& operator << (Emitter& out, const Node& node)
	{
		EmitFromEvents emitFromEvents(out);
		node.EmitEvents(emitFromEvents);
		return out;
	}

	int Node::Compare(const Node& rhs) const
	{
		if(m_type != rhs.m_type)
			return rhs.m_type - m_type;
		
		switch(m_type) {
			case NodeType::Null:
				return 0;
			case NodeType::Scalar:
				return m_scalarData.compare(rhs.m_scalarData);
			case NodeType::Sequence:
				if(m_seqData.size() < rhs.m_seqData.size())
					return 1;
				else if(m_seqData.size() > rhs.m_seqData.size())
					return -1;
				for(std::size_t i=0;i<m_seqData.size();i++)
					if(int cmp = m_seqData[i]->Compare(*rhs.m_seqData[i]))
						return cmp;
				return 0;
			case NodeType::Map:
				if(m_mapData.size() < rhs.m_mapData.size())
					return 1;
				else if(m_mapData.size() > rhs.m_mapData.size())
					return -1;
				node_map::const_iterator it = m_mapData.begin();
				node_map::const_iterator jt = rhs.m_mapData.begin();
				for(;it!=m_mapData.end() && jt!=rhs.m_mapData.end();it++, jt++) {
					if(int cmp = it->first->Compare(*jt->first))
						return cmp;
					if(int cmp = it->second->Compare(*jt->second))
						return cmp;
				}
				return 0;
		}
		
		assert(false);
		return 0;
	}

	bool operator < (const Node& n1, const Node& n2)
	{
		return n1.Compare(n2) < 0;
	}
}

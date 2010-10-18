#include "yaml-cpp/node.h"
#include "aliascontent.h"
#include "yaml-cpp/aliasmanager.h"
#include "content.h"
#include "yaml-cpp/emitfromevents.h"
#include "yaml-cpp/emitter.h"
#include "yaml-cpp/eventhandler.h"
#include "iterpriv.h"
#include "map.h"
#include "nodebuilder.h"
#include "yaml-cpp/nodeproperties.h"
#include "scalar.h"
#include "scanner.h"
#include "sequence.h"
#include "tag.h"
#include "token.h"
#include <cassert>
#include <stdexcept>

namespace YAML
{
	// the ordering!
	bool ltnode::operator ()(const Node *pNode1, const Node *pNode2) const
	{
		return *pNode1 < *pNode2;
	}

	Node::Node(): m_type(CT_NONE), m_pContent(0), m_alias(false), m_pIdentity(this), m_referenced(false)
	{
	}

	Node::~Node()
	{
		Clear();
	}

	void Node::Clear()
	{
		delete m_pContent;
		m_type = CT_NONE;
		m_pContent = 0;
		m_alias = false;
		m_referenced = false;
		m_tag.clear();
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
		if(m_referenced || m_alias) {
			if(const Node *pOther = am.LookupReference(*this)) {
				eventHandler.OnAlias(m_mark, am.LookupAnchor(*pOther));
				return;
			}
			
			am.RegisterReference(*this);
			anchor = am.LookupAnchor(*this);
		}
		
		if(m_pContent)
			m_pContent->EmitEvents(am, eventHandler, m_mark, GetTag(), anchor);
		else
			eventHandler.OnNull(GetTag(), anchor);
	}

	void Node::Init(CONTENT_TYPE type, const Mark& mark, const std::string& tag)
	{
		Clear();
		m_mark = mark;
		m_type = type;
		m_tag = tag;
		m_alias = false;
		m_pIdentity = this;
		m_referenced = false;

		switch(type) {
			case CT_SCALAR:
				m_pContent = new Scalar;
				break;
			case CT_SEQUENCE:
				m_pContent = new Sequence;
				break;
			case CT_MAP:
				m_pContent = new Map;
				break;
			default:
				m_pContent = 0;
				break;
		}
	}

	void Node::InitNull(const std::string& tag)
	{
		Clear();
		m_tag = tag;
		m_alias = false;
		m_pIdentity = this;
		m_referenced = false;
	}

	void Node::InitAlias(const Mark& mark, const Node& identity)
	{
		Clear();
		m_mark = mark;
		m_alias = true;
		m_pIdentity = &identity;
		if(identity.m_pContent) {
			m_pContent = new AliasContent(identity.m_pContent);
			m_type = identity.GetType();
		}
		identity.m_referenced = true;
	}

	void Node::SetData(const std::string& data)
	{
		assert(m_pContent); // TODO: throw
		m_pContent->SetData(data);
	}

	void Node::Append(std::auto_ptr<Node> pNode)
	{
		assert(m_pContent); // TODO: throw
		m_pContent->Append(pNode);
	}
	
	void Node::Insert(std::auto_ptr<Node> pKey, std::auto_ptr<Node> pValue)
	{
		assert(m_pContent); // TODO: throw
		m_pContent->Insert(pKey, pValue);
	}

	// begin
	// Returns an iterator to the beginning of this (sequence or map).
	Iterator Node::begin() const
	{
		if(!m_pContent)
			return Iterator();

		std::vector <Node *>::const_iterator seqIter;
		if(m_pContent->GetBegin(seqIter))
			return Iterator(new IterPriv(seqIter));

		std::map <Node *, Node *, ltnode>::const_iterator mapIter;
		if(m_pContent->GetBegin(mapIter))
			return Iterator(new IterPriv(mapIter));

		return Iterator();
	}

	// end
	// . Returns an iterator to the end of this (sequence or map).
	Iterator Node::end() const
	{
		if(!m_pContent)
			return Iterator();

		std::vector <Node *>::const_iterator seqIter;
		if(m_pContent->GetEnd(seqIter))
			return Iterator(new IterPriv(seqIter));

		std::map <Node *, Node *, ltnode>::const_iterator mapIter;
		if(m_pContent->GetEnd(mapIter))
			return Iterator(new IterPriv(mapIter));

		return Iterator();
	}

	// size
	// . Returns the size of this node, if it's a sequence node.
	// . Otherwise, returns zero.
	std::size_t Node::size() const
	{
		if(!m_pContent)
			return 0;

		return m_pContent->GetSize();
	}

	const Node *Node::FindAtIndex(std::size_t i) const
	{
		if(!m_pContent)
			return 0;
		
		return m_pContent->GetNode(i);
	}

	bool Node::GetScalar(std::string& s) const
	{
		if(!m_pContent) {
			if(m_tag.empty())
				s = "~";
			else
				s = "";
			return true;
		}
		
		return m_pContent->GetScalar(s);
	}

	Emitter& operator << (Emitter& out, const Node& node)
	{
		EmitFromEvents emitFromEvents(out);
		node.EmitEvents(emitFromEvents);
		return out;
	}

	int Node::Compare(const Node& rhs) const
	{
		// Step 1: no content is the smallest
		if(!m_pContent) {
			if(rhs.m_pContent)
				return -1;
			else
				return 0;
		}
		if(!rhs.m_pContent)
			return 1;

		return m_pContent->Compare(rhs.m_pContent);
	}

	bool operator < (const Node& n1, const Node& n2)
	{
		return n1.Compare(n2) < 0;
	}
}

#include "crt.h"
#include "node.h"
#include "token.h"
#include "scanner.h"
#include "content.h"
#include "parser.h"
#include "scalar.h"
#include "sequence.h"
#include "map.h"
#include "alias.h"
#include "iterpriv.h"
#include <iostream>

namespace YAML
{
	// the ordering!
	bool ltnode::operator ()(const Node *pNode1, const Node *pNode2) const
	{
		return *pNode1 < *pNode2;
	}

	Node::Node(): m_pContent(0), m_alias(false), m_pIdentity(this), m_referenced(true)
	{
	}

	Node::~Node()
	{
		Clear();
	}

	void Node::Clear()
	{
		delete m_pContent;
		m_pContent = 0;
		m_alias = false;
		m_referenced = false;
		m_anchor.clear();
		m_tag.clear();
	}

	void Node::Parse(Scanner *pScanner, const ParserState& state)
	{
		Clear();

		// an empty node *is* a possibility
		if(pScanner->empty())
			return;

		// save location
		m_line = pScanner->peek().line;
		m_column = pScanner->peek().column;

		ParseHeader(pScanner, state);

		// is this an alias? if so, its contents are an alias to
		// a previously defined anchor
		if(m_alias) {
			// the scanner throws an exception if it doesn't know this anchor name
			const Node *pReferencedNode = pScanner->Retrieve(m_anchor);
			m_pIdentity = pReferencedNode;

			// mark the referenced node for the sake of the client code
			pReferencedNode->m_referenced = true;

			// use of an Alias object keeps the referenced content from
			// being deleted twice
			Content *pAliasedContent = pReferencedNode->m_pContent;
			if(pAliasedContent)
				m_pContent = new Alias(pAliasedContent);
			
			return;
		}

		// now split based on what kind of node we should be
		switch(pScanner->peek().type) {
			case TT_SCALAR:
				m_pContent = new Scalar;
				break;
			case TT_FLOW_SEQ_START:
			case TT_BLOCK_SEQ_START:
			case TT_BLOCK_ENTRY:
				m_pContent = new Sequence;
				break;
			case TT_FLOW_MAP_START:
			case TT_BLOCK_MAP_START:
				m_pContent = new Map;
				break;
			default:
				break;
		}

		// Have to save anchor before parsing to allow for aliases as
		// contained node (recursive structure)
		if(!m_anchor.empty())
			pScanner->Save(m_anchor, this);

		if(m_pContent)
			m_pContent->Parse(pScanner, state);
	}

	// ParseHeader
	// . Grabs any tag, alias, or anchor tokens and deals with them.
	void Node::ParseHeader(Scanner *pScanner, const ParserState& state)
	{
		while(1) {
			if(pScanner->empty())
				return;

			switch(pScanner->peek().type) {
				case TT_TAG: ParseTag(pScanner, state); break;
				case TT_ANCHOR: ParseAnchor(pScanner, state); break;
				case TT_ALIAS: ParseAlias(pScanner, state); break;
				default: return;
			}
		}
	}

	void Node::ParseTag(Scanner *pScanner, const ParserState& state)
	{
		Token& token = pScanner->peek();
		if(m_tag != "")
			throw ParserException(token.line, token.column, ErrorMsg::MULTIPLE_TAGS);

		m_tag = state.TranslateTag(token.value);

		for(unsigned i=0;i<token.params.size();i++)
			m_tag += token.params[i];
		pScanner->pop();
	}
	
	void Node::ParseAnchor(Scanner *pScanner, const ParserState& state)
	{
		Token& token = pScanner->peek();
		if(m_anchor != "")
			throw ParserException(token.line, token.column, ErrorMsg::MULTIPLE_ANCHORS);

		m_anchor = token.value;
		m_alias = false;
		pScanner->pop();
	}

	void Node::ParseAlias(Scanner *pScanner, const ParserState& state)
	{
		Token& token = pScanner->peek();
		if(m_anchor != "")
			throw ParserException(token.line, token.column, ErrorMsg::MULTIPLE_ALIASES);
		if(m_tag != "")
			throw ParserException(token.line, token.column, ErrorMsg::ALIAS_CONTENT);

		m_anchor = token.value;
		m_alias = true;
		pScanner->pop();
	}

	void Node::Write(std::ostream& out, int indent, bool startedLine, bool onlyOneCharOnLine) const
	{
		// If using an anchor or tag for the whole document, document start
		// must be explicit
		bool indicateDocStart = (indent == 0);

		// write anchor/alias
		if(m_anchor != "") {
			if (indicateDocStart) {
				out << "--- ";
				indicateDocStart = false;
			}

			if(m_alias)
				out << "*";
			else
				out << "&";
			out << m_anchor << " ";
			startedLine = true;
			onlyOneCharOnLine = false;
		}

		// write tag
		if(m_tag != "") {
			if (indicateDocStart) {
				out << "--- ";
				indicateDocStart = false;
			}

			// put the tag in the "proper" brackets
			if(m_tag.substr(0, 2) == std::string("!<") && m_tag.substr(m_tag.size() - 1) == std::string(">"))
				out << m_tag << " ";
			else
				out << "!<" << m_tag << "> ";
			startedLine = true;
			onlyOneCharOnLine = false;
		}

		if(!m_pContent)
			out << "\n";
		else
			m_pContent->Write(out, indent, startedLine, onlyOneCharOnLine);
	}

	CONTENT_TYPE Node::GetType() const
	{
		if(!m_pContent)
			return CT_NONE;

		if(m_pContent->IsScalar())
			return CT_SCALAR;
		else if(m_pContent->IsSequence())
			return CT_SEQUENCE;
		else if(m_pContent->IsMap())
			return CT_MAP;
			
		return CT_NONE;
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
	unsigned Node::size() const
	{
		if(!m_pContent)
			return 0;

		return m_pContent->GetSize();
	}

	const Node& Node::operator [] (unsigned u) const
	{
		if(!m_pContent)
			throw BadDereference();

		Node *pNode = m_pContent->GetNode(u);
		if(pNode)
			return *pNode;

		return GetValue(u);
	}

	const Node& Node::operator [] (int i) const
	{
		if(!m_pContent)
			throw BadDereference();

		Node *pNode = m_pContent->GetNode(i);
		if(pNode)
			return *pNode;

		return GetValue(i);
	}

	///////////////////////////////////////////////////////
	// Extraction
	// Note: these Read() functions are identical, but
	// they're not templated because they use a Content virtual
	// function, so we'd have to #include that in node.h, and
	// I don't want to.

	bool Node::Read(std::string& s) const
	{
		if(!m_pContent)
			return false;

		return m_pContent->Read(s);
	}

	bool Node::Read(int& i) const
	{
		if(!m_pContent)
			return false;

		return m_pContent->Read(i);
	}

	bool Node::Read(unsigned& u) const
	{
		if(!m_pContent)
			return false;

		return m_pContent->Read(u);
	}

	bool Node::Read(long& l) const
	{
		if(!m_pContent)
			return false;

		return m_pContent->Read(l);
	}

	bool Node::Read(float& f) const
	{
		if(!m_pContent)
			return false;

		return m_pContent->Read(f);
	}

	bool Node::Read(double& d) const
	{
		if(!m_pContent)
			return false;

		return m_pContent->Read(d);
	}

	bool Node::Read(char& c) const
	{
		if(!m_pContent)
			return false;

		return m_pContent->Read(c);
	}

	bool Node::Read(bool& b) const
	{
		if(!m_pContent)
			return false;

		return m_pContent->Read(b);
	}

	std::ostream& operator << (std::ostream& out, const Node& node)
	{
		node.Write(out, 0, false, false);
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

#pragma once

#ifndef NODE_H_62B23520_7C8E_11DE_8A39_0800200C9A66
#define NODE_H_62B23520_7C8E_11DE_8A39_0800200C9A66


#include "conversion.h"
#include "exceptions.h"
#include "iterator.h"
#include "mark.h"
#include "noncopyable.h"
#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <memory>

namespace YAML
{
	class Content;
	class Scanner;
	class Emitter;
	struct ParserState;

	enum CONTENT_TYPE { CT_NONE, CT_SCALAR, CT_SEQUENCE, CT_MAP };

	class Node: private noncopyable
	{
	public:
		Node();
		~Node();

		void Clear();
		std::auto_ptr<Node> Clone() const;
		void Parse(Scanner *pScanner, ParserState& state);

		CONTENT_TYPE GetType() const;

		// file location of start of this node
		const Mark GetMark() const { return m_mark; }

		// accessors
		Iterator begin() const;
		Iterator end() const;
		std::size_t size() const;

		// extraction of scalars
		bool GetScalar(std::string& s) const;

		// we can specialize this for other values
		template <typename T>
		bool Read(T& value) const;

		template <typename T>
		const T Read() const;
		
		template <typename T>
		operator T() const;

		template <typename T>
		friend void operator >> (const Node& node, T& value);

		// retrieval for maps and sequences
		template <typename T>
		const Node *FindValue(const T& key) const;

		template <typename T>
		const Node& operator [] (const T& key) const;
		
		// specific to maps
		const Node *FindValue(const char *key) const;
		const Node& operator [] (const char *key) const;

		// for anchors/aliases
		const Node *Identity() const { return m_pIdentity; }
		bool IsAlias() const { return m_alias; }
		bool IsReferenced() const { return m_referenced; }
		
		// for tags
		const std::string GetTag() const { return IsAlias() ? m_pIdentity->GetTag() : m_tag; }

		// emitting
		friend Emitter& operator << (Emitter& out, const Node& node);

		// ordering
		int Compare(const Node& rhs) const;
		friend bool operator < (const Node& n1, const Node& n2);

	private:
		// helper for sequences
		template <typename, bool> friend struct _FindFromNodeAtIndex;
		const Node *FindAtIndex(std::size_t i) const;
		
		// helper for maps
		template <typename T>
		const Node& GetValue(const T& key) const;

		template <typename T>
		const Node *FindValueForKey(const T& key) const;
		
		// helper for cloning
		Node(const Mark& mark, const std::string& anchor, const std::string& tag, const Content *pContent);

		// helpers for parsing
		void ParseHeader(Scanner *pScanner, ParserState& state);
		void ParseTag(Scanner *pScanner, ParserState& state);
		void ParseAnchor(Scanner *pScanner, ParserState& state);
		void ParseAlias(Scanner *pScanner, ParserState& state);

	private:
		Mark m_mark;
		std::string m_anchor, m_tag;
		Content *m_pContent;
		bool m_alias;
		const Node *m_pIdentity;
		mutable bool m_referenced;
	};
	
	// comparisons with auto-conversion
	template <typename T>
	bool operator == (const T& value, const Node& node);
	
	template <typename T>
	bool operator == (const Node& node, const T& value);
	
	template <typename T>
	bool operator != (const T& value, const Node& node);
	
	template <typename T>
	bool operator != (const Node& node, const T& value);
	
	bool operator == (const char *value, const Node& node);
	bool operator == (const Node& node, const char *value);
	bool operator != (const char *value, const Node& node);
	bool operator != (const Node& node, const char *value);
}

#include "nodeimpl.h"
#include "nodereadimpl.h"

#endif // NODE_H_62B23520_7C8E_11DE_8A39_0800200C9A66

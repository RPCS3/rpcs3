#pragma once

#ifndef NODE_H_62B23520_7C8E_11DE_8A39_0800200C9A66
#define NODE_H_62B23520_7C8E_11DE_8A39_0800200C9A66


#include "conversion.h"
#include "exceptions.h"
#include "iterator.h"
#include "mark.h"
#include "noncopyable.h"
#include "parserstate.h"
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

	enum CONTENT_TYPE { CT_NONE, CT_SCALAR, CT_SEQUENCE, CT_MAP };

	class Node: private noncopyable
	{
	public:
		Node();
		~Node();

		void Clear();
		std::auto_ptr<Node> Clone() const;
		void Parse(Scanner *pScanner, const ParserState& state);

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
		void ParseHeader(Scanner *pScanner, const ParserState& state);
		void ParseTag(Scanner *pScanner, const ParserState& state);
		void ParseAnchor(Scanner *pScanner, const ParserState& state);
		void ParseAlias(Scanner *pScanner, const ParserState& state);

	private:
		Mark m_mark;
		std::string m_anchor, m_tag;
		Content *m_pContent;
		bool m_alias;
		const Node *m_pIdentity;
		mutable bool m_referenced;
	};
}

#include "nodeimpl.h"

#endif // NODE_H_62B23520_7C8E_11DE_8A39_0800200C9A66

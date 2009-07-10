#pragma once

#include <string>
#include <vector>
#include <map>
#include "parserstate.h"
#include "exceptions.h"
#include "iterator.h"
#include "conversion.h"

namespace YAML
{
	class Content;
	class Scanner;
	class Emitter;

	enum CONTENT_TYPE { CT_NONE, CT_SCALAR, CT_SEQUENCE, CT_MAP };

	class Node
	{
	public:
		Node();
		~Node();

		void Clear();
		void Parse(Scanner *pScanner, const ParserState& state);

		CONTENT_TYPE GetType() const;

		// file location of start of this node
		int GetLine() const { return m_line; }
		int GetColumn() const { return m_column; }

		// accessors
		Iterator begin() const;
		Iterator end() const;
		unsigned size() const;

		// extraction of scalars
		bool GetScalar(std::string& s) const;

		// we can specialize this for other values
		template <typename T>
		bool Read(T& value) const;

		template <typename T>
		friend void operator >> (const Node& node, T& value);

		// just for maps
		template <typename T>
		const Node& GetValue(const T& key) const;

		template <typename T>
		const Node& operator [] (const T& key) const;

		const Node& operator [] (const char *key) const;

		// just for sequences
		const Node& operator [] (unsigned u) const;
		const Node& operator [] (int i) const;

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
		// shouldn't be copyable! (at least for now)
		Node(const Node& rhs);
		Node& operator = (const Node& rhs);

	private:
		void ParseHeader(Scanner *pScanner, const ParserState& state);
		void ParseTag(Scanner *pScanner, const ParserState& state);
		void ParseAnchor(Scanner *pScanner, const ParserState& state);
		void ParseAlias(Scanner *pScanner, const ParserState& state);

	private:
		int m_line, m_column;
		std::string m_anchor, m_tag;
		Content *m_pContent;
		bool m_alias;
		const Node *m_pIdentity;
		mutable bool m_referenced;
	};

	// templated things we need to keep inline in the header
	template <typename T>
	inline bool Node::Read(T& value) const {
		std::string scalar;
		if(!GetScalar(scalar))
			return false;

		return Convert(scalar, value);
	}

	template <typename T>
	inline void operator >> (const Node& node, T& value)
	{
		if(!node.Read(value))
			throw InvalidScalar(node.m_line, node.m_column);
	}

	template <typename T>
	inline const Node& Node::GetValue(const T& key) const
	{
		if(!m_pContent)
			throw BadDereference();

		for(Iterator it=begin();it!=end();++it) {
			T t;
			if(it.first().Read(t)) {
				if(key == t)
					return it.second();
			}
		}

		throw MakeTypedKeyNotFound(m_line, m_column, key);
	}

	template <typename T>
	inline const Node& Node::operator [] (const T& key) const
	{
		return GetValue(key);
	}

	inline const Node& Node::operator [] (const char *key) const
	{
		return GetValue(std::string(key));
	}
}

#pragma once

#include <string>
#include <ios>
#include <vector>
#include <map>
#include "parserstate.h"
#include "exceptions.h"
#include "iterator.h"

namespace YAML
{
	class Content;
	class Scanner;

	enum CONTENT_TYPE { CT_NONE, CT_SCALAR, CT_SEQUENCE, CT_MAP };

	class Node
	{
	public:
		Node();
		~Node();

		void Clear();
		void Parse(Scanner *pScanner, const ParserState& state);
		void Write(std::ostream& out, int indent, bool startedLine, bool onlyOneCharOnLine) const;

		CONTENT_TYPE GetType() const;

		// file location of start of this node
		int GetLine() const { return m_line; }
		int GetColumn() const { return m_column; }

		// accessors
		Iterator begin() const;
		Iterator end() const;
		unsigned size() const;

		// extraction of scalars
		bool Read(std::string& s) const;
		bool Read(int& i) const;
		bool Read(unsigned& u) const;
		bool Read(long& l) const;
		bool Read(float& f) const;
		bool Read(double& d) const;
		bool Read(char& c) const;
		bool Read(bool& b) const;

		// so you can specialize for other values
		template <typename T>
		friend bool Read(const Node& node, T& value);

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

		// insertion
		friend std::ostream& operator << (std::ostream& out, const Node& node);

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
	};

	// templated things we need to keep inline in the header
	template <typename T>
	inline bool Read(const Node& node, T& value)
	{
		return node.Read(value);
	}

	template <typename T>
	inline void operator >> (const Node& node, T& value)
	{
		if(!Read(node, value))
			throw InvalidScalar(node.m_line, node.m_column);
	}

	template <typename T>
	inline const Node& Node::GetValue(const T& key) const
	{
		if(!m_pContent)
			throw BadDereference();

		for(Iterator it=begin();it!=end();++it) {
			T t;
			if(YAML::Read(it.first(), t)) {
				if(key == t)
					return it.second();
			}
		}

		throw KeyNotFound(m_line, m_column);
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

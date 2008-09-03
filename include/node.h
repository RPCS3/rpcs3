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

		// accessors
		Iterator begin() const;
		Iterator end() const;
		unsigned size() const;

		template <typename T>
		const Node& GetValue(const T& key) const {
			if(!m_pContent)
				throw BadDereference();

			for(Iterator it=begin();it!=end();++it) {
				T t;
				try {
					it.first() >> t;
					if(key == t)
						return it.second();
				} catch(RepresentationException&) {
				}
			}

			throw BadDereference();
		}

		template <typename T>
		const Node& operator [] (const T& key) const {
			return GetValue(key);
		}

		const Node& operator [] (const char *key) const {
			return GetValue(std::string(key));
		}

		const Node& operator [] (unsigned u) const;
		const Node& operator [] (int i) const;

		// extraction
		friend void operator >> (const Node& node, std::string& s);
		friend void operator >> (const Node& node, int& i);
		friend void operator >> (const Node& node, unsigned& u);
		friend void operator >> (const Node& node, long& l);
		friend void operator >> (const Node& node, float& f);
		friend void operator >> (const Node& node, double& d);
		friend void operator >> (const Node& node, char& c);

		// insertion
		friend std::ostream& operator << (std::ostream& out, const Node& node);

		// ordering
		int Compare(const Node& rhs) const;
		friend bool operator < (const Node& n1, const Node& n2);

	private:
		void ParseHeader(Scanner *pScanner, const ParserState& state);
		void ParseTag(Scanner *pScanner, const ParserState& state);
		void ParseAnchor(Scanner *pScanner, const ParserState& state);
		void ParseAlias(Scanner *pScanner, const ParserState& state);

	private:
		std::string m_anchor, m_tag;
		Content *m_pContent;
		bool m_alias;
	};
}

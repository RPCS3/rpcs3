#pragma once

#include <string>
#include <ios>
#include <vector>
#include <map>
#include "parserstate.h"
#include "exceptions.h"

namespace YAML
{
	class Content;
	class Scanner;

	class Node
	{
	public:
		class Iterator
		{
		public:
			Iterator();
			Iterator(std::vector <Node *>::const_iterator it);
			Iterator(std::map <Node *, Node *>::const_iterator it);
			~Iterator();

			friend bool operator == (const Iterator& it, const Iterator& jt);
			friend bool operator != (const Iterator& it, const Iterator& jt);
			Iterator& operator ++ ();
			Iterator operator ++ (int);
			const Node& operator * ();
			const Node *operator -> ();
			const Node& first();
			const Node& second();

		private:
			enum ITER_TYPE { IT_NONE, IT_SEQ, IT_MAP };
			ITER_TYPE type;

			std::vector <Node *>::const_iterator seqIter;
			std::map <Node *, Node *>::const_iterator mapIter;
		};

	public:
		Node();
		~Node();

		void Clear();
		void Parse(Scanner *pScanner, const ParserState& state);
		void Write(std::ostream& out, int indent);

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

	private:
		void ParseHeader(Scanner *pScanner, const ParserState& state);
		void ParseTag(Scanner *pScanner, const ParserState& state);
		void ParseAnchor(Scanner *pScanner, const ParserState& state);
		void ParseAlias(Scanner *pScanner, const ParserState& state);

	private:
		bool m_alias;
		std::string m_anchor, m_tag;
		Content *m_pContent;
	};
}

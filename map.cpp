#include "map.h"
#include "node.h"
#include "scanner.h"
#include "token.h"
#include "exceptions.h"

namespace YAML
{
	Map::Map()
	{
	}

	Map::~Map()
	{
		Clear();
	}

	void Map::Clear()
	{
		for(node_map::const_iterator it=m_data.begin();it!=m_data.end();++it) {
			delete it->first;
			delete it->second;
		}
		m_data.clear();
	}

	bool Map::GetBegin(std::map <Node *, Node *, ltnode>::const_iterator& it) const
	{
		it = m_data.begin();
		return true;
	}

	bool Map::GetEnd(std::map <Node *, Node *, ltnode>::const_iterator& it) const
	{
		it = m_data.end();
		return true;
	}

	void Map::Parse(Scanner *pScanner, const ParserState& state)
	{
		Clear();

		// split based on start token
		Token *pToken = pScanner->PeekNextToken();

		switch(pToken->type) {
			case TT_BLOCK_MAP_START: ParseBlock(pScanner, state); break;
			case TT_FLOW_MAP_START: ParseFlow(pScanner, state); break;
		}
	}

	void Map::ParseBlock(Scanner *pScanner, const ParserState& state)
	{
		// eat start token
		pScanner->EatNextToken();

		while(1) {
			Token *pToken = pScanner->PeekNextToken();
			if(!pToken)
				throw ParserException(-1, -1, ErrorMsg::END_OF_MAP);

			if(pToken->type != TT_KEY && pToken->type != TT_BLOCK_END)
				throw ParserException(pToken->line, pToken->column, ErrorMsg::END_OF_MAP);

			pScanner->PopNextToken();
			if(pToken->type == TT_BLOCK_END)
				break;

			Node *pKey = new Node;
			Node *pValue = new Node;

			try {
				// grab key
				pKey->Parse(pScanner, state);

				// now grab value (optional)
				if(pScanner->PeekNextToken() && pScanner->PeekNextToken()->type == TT_VALUE) {
					pScanner->PopNextToken();
					pValue->Parse(pScanner, state);
				}

				m_data[pKey] = pValue;
			} catch(Exception& e) {
				delete pKey;
				delete pValue;
				throw e;
			}
		}
	}

	void Map::ParseFlow(Scanner *pScanner, const ParserState& state)
	{
		// eat start token
		pScanner->EatNextToken();

		while(1) {
			Token *pToken = pScanner->PeekNextToken();
			if(!pToken)
				throw ParserException(-1, -1, ErrorMsg::END_OF_MAP_FLOW);

			// first check for end
			if(pToken->type == TT_FLOW_MAP_END) {
				pScanner->EatNextToken();
				break;
			}

			// now it better be a key
			if(pToken->type != TT_KEY)
				throw ParserException(pToken->line, pToken->column, ErrorMsg::END_OF_MAP_FLOW);

			pScanner->PopNextToken();

			Node *pKey = new Node;
			Node *pValue = new Node;

			try {
				// grab key
				pKey->Parse(pScanner, state);

				// now grab value (optional)
				if(pScanner->PeekNextToken() && pScanner->PeekNextToken()->type == TT_VALUE) {
					pScanner->PopNextToken();
					pValue->Parse(pScanner, state);
				}

				// now eat the separator (or could be a map end, which we ignore - but if it's neither, then it's a bad node)
				pToken = pScanner->PeekNextToken();
				if(pToken->type == TT_FLOW_ENTRY)
					pScanner->EatNextToken();
				else if(pToken->type != TT_FLOW_MAP_END)
					throw ParserException(pToken->line, pToken->column, ErrorMsg::END_OF_MAP_FLOW);

				m_data[pKey] = pValue;
			} catch(Exception& e) {
				// clean up and rethrow
				delete pKey;
				delete pValue;
				throw e;
			}
		}
	}

	void Map::Write(std::ostream& out, int indent, bool startedLine, bool onlyOneCharOnLine)
	{
		if(startedLine && !onlyOneCharOnLine)
			out << std::endl;

		for(node_map::const_iterator it=m_data.begin();it!=m_data.end();++it) {
			if((startedLine && !onlyOneCharOnLine) || it != m_data.begin()) {
				for(int i=0;i<indent;i++)
					out << "  ";
			}

			out << "? ";
			it->first->Write(out, indent + 1, true, it!= m_data.begin() || !startedLine || onlyOneCharOnLine);

			for(int i=0;i<indent;i++)
				out << "  ";
			out << ": ";
			it->second->Write(out, indent + 1, true, true);
		}

		if(m_data.empty())
			out << std::endl;
	}

	int Map::Compare(Content *pContent)
	{
		return -pContent->Compare(this);
	}

	int Map::Compare(Map *pMap)
	{
		node_map::const_iterator it = m_data.begin(), jt = pMap->m_data.begin();
		while(1) {
			if(it == m_data.end()) {
				if(jt == pMap->m_data.end())
					return 0;
				else
					return -1;
			}
			if(jt == pMap->m_data.end())
				return 1;

			int cmp = it->first->Compare(*jt->first);
			if(cmp != 0)
				return cmp;

			cmp = it->second->Compare(*jt->second);
			if(cmp != 0)
				return cmp;
		}

		return 0;
	}
}

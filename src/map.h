#pragma once

#ifndef MAP_H_62B23520_7C8E_11DE_8A39_0800200C9A66
#define MAP_H_62B23520_7C8E_11DE_8A39_0800200C9A66


#include "content.h"
#include <map>
#include <memory>

namespace YAML
{
	class Node;

	class Map: public Content
	{
	private:
		typedef std::map <Node *, Node *, ltnode> node_map;

	public:
		Map();
		Map(const node_map& data);
		virtual ~Map();

		void Clear();
		virtual Content *Clone() const;

		virtual bool GetBegin(std::map <Node *, Node *, ltnode>::const_iterator& it) const;
		virtual bool GetEnd(std::map <Node *, Node *, ltnode>::const_iterator& it) const;
		virtual std::size_t GetSize() const;
		virtual void Parse(Scanner *pScanner, ParserState& state);
		virtual void Write(Emitter& out) const;

		virtual bool IsMap() const { return true; }

		// ordering
		virtual int Compare(Content *pContent);
		virtual int Compare(Scalar *) { return 1; }
		virtual int Compare(Sequence *) { return 1; }
		virtual int Compare(Map *pMap);

	private:
		void ParseBlock(Scanner *pScanner, ParserState& state);
		void ParseFlow(Scanner *pScanner, ParserState& state);
		void ParseCompact(Scanner *pScanner, ParserState& state);
		void ParseCompactWithNoKey(Scanner *pScanner, ParserState& state);
		
		void AddEntry(std::auto_ptr<Node> pKey, std::auto_ptr<Node> pValue);

	private:
		node_map m_data;
	};
}

#endif // MAP_H_62B23520_7C8E_11DE_8A39_0800200C9A66

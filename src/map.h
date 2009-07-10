#pragma once

#include "content.h"
#include <map>

namespace YAML
{
	class Node;

	class Map: public Content
	{
	public:
		Map();
		virtual ~Map();

		void Clear();
		virtual bool GetBegin(std::map <Node *, Node *, ltnode>::const_iterator& it) const;
		virtual bool GetEnd(std::map <Node *, Node *, ltnode>::const_iterator& it) const;
		virtual void Parse(Scanner *pScanner, const ParserState& state);
		virtual void Write(Emitter& out) const;

		virtual bool IsMap() const { return true; }

		// ordering
		virtual int Compare(Content *pContent);
		virtual int Compare(Scalar *) { return 1; }
		virtual int Compare(Sequence *) { return 1; }
		virtual int Compare(Map *pMap);

	private:
		void ParseBlock(Scanner *pScanner, const ParserState& state);
		void ParseFlow(Scanner *pScanner, const ParserState& state);

	protected:
		typedef std::map <Node *, Node *, ltnode> node_map;
		node_map m_data;
	};
}

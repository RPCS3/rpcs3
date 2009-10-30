#pragma once

#ifndef PARSERSTATE_H_62B23520_7C8E_11DE_8A39_0800200C9A66
#define PARSERSTATE_H_62B23520_7C8E_11DE_8A39_0800200C9A66


#include <string>
#include <map>
#include <stack>
#include <cassert>

namespace YAML
{
	struct Version {
		bool isDefault;
		int major, minor;
	};
	
	struct ParserState
	{
		enum COLLECTION_TYPE { NONE, BLOCK_MAP, BLOCK_SEQ, FLOW_MAP, FLOW_SEQ, COMPACT_MAP };
		
		ParserState();

		const std::string TranslateTagHandle(const std::string& handle) const;
		COLLECTION_TYPE GetCurCollectionType() const { if(collectionStack.empty()) return NONE; return collectionStack.top(); }
		
		void PushCollectionType(COLLECTION_TYPE type) { collectionStack.push(type); }
		void PopCollectionType(COLLECTION_TYPE type) { assert(type == GetCurCollectionType()); collectionStack.pop(); }
	
		Version version;
		std::map <std::string, std::string> tags;
		std::stack <COLLECTION_TYPE> collectionStack;
	};
}

#endif // PARSERSTATE_H_62B23520_7C8E_11DE_8A39_0800200C9A66

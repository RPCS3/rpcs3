#pragma once

#ifndef COLLECTIONSTACK_H_62B23520_7C8E_11DE_8A39_0800200C9A66
#define COLLECTIONSTACK_H_62B23520_7C8E_11DE_8A39_0800200C9A66


#include <stack>
#include <cassert>

namespace YAML
{
	struct CollectionType {
		enum value { None, BlockMap, BlockSeq, FlowMap, FlowSeq, CompactMap };
	};

	class CollectionStack
	{
	public:
		CollectionType::value GetCurCollectionType() const {
			if(collectionStack.empty())
				return CollectionType::None;
			return collectionStack.top();
		}
		
		void PushCollectionType(CollectionType::value type) { collectionStack.push(type); }
		void PopCollectionType(CollectionType::value type) { assert(type == GetCurCollectionType()); collectionStack.pop(); }
		
	private:
		std::stack<CollectionType::value> collectionStack;
	};
}

#endif // COLLECTIONSTACK_H_62B23520_7C8E_11DE_8A39_0800200C9A66

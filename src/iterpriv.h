#pragma once

#include "ltnode.h"
#include <vector>
#include <map>

namespace YAML
{
	class Node;

	// IterPriv
	// . The implementation for iterators - essentially a union of sequence and map iterators.
	struct IterPriv
	{
		IterPriv(): type(IT_NONE) {}
		IterPriv(std::vector <Node *>::const_iterator it): seqIter(it), type(IT_SEQ) {}
		IterPriv(std::map <Node *, Node *, ltnode>::const_iterator it): mapIter(it), type(IT_MAP) {}

		enum ITER_TYPE { IT_NONE, IT_SEQ, IT_MAP };
		ITER_TYPE type;

		std::vector <Node *>::const_iterator seqIter;
		std::map <Node *, Node *, ltnode>::const_iterator mapIter;
	};
}

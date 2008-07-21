#pragma once

#include "ltnode.h"
#include <vector>
#include <map>

namespace YAML
{
	class Node;

	struct IterPriv
	{
		IterPriv();
		IterPriv(std::vector <Node *>::const_iterator it);
		IterPriv(std::map <Node *, Node *, ltnode>::const_iterator it);

		enum ITER_TYPE { IT_NONE, IT_SEQ, IT_MAP };
		ITER_TYPE type;

		std::vector <Node *>::const_iterator seqIter;
		std::map <Node *, Node *, ltnode>::const_iterator mapIter;
	};
}

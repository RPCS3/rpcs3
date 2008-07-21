#include "crt.h"
#include "iterpriv.h"

namespace YAML
{
	IterPriv::IterPriv(): type(IT_NONE)
	{
	}

	IterPriv::IterPriv(std::vector <Node *>::const_iterator it): seqIter(it), type(IT_SEQ)
	{
	}

	IterPriv::IterPriv(std::map <Node *, Node *, ltnode>::const_iterator it): mapIter(it), type(IT_MAP)
	{
	}
}

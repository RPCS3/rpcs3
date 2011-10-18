#include "yaml-cpp/null.h"

#ifdef YAML_CPP_OLD_API
#include "yaml-cpp/old-api/node.h"
#endif

namespace YAML
{
	_Null Null;

#ifdef YAML_CPP_OLD_API
	bool IsNull(const Node& node)
	{
		return node.Read(Null);
	}
#endif
}

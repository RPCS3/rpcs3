#include "yaml-cpp/value/detail/node_data.h"

namespace YAML
{
	namespace detail
	{
		node_data::node_data(const std::string& scalar): m_type(ValueType::Scalar), m_scalar(scalar)
		{
		}
	}
}

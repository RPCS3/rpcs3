#ifndef VALUE_ITERATOR_H_62B23520_7C8E_11DE_8A39_0800200C9A66
#define VALUE_ITERATOR_H_62B23520_7C8E_11DE_8A39_0800200C9A66

#if defined(_MSC_VER) || (defined(__GNUC__) && (__GNUC__ == 3 && __GNUC_MINOR__ >= 4) || (__GNUC__ >= 4)) // GCC supports "pragma once" correctly since 3.4
#pragma once
#endif


#include "yaml-cpp/dll.h"
#include "yaml-cpp/value/value.h"
#include "yaml-cpp/value/detail/iterator_fwd.h"
#include "yaml-cpp/value/detail/iterator.h"
#include <list>
#include <utility>
#include <vector>

namespace YAML
{
	namespace detail {
		struct iterator_value: public Value, std::pair<Value, Value> {
			iterator_value() {}
			explicit iterator_value(const Value& rhs): Value(rhs) {}
			explicit iterator_value(const Value& key, const Value& value): std::pair<Value, Value>(key, value) {}
		};
	}
}

#endif // VALUE_ITERATOR_H_62B23520_7C8E_11DE_8A39_0800200C9A66

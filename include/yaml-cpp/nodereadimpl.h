#ifndef NODEREADIMPL_H_62B23520_7C8E_11DE_8A39_0800200C9A66
#define NODEREADIMPL_H_62B23520_7C8E_11DE_8A39_0800200C9A66

#if defined(_MSC_VER) || (defined(__GNUC__) && (__GNUC__ == 3 && __GNUC_MINOR__ >= 4) || (__GNUC__ >= 4)) // GCC supports "pragma once" correctly since 3.4
#pragma once
#endif


namespace YAML
{
	// implementation for Node::Read
	// (the goal is to call ConvertScalar if we can, and fall back to operator >> if not)
	// thanks to litb from stackoverflow.com
	// http://stackoverflow.com/questions/1386183/how-to-call-a-templated-function-if-it-exists-and-something-else-otherwise/1386390#1386390

	// Note: this doesn't work on gcc 3.2, but does on gcc 3.4 and above. I'm not sure about 3.3.
	
#if __GNUC__ && (__GNUC__ < 3 || (__GNUC__ == 3 && __GNUC_MINOR__ <= 3))
	// trick doesn't work? Just fall back to ConvertScalar.
	// This means that we can't use any user-defined types as keys in a map
	template <typename T>
	inline bool Node::Read(T& value) const {
		return ConvertScalar(*this, value);
	}
#else
	// usual case: the trick!
	template<bool>
	struct read_impl;
	
	// ConvertScalar available
	template<>
	struct read_impl<true> {
		template<typename T>
		static bool read(const Node& node, T& value) {
			return ConvertScalar(node, value);
		}
	};

	// ConvertScalar not available
	template<>
	struct read_impl<false> {
		template<typename T>
		static bool read(const Node& node, T& value) {
			try {
				node >> value;
			} catch(const Exception&) {
				return false;
			}
			return true;
		}
	};
	
	namespace fallback {
		// sizeof > 1
		struct flag { char c[2]; };
		flag Convert(...);
		
		int operator,(flag, flag);
		
		template<typename T>
		char operator,(flag, T const&);
		
		char operator,(int, flag);
		int operator,(char, flag);
	}

	template <typename T>
	inline bool Node::Read(T& value) const {
		using namespace fallback;

		return read_impl<sizeof (fallback::flag(), Convert(std::string(), value), fallback::flag()) != 1>::read(*this, value);
	}
#endif // done with trick
	
	// the main conversion function
	template <typename T>
	inline bool ConvertScalar(const Node& node, T& value) {
		std::string scalar;
		if(!node.GetScalar(scalar))
			return false;
		
		return Convert(scalar, value);
	}
}

#endif // NODEREADIMPL_H_62B23520_7C8E_11DE_8A39_0800200C9A66

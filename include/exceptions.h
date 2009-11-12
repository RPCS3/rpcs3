#pragma once

#ifndef EXCEPTIONS_H_62B23520_7C8E_11DE_8A39_0800200C9A66
#define EXCEPTIONS_H_62B23520_7C8E_11DE_8A39_0800200C9A66


#include "mark.h"
#include "traits.h"
#include <exception>
#include <string>
#include <sstream>

namespace YAML
{
	// error messages
	namespace ErrorMsg
	{
		const std::string YAML_DIRECTIVE_ARGS    = "YAML directives must have exactly one argument";
		const std::string YAML_VERSION           = "bad YAML version: ";
		const std::string YAML_MAJOR_VERSION     = "YAML major version too large";
		const std::string REPEATED_YAML_DIRECTIVE= "repeated YAML directive";
		const std::string TAG_DIRECTIVE_ARGS     = "TAG directives must have exactly two arguments";
		const std::string REPEATED_TAG_DIRECTIVE = "repeated TAG directive";
		const std::string CHAR_IN_TAG_HANDLE     = "illegal character found while scanning tag handle";
		const std::string TAG_WITH_NO_SUFFIX     = "tag handle with no suffix";
		const std::string END_OF_VERBATIM_TAG    = "end of verbatim tag not found";
		const std::string END_OF_MAP             = "end of map not found";
		const std::string END_OF_MAP_FLOW        = "end of map flow not found";
		const std::string END_OF_SEQ             = "end of sequence not found";
		const std::string END_OF_SEQ_FLOW        = "end of sequence flow not found";
		const std::string MULTIPLE_TAGS          = "cannot assign multiple tags to the same node";
		const std::string MULTIPLE_ANCHORS       = "cannot assign multiple anchors to the same node";
		const std::string MULTIPLE_ALIASES       = "cannot assign multiple aliases to the same node";
		const std::string ALIAS_CONTENT          = "aliases can't have any content, *including* tags";
		const std::string INVALID_HEX            = "bad character found while scanning hex number";
		const std::string INVALID_UNICODE        = "invalid unicode: ";
		const std::string INVALID_ESCAPE         = "unknown escape character: ";
		const std::string UNKNOWN_TOKEN          = "unknown token";
		const std::string DOC_IN_SCALAR          = "illegal document indicator in scalar";
		const std::string EOF_IN_SCALAR          = "illegal EOF in scalar";
		const std::string CHAR_IN_SCALAR         = "illegal character in scalar";
		const std::string TAB_IN_INDENTATION     = "illegal tab when looking for indentation";
		const std::string FLOW_END               = "illegal flow end";
		const std::string BLOCK_ENTRY            = "illegal block entry";
		const std::string MAP_KEY                = "illegal map key";
		const std::string MAP_VALUE              = "illegal map value";
		const std::string ALIAS_NOT_FOUND        = "alias not found after *";
		const std::string ANCHOR_NOT_FOUND       = "anchor not found after &";
		const std::string CHAR_IN_ALIAS          = "illegal character found while scanning alias";
		const std::string CHAR_IN_ANCHOR         = "illegal character found while scanning anchor";
		const std::string ZERO_INDENT_IN_BLOCK   = "cannot set zero indentation for a block scalar";
		const std::string CHAR_IN_BLOCK          = "unexpected character in block scalar";
		const std::string AMBIGUOUS_ANCHOR     = "cannot assign the same alias to multiple nodes";
		const std::string UNKNOWN_ANCHOR       = "the referenced anchor is not defined";

		const std::string INVALID_SCALAR         = "invalid scalar";
		const std::string KEY_NOT_FOUND          = "key not found";
		const std::string BAD_DEREFERENCE        = "bad dereference";
		
		const std::string UNMATCHED_GROUP_TAG    = "unmatched group tag";
		const std::string UNEXPECTED_END_SEQ     = "unexpected end sequence token";
		const std::string UNEXPECTED_END_MAP     = "unexpected end map token";
		const std::string SINGLE_QUOTED_CHAR     = "invalid character in single-quoted string";
		const std::string INVALID_ANCHOR         = "invalid anchor";
		const std::string INVALID_ALIAS          = "invalid alias";
		const std::string INVALID_TAG            = "invalid tag";
		const std::string EXPECTED_KEY_TOKEN     = "expected key token";
		const std::string EXPECTED_VALUE_TOKEN   = "expected value token";
		const std::string UNEXPECTED_KEY_TOKEN   = "unexpected key token";
		const std::string UNEXPECTED_VALUE_TOKEN = "unexpected value token";

		template <typename T>
		inline const std::string KEY_NOT_FOUND_WITH_KEY(const T&, typename disable_if<is_numeric<T> >::type * = 0) {
			return KEY_NOT_FOUND;
		}

		inline const std::string KEY_NOT_FOUND_WITH_KEY(const std::string& key) {
			return KEY_NOT_FOUND + ": " + key;
		}
		
		template <typename T>
		inline const std::string KEY_NOT_FOUND_WITH_KEY(const T& key, typename enable_if<is_numeric<T> >::type * = 0) {
			std::stringstream stream;
			stream << KEY_NOT_FOUND << ": " << key;
			return stream.str();
		}
	}

	class Exception: public std::exception {
	public:
		Exception(const Mark& mark_, const std::string& msg_)
			: mark(mark_), msg(msg_) {
				std::stringstream output;
				output << "yaml-cpp: error at line " << mark.line+1 << ", column " << mark.column+1 << ": " << msg;
				what_ = output.str();
			}
		virtual ~Exception() throw() {}
		virtual const char *what() const throw() { return what_.c_str(); }

		Mark mark;
		std::string msg;
		
	private:
		std::string what_;
	};

	class ParserException: public Exception {
	public:
		ParserException(const Mark& mark_, const std::string& msg_)
			: Exception(mark_, msg_) {}
	};

	class RepresentationException: public Exception {
	public:
		RepresentationException(const Mark& mark_, const std::string& msg_)
			: Exception(mark_, msg_) {}
	};

	// representation exceptions
	class InvalidScalar: public RepresentationException {
	public:
		InvalidScalar(const Mark& mark_)
			: RepresentationException(mark_, ErrorMsg::INVALID_SCALAR) {}
	};

	class KeyNotFound: public RepresentationException {
	public:
		template <typename T>
		KeyNotFound(const Mark& mark_, const T& key_)
			: RepresentationException(mark_, ErrorMsg::KEY_NOT_FOUND_WITH_KEY(key_)) {}
	};
	
	template <typename T>
	class TypedKeyNotFound: public KeyNotFound {
	public:
		TypedKeyNotFound(const Mark& mark_, const T& key_)
			: KeyNotFound(mark_, key_), key(key_) {}
		virtual ~TypedKeyNotFound() throw() {}

		T key;
	};

	template <typename T>
	inline TypedKeyNotFound <T> MakeTypedKeyNotFound(const Mark& mark, const T& key) {
		return TypedKeyNotFound <T> (mark, key);
	}

	class BadDereference: public RepresentationException {
	public:
		BadDereference()
		: RepresentationException(Mark::null(), ErrorMsg::BAD_DEREFERENCE) {}
	};
	
	class EmitterException: public Exception {
	public:
		EmitterException(const std::string& msg_)
		: Exception(Mark::null(), msg_) {}
	};
}

#endif // EXCEPTIONS_H_62B23520_7C8E_11DE_8A39_0800200C9A66

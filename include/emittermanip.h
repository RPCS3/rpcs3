#pragma once

#ifndef EMITTERMANIP_H_62B23520_7C8E_11DE_8A39_0800200C9A66
#define EMITTERMANIP_H_62B23520_7C8E_11DE_8A39_0800200C9A66


#include <string>

namespace YAML
{
	enum EMITTER_MANIP {
		// general manipulators
		Auto,

		// output character set
		EmitNonAscii,
		EscapeNonAscii,
		
		// string manipulators
		// Auto, // duplicate
		SingleQuoted,
		DoubleQuoted,
		Literal,
		
		// bool manipulators
		YesNoBool,  // yes, no
		TrueFalseBool,  // true, false
		OnOffBool,  // on, off
		UpperCase,  // TRUE, N
		LowerCase,  // f, yes
		CamelCase,  // No, Off
		LongBool,  // yes, On
		ShortBool,  // y, t
		
		// int manipulators
		Dec,
		Hex,
		Oct,
		
		// sequence manipulators
		BeginSeq,
		EndSeq,
		Flow,
		Block,
		
		// map manipulators
		BeginMap,
		EndMap,
		Key,
		Value,
		// Flow, // duplicate
		// Block, // duplicate
		// Auto, // duplicate
		LongKey
	};
	
	struct _Indent {
		_Indent(int value_): value(value_) {}
		int value;
	};
	
	inline _Indent Indent(int value) {
		return _Indent(value);
	}
	
	struct _Alias {
		_Alias(const std::string& content_): content(content_) {}
		std::string content;
	};
	
	inline _Alias Alias(const std::string content) {
		return _Alias(content);
	}
	
	struct _Anchor {
		_Anchor(const std::string& content_): content(content_) {}
		std::string content;
	};

	inline _Anchor Anchor(const std::string content) {
		return _Anchor(content);
	}
	
	struct _Tag {
		_Tag(const std::string& content_): content(content_), verbatim(true) {}
		std::string content;
		bool verbatim;
	};
	
	inline _Tag VerbatimTag(const std::string& content) {
		return _Tag(content);
	}

	struct _Comment {
		_Comment(const std::string& content_): content(content_) {}
		std::string content;
	};
	
	inline _Comment Comment(const std::string content) {
		return _Comment(content);
	}
}

#endif // EMITTERMANIP_H_62B23520_7C8E_11DE_8A39_0800200C9A66

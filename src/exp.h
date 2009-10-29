#pragma once

#ifndef EXP_H_62B23520_7C8E_11DE_8A39_0800200C9A66
#define EXP_H_62B23520_7C8E_11DE_8A39_0800200C9A66


#include "regex.h"
#include <string>
#include <ios>
#include "stream.h"

namespace YAML
{
	////////////////////////////////////////////////////////////////////////////////
	// Here we store a bunch of expressions for matching different parts of the file.

	namespace Exp
	{
		// misc
		const RegEx Space = RegEx(' ');
		const RegEx Tab = RegEx('\t');
		const RegEx Blank = Space || Tab;
		const RegEx Break = RegEx('\n') || RegEx("\r\n");
		const RegEx BlankOrBreak = Blank || Break;
		const RegEx Digit = RegEx('0', '9');
		const RegEx Alpha = RegEx('a', 'z') || RegEx('A', 'Z');
		const RegEx AlphaNumeric = Alpha || Digit;
		const RegEx Word = AlphaNumeric || RegEx('-');
		const RegEx Hex = Digit || RegEx('A', 'F') || RegEx('a', 'f');
		// Valid Unicode code points that are not part of c-printable (YAML 1.2, sec. 5.1)
		const RegEx NotPrintable = RegEx(0) || 
			RegEx("\x01\x02\x03\x04\x05\x06\x07\x08\x0B\x0C\x7F", REGEX_OR) || 
			RegEx(0x0E, 0x1F) ||
			(RegEx('\xC2') + (RegEx('\x80', '\x84') || RegEx('\x86', '\x9F')));
		const RegEx Utf8_ByteOrderMark = RegEx("\xEF\xBB\xBF");

		// actual tags

		const RegEx DocStart = RegEx("---") + (BlankOrBreak || RegEx());
		const RegEx DocEnd = RegEx("...") + (BlankOrBreak || RegEx());
		const RegEx DocIndicator = DocStart || DocEnd;
		const RegEx BlockEntry = RegEx('-') + (BlankOrBreak || RegEx());
		const RegEx Key = RegEx('?'),
		            KeyInFlow = RegEx('?') + BlankOrBreak;
		const RegEx Value = RegEx(':') + (BlankOrBreak || RegEx()),
		            ValueInFlow = RegEx(':') + (BlankOrBreak || RegEx(",}", REGEX_OR));
		const RegEx Comment = RegEx('#');
		const RegEx AnchorEnd = RegEx("?:,]}%@`", REGEX_OR) || BlankOrBreak;
		const RegEx URI = Word || RegEx("#;/?:@&=+$,_.!~*'()[]", REGEX_OR) || (RegEx('%') + Hex + Hex);
		const RegEx Tag = Word || RegEx("#;/?:@&=+$_.~*'", REGEX_OR) || (RegEx('%') + Hex + Hex);

		// Plain scalar rules:
		// . Cannot start with a blank.
		// . Can never start with any of , [ ] { } # & * ! | > \' \" % @ `
		// . In the block context - ? : must be not be followed with a space.
		// . In the flow context ? is illegal and : and - must not be followed with a space.
		const RegEx PlainScalar = !(BlankOrBreak || RegEx(",[]{}#&*!|>\'\"%@`", REGEX_OR) || (RegEx("-?:", REGEX_OR) + Blank)),
		            PlainScalarInFlow = !(BlankOrBreak || RegEx("?,[]{}#&*!|>\'\"%@`", REGEX_OR) || (RegEx("-:", REGEX_OR) + Blank));
		const RegEx EndScalar = RegEx(':') + (BlankOrBreak || RegEx()),
		            EndScalarInFlow = (RegEx(':') + (BlankOrBreak || RegEx(",]}", REGEX_OR))) || RegEx(",?[]{}", REGEX_OR);

		const RegEx EscSingleQuote = RegEx("\'\'");
		const RegEx EscBreak = RegEx('\\') + Break;

		const RegEx ChompIndicator = RegEx("+-", REGEX_OR);
		const RegEx Chomp = (ChompIndicator + Digit) || (Digit + ChompIndicator) || ChompIndicator || Digit;

		// and some functions
		std::string Escape(Stream& in);
	}

	namespace Keys
	{
		const char Directive = '%';
		const char FlowSeqStart = '[';
		const char FlowSeqEnd = ']';
		const char FlowMapStart = '{';
		const char FlowMapEnd = '}';
		const char FlowEntry = ',';
		const char Alias = '*';
		const char Anchor = '&';
		const char Tag = '!';
		const char LiteralScalar = '|';
		const char FoldedScalar = '>';
		const char VerbatimTagStart = '<';
		const char VerbatimTagEnd = '>';
	}
}

#endif // EXP_H_62B23520_7C8E_11DE_8A39_0800200C9A66

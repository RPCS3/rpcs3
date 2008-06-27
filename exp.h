#pragma once

#include "regex.h"

namespace YAML
{
	////////////////////////////////////////////////////////////////////////////////
	// Here we store a bunch of expressions for matching different parts of the file.

	namespace Exp
	{
		// misc
		const RegEx Blank = RegEx(' ') || RegEx('\t');
		const RegEx Break = RegEx('\n');
		const RegEx BlankOrBreak = Blank || Break;

		// actual tags

		const RegEx DocStart = RegEx("---") + (BlankOrBreak || RegEx(EOF) || RegEx());
		const RegEx DocEnd = RegEx("...") + (BlankOrBreak || RegEx(EOF) || RegEx());
		const RegEx BlockEntry = RegEx('-') + (BlankOrBreak || RegEx(EOF));
		const RegEx Key = RegEx('?'),
		            KeyInFlow = RegEx('?') + BlankOrBreak;
		const RegEx Value = RegEx(':'),
		            ValueInFlow = RegEx(':') + BlankOrBreak;
		const RegEx Comment = RegEx('#');

		// Plain scalar rules:
		// . Cannot start with a blank.
		// . Can never start with any of , [ ] { } # & * ! | > \' \" % @ `
		// . In the block context - ? : must be not be followed with a space.
		// . In the flow context ? : are illegal and - must not be followed with a space.
		const RegEx PlainScalar = !(BlankOrBreak || RegEx(",[]{}#&*!|>\'\"%@`", REGEX_OR) || (RegEx("-?:") + Blank)),
	                PlainScalarInFlow = !(BlankOrBreak || RegEx("?:,[]{}#&*!|>\'\"%@`", REGEX_OR) || (RegEx('-') + Blank));
		const RegEx IllegalColonInScalar = RegEx(':') + !BlankOrBreak;
		const RegEx EndScalar = RegEx(':') + BlankOrBreak,
		            EndScalarInFlow = (RegEx(':') + BlankOrBreak) || RegEx(",:?[]{}");
	}

	namespace Keys
	{
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
	}
}

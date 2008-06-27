#pragma once

#include <ios>
#include <string>
#include <queue>
#include <stack>
#include <set>
#include "regex.h"

namespace YAML
{
	class Token;

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

	class Scanner
	{
	public:
		Scanner(std::istream& in);
		~Scanner();

		void ScanNextToken();
		void ScanToNextToken();
		void PushIndentTo(int column, bool sequence);
		void PopIndentTo(int column);

		void Scan();

	private:
		char GetChar();
		void Eat(int n = 1);
		std::string Peek(int n);

		void EatLineBreak();

		bool IsWhitespaceToBeEaten(char ch);
		bool IsDocumentStart();
		bool IsDocumentEnd();
		bool IsBlockEntry();
		bool IsKey();
		bool IsValue();
		bool IsPlainScalar();

		template <typename T> void ScanAndEnqueue(T *pToken);
		template <typename T> T *ScanToken(T *pToken);

	private:
		// the stream
		std::istream& INPUT;
		int m_column;

		// the output (tokens)
		std::queue <Token *> m_tokens;
		std::set <Token *> m_limboTokens;

		// state info
		bool m_startedStream, m_endedStream;
		bool m_simpleKeyAllowed;
		int m_flowLevel;                // number of unclosed '[' and '{' indicators
		std::stack <int> m_indents;
	};
}

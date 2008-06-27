#pragma once

#include <ios>
#include <string>
#include <queue>
#include <stack>
#include <set>

namespace YAML
{
	class Token;

	namespace Keys
	{
		const char Comment = '#';
		const char FlowSeqStart = '[';
		const char FlowSeqEnd = ']';
		const char FlowMapStart = '{';
		const char FlowMapEnd = '}';
		const char FlowEntry = ',';
		const char BlockEntry = '-';
		const char Key = '?';
		const char Value = ':';
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
		bool IsLineBreak(char ch);
		bool IsBlank(char ch);
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

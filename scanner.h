#pragma once

#include <ios>
#include <string>
#include <queue>
#include <stack>
#include <set>

namespace YAML
{
	struct Token;

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
		std::string GetChar(int n);
		void Eat(int n = 1);
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

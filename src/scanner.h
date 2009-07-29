#pragma once

#ifndef SCANNER_H_62B23520_7C8E_11DE_8A39_0800200C9A66
#define SCANNER_H_62B23520_7C8E_11DE_8A39_0800200C9A66


#include <ios>
#include <string>
#include <queue>
#include <stack>
#include <set>
#include <map>
#include "stream.h"
#include "token.h"

namespace YAML
{
	class Node;

	class Scanner
	{
	public:
		Scanner(std::istream& in);
		~Scanner();

		// token queue management (hopefully this looks kinda stl-ish)
		bool empty();
		void pop();
		Token& peek();

		// anchor management
		void Save(const std::string& anchor, Node* value);
		const Node *Retrieve(const std::string& anchor) const;
		void ClearAnchors();

	private:
		// scanning
		void EnsureTokensInQueue();
		void ScanNextToken();
		void ScanToNextToken();
		void StartStream();
		void EndStream();
		Token *PushIndentTo(int column, bool sequence);
		void PopIndentTo(int column);

		// checking input
		void InsertSimpleKey();
		bool VerifySimpleKey(bool force = false);
		void VerifyAllSimpleKeys();
		void ThrowParserException(const std::string& msg) const;

		bool IsWhitespaceToBeEaten(char ch);

		struct SimpleKey {
			SimpleKey(const Mark& mark_, int flowLevel_);

			void Validate();
			void Invalidate();
			
			Mark mark;
			int flowLevel;
			Token *pMapStart, *pKey;
		};

		// and the tokens
		void ScanDirective();
		void ScanDocStart();
		void ScanDocEnd();
		void ScanBlockSeqStart();
		void ScanBlockMapSTart();
		void ScanBlockEnd();
		void ScanBlockEntry();
		void ScanFlowStart();
		void ScanFlowEnd();
		void ScanFlowEntry();
		void ScanKey();
		void ScanValue();
		void ScanAnchorOrAlias();
		void ScanTag();
		void ScanPlainScalar();
		void ScanQuotedScalar();
		void ScanBlockScalar();

	private:
		// the stream
		Stream INPUT;

		// the output (tokens)
		std::queue <Token> m_tokens;

		// state info
		bool m_startedStream, m_endedStream;
		bool m_simpleKeyAllowed;
		int m_flowLevel;                // number of unclosed '[' and '{' indicators
		bool m_isLastKeyValid;
		std::stack <SimpleKey> m_simpleKeys;
		std::stack <int> m_indents;
		std::map <std::string, const Node *> m_anchors;
	};
}

#endif // SCANNER_H_62B23520_7C8E_11DE_8A39_0800200C9A66

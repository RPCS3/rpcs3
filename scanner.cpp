#include "scanner.h"
#include "token.h"

namespace YAML
{
	Scanner::Scanner(std::istream& in)
		: INPUT(in), m_startedStream(false), m_simpleKeyAllowed(false), m_flowLevel(0), m_column(0)
	{
	}

	Scanner::~Scanner()
	{
	}

	///////////////////////////////////////////////////////////////////////
	// Misc. helpers

	// GetChar
	// . Extracts a character from the stream and updates our position
	char Scanner::GetChar()
	{
		m_column++;
		return INPUT.get();
	}

	// GetLineBreak
	// . Eats with no checking
	void Scanner::EatLineBreak()
	{
		m_column = 0;
		INPUT.get();
	}

	// EatDocumentStart
	// . Eats with no checking
	void Scanner::EatDocumentStart()
	{
		INPUT.get();
		INPUT.get();
		INPUT.get();
	}

	// EatDocumentEnd
	// . Eats with no checking
	void Scanner::EatDocumentEnd()
	{
		INPUT.get();
		INPUT.get();
		INPUT.get();
	}

	// IsWhitespaceToBeEaten
	// . We can eat whitespace if:
	//   1. It's a space
	//   2. It's a tab, and we're either:
	//      a. In the flow context
	//      b. In the block context but not where a simple key could be allowed
	//         (i.e., not at the beginning of a line, or following '-', '?', or ':')
	bool Scanner::IsWhitespaceToBeEaten()
	{
		char ch = INPUT.peek();

		if(ch == ' ')
			return true;

		if(ch == '\t' && (m_flowLevel >= 0 || !m_simpleKeyAllowed))
			return true;

		return false;
	}

	// IsLineBreak
	bool Scanner::IsLineBreak()
	{
		char ch = INPUT.peek();
		return ch == '\n'; // TODO: More types of line breaks
	}

	// IsBlank
	bool Scanner::IsBlank()
	{
		char ch = INPUT.peek();
		return IsLineBreak() || ch == ' ' || ch == '\t' || ch == EOF;
	}

	// IsDocumentStart
	bool Scanner::IsDocumentStart()
	{
		// needs to be at the start of a new line
		if(m_column != 0)
			return false;

		// then needs '---'
		for(int i=0;i<3;i++) {
			if(INPUT.peek() != '-') {
				// first put 'em back
				for(int j=0;j<i;j++)
					INPUT.putback('-');

				// and return
				return false;
			}
			INPUT.get();
		}

		// then needs a blank character (or eof)
		if(!IsBlank()) {
			// put 'em back
			for(int i=0;i<3;i++)
				INPUT.putback('-');

			// and return
			return false;
		}

		// finally, put 'em back and go
		for(int i=0;i<3;i++)
			INPUT.putback('-');

		return true;
	}

	// IsDocumentEnd
	bool Scanner::IsDocumentEnd()
	{
		// needs to be at the start of a new line
		if(m_column != 0)
			return false;

		// then needs '...'
		for(int i=0;i<3;i++) {
			if(INPUT.peek() != '.') {
				// first put 'em back
				for(int j=0;j<i;j++)
					INPUT.putback('.');

				// and return
				return false;
			}
			INPUT.get();
		}

		// then needs a blank character (or eof)
		if(!IsBlank()) {
			// put 'em back
			for(int i=0;i<3;i++)
				INPUT.putback('.');

			// and return
			return false;
		}

		// finally, put 'em back and go
		for(int i=0;i<3;i++)
			INPUT.putback('-');

		return true;
	}

	///////////////////////////////////////////////////////////////////////
	// Specialization for scanning specific tokens

	// StreamStartToken
	template <> StreamStartToken *Scanner::ScanToken(StreamStartToken *pToken)
	{
		m_startedStream = true;
		m_simpleKeyAllowed = true;
		m_indents.push(-1);

		return pToken;
	}

	// StreamEndToken
	template <> StreamEndToken *Scanner::ScanToken(StreamEndToken *pToken)
	{
		// force newline
		if(m_column > 0)
			m_column = 0;

		// TODO: unroll indentation
		// TODO: "reset simple keys"

		m_simpleKeyAllowed = false;

		return pToken;
	}

	// DocumentStartToken
	template <> DocumentStartToken *Scanner::ScanToken(DocumentStartToken *pToken)
	{
		// TODO: unroll indentation
		// TODO: reset simple keys

		m_simpleKeyAllowed = false;

		// eat it
		EatDocumentStart();

		return pToken;
	}

	// DocumentEndToken
	template <> DocumentEndToken *Scanner::ScanToken(DocumentEndToken *pToken)
	{
		// TODO: unroll indentation
		// TODO: reset simple keys

		m_simpleKeyAllowed = false;

		// eat it
		EatDocumentEnd();

		return pToken;
	}

	///////////////////////////////////////////////////////////////////////
	// The main scanning function

	Token *Scanner::ScanNextToken()
	{
		if(!m_startedStream)
			return ScanToken(new StreamStartToken);

		ScanToNextToken();
		// TODO: remove "obsolete potential simple keys"
		// TODO: unroll indent

		if(INPUT.peek() == EOF)
			return ScanToken(new StreamEndToken);

		if(IsDocumentStart())
			return ScanToken(new DocumentStartToken);

		if(IsDocumentEnd())
			return ScanToken(new DocumentEndToken);

		return 0;
	}

	// ScanToNextToken
	// . Eats input until we reach the next token-like thing.
	void Scanner::ScanToNextToken()
	{
		while(1) {
			// first eat whitespace
			while(IsWhitespaceToBeEaten())
				INPUT.get();

			// then eat a comment
			if(INPUT.peek() == Keys::Comment) {
				// eat until line break
				while(INPUT && !IsLineBreak())
					INPUT.get();
			}

			// if it's NOT a line break, then we're done!
			if(!IsLineBreak())
				break;

			// otherwise, let's eat the line break and keep going
			EatLineBreak();

			// new line - we may be able to accept a simple key now
			if(m_flowLevel == 0)
				m_simpleKeyAllowed = true;
        }
	}

	// temporary function for testing
	void Scanner::Scan()
	{
		while(Token *pToken = ScanNextToken())
			delete pToken;
	}
}

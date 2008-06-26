#include "scanner.h"
#include "token.h"
#include "exceptions.h"
#include <iostream>

namespace YAML
{
	Scanner::Scanner(std::istream& in)
		: INPUT(in), m_startedStream(false), m_simpleKeyAllowed(false), m_flowLevel(0), m_column(0)
	{
	}

	Scanner::~Scanner()
	{
		while(!m_tokens.empty()) {
			delete m_tokens.front();
			m_tokens.pop();
		}

		// delete limbo tokens (they're here for RAII)
		for(std::set <Token *>::const_iterator it=m_limboTokens.begin();it!=m_limboTokens.end();++it)
			delete *it;
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

	// Eat
	// . Eats 'n' characters and updates our position.
	void Scanner::Eat(int n)
	{
		for(int i=0;i<n;i++) {
			m_column++;
			char ch = INPUT.get();
			if(ch == '\n')
				m_column = 0;
		}
	}

	// Peek
	// . Peeks at the next 'n' characters and returns them in a string.
	std::string Scanner::Peek(int n)
	{
		std::string ret;

		// extract n - 1 characters, and peek at the nth
		for(int i=0;i<n-1;i++)
			ret += INPUT.get();
		ret += INPUT.peek();

		// and put back the n - 1 characters we STOLE
		for(int i=n-2;i>=0;i--)
			INPUT.putback(ret[i]);

		return ret;
	}

	// GetLineBreak
	// . Eats with no checking
	void Scanner::EatLineBreak()
	{
		Eat(1);
		m_column = 0;
	}

	// IsWhitespaceToBeEaten
	// . We can eat whitespace if:
	//   1. It's a space
	//   2. It's a tab, and we're either:
	//      a. In the flow context
	//      b. In the block context but not where a simple key could be allowed
	//         (i.e., not at the beginning of a line, or following '-', '?', or ':')
	bool Scanner::IsWhitespaceToBeEaten(char ch)
	{
		if(ch == ' ')
			return true;

		if(ch == '\t' && (m_flowLevel >= 0 || !m_simpleKeyAllowed))
			return true;

		return false;
	}

	// IsLineBreak
	bool Scanner::IsLineBreak(char ch)
	{
		return ch == '\n'; // TODO: More types of line breaks
	}

	// IsBlank
	bool Scanner::IsBlank(char ch)
	{
		return IsLineBreak(ch) || ch == ' ' || ch == '\t' || ch == EOF;
	}

	// IsDocumentStart
	bool Scanner::IsDocumentStart()
	{
		// needs to be at the start of a new line
		if(m_column != 0)
			return false;

		std::string next = Peek(4);
		return next[0] == '-' && next[1] == '-' && next[2] == '-' && IsBlank(next[3]);
	}

	// IsDocumentEnd
	bool Scanner::IsDocumentEnd()
	{
		// needs to be at the start of a new line
		if(m_column != 0)
			return false;

		std::string next = Peek(4);
		return next[0] == '.' && next[1] == '.' && next[2] == '.' && IsBlank(next[3]);
	}

	// IsBlockEntry
	bool Scanner::IsBlockEntry()
	{
		std::string next = Peek(2);
		return next[0] == Keys::BlockEntry && IsBlank(next[1]);
	}

	// IsKey
	bool Scanner::IsKey()
	{
		std::string next = Peek(2);
		return next[0] == Keys::Key && (IsBlank(next[1]) || m_flowLevel > 0);
	}

	// IsValue
	bool Scanner::IsValue()
	{
		std::string next = Peek(2);
		return next[0] == Keys::Value && (IsBlank(next[1]) || m_flowLevel > 0);
	}

	// IsPlainScalar
	// . Rules:
	//   . Cannot start with a blank.
	//   . Can never start with any of , [ ] { } # & * ! | > \' \" % @ `
	//   . In the block context - ? : must be not be followed with a space.
	//   . In the flow context ? : are illegal and - must not be followed with a space.
	bool Scanner::IsPlainScalar()
	{
		std::string next = Peek(2);

		if(IsBlank(next[0]))
			return false;

		// never characters
		if(std::string(",[]{}#&*!|>\'\"%@`").find(next[0]) != std::string::npos)
			return false;

		// specific block/flow characters
		if(m_flowLevel == 0) {
			if((next[0] == '-' || next[0] == '?' || next[0] == ':') && IsBlank(next[1]))
				return false;
		} else {
			if(next[0] == '?' || next[0]  == ':')
				return false;

			if(next[0] == '-' && IsBlank(next[1]))
				return false;
		}

		return true;
	}

	///////////////////////////////////////////////////////////////////////
	// Specialization for scanning specific tokens

	// ScanAndEnqueue
	// . Scans the token, then pushes it in the queue.
	// . Note: we also use a set of "limbo tokens", i.e., tokens
	//   that haven't yet been pushed. This way, if ScanToken()
	//   throws an exception, we'll be keeping track of 'pToken'
	//   somewhere, and it will be automatically cleaned up when
	//   the Scanner destructs.
	template <typename T> void Scanner::ScanAndEnqueue(T *pToken)
	{
		m_limboTokens.insert(pToken);
		m_tokens.push(ScanToken(pToken));
		m_limboTokens.erase(pToken);
	}

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

		PopIndentTo(-1);
		// TODO: "reset simple keys"

		m_simpleKeyAllowed = false;

		return pToken;
	}

	// DocumentStartToken
	template <> DocumentStartToken *Scanner::ScanToken(DocumentStartToken *pToken)
	{
		PopIndentTo(m_column);
		// TODO: "reset simple keys"

		m_simpleKeyAllowed = false;

		// eat
		Eat(3);

		return pToken;
	}

	// DocumentEndToken
	template <> DocumentEndToken *Scanner::ScanToken(DocumentEndToken *pToken)
	{
		PopIndentTo(m_column);
		// TODO: "reset simple keys"

		m_simpleKeyAllowed = false;

		// eat
		Eat(3);

		return pToken;
	}

	// FlowSeqStartToken
	template <> FlowSeqStartToken *Scanner::ScanToken(FlowSeqStartToken *pToken)
	{
		// TODO: "save simple key"
		// TODO: increase flow level

		m_simpleKeyAllowed = true;

		// eat it
		INPUT.get();

		return pToken;
	}

	// FlowMapStartToken
	template <> FlowMapStartToken *Scanner::ScanToken(FlowMapStartToken *pToken)
	{
		// TODO: "save simple key"
		// TODO: increase flow level

		m_simpleKeyAllowed = true;

		// eat it
		INPUT.get();

		return pToken;
	}

	// FlowSeqEndToken
	template <> FlowSeqEndToken *Scanner::ScanToken(FlowSeqEndToken *pToken)
	{
		// TODO: "remove simple key"
		// TODO: decrease flow level

		m_simpleKeyAllowed = false;

		// eat it
		INPUT.get();

		return pToken;
	}

	// FlowMapEndToken
	template <> FlowMapEndToken *Scanner::ScanToken(FlowMapEndToken *pToken)
	{
		// TODO: "remove simple key"
		// TODO: decrease flow level

		m_simpleKeyAllowed = false;

		// eat it
		INPUT.get();

		return pToken;
	}

	// FlowEntryToken
	template <> FlowEntryToken *Scanner::ScanToken(FlowEntryToken *pToken)
	{
		// TODO: "remove simple key"

		m_simpleKeyAllowed = true;

		// eat it
		INPUT.get();

		return pToken;
	}

	// BlockEntryToken
	template <> BlockEntryToken *Scanner::ScanToken(BlockEntryToken *pToken)
	{
		// we better be in the block context!
		if(m_flowLevel == 0) {
			// can we put it here?
			if(!m_simpleKeyAllowed)
				throw IllegalBlockEntry();

			PushIndentTo(m_column, true);	// , -1
		} else {
			// TODO: throw?
		}

		// TODO: "remove simple key"

		m_simpleKeyAllowed = true;

		// eat
		Eat(1);
		return pToken;
	}

	// KeyToken
	template <> KeyToken *Scanner::ScanToken(KeyToken *pToken)
	{
		// are we in block context?
		if(m_flowLevel == 0) {
			if(!m_simpleKeyAllowed)
				throw IllegalMapKey();

			PushIndentTo(m_column, false);
		}

		// TODO: "remove simple key"

		// can only put a simple key here if we're in block context
		if(m_flowLevel == 0)
			m_simpleKeyAllowed = true;
		else
			m_simpleKeyAllowed = false;

		// eat
		Eat(1);
		return pToken;
	}

	// ValueToken
	template <> ValueToken *Scanner::ScanToken(ValueToken *pToken)
	{
		// TODO: Is it a simple key?
		if(false) {
		} else {
			// If not, ...
			// are we in block context?
			if(m_flowLevel == 0) {
				if(!m_simpleKeyAllowed)
					throw IllegalMapValue();

				PushIndentTo(m_column, false);
			}
		}

		// can only put a simple key here if we're in block context
		if(m_flowLevel == 0)
			m_simpleKeyAllowed = true;
		else
			m_simpleKeyAllowed = false;

		// eat
		Eat(1);
		return pToken;
	}

	// PlainScalarToken
	template <> PlainScalarToken *Scanner::ScanToken(PlainScalarToken *pToken)
	{
		// TODO: "save simple key"

		m_simpleKeyAllowed = false;

		// now eat and store the scalar
		std::string scalar;
		bool leadingBlanks = true;

		while(INPUT) {
			// doc start/end tokens
			if(IsDocumentStart() || IsDocumentEnd())
				break;

			// comment
			if(INPUT.peek() == Keys::Comment)
				break;

			// first eat non-blanks
			while(INPUT && !IsBlank(INPUT.peek())) {
				std::string next = Peek(2);

				// illegal colon in flow context
				if(m_flowLevel > 0 && next[0] == ':') {
					if(!IsBlank(next[1]))
						throw IllegalScalar();
				}

				// characters that might end the scalar
				if(next[0] == ':' && IsBlank(next[1]))
					break;
				if(m_flowLevel > 0 && std::string(",:?[]{}").find(next[0]) != std::string::npos)
					break;

				scalar += GetChar();
			}

			// now eat blanks
			while(IsBlank(INPUT.peek()) /* || IsBreak(INPUT.peek()) */) {
				if(IsBlank(INPUT.peek())) {
					if(leadingBlanks && m_column <= m_indents.top())
						throw IllegalTabInScalar();

					// TODO: Store some blanks?
					Eat(1);
				} else {
					Eat(1);
				}
			}

			// TODO: join whitespace

			// and finally break if we're below the indentation level
			if(m_flowLevel == 0 && m_column <= m_indents.top())
				break;
		}

		// now modify our token
		if(leadingBlanks)
			m_simpleKeyAllowed = true;

		return pToken;
	}

	///////////////////////////////////////////////////////////////////////
	// The main scanning function

	void Scanner::ScanNextToken()
	{
		if(!m_startedStream)
			return ScanAndEnqueue(new StreamStartToken);

		ScanToNextToken();
		// TODO: remove "obsolete potential simple keys"
		PopIndentTo(m_column);

		if(INPUT.peek() == EOF)
			return ScanAndEnqueue(new StreamEndToken);

		// are we at a document token?
		if(IsDocumentStart())
			return ScanAndEnqueue(new DocumentStartToken);

		if(IsDocumentEnd())
			return ScanAndEnqueue(new DocumentEndToken);

		// are we at a flow start/end/entry?
		if(INPUT.peek() == Keys::FlowSeqStart)
			return ScanAndEnqueue(new FlowSeqStartToken);

		if(INPUT.peek() == Keys::FlowSeqEnd)
			return ScanAndEnqueue(new FlowSeqEndToken);
		
		if(INPUT.peek() == Keys::FlowMapStart)
			return ScanAndEnqueue(new FlowMapStartToken);
		
		if(INPUT.peek() == Keys::FlowMapEnd)
			return ScanAndEnqueue(new FlowMapEndToken);

		if(INPUT.peek() == Keys::FlowEntry)
			return ScanAndEnqueue(new FlowEntryToken);

		// block/map stuff?
		if(IsBlockEntry())
			return ScanAndEnqueue(new BlockEntryToken);

		if(IsKey())
			return ScanAndEnqueue(new KeyToken);

		if(IsValue())
			return ScanAndEnqueue(new ValueToken);

		// TODO: alias/anchor/tag

		// TODO: special scalars
		if(INPUT.peek() == Keys::LiteralScalar && m_flowLevel == 0)
			return;

		if(INPUT.peek() == Keys::FoldedScalar && m_flowLevel == 0)
			return;

		if(INPUT.peek() == '\'')
			return;

		if(INPUT.peek() == '\"')
			return;

		// plain scalars
		if(IsPlainScalar())
			return ScanAndEnqueue(new PlainScalarToken);

		// don't know what it is!
		throw UnknownToken();
	}

	// ScanToNextToken
	// . Eats input until we reach the next token-like thing.
	void Scanner::ScanToNextToken()
	{
		while(1) {
			// first eat whitespace
			while(IsWhitespaceToBeEaten(INPUT.peek()))
				Eat(1);

			// then eat a comment
			if(INPUT.peek() == Keys::Comment) {
				// eat until line break
				while(INPUT && !IsLineBreak(INPUT.peek()))
					Eat(1);
			}

			// if it's NOT a line break, then we're done!
			if(!IsLineBreak(INPUT.peek()))
				break;

			// otherwise, let's eat the line break and keep going
			EatLineBreak();

			// new line - we may be able to accept a simple key now
			if(m_flowLevel == 0)
				m_simpleKeyAllowed = true;
        }
	}

	// PushIndentTo
	// . Pushes an indentation onto the stack, and enqueues the
	//   proper token (sequence start or mapping start).
	void Scanner::PushIndentTo(int column, bool sequence)
	{
		// are we in flow?
		if(m_flowLevel > 0)
			return;

		// is this actually an indentation?
		if(column <= m_indents.top())
			return;

		// now push
		m_indents.push(column);
		if(sequence)
			m_tokens.push(new BlockSeqStartToken);
		else
			m_tokens.push(new BlockMapStartToken);
	}

	// PopIndentTo
	// . Pops indentations off the stack until we reach 'column' indentation,
	//   and enqueues the proper token each time.
	void Scanner::PopIndentTo(int column)
	{
		// are we in flow?
		if(m_flowLevel > 0)
			return;

		// now pop away
		while(!m_indents.empty() && m_indents.top() > column) {
			m_indents.pop();
			m_tokens.push(new BlockEndToken);
		}
	}

	// temporary function for testing
	void Scanner::Scan()
	{
		while(INPUT) {
			ScanNextToken();

			while(!m_tokens.empty()) {
				Token *pToken = m_tokens.front();
				m_tokens.pop();
				std::cout << typeid(*pToken).name() << std::endl;
				delete pToken;
			}
		}
	}
}

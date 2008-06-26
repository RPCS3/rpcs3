#include "scanner.h"
#include "token.h"
#include "exceptions.h"

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

	// IsBlockEntry
	bool Scanner::IsBlockEntry()
	{
		if(INPUT.peek() != Keys::BlockEntry)
			return false;

		INPUT.get();

		// then needs a blank character (or eof)
		if(!IsBlank()) {
			INPUT.putback(Keys::BlockEntry);
			return false;
		}

		INPUT.putback(Keys::BlockEntry);
		return true;
	}

	// IsKey
	bool Scanner::IsKey()
	{
		if(INPUT.peek() != Keys::Key)
			return false;

		INPUT.get();

		// then needs a blank character (or eof), if we're in block context
		if(m_flowLevel == 0 && !IsBlank()) {
			INPUT.putback(Keys::BlockEntry);
			return false;
		}

		INPUT.putback(Keys::BlockEntry);
		return true;
	}

	// IsValue
	bool Scanner::IsValue()
	{
		if(INPUT.peek() != Keys::Value)
			return false;

		INPUT.get();

		// then needs a blank character (or eof), if we're in block context
		if(m_flowLevel == 0 && !IsBlank()) {
			INPUT.putback(Keys::BlockEntry);
			return false;
		}

		INPUT.putback(Keys::BlockEntry);
		return true;
	}

	// IsPlainScalar
	// . Rules:
	//   . Cannot start with a blank.
	//   . Can never start with any of , [ ] { } # & * ! | > \' \" % @ `
	//   . In the block context - ? : must be not be followed with a space.
	//   . In the flow context ? : are illegal and - must not be followed with a space.
	bool Scanner::IsPlainScalar()
	{
		if(IsBlank())
			return false;

		// never characters
		std::string never = ",[]{}#&*!|>\'\"%@`";
		for(unsigned i=0;i<never.size();i++)
			if(INPUT.peek() == never[i])
				return false;

		// specific block/flow characters
		if(m_flowLevel == 0) {
			if(INPUT.peek() == '-' || INPUT.peek() == '?' || INPUT.peek() == ':') {
				char ch = INPUT.get();
				if(IsBlank()) {
					INPUT.putback(ch);
					return false;
				}
			}
		} else {
			if(INPUT.peek() == '?' || INPUT.peek() == ':')
				return false;
			if(INPUT.peek() == '-') {
				INPUT.get();
				if(IsBlank()) {
					INPUT.putback('-');
					return false;
				}
			}
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

		// eat it
		EatDocumentStart();

		return pToken;
	}

	// DocumentEndToken
	template <> DocumentEndToken *Scanner::ScanToken(DocumentEndToken *pToken)
	{
		PopIndentTo(m_column);
		// TODO: "reset simple keys"

		m_simpleKeyAllowed = false;

		// eat it
		EatDocumentEnd();

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
		INPUT.get();
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
		INPUT.get();
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
		INPUT.get();
		return pToken;
	}

	// PlainScalarToken
	template <> PlainScalarToken *Scanner::ScanToken(PlainScalarToken *pToken)
	{
		// TODO: "save simple key"

		m_simpleKeyAllowed = false;

		// now eat and store the scalar
		while(1) {
			// doc start/end tokens
			if(IsDocumentStart() || IsDocumentEnd())
				break;

			// comment
			if(INPUT.peek() == Keys::Comment)
				break;

			// first eat non-blanks
			while(!IsBlank()) {
				// illegal colon in flow context
				if(m_flowLevel > 0 && INPUT.peek() == ':') {
					INPUT.get();
					if(!IsBlank()) {
						INPUT.putback(':');
						throw IllegalScalar();
					}
					INPUT.putback(':');
				}

				// characters that might end the scalar
				// TODO: scanner.c line 3434
			}
		}

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
		while(INPUT)
			ScanNextToken();
	}
}

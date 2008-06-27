#include "scanner.h"
#include "token.h"
#include "exceptions.h"
#include "exp.h"
#include <iostream>

namespace YAML
{
	Scanner::Scanner(std::istream& in)
		: INPUT(in), m_startedStream(false), m_endedStream(false), m_simpleKeyAllowed(false), m_flowLevel(0), m_column(0)
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
		char ch = INPUT.get();
		if(ch == '\n')
			m_column = 0;
		return ch;
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

	// IsDocumentStart
	bool Scanner::IsDocumentStart()
	{
		// needs to be at the start of a new line
		if(m_column != 0)
			return false;

		return Exp::DocStart.Matches(INPUT);
	}

	// IsDocumentEnd
	bool Scanner::IsDocumentEnd()
	{
		// needs to be at the start of a new line
		if(m_column != 0)
			return false;

		return Exp::DocEnd.Matches(INPUT);
	}

	// IsBlockEntry
	bool Scanner::IsBlockEntry()
	{
		return Exp::BlockEntry.Matches(INPUT);
	}

	// IsKey
	bool Scanner::IsKey()
	{
		if(m_flowLevel > 0)
			return Exp::KeyInFlow.Matches(INPUT);
		return Exp::Key.Matches(INPUT);
	}

	// IsValue
	bool Scanner::IsValue()
	{
		if(m_flowLevel > 0)
			return Exp::ValueInFlow.Matches(INPUT);
		return Exp::Value.Matches(INPUT);
	}

	// IsPlainScalar
	bool Scanner::IsPlainScalar()
	{
		if(m_flowLevel > 0)
			return Exp::PlainScalarInFlow.Matches(INPUT);
		return Exp::PlainScalar.Matches(INPUT);
	}

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

	///////////////////////////////////////////////////////////////////////
	// The main scanning function

	void Scanner::ScanNextToken()
	{
		if(m_endedStream)
			return;

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
			if(Exp::Comment.Matches(INPUT.peek())) {
				// eat until line break
				while(INPUT && !Exp::Break.Matches(INPUT.peek()))
					Eat(1);
			}

			// if it's NOT a line break, then we're done!
			if(!Exp::Break.Matches(INPUT.peek()))
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
		while(1) {
			ScanNextToken();
			if(m_tokens.empty())
				break;

			while(!m_tokens.empty()) {
				Token *pToken = m_tokens.front();
				m_tokens.pop();
				std::cout << typeid(*pToken).name() << std::endl;
				delete pToken;
			}
		}
	}
}

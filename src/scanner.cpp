#include "crt.h"
#include "scanner.h"
#include "token.h"
#include "exceptions.h"
#include "exp.h"

namespace YAML
{
	Scanner::Scanner(std::istream& in)
		: INPUT(in), m_startedStream(false), m_endedStream(false), m_simpleKeyAllowed(false), m_flowLevel(0)
	{
	}

	Scanner::~Scanner()
	{
	}

	// IsEmpty
	// . Returns true if there are no more tokens to be read
	bool Scanner::IsEmpty()
	{
		PeekToken();  // to ensure that there are tokens in the queue, if possible
		return m_tokens.empty();
	}

	// PopToken
	// . Simply removes the next token on the queue.
	void Scanner::PopToken()
	{
		PeekToken();  // to ensure that there are tokens in the queue
		if(!m_tokens.empty())
			m_tokens.pop();
	}

	// PeekToken
	// . Returns (but does not remove) the next token on the queue, and scans if only we need to.
	Token& Scanner::PeekToken()
	{
		while(1) {
			if(!m_tokens.empty()) {
				Token& token = m_tokens.front();

				// return this guy if it's valid
				if(token.status == TS_VALID)
					return token;

				// here's where we clean up the impossible tokens
				if(token.status == TS_INVALID) {
					m_tokens.pop();
					continue;
				}

				// note: what's left are the unverified tokens
			}

			// no token? maybe we've actually finished
			if(m_endedStream)
				break;

			// no? then scan...
			ScanNextToken();
		}

		// TODO: find something to return here, or assert (but can't do that! maybe split into two functions?)
	}

	// ScanNextToken
	// . The main scanning function; here we branch out and
	//   scan whatever the next token should be.
	void Scanner::ScanNextToken()
	{
		if(m_endedStream)
			return;

		if(!m_startedStream)
			return StartStream();

		// get rid of whitespace, etc. (in between tokens it should be irrelevent)
		ScanToNextToken();

		// check the latest simple key
		VerifySimpleKey();

		// maybe need to end some blocks
		PopIndentTo(INPUT.column);

		// *****
		// And now branch based on the next few characters!
		// *****

		// end of stream
		if(INPUT.peek() == EOF)
			return EndStream();

		if(INPUT.column == 0 && INPUT.peek() == Keys::Directive)
			return ScanDirective();

		// document token
		if(INPUT.column == 0 && Exp::DocStart.Matches(INPUT))
			return ScanDocStart();

		if(INPUT.column == 0 && Exp::DocEnd.Matches(INPUT))
			return ScanDocEnd();

		// flow start/end/entry
		if(INPUT.peek() == Keys::FlowSeqStart || INPUT.peek() == Keys::FlowMapStart)
			return ScanFlowStart();

		if(INPUT.peek() == Keys::FlowSeqEnd || INPUT.peek() == Keys::FlowMapEnd)
			return ScanFlowEnd();
	
		if(INPUT.peek() == Keys::FlowEntry)
			return ScanFlowEntry();

		// block/map stuff
		if(Exp::BlockEntry.Matches(INPUT))
			return ScanBlockEntry();

		if((m_flowLevel == 0 ? Exp::Key : Exp::KeyInFlow).Matches(INPUT))
			return ScanKey();

		if((m_flowLevel == 0 ? Exp::Value : Exp::ValueInFlow).Matches(INPUT))
			return ScanValue();

		// alias/anchor
		if(INPUT.peek() == Keys::Alias || INPUT.peek() == Keys::Anchor)
			return ScanAnchorOrAlias();

		// tag
		if(INPUT.peek() == Keys::Tag)
			return ScanTag();

		// special scalars
		if(m_flowLevel == 0 && (INPUT.peek() == Keys::LiteralScalar || INPUT.peek() == Keys::FoldedScalar))
			return ScanBlockScalar();

		if(INPUT.peek() == '\'' || INPUT.peek() == '\"')
			return ScanQuotedScalar();

		// plain scalars
		if((m_flowLevel == 0 ? Exp::PlainScalar : Exp::PlainScalarInFlow).Matches(INPUT))
			return ScanPlainScalar();

		// don't know what it is!
		throw ParserException(INPUT.line, INPUT.column, ErrorMsg::UNKNOWN_TOKEN);
	}

	// ScanToNextToken
	// . Eats input until we reach the next token-like thing.
	void Scanner::ScanToNextToken()
	{
		while(1) {
			// first eat whitespace
			while(IsWhitespaceToBeEaten(INPUT.peek()))
				INPUT.eat(1);

			// then eat a comment
			if(Exp::Comment.Matches(INPUT)) {
				// eat until line break
				while(INPUT && !Exp::Break.Matches(INPUT))
					INPUT.eat(1);
			}

			// if it's NOT a line break, then we're done!
			if(!Exp::Break.Matches(INPUT))
				break;

			// otherwise, let's eat the line break and keep going
			int n = Exp::Break.Match(INPUT);
			INPUT.eat(n);

			// oh yeah, and let's get rid of that simple key
			VerifySimpleKey();

			// new line - we may be able to accept a simple key now
			if(m_flowLevel == 0)
				m_simpleKeyAllowed = true;
        }
	}

	///////////////////////////////////////////////////////////////////////
	// Misc. helpers

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

	// StartStream
	// . Set the initial conditions for starting a stream.
	void Scanner::StartStream()
	{
		m_startedStream = true;
		m_simpleKeyAllowed = true;
		m_indents.push(-1);
	}

	// EndStream
	// . Close out the stream, finish up, etc.
	void Scanner::EndStream()
	{
		// force newline
		if(INPUT.column > 0)
			INPUT.column = 0;

		PopIndentTo(-1);
		VerifyAllSimpleKeys();

		m_simpleKeyAllowed = false;
		m_endedStream = true;
	}

	// PushIndentTo
	// . Pushes an indentation onto the stack, and enqueues the
	//   proper token (sequence start or mapping start).
	// . Returns the token it generates (if any).
	Token *Scanner::PushIndentTo(int column, bool sequence)
	{
		// are we in flow?
		if(m_flowLevel > 0)
			return 0;

		// is this actually an indentation?
		if(column <= m_indents.top())
			return 0;

		// now push
		m_indents.push(column);
		if(sequence)
			m_tokens.push(Token(TT_BLOCK_SEQ_START, INPUT.line, INPUT.column));
		else
			m_tokens.push(Token(TT_BLOCK_MAP_START, INPUT.line, INPUT.column));

		return &m_tokens.back();
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
			m_tokens.push(Token(TT_BLOCK_END, INPUT.line, INPUT.column));
		}
	}
}

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
		while(!m_tokens.empty()) {
			delete m_tokens.front();
			m_tokens.pop();
		}

		// delete limbo tokens (they're here for RAII)
		for(std::set <Token *>::const_iterator it=m_limboTokens.begin();it!=m_limboTokens.end();++it)
			delete *it;
	}

	// GetNextToken
	// . Removes and returns the next token on the queue.
	Token *Scanner::GetNextToken()
	{
		Token *pToken = PeekNextToken();
		if(!m_tokens.empty())
			m_tokens.pop();
		return pToken;
	}

	// PeekNextToken
	// . Returns (but does not remove) the next token on the queue, and scans if only we need to.
	Token *Scanner::PeekNextToken()
	{
		while(1) {
			Token *pToken = 0;

			// is there a token in the queue?
			if(!m_tokens.empty())
				pToken = m_tokens.front();

			// (here's where we clean up the impossible tokens)
			if(pToken && pToken->status == TS_INVALID) {
				m_tokens.pop();
				delete pToken;
				continue;
			}

			// on unverified tokens, we just have to wait
			if(pToken && pToken->status == TS_UNVERIFIED)
				pToken = 0;

			// then that's what we want
			if(pToken)
				return pToken;

			// no token? maybe we've actually finished
			if(m_endedStream)
				break;

			// no? then scan...
			ScanNextToken();
		}

		return 0;
	}

	// ScanNextToken
	// . The main scanning function; here we branch out and
	//   scan whatever the next token should be.
	void Scanner::ScanNextToken()
	{
		if(m_endedStream)
			return;

		if(!m_startedStream)
			return ScanAndEnqueue(new StreamStartToken);

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
			return ScanAndEnqueue(new StreamEndToken);

		if(INPUT.column == 0 && INPUT.peek() == Keys::Directive)
			return ScanAndEnqueue(new DirectiveToken);

		// document token
		if(INPUT.column == 0 && Exp::DocStart.Matches(INPUT))
			return ScanAndEnqueue(new DocumentStartToken);

		if(INPUT.column == 0 && Exp::DocEnd.Matches(INPUT))
			return ScanAndEnqueue(new DocumentEndToken);

		// flow start/end/entry
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

		// block/map stuff
		if(Exp::BlockEntry.Matches(INPUT))
			return ScanAndEnqueue(new BlockEntryToken);

		if((m_flowLevel == 0 ? Exp::Key : Exp::KeyInFlow).Matches(INPUT))
			return ScanAndEnqueue(new KeyToken);

		if((m_flowLevel == 0 ? Exp::Value : Exp::ValueInFlow).Matches(INPUT))
			return ScanAndEnqueue(new ValueToken);

		// alias/anchor
		if(INPUT.peek() == Keys::Alias || INPUT.peek() == Keys::Anchor)
			return ScanAndEnqueue(new AnchorToken);

		// tag
		if(INPUT.peek() == Keys::Tag)
			return ScanAndEnqueue(new TagToken);

		// special scalars
		if(m_flowLevel == 0 && (INPUT.peek() == Keys::LiteralScalar || INPUT.peek() == Keys::FoldedScalar))
			return ScanAndEnqueue(new BlockScalarToken);

		if(INPUT.peek() == '\'' || INPUT.peek() == '\"')
			return ScanAndEnqueue(new QuotedScalarToken);

		// plain scalars
		if((m_flowLevel == 0 ? Exp::PlainScalar : Exp::PlainScalarInFlow).Matches(INPUT))
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
				INPUT.Eat(1);

			// then eat a comment
			if(Exp::Comment.Matches(INPUT)) {
				// eat until line break
				while(INPUT && !Exp::Break.Matches(INPUT))
					INPUT.Eat(1);
			}

			// if it's NOT a line break, then we're done!
			if(!Exp::Break.Matches(INPUT))
				break;

			// otherwise, let's eat the line break and keep going
			int n = Exp::Break.Match(INPUT);
			INPUT.Eat(n);

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
			m_tokens.push(new BlockSeqStartToken);
		else
			m_tokens.push(new BlockMapStartToken);

		return m_tokens.front();
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
}

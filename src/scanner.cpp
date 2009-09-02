#include "crt.h"
#include "scanner.h"
#include "token.h"
#include "exceptions.h"
#include "exp.h"
#include <cassert>

namespace YAML
{
	Scanner::Scanner(std::istream& in)
		: INPUT(in), m_startedStream(false), m_endedStream(false), m_simpleKeyAllowed(false), m_flowLevel(0)
	{
	}

	Scanner::~Scanner()
	{
	}

	// empty
	// . Returns true if there are no more tokens to be read
	bool Scanner::empty()
	{
		EnsureTokensInQueue();
		return m_tokens.empty();
	}

	// pop
	// . Simply removes the next token on the queue.
	void Scanner::pop()
	{
		EnsureTokensInQueue();
		if(!m_tokens.empty()) {
			// Saved anchors shouldn't survive popping the document end marker
			if (m_tokens.front().type == TT_DOC_END) {
				ClearAnchors();
			}
			m_tokens.pop();
		}
	}

	// peek
	// . Returns (but does not remove) the next token on the queue.
	Token& Scanner::peek()
	{
		EnsureTokensInQueue();
		assert(!m_tokens.empty());  // should we be asserting here? I mean, we really just be checking
		                            // if it's empty before peeking.

//		std::cerr << "peek: (" << &m_tokens.front() << ") " << m_tokens.front() << "\n";
		return m_tokens.front();
	}

	// EnsureTokensInQueue
	// . Scan until there's a valid token at the front of the queue,
	//   or we're sure the queue is empty.
	void Scanner::EnsureTokensInQueue()
	{
		while(1) {
			if(!m_tokens.empty()) {
				Token& token = m_tokens.front();

				// if this guy's valid, then we're done
				if(token.status == TS_VALID)
					return;

				// here's where we clean up the impossible tokens
				if(token.status == TS_INVALID) {
					m_tokens.pop();
					continue;
				}

				// note: what's left are the unverified tokens
			}

			// no token? maybe we've actually finished
			if(m_endedStream)
				return;

			// no? then scan...
			ScanNextToken();
		}
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
		PopIndentToHere();

		// *****
		// And now branch based on the next few characters!
		// *****
		
		// end of stream
		if(!INPUT)
			return EndStream();

		if(INPUT.column() == 0 && INPUT.peek() == Keys::Directive)
			return ScanDirective();

		// document token
		if(INPUT.column() == 0 && Exp::DocStart.Matches(INPUT))
			return ScanDocStart();

		if(INPUT.column() == 0 && Exp::DocEnd.Matches(INPUT))
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
		throw ParserException(INPUT.mark(), ErrorMsg::UNKNOWN_TOKEN);
	}

	// ScanToNextToken
	// . Eats input until we reach the next token-like thing.
	void Scanner::ScanToNextToken()
	{
		while(1) {
			// first eat whitespace
			while(INPUT && IsWhitespaceToBeEaten(INPUT.peek()))
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
		m_indents.push(IndentMarker(-1, IndentMarker::NONE));
		m_anchors.clear();
	}

	// EndStream
	// . Close out the stream, finish up, etc.
	void Scanner::EndStream()
	{
		// force newline
		if(INPUT.column() > 0)
			INPUT.ResetColumn();

		PopAllIndents();
		VerifyAllSimpleKeys();

		m_simpleKeyAllowed = false;
		m_endedStream = true;
	}

	// PushIndentTo
	// . Pushes an indentation onto the stack, and enqueues the
	//   proper token (sequence start or mapping start).
	// . Returns the token it generates (if any).
	Token *Scanner::PushIndentTo(int column, IndentMarker::INDENT_TYPE type)
	{
		// are we in flow?
		if(m_flowLevel > 0)
			return 0;
		
		IndentMarker indent(column, type);
		const IndentMarker& lastIndent = m_indents.top();

		// is this actually an indentation?
		if(indent.column < lastIndent.column)
			return 0;
		if(indent.column == lastIndent.column && !(indent.type == IndentMarker::SEQ && lastIndent.type == IndentMarker::MAP))
			return 0;

		// now push
		m_indents.push(indent);
		if(type == IndentMarker::SEQ)
			m_tokens.push(Token(TT_BLOCK_SEQ_START, INPUT.mark()));
		else if(type == IndentMarker::MAP)
			m_tokens.push(Token(TT_BLOCK_MAP_START, INPUT.mark()));
		else
			assert(false);

		return &m_tokens.back();
	}

	// PopIndentToHere
	// . Pops indentations off the stack until we reach the current indentation level,
	//   and enqueues the proper token each time.
	void Scanner::PopIndentToHere()
	{
		// are we in flow?
		if(m_flowLevel > 0)
			return;

		// now pop away
		while(!m_indents.empty()) {
			const IndentMarker& indent = m_indents.top();
			if(indent.column < INPUT.column())
				break;
			if(indent.column == INPUT.column() && !(indent.type == IndentMarker::SEQ && !Exp::BlockEntry.Matches(INPUT)))
				break;
				
			PopIndent();
		}
	}
	
	// PopAllIndents
	// . Pops all indentations (except for the base empty one) off the stack,
	//   and enqueues the proper token each time.
	void Scanner::PopAllIndents()
	{
		// are we in flow?
		if(m_flowLevel > 0)
			return;

		// now pop away
		while(!m_indents.empty()) {
			const IndentMarker& indent = m_indents.top();
			if(indent.type == IndentMarker::NONE)
				break;
			
			PopIndent();
		}
	}
	
	// PopIndent
	// . Pops a single indent, pushing the proper token
	void Scanner::PopIndent()
	{
		IndentMarker::INDENT_TYPE type = m_indents.top().type;
		m_indents.pop();
		if(type == IndentMarker::SEQ)
			m_tokens.push(Token(TT_BLOCK_SEQ_END, INPUT.mark()));
		else if(type == IndentMarker::MAP)
			m_tokens.push(Token(TT_BLOCK_MAP_END, INPUT.mark()));
	}

	// GetTopIndent
	int Scanner::GetTopIndent() const
	{
		if(m_indents.empty())
			return 0;
		return m_indents.top().column;
	}

	// Save
	// . Saves a pointer to the Node object referenced by a particular anchor
	//   name.
	void Scanner::Save(const std::string& anchor, Node* value)
	{
		m_anchors[anchor] = value;
	}

	// Retrieve
	// . Retrieves a pointer previously saved for an anchor name.
	// . Throws an exception if the anchor has not been defined.
	const Node *Scanner::Retrieve(const std::string& anchor) const
	{
		typedef std::map<std::string, const Node *> map;

		map::const_iterator itNode = m_anchors.find(anchor);

		if(m_anchors.end() == itNode)
			ThrowParserException(ErrorMsg::UNKNOWN_ANCHOR);

		return itNode->second;
	}

	// ThrowParserException
	// . Throws a ParserException with the current token location
	//   (if available).
	// . Does not parse any more tokens.
	void Scanner::ThrowParserException(const std::string& msg) const
	{
		Mark mark = Mark::null();
		if(!m_tokens.empty()) {
			const Token& token = m_tokens.front();
			mark = token.mark;
		}
		throw ParserException(mark, msg);
	}

	void Scanner::ClearAnchors()
	{
		m_anchors.clear();
	}
}

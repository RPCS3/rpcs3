#include "parser.h"
#include "node.h"

namespace YAML
{
	Parser::Parser(std::istream& in): INPUT(in), m_ok(true)
	{
		m_state.push(State(C_BLOCK, -1, true));

		// read header
		std::string token = ReadNextToken();
		if(token != DocStart)
			m_ok = false;
	}

	Parser::~Parser()
	{
	}

	Parser::operator bool() const
	{
		return m_ok;
	}

	bool Parser::operator !() const
	{
		return !m_ok;
	}

	bool Parser::IsWhitespace(char ch)
	{
		return (ch == ' ' || ch == '\n' || ch == '\r' || ch == '\t');
	}

	void Parser::Putback(const std::string& str)
	{
		for(int i=str.size()-1;i>=0;i--)
			INPUT.putback(str[i]);
	}

	// StringWhitespace
	// . Strips up to n whitespace characters (or as many
	//   as there are, if n is -1)
	void Parser::StripWhitespace(int n)
	{
		while(--n >= 0 && IsWhitespace(INPUT.peek()))
			INPUT.get();
	}

	int Parser::GetNumOfSpaces()
	{
		// get 'em out
		int n = 0;
		while(INPUT.peek() == ' ') {
			INPUT.get();
			n++;
		}

		// put 'em back
		for(int i=0;i<n;i++)
			INPUT.putback(' ');

		return n;
	}

	// SeqContinues
	// . Returns true if the next item to be read is a continuation of the current sequence.
	bool Parser::SeqContinues()
	{
		const State& state = m_state.top();
		if(state.context != C_BLOCK)
			return false;

		int n = GetNumOfSpaces();
		return (n == state.indent);
	}

	// ReadNextNode
	// . Reads and returns the next node from the current input (to be used recursively).
	// . The caller is responsible for cleanup!
	Node *Parser::ReadNextNode()
	{
		if(!INPUT)
			return 0;

		std::string token = ReadNextToken();
		if(token == DocStart || token == DocEnd)
			return 0;	// TODO: putback DocStart?

		Node *pNode = new Node;
		pNode->Read(this, token);
		return pNode;
	}

	// ReadNextToken
	// . Reads:
	//   . If the first character is non-whitespace, non-special token, then until
	//     the end of this scalar.
	std::string Parser::ReadNextToken()
	{
		const State& state = m_state.top();

		if(state.startingNewLine) {
			int n = GetNumOfSpaces();
			StripWhitespace(n);
			if(n > state.indent) {
				m_state.push(State(C_BLOCK, n, true));
			};

			while(m_state.top().startingNewLine && n < m_state.top().indent)
				m_state.pop();
		}

		char ch = INPUT.peek();
		if(IsWhitespace(ch))
			return "";	// TODO

		if(ch == SeqToken) {
			// grab token
			INPUT.get();

			// is next token whitespace?
			if(!IsWhitespace(INPUT.peek())) {
				// read entire line
				std::string line;
				std::getline(INPUT, line);
				return ch + line;
			}

			// if so, strip whitespace and go
			StripWhitespace();

			return std::string("") + SeqToken;
		}

		// read until end-of-line
		std::string line;
		std::getline(INPUT, line);
		return line;
	}
}

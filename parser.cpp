#include "parser.h"
#include "scanner.h"
#include "token.h"
#include "exceptions.h"
#include <sstream>

namespace YAML
{
	Parser::Parser(std::istream& in): m_pScanner(0)
	{
		Load(in);
	}

	Parser::~Parser()
	{
		delete m_pScanner;
	}

	Parser::operator bool() const
	{
		return m_pScanner->PeekNextToken() != 0;
	}

	void Parser::Load(std::istream& in)
	{
		delete m_pScanner;
		m_pScanner = new Scanner(in);
		m_state.Reset();
	}

	// GetNextDocument
	// . Reads the next document in the queue (of tokens).
	// . Throws (ParserException|ParserException)s on errors.
	void Parser::GetNextDocument(Node& document)
	{
		// clear node
		document.Clear();

		// first read directives
		ParseDirectives();

		// we better have some tokens in the queue
		if(!m_pScanner->PeekNextToken())
			return;

		// first eat doc start (optional)
		if(m_pScanner->PeekNextToken()->type == TT_DOC_START)
			m_pScanner->EatNextToken();

		// now parse our root node
		document.Parse(m_pScanner, m_state);

		// and finally eat any doc ends we see
		while(m_pScanner->PeekNextToken() && m_pScanner->PeekNextToken()->type == TT_DOC_END)
			m_pScanner->EatNextToken();
	}

	// ParseDirectives
	// . Reads any directives that are next in the queue.
	void Parser::ParseDirectives()
	{
		bool readDirective = false;

		while(1) {
			Token *pToken = m_pScanner->PeekNextToken();
			if(!pToken || pToken->type != TT_DIRECTIVE)
				break;

			// we keep the directives from the last document if none are specified;
			// but if any directives are specific, then we reset them
			if(!readDirective)
				m_state.Reset();

			readDirective = true;
			HandleDirective(pToken);
			m_pScanner->PopNextToken();
		}
	}

	void Parser::HandleDirective(Token *pToken)
	{
		if(pToken->value == "YAML")
			HandleYamlDirective(pToken);
		else if(pToken->value == "TAG")
			HandleTagDirective(pToken);
	}

	// HandleYamlDirective
	// . Should be of the form 'major.minor' (like a version number)
	void Parser::HandleYamlDirective(Token *pToken)
	{
		if(pToken->params.size() != 1)
			throw ParserException(pToken->line, pToken->column, "YAML directives must have exactly one argument");

		std::stringstream str(pToken->params[0]);
		str >> m_state.version.major;
		str.get();
		str >> m_state.version.minor;
		if(!str || str.peek() != EOF)
			throw ParserException(pToken->line, pToken->column, "bad YAML version: " + pToken->params[0]);

		if(m_state.version.major > 1)
			throw ParserException(pToken->line, pToken->column, "YAML major version > 1");

		// TODO: warning on major == 1, minor > 2?
	}

	// HandleTagDirective
	// . Should be of the form 'handle prefix', where 'handle' is converted to 'prefix' in the file.
	void Parser::HandleTagDirective(Token *pToken)
	{
		if(pToken->params.size() != 2)
			throw ParserException(pToken->line, pToken->column, "TAG directives must have exactly two arguments");

		std::string handle = pToken->params[0], prefix = pToken->params[1];
		m_state.tags[handle] = prefix;
	}

	void Parser::PrintTokens(std::ostream& out)
	{
		while(1) {
			Token *pToken = m_pScanner->GetNextToken();
			if(!pToken)
				break;

			out << *pToken << std::endl;
		}
	}
}

#include "crt.h"
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
		return !m_pScanner->empty();
	}

	void Parser::Load(std::istream& in)
	{
		delete m_pScanner;
		m_pScanner = new Scanner(in);
		m_state.Reset();
	}

	// GetNextDocument
	// . Reads the next document in the queue (of tokens).
	// . Throws a ParserException on error.
	void Parser::GetNextDocument(Node& document)
	{
		// clear node
		document.Clear();

		// first read directives
		ParseDirectives();

		// we better have some tokens in the queue
		if(m_pScanner->empty())
			return;

		// first eat doc start (optional)
		if(m_pScanner->peek().type == TT_DOC_START)
			m_pScanner->pop();

		// now parse our root node
		document.Parse(m_pScanner, m_state);

		// and finally eat any doc ends we see
		while(!m_pScanner->empty() && m_pScanner->peek().type == TT_DOC_END)
			m_pScanner->pop();
	}

	// ParseDirectives
	// . Reads any directives that are next in the queue.
	void Parser::ParseDirectives()
	{
		bool readDirective = false;

		while(1) {
			if(m_pScanner->empty())
				break;

			Token& token = m_pScanner->peek();
			if(token.type != TT_DIRECTIVE)
				break;

			// we keep the directives from the last document if none are specified;
			// but if any directives are specific, then we reset them
			if(!readDirective)
				m_state.Reset();

			readDirective = true;
			HandleDirective(&token);
			m_pScanner->pop();
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
			throw ParserException(pToken->line, pToken->column, ErrorMsg::YAML_DIRECTIVE_ARGS);

		std::stringstream str(pToken->params[0]);
		str >> m_state.version.major;
		str.get();
		str >> m_state.version.minor;
		if(!str || str.peek() != EOF)
			throw ParserException(pToken->line, pToken->column, ErrorMsg::YAML_VERSION + pToken->params[0]);

		if(m_state.version.major > 1)
			throw ParserException(pToken->line, pToken->column, ErrorMsg::YAML_MAJOR_VERSION);

		// TODO: warning on major == 1, minor > 2?
	}

	// HandleTagDirective
	// . Should be of the form 'handle prefix', where 'handle' is converted to 'prefix' in the file.
	void Parser::HandleTagDirective(Token *pToken)
	{
		if(pToken->params.size() != 2)
			throw ParserException(pToken->line, pToken->column, ErrorMsg::TAG_DIRECTIVE_ARGS);

		std::string handle = pToken->params[0], prefix = pToken->params[1];
		m_state.tags[handle] = prefix;
	}

	void Parser::PrintTokens(std::ostream& out)
	{
		while(1) {
			if(m_pScanner->empty())
				break;

			out << m_pScanner->peek() << std::endl;
			m_pScanner->pop();
		}
	}
}

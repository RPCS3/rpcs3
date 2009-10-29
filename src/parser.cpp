#include "parser.h"
#include "scanner.h"
#include "token.h"
#include "exceptions.h"
#include "parserstate.h"
#include <sstream>
#include <cstdio>

namespace YAML
{
	Parser::Parser()
	{
	}
	
	Parser::Parser(std::istream& in)
	{
		Load(in);
	}

	Parser::~Parser()
	{
	}

	Parser::operator bool() const
	{
		return m_pScanner.get() && !m_pScanner->empty();
	}

	void Parser::Load(std::istream& in)
	{
		m_pScanner.reset(new Scanner(in));
		m_pState.reset(new ParserState);
	}

	// GetNextDocument
	// . Reads the next document in the queue (of tokens).
	// . Throws a ParserException on error.
	bool Parser::GetNextDocument(Node& document)
	{
		if(!m_pScanner.get())
			return false;
		
		// clear node
		document.Clear();

		// first read directives
		ParseDirectives();

		// we better have some tokens in the queue
		if(m_pScanner->empty())
			return false;

		// first eat doc start (optional)
		if(m_pScanner->peek().type == Token::DOC_START)
			m_pScanner->pop();

		// now parse our root node
		document.Parse(m_pScanner.get(), *m_pState);

		// and finally eat any doc ends we see
		while(!m_pScanner->empty() && m_pScanner->peek().type == Token::DOC_END)
			m_pScanner->pop();

		// clear anchors from the scanner, which are no longer relevant
		m_pScanner->ClearAnchors();
		
		return true;
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
			if(token.type != Token::DIRECTIVE)
				break;

			// we keep the directives from the last document if none are specified;
			// but if any directives are specific, then we reset them
			if(!readDirective)
				m_pState.reset(new ParserState);

			readDirective = true;
			HandleDirective(token);
			m_pScanner->pop();
		}
	}

	void Parser::HandleDirective(const Token& token)
	{
		if(token.value == "YAML")
			HandleYamlDirective(token);
		else if(token.value == "TAG")
			HandleTagDirective(token);
	}

	// HandleYamlDirective
	// . Should be of the form 'major.minor' (like a version number)
	void Parser::HandleYamlDirective(const Token& token)
	{
		if(token.params.size() != 1)
			throw ParserException(token.mark, ErrorMsg::YAML_DIRECTIVE_ARGS);
		
		if(!m_pState->version.isDefault)
			throw ParserException(token.mark, ErrorMsg::REPEATED_YAML_DIRECTIVE);

		std::stringstream str(token.params[0]);
		str >> m_pState->version.major;
		str.get();
		str >> m_pState->version.minor;
		if(!str || str.peek() != EOF)
			throw ParserException(token.mark, ErrorMsg::YAML_VERSION + token.params[0]);

		if(m_pState->version.major > 1)
			throw ParserException(token.mark, ErrorMsg::YAML_MAJOR_VERSION);

		m_pState->version.isDefault = false;
		// TODO: warning on major == 1, minor > 2?
	}

	// HandleTagDirective
	// . Should be of the form 'handle prefix', where 'handle' is converted to 'prefix' in the file.
	void Parser::HandleTagDirective(const Token& token)
	{
		if(token.params.size() != 2)
			throw ParserException(token.mark, ErrorMsg::TAG_DIRECTIVE_ARGS);

		const std::string& handle = token.params[0];
		const std::string& prefix = token.params[1];
		if(m_pState->tags.find(handle) != m_pState->tags.end())
			throw ParserException(token.mark, ErrorMsg::REPEATED_TAG_DIRECTIVE);
		
		m_pState->tags[handle] = prefix;
	}

	void Parser::PrintTokens(std::ostream& out)
	{
		if(!m_pScanner.get())
			return;
		
		while(1) {
			if(m_pScanner->empty())
				break;

			out << m_pScanner->peek() << "\n";
			m_pScanner->pop();
		}
	}
}

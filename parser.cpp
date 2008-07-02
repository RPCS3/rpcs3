#include "parser.h"
#include "scanner.h"
#include "token.h"
#include "exceptions.h"
#include <sstream>

namespace YAML
{
	Parser::Parser(std::istream& in): m_pScanner(0)
	{
		m_pScanner = new Scanner(in);
		m_state.Reset();
	}

	Parser::~Parser()
	{
		delete m_pScanner;
	}

	Parser::operator bool() const
	{
		return m_pScanner->PeekNextToken() != 0;
	}

	void Parser::GetNextDocument(Document& document)
	{
		// first read directives
		ParseDirectives();

		// then parse the document
		document.Parse(m_pScanner, m_state);
	}

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
			HandleDirective(pToken->value, pToken->params);
			m_pScanner->PopNextToken();
		}
	}

	void Parser::HandleDirective(const std::string& name, const std::vector <std::string>& params)
	{
		if(name == "YAML")
			HandleYamlDirective(params);
		else if(name == "TAG")
			HandleTagDirective(params);
	}

	// HandleYamlDirective
	// . Should be of the form 'major.minor' (like a version number)
	void Parser::HandleYamlDirective(const std::vector <std::string>& params)
	{
		if(params.size() != 1)
			throw BadYAMLDirective();

		std::stringstream str(params[0]);
		str >> m_state.version.major;
		str.get();
		str >> m_state.version.minor;
		if(!str)
			throw BadYAMLDirective();  // TODO: or throw if there are any more characters in the stream?

		// TODO: throw on major > 1? warning on major == 1, minor > 2?
	}

	void Parser::HandleTagDirective(const std::vector <std::string>& params)
	{
		if(params.size() != 2)
			throw BadTAGDirective();

		std::string handle = params[0], prefix = params[1];
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

#pragma once

#ifndef PUGIXML_HEADER_ONLY
#define PUGIXML_HEADER_ONLY 1
#endif // !PUGIXML_HEADER_ONLY
#include "pugixml.hpp"
#undef PUGIXML_HEADER_ONLY

struct rXmlNode
{
	rXmlNode();
	rXmlNode(const pugi::xml_node &);
	std::shared_ptr<rXmlNode> GetChildren();
	std::shared_ptr<rXmlNode> GetNext();
	std::string GetName();
	std::string GetAttribute( const std::string &name);
	std::string GetNodeContent();

	pugi::xml_node handle;
};

struct rXmlDocument
{
	rXmlDocument();
	rXmlDocument(const rXmlDocument& other) = delete;
	rXmlDocument &operator=(const rXmlDocument& other) = delete;
	void Load(const std::string & path);
	std::shared_ptr<rXmlNode> GetRoot();

	pugi::xml_document handle;
};

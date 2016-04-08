#pragma once

#ifndef PUGIXML_HEADER_ONLY
#define PUGIXML_HEADER_ONLY 1
#endif // !PUGIXML_HEADER_ONLY
#include "pugixml.hpp"
#undef PUGIXML_HEADER_ONLY

struct rXmlNode
{
	rXmlNode();
	rXmlNode(pugi::xml_node *);
	rXmlNode(const rXmlNode& other);
	rXmlNode &operator=(const rXmlNode& other);
	~rXmlNode();
	std::shared_ptr<rXmlNode> GetChildren();
	std::shared_ptr<rXmlNode> GetNext();
	std::string GetName();
	std::string GetAttribute( const std::string &name);
	std::string GetNodeContent();
	void *AsVoidPtr();

	pugi::xml_node *handle;
	bool ownPtr;
};

struct rXmlDocument
{
	rXmlDocument();
	rXmlDocument(const rXmlDocument& other) = delete;
	rXmlDocument &operator=(const rXmlDocument& other) = delete;
	~rXmlDocument();
	void Load(const std::string & path);
	std::shared_ptr<rXmlNode> GetRoot();
	void *AsVoidPtr();

	pugi::xml_document *handle;
};
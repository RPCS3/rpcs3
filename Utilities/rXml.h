#pragma once

#include <pugixml.hpp>
#include <memory>

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
	void Read(const std::string& data);
	std::shared_ptr<rXmlNode> GetRoot();

	pugi::xml_document handle;
};

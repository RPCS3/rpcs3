#pragma once

#ifdef _MSC_VER
#pragma warning(push, 0)
#include <pugixml.hpp>
#pragma warning(pop)
#else
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wall"
#pragma GCC diagnostic ignored "-Wextra"
#pragma GCC diagnostic ignored "-Wold-style-cast"
#pragma GCC diagnostic ignored "-Weffc++"
#include <pugixml.hpp>
#pragma GCC diagnostic pop
#endif

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

	pugi::xml_node handle{};
};

struct rXmlDocument
{
	rXmlDocument();
	rXmlDocument(const rXmlDocument& other) = delete;
	rXmlDocument &operator=(const rXmlDocument& other) = delete;
	void Read(const std::string& data);
	std::shared_ptr<rXmlNode> GetRoot();

	pugi::xml_document handle{};
};

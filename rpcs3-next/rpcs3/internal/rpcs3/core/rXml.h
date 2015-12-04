#pragma once

struct rXmlNode
{
	rXmlNode();
	rXmlNode(void *);
	rXmlNode(const rXmlNode& other);
	rXmlNode &operator=(const rXmlNode& other);
	~rXmlNode();
	std::shared_ptr<rXmlNode> GetChildren();
	std::shared_ptr<rXmlNode> GetNext();
	std::string GetName();
	std::string GetAttribute( const std::string &name);
	std::string GetNodeContent();

	void *handle;
	bool ownPtr;
};

struct rXmlDocument
{
	rXmlDocument();
	rXmlDocument(const rXmlDocument& other);
	rXmlDocument &operator=(const rXmlDocument& other);
	~rXmlDocument();
	void Load(const std::string & path);
	std::shared_ptr<rXmlNode> GetRoot();

	void *handle;
};
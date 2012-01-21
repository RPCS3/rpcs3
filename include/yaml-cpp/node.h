#ifndef NODE_H_62B23520_7C8E_11DE_8A39_0800200C9A66
#define NODE_H_62B23520_7C8E_11DE_8A39_0800200C9A66

#if defined(_MSC_VER) || (defined(__GNUC__) && (__GNUC__ == 3 && __GNUC_MINOR__ >= 4) || (__GNUC__ >= 4)) // GCC supports "pragma once" correctly since 3.4
#pragma once
#endif


#include "yaml-cpp/dll.h"
#include "yaml-cpp/exceptions.h"
#include "yaml-cpp/mark.h"
#include "yaml-cpp/noncopyable.h"
#include "yaml-cpp/conversion.h"
#include "yaml-cpp/iterator.h"
#include "yaml-cpp/ltnode.h"
#include <iostream>
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace YAML
{
	class AliasManager;
	class Content;
	class NodeOwnership;
	class Scanner;
	class Emitter;
	class EventHandler;

	struct NodeType { enum value { Null, Scalar, Sequence, Map }; };

	class YAML_CPP_API Node: private noncopyable
	{
	public:
		friend class NodeOwnership;
		friend class NodeBuilder;
		
		Node();
		~Node();

		void Clear();
		std::auto_ptr<Node> Clone() const;
		void EmitEvents(EventHandler& eventHandler) const;
		void EmitEvents(AliasManager& am, EventHandler& eventHandler) const;
		
		NodeType::value Type() const { return m_type; }
		bool IsAliased() const;

		// file location of start of this node
		const Mark GetMark() const { return m_mark; }

		// accessors
		Iterator begin() const;
		Iterator end() const;
		std::size_t size() const;

		// extraction of scalars
		bool GetScalar(std::string& s) const;

		// we can specialize this for other values
		template <typename T>
		bool Read(T& value) const;

		template <typename T>
		const T to() const;

		template <typename T>
		friend YAML_CPP_API typename enable_if<is_scalar_convertible<T> >::type operator >> (const Node& node, T& value);

		// retrieval for maps and sequences
		template <typename T>
		const Node *FindValue(const T& key) const;

		template <typename T>
		const Node& operator [] (const T& key) const;
		
		// specific to maps
		const Node *FindValue(const char *key) const;
		const Node *FindValue(char *key) const;
		const Node& operator [] (const char *key) const;
		const Node& operator [] (char *key) const;

		// for tags
		const std::string& Tag() const { return m_tag; }

		// emitting
		friend YAML_CPP_API Emitter& operator << (Emitter& out, const Node& node);

		// ordering
		int Compare(const Node& rhs) const;
		friend bool operator < (const Node& n1, const Node& n2);

	private:
		explicit Node(NodeOwnership& owner);
		Node& CreateNode();
		
		void Init(NodeType::value type, const Mark& mark, const std::string& tag);
		
		void MarkAsAliased();
		void SetScalarData(const std::string& data);
		void Append(Node& node);
		void Insert(Node& key, Node& value);

		// helper for sequences
		template <typename, bool> friend struct _FindFromNodeAtIndex;
		const Node *FindAtIndex(std::size_t i) const;
		
		// helper for maps
		template <typename T>
		const Node& GetValue(const T& key) const;

		template <typename T>
		const Node *FindValueForKey(const T& key) const;

	private:
		std::auto_ptr<NodeOwnership> m_pOwnership;

		Mark m_mark;
		std::string m_tag;

		typedef std::vector<Node *> node_seq;
		typedef std::map<Node *, Node *, ltnode> node_map;

		NodeType::value m_type;
		std::string m_scalarData;
		node_seq m_seqData;
		node_map m_mapData;
	};
}

#include "yaml-cpp/nodeimpl.h"
#include "yaml-cpp/nodereadimpl.h"

#endif // NODE_H_62B23520_7C8E_11DE_8A39_0800200C9A66

#pragma once

#ifndef STLNODE_H_62B23520_7C8E_11DE_8A39_0800200C9A66
#define STLNODE_H_62B23520_7C8E_11DE_8A39_0800200C9A66


#include <vector>
#include <map>

namespace YAML
{
	template <typename T>
	void operator >> (const Node& node, std::vector<T>& v)
	{
		v.clear();
		v.resize(node.size());
		for(unsigned i=0;i<node.size();++i)
			node[i] >> v[i];
	}
	
	
	template <typename K, typename V>
	void operator >> (const Node& node, std::map<K, V>& m)
	{
		m.clear();
		for(Iterator it=node.begin();it!=node.end();++it) {
			K k;
			V v;
			it.first() >> k;
			it.second() >> v;
			m[k] = v;
		}
	}
}

#endif // STLNODE_H_62B23520_7C8E_11DE_8A39_0800200C9A66

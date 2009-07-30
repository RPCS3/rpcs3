#pragma once

#ifndef NULL_H_62B23520_7C8E_11DE_8A39_0800200C9A66
#define NULL_H_62B23520_7C8E_11DE_8A39_0800200C9A66


namespace YAML
{
	struct _Null {};
	inline bool operator == (const _Null&, const _Null&) { return true; }
	inline bool operator != (const _Null&, const _Null&) { return false; }
	
	extern _Null Null;
}

#endif // NULL_H_62B23520_7C8E_11DE_8A39_0800200C9A66
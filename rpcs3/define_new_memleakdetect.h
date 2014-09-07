//Override the new operator to use the memory leak detection from visual studio
//Does not work with placement new
#if defined(MSVC_CRT_MEMLEAK_DETECTION) && defined(_DEBUG) && defined(DBG_NEW)
	#pragma push_macro("new")
	#define new DBG_NEW
#endif

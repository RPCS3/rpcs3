//Restore the new operator if previously saved before overriding
//Allow the use of placement new
#if defined(MSVC_CRT_MEMLEAK_DETECTION) && defined(_DEBUG) && defined(DBG_NEW)
	#pragma pop_macro("new")
#endif

#ifndef SPARSEHASH_SPARSECONFIG_H__
#define SPARSEHASH_SPARSECONFIG_H__

// [AIR] : I couldn't make the google "windows" folder concept work.
// This does, and we only care of GCC and MSVC right now anyway.

#if defined( _MSC_VER )

	#define GOOGLE_NAMESPACE  google
	#define HASH_NAMESPACE  stdext
	#define HASH_FUN_H   <hash_map>
	#define SPARSEHASH_HASH  HASH_NAMESPACE::hash_compare
	#undef HAVE_UINT16_T
	#undef HAVE_U_INT16_T
	#define HAVE___UINT16  1
	#define HAVE_LONG_LONG  1
	#define HAVE_SYS_TYPES_H  1
	#undef HAVE_STDINT_H
	#undef HAVE_INTTYPES_H
	#define HAVE_MEMCPY  1
	#define STL_NAMESPACE  std
	#define _END_GOOGLE_NAMESPACE_  }
	#define _START_GOOGLE_NAMESPACE_  namespace GOOGLE_NAMESPACE {

#else    //if defined( GNUC )

	/* Namespace for Google classes */
	#define GOOGLE_NAMESPACE google

	/* the location of <hash_fun.h>/<stl_hash_fun.h> */
	#define HASH_FUN_H <ext/hash_fun.h>

	/* the namespace of hash_map/hash_set */
	#define HASH_NAMESPACE __gnu_cxx

	/* Define to 1 if you have the <inttypes.h> header file. */
	#define HAVE_INTTYPES_H 1

	/* Define to 1 if the system has the type `long long'. */
	#define HAVE_LONG_LONG 1

	/* Define to 1 if you have the `memcpy' function. */
	#define HAVE_MEMCPY 1

	/* Define to 1 if you have the <stdint.h> header file. */
	#define HAVE_STDINT_H 1

	/* Define to 1 if you have the <sys/types.h> header file. */
	#define HAVE_SYS_TYPES_H 1

	/* Define to 1 if the system has the type `uint16_t'. */
	#define HAVE_UINT16_T 1

	/* Define to 1 if the system has the type `u_int16_t'. */
	#define HAVE_U_INT16_T 1

	/* Define to 1 if the system has the type `__uint16'. */
	/* #undef HAVE___UINT16 */

	/* The system-provided hash function including the namespace. */
	#define SPARSEHASH_HASH HASH_NAMESPACE::hash

	/* the namespace where STL code like vector<> is defined */
	#define STL_NAMESPACE std

	/* Stops putting the code inside the Google namespace */
	#define _END_GOOGLE_NAMESPACE_ }

	/* Puts following code inside the Google namespace */
	#define _START_GOOGLE_NAMESPACE_ namespace google {

#endif

#endif

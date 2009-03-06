
#include "PrecompiledHeader.h"
#include "HashMap.h"

namespace HashTools {

/// <summary>
///   Provides access to a set of common hash methods for C fundamental types.
/// </summary>
/// <remarks>
///   The CommonHashClass is implemented using the () operator (sometimes referred to
///   as a predicate), which means that an instance of <see cref="CommonHashClass" />
///   is required to use the methods.  This public global variable provides that instance.
/// </remarks>
/// <seealso cref="CommonHashClass"/>
const CommonHashClass GetCommonHash;

#undef get16bits
#if (defined(__GNUC__) && defined(__i386__)) || defined(__WATCOMC__) \
  || defined(_MSC_VER) || defined (__BORLANDC__) || defined (__TURBOC__)
#define get16bits(d) (*((const u16 *) (d)))
#endif

#if !defined (get16bits)
#define get16bits(d) ((((u32)(((const ubyte *)(d))[1])) << 8)\
					   +(u32)(((const ubyte *)(d))[0]) )
#endif

/// <summary>
///   Calculates a hash value for an arbitrary set of binary data.
/// </summary>
/// <remarks>
///   This method produces a 32 bit hash result from an array of source data, and
///   is suitable for generating string hashes.  It can also be used to generate
///   hashes for struct-style data (example below).  This is the method used by the
///   <c>std::string / std::wstring</c> overloads of the <see cref="CommonHashes"/>
///   class.
///
///   Note:
///   This method is an ideal use for any set of data that has five or more
///   components to it.  For smaller data types, such as Point or Rectangle
///   structs for example, you can use the int32 / uint32 hashs for faster
///   results.
/// </remarks>
/// <example>
///   This is an example of taking the hash of a structure.  Keep in mind that
///   this method only works reliably for structures that do not contain objects
///   or pointers.
///   <code>
///		struct SomeData
///		{
///			int val;
///			double floats[3];
///			char id[4];
///		};
///
///		SomeData data;
///		uint32 hashval = Hash( (const char*)&data, sizeof( data ) );
///   </code>
/// </example>
u32 Hash(const s8 * data, int len)
{
	u32 hash = len;
	int rem;

	if (len <= 0 || data == NULL) return 0;

	rem = len & 3;
	len >>= 2;

	/* Main loop */
	for (;len > 0; --len)
	{
		hash  += get16bits (data);
		u32 tmp = (get16bits (data+2) << 11) ^ hash;
		hash   = (hash << 16) ^ tmp;
		data  += 2*sizeof (u16);
		hash  += hash >> 11;
	}

	/* Handle end cases */
	switch (rem)
	{
		case 3: hash += get16bits (data);
				hash ^= hash << 16;
				hash ^= data[sizeof (u16)] << 18;
				hash += hash >> 11;
				break;
		case 2: hash += get16bits (data);
				hash ^= hash << 11;
				hash += hash >> 17;
				break;
		case 1: hash += *data;
				hash ^= hash << 10;
				hash += hash >> 1;
	}

	/* Force "avalanching" of final 127 bits */
	hash ^= hash << 3;
	hash += hash >> 5;
	hash ^= hash << 4;
	hash += hash >> 17;
	hash ^= hash << 25;
	hash += hash >> 6;

	return hash;
}
}
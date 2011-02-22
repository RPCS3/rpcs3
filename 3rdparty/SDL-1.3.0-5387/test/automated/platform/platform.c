/**
 * Automated SDL platform test.
 *
 * Based off of testplatform.c.
 *
 * Written by Edgar Simo "bobbens"
 *
 * Released under Public Domain.
 */




#include "SDL.h"
#include "SDL_endian.h"
#include "SDL_cpuinfo.h"
#include "../SDL_at.h"


/*
 * Prototypes.
 */
static int plat_testSize( size_t sizeoftype, size_t hardcodetype );
static void plat_testTypes (void);


/**
 * @brief Test size.
 *
 * @note Watcom C flags these as Warning 201: "Unreachable code" if you just
 *  compare them directly, so we push it through a function to keep the
 *  compiler quiet.  --ryan.
 */
static int plat_testSize( size_t sizeoftype, size_t hardcodetype )
{
    return sizeoftype != hardcodetype;
}


/**
 * @brief Tests type size.
 */
static void plat_testTypes (void)
{
   int ret;

   SDL_ATbegin( "Type size" );

   ret = plat_testSize( sizeof(Uint8), 1 );
   if (SDL_ATvassert( ret == 0, "sizeof(Uint8) = %lu instead of 1", sizeof(Uint8) ))
      return;

   ret = plat_testSize( sizeof(Uint16), 2 );
   if (SDL_ATvassert( ret == 0, "sizeof(Uint16) = %lu instead of 2", sizeof(Uint16) ))
      return;

   ret = plat_testSize( sizeof(Uint32), 4 );
   if (SDL_ATvassert( ret == 0, "sizeof(Uint32) = %lu instead of 4", sizeof(Uint32) ))
      return;

#ifdef SDL_HAS_64BIT_TYPE
   ret = plat_testSize( sizeof(Uint64), 8 );
   if (SDL_ATvassert( ret == 0, "sizeof(Uint64) = %lu instead of 8", sizeof(Uint64) ))
      return;
#endif /* SDL_HAS_64BIT_TYPE */

   SDL_ATend();
}


/**
 * @brief Tests platform endianness.
 */
static void plat_testEndian (void)
{
    Uint16 value = 0x1234;
    int real_byteorder;
    Uint16 value16 = 0xCDAB;
    Uint16 swapped16 = 0xABCD;
    Uint32 value32 = 0xEFBEADDE;
    Uint32 swapped32 = 0xDEADBEEF;
#ifdef SDL_HAS_64BIT_TYPE
    Uint64 value64, swapped64;
    value64 = 0xEFBEADDE;
    value64 <<= 32;
    value64 |= 0xCDAB3412;
    swapped64 = 0x1234ABCD;
    swapped64 <<= 32;
    swapped64 |= 0xDEADBEEF;
#endif

    SDL_ATbegin( "Endianness" );

    /* Test endianness. */
    if ((*((char *) &value) >> 4) == 0x1) {
        real_byteorder = SDL_BIG_ENDIAN;
    } else {
        real_byteorder = SDL_LIL_ENDIAN;
    }
    if (SDL_ATvassert( real_byteorder == SDL_BYTEORDER,
             "Machine detected as %s endian but appears to be %s endian.",
             (SDL_BYTEORDER == SDL_LIL_ENDIAN) ? "little" : "big",
             (real_byteorder == SDL_LIL_ENDIAN) ? "little" : "big" ))
       return;

    /* Test 16 swap. */
    if (SDL_ATvassert( SDL_Swap16(value16) == swapped16,
             "16 bit swapped incorrectly: 0x%X => 0x%X",
             value16, SDL_Swap16(value16) ))
       return;

    /* Test 32 swap. */
    if (SDL_ATvassert( SDL_Swap32(value32) == swapped32,
             "32 bit swapped incorrectly: 0x%X => 0x%X",
             value32, SDL_Swap32(value32) ))
       return;

#ifdef SDL_HAS_64BIT_TYPE
    /* Test 64 swap. */
    if (SDL_ATvassert( SDL_Swap64(value64) == swapped64,
#ifdef _MSC_VER
             "64 bit swapped incorrectly: 0x%I64X => 0x%I64X",
#else
             "64 bit swapped incorrectly: 0x%llX => 0x%llX",
#endif
             value64, SDL_Swap64(value64) ))
       return;
#endif

    SDL_ATend();
}


/**
 * @brief Platform test entrypoint.
 */
#ifdef TEST_STANDALONE
int main( int argc, const char *argv[] )
{
   (void) argc;
   (void) argv;
#else /* TEST_STANDALONE */
int test_platform (void)
{
#endif /* TEST_STANDALONE */

   SDL_ATinit( "Platform" );

   /* Debug information. */
   SDL_ATprintVerbose( 1, "%s System detected\n", SDL_GetPlatform() );
   SDL_ATprintVerbose( 1, "System is %s endian\n",
#ifdef SDL_LIL_ENDIAN
         "little"
#else
         "big"
#endif
         );
   SDL_ATprintVerbose( 1, "CPU count: %d\n", SDL_GetCPUCount());
   SDL_ATprintVerbose( 1, "Available extensions:\n" );
   SDL_ATprintVerbose( 1, "   RDTSC %s\n", SDL_HasRDTSC()? "detected" : "not detected" );
   SDL_ATprintVerbose( 1, "   MMX %s\n", SDL_HasMMX()? "detected" : "not detected" );
   SDL_ATprintVerbose( 1, "   SSE %s\n", SDL_HasSSE()? "detected" : "not detected" );
   SDL_ATprintVerbose( 1, "   SSE2 %s\n", SDL_HasSSE2()? "detected" : "not detected" );
   SDL_ATprintVerbose( 1, "   SSE3 %s\n", SDL_HasSSE3()? "detected" : "not detected" );
   SDL_ATprintVerbose( 1, "   SSE4.1 %s\n", SDL_HasSSE41()? "detected" : "not detected" );
   SDL_ATprintVerbose( 1, "   SSE4.2 %s\n", SDL_HasSSE42()? "detected" : "not detected" );

   plat_testTypes();
   plat_testEndian();

   return SDL_ATfinish();
}

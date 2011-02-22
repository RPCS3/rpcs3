
#include <stdio.h>

#include "SDL.h"
#include "SDL_endian.h"
#include "SDL_cpuinfo.h"
#include "SDL_assert.h"

/*
 * Watcom C flags these as Warning 201: "Unreachable code" if you just
 *  compare them directly, so we push it through a function to keep the
 *  compiler quiet.  --ryan.
 */
static int
badsize(size_t sizeoftype, size_t hardcodetype)
{
    return sizeoftype != hardcodetype;
}

int
TestTypes(SDL_bool verbose)
{
    int error = 0;

    if (badsize(sizeof(Uint8), 1)) {
        if (verbose)
            printf("sizeof(Uint8) != 1, instead = %u\n",
                   (unsigned int)sizeof(Uint8));
        ++error;
    }
    if (badsize(sizeof(Uint16), 2)) {
        if (verbose)
            printf("sizeof(Uint16) != 2, instead = %u\n",
                   (unsigned int)sizeof(Uint16));
        ++error;
    }
    if (badsize(sizeof(Uint32), 4)) {
        if (verbose)
            printf("sizeof(Uint32) != 4, instead = %u\n",
                   (unsigned int)sizeof(Uint32));
        ++error;
    }
#ifdef SDL_HAS_64BIT_TYPE
    if (badsize(sizeof(Uint64), 8)) {
        if (verbose)
            printf("sizeof(Uint64) != 8, instead = %u\n",
                   (unsigned int)sizeof(Uint64));
        ++error;
    }
#else
    if (verbose) {
        printf("WARNING: No 64-bit datatype on this platform\n");
    }
#endif
    if (verbose && !error)
        printf("All data types are the expected size.\n");

    return (error ? 1 : 0);
}

int
TestEndian(SDL_bool verbose)
{
    int error = 0;
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

    if (verbose) {
        printf("Detected a %s endian machine.\n",
               (SDL_BYTEORDER == SDL_LIL_ENDIAN) ? "little" : "big");
    }
    if ((*((char *) &value) >> 4) == 0x1) {
        real_byteorder = SDL_BIG_ENDIAN;
    } else {
        real_byteorder = SDL_LIL_ENDIAN;
    }
    if (real_byteorder != SDL_BYTEORDER) {
        if (verbose) {
            printf("Actually a %s endian machine!\n",
                   (real_byteorder == SDL_LIL_ENDIAN) ? "little" : "big");
        }
        ++error;
    }
    if (verbose) {
        printf("Value 16 = 0x%X, swapped = 0x%X\n", value16,
               SDL_Swap16(value16));
    }
    if (SDL_Swap16(value16) != swapped16) {
        if (verbose) {
            printf("16 bit value swapped incorrectly!\n");
        }
        ++error;
    }
    if (verbose) {
        printf("Value 32 = 0x%X, swapped = 0x%X\n", value32,
               SDL_Swap32(value32));
    }
    if (SDL_Swap32(value32) != swapped32) {
        if (verbose) {
            printf("32 bit value swapped incorrectly!\n");
        }
        ++error;
    }
#ifdef SDL_HAS_64BIT_TYPE
    if (verbose) {
#ifdef _MSC_VER
        printf("Value 64 = 0x%I64X, swapped = 0x%I64X\n", value64,
               SDL_Swap64(value64));
#else
        printf("Value 64 = 0x%llX, swapped = 0x%llX\n", value64,
               SDL_Swap64(value64));
#endif
    }
    if (SDL_Swap64(value64) != swapped64) {
        if (verbose) {
            printf("64 bit value swapped incorrectly!\n");
        }
        ++error;
    }
#endif
    return (error ? 1 : 0);
}


int
TestCPUInfo(SDL_bool verbose)
{
    if (verbose) {
        printf("CPU count: %d\n", SDL_GetCPUCount());
	printf("CPU cache line size: %d\n", SDL_GetCPUCacheLineSize());
        printf("RDTSC %s\n", SDL_HasRDTSC()? "detected" : "not detected");
        printf("MMX %s\n", SDL_HasMMX()? "detected" : "not detected");
        printf("SSE %s\n", SDL_HasSSE()? "detected" : "not detected");
        printf("SSE2 %s\n", SDL_HasSSE2()? "detected" : "not detected");
        printf("SSE3 %s\n", SDL_HasSSE3()? "detected" : "not detected");
        printf("SSE4.1 %s\n", SDL_HasSSE41()? "detected" : "not detected");
        printf("SSE4.2 %s\n", SDL_HasSSE42()? "detected" : "not detected");
    }
    return (0);
}

int
TestAssertions(SDL_bool verbose)
{
    SDL_assert(1);
    SDL_assert_release(1);
    SDL_assert_paranoid(1);
    SDL_assert(0 || 1);
    SDL_assert_release(0 || 1);
    SDL_assert_paranoid(0 || 1);

#if 0   /* enable this to test assertion failures. */
    SDL_assert_release(1 == 2);
    SDL_assert_release(5 < 4);
    SDL_assert_release(0 && "This is a test");
#endif

    return (0);
}

int
main(int argc, char *argv[])
{
    SDL_bool verbose = SDL_TRUE;
    int status = 0;

    if (argv[1] && (SDL_strcmp(argv[1], "-q") == 0)) {
        verbose = SDL_FALSE;
    }
    if (verbose) {
        printf("This system is running %s\n", SDL_GetPlatform());
    }

    status += TestTypes(verbose);
    status += TestEndian(verbose);
    status += TestCPUInfo(verbose);
    status += TestAssertions(verbose);

    return status;
}

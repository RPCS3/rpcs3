/**
 * Automated SDL_RWops test.
 *
 * Written by Edgar Simo "bobbens"
 *
 * Released under Public Domain.
 */


#include "SDL.h"
#include "../SDL_at.h"
#include "TestSupportRWops.h"

/* Defined in TestSupportRWops implementation to allow flexibility. */
extern const char* RWOPS_READ;
extern const char* RWOPS_WRITE;

static const char hello_world[] = "Hello World!";


/**
 * @brief Makes sure parameters work properly.
 */
static void rwops_testParam (void)
{
   SDL_RWops *rwops;

   /* Begin testcase. */
   SDL_ATbegin( "RWops Parameters" );

   /* These should all fail. */
   rwops = SDL_RWFromFile(NULL, NULL);
   if (SDL_ATassert( "SDL_RWFromFile(NULL, NULL) worked", rwops == NULL ))
      return;
   rwops = SDL_RWFromFile(NULL, "ab+");
   if (SDL_ATassert( "SDL_RWFromFile(NULL, \"ab+\") worked", rwops == NULL ))
      return;
   rwops = SDL_RWFromFile(NULL, "sldfkjsldkfj");
   if (SDL_ATassert( "SDL_RWFromFile(NULL, \"sldfkjsldkfj\") worked", rwops == NULL ))
      return;
   rwops = SDL_RWFromFile("something", "");
   if (SDL_ATassert( "SDL_RWFromFile(\"something\", \"\") worked", rwops == NULL ))
      return;
   rwops = SDL_RWFromFile("something", NULL);
   if (SDL_ATassert( "SDL_RWFromFile(\"something\", NULL) worked", rwops == NULL ))
      return;


   /* End testcase. */
   SDL_ATend();
}


/**
 * @brief Does a generic rwops test.
 *
 * RWops should have "Hello World!" in it already if write is disabled.
 *
 *    @param write Test writing also.
 *    @return 1 if an assert is failed.
 */
static int rwops_testGeneric( SDL_RWops *rw, int write )
{
   char buf[sizeof(hello_world)];
   int i;

   /* Set to start. */
   i = SDL_RWseek( rw, 0, RW_SEEK_SET );
   if (SDL_ATvassert( i == 0,
            "Seeking with SDL_RWseek (RW_SEEK_SET): got %d, expected %d",
            i, 0 ))
      return 1;

   /* Test write. */
   i = SDL_RWwrite( rw, hello_world, sizeof(hello_world)-1, 1 );
   if (write) {
      if (SDL_ATassert( "Writing with SDL_RWwrite (failed to write)", i == 1 ))
         return 1;
   }
   else {
      if (SDL_ATassert( "Writing with SDL_RWwrite (wrote when shouldn't have)", i <= 0 ))
         return 1;
   }

   /* Test seek. */
   i = SDL_RWseek( rw, 6, RW_SEEK_SET );
   if (SDL_ATvassert( i == 6,
            "Seeking with SDL_RWseek (RW_SEEK_SET): got %d, expected %d",
            i, 0 ))
       return 1;

   /* Test seek. */
   i = SDL_RWseek( rw, 0, RW_SEEK_SET );
   if (SDL_ATvassert( i == 0,
            "Seeking with SDL_RWseek (RW_SEEK_SET): got %d, expected %d",
            i, 0 ))
      return 1;

   /* Test read. */
   i = SDL_RWread( rw, buf, 1, sizeof(hello_world)-1 );
   if (SDL_ATassert( "Reading with SDL_RWread", i == sizeof(hello_world)-1 ))
      return 1;
   if (SDL_ATassert( "Memory read does not match memory written",
            SDL_memcmp( buf, hello_world, sizeof(hello_world)-1 ) == 0 ))
      return 1;

   /* More seek tests. */
   i = SDL_RWseek( rw, -4, RW_SEEK_CUR );
   if (SDL_ATvassert( i == sizeof(hello_world)-5,
            "Seeking with SDL_RWseek (RW_SEEK_CUR): got %d, expected %d",
            i, sizeof(hello_world)-5 ))
      return 1;
   i = SDL_RWseek( rw, -1, RW_SEEK_END );
   if (SDL_ATvassert( i == sizeof(hello_world)-2,
            "Seeking with SDL_RWseek (RW_SEEK_END): got %d, expected %d",
            i, sizeof(hello_world)-2 ))
      return 1;

   return 0;
}


/**
 * @brief Tests opening from memory.
 */
static void rwops_testMem (void)
{
   char mem[sizeof(hello_world)];
   SDL_RWops *rw;

   /* Begin testcase. */
   SDL_ATbegin( "SDL_RWFromMem" );

   /* Open. */
   rw = SDL_RWFromMem( mem, sizeof(hello_world)-1 );
   if (SDL_ATassert( "Opening memory with SDL_RWFromMem", rw != NULL ))
      return;

   /* Run generic tests. */
   if (rwops_testGeneric( rw, 1 ))
      return;

   /* Close. */
   SDL_FreeRW( rw );

   /* End testcase. */
   SDL_ATend();
}


static const char const_mem[] = "Hello World!";
/**
 * @brief Tests opening from memory.
 */
static void rwops_testConstMem (void)
{
   SDL_RWops *rw;

   /* Begin testcase. */
   SDL_ATbegin( "SDL_RWFromConstMem" );

   /* Open. */
   rw = SDL_RWFromConstMem( const_mem, sizeof(const_mem)-1 );
   if (SDL_ATassert( "Opening memory with SDL_RWFromConstMem", rw != NULL ))
      return;

   /* Run generic tests. */
   if (rwops_testGeneric( rw, 0 ))
      return;

   /* Close. */
   SDL_FreeRW( rw );

   /* End testcase. */
   SDL_ATend();
}


/**
 * @brief Tests opening from memory.
 */
static void rwops_testFile (void)
{
   SDL_RWops *rw;

   /* Begin testcase. */
   SDL_ATbegin( "SDL_RWFromFile" );

   /* Read test. */
   rw = TestSupportRWops_OpenRWopsFromReadDir( RWOPS_READ, "r" );
   if (SDL_ATassert( "Opening memory with SDL_RWFromFile RWOPS_READ", rw != NULL ))
      return;
   if (rwops_testGeneric( rw, 0 ))
      return;
   SDL_FreeRW( rw );

   /* Write test. */
   rw = TestSupportRWops_OpenRWopsFromWriteDir( RWOPS_WRITE, "w+" );
   if (SDL_ATassert( "Opening memory with SDL_RWFromFile RWOPS_WRITE", rw != NULL ))
      return;
   if (rwops_testGeneric( rw, 1 ))
      return;
   SDL_FreeRW( rw );

   /* End testcase. */
   SDL_ATend();
}


/**
 * @brief Tests opening from memory.
 */
static void rwops_testFP (void)
{
#ifdef HAVE_STDIO_H
   FILE *fp;
   SDL_RWops *rw;

   /* Begin testcase. */
   SDL_ATbegin( "SDL_RWFromFP" );

   /* Run read tests. */
   fp = TestSupportRWops_OpenFPFromReadDir( RWOPS_READ, "r" );
   if (SDL_ATassert( "Failed to open file 'WOPS_READ", fp != NULL))
      return;
   rw = SDL_RWFromFP( fp, 1 );
   if (SDL_ATassert( "Opening memory with SDL_RWFromFP", rw != NULL ))
      return;
   if (rwops_testGeneric( rw, 0 ))
      return;
   SDL_FreeRW( rw );

   /* Run write tests. */
   fp = TestSupportRWops_OpenFPFromWriteDir( RWOPS_WRITE, "w+" );
   if (SDL_ATassert( "Failed to open file RWOPS_WRITE", fp != NULL))
      return;
   rw = SDL_RWFromFP( fp, 1 );
   if (SDL_ATassert( "Opening memory with SDL_RWFromFP", rw != NULL ))
      return;
   if (rwops_testGeneric( rw, 1 ))
      return;
   SDL_FreeRW( rw );

   /* End testcase. */
   SDL_ATend();
#endif /* HAVE_STDIO_H */
}


/**
 * @brief Entry point.
 */
#ifdef TEST_STANDALONE
int main( int argc, const char *argv[] )
{
   (void) argc;
   (void) argv;
#else /* TEST_STANDALONE */
int test_rwops (void)
{
#endif /* TEST_STANDALONE */

   SDL_ATinit( "SDL_RWops" );

   rwops_testParam();
   rwops_testMem();
   rwops_testConstMem();
   rwops_testFile();
   rwops_testFP();

   return SDL_ATfinish();
}

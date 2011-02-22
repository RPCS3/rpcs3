/*
 * SDL test suite framework code.
 *
 * Written by Edgar Simo "bobbens"
 *
 * Released under Public Domain.
 */

#include "SDL.h"
#include "SDL_at.h"

#include "platform/platform.h"
#include "rwops/rwops.h"
#include "rect/rect.h"
#include "surface/surface.h"
#include "render/render.h"
#include "audio/audio.h"

#if defined(WIN32)
#define NO_GETOPT
#endif
#if defined(__QNXNTO__)
#define NO_GETOPT_LONG 1
#endif /* __QNXNTO__ */

#include <stdio.h> /* printf */
#include <stdlib.h> /* exit */
#ifndef NO_GETOPT
#include <unistd.h> /* getopt */
#if !defined(NO_GETOPT_LONG)
#include <getopt.h> /* getopt_long */
#endif /* !NO_GETOPT_LONG */
#endif /* !NO_GETOPT */


/*
 * Tests to run.
 */
static int run_manual      = 0; /**< Run manual tests. */
/* Manual. */
/* Automatic. */
static int run_platform    = 1; /**< Run platform tests. */
static int run_rwops       = 1; /**< Run RWops tests. */
static int run_rect        = 1; /**< Run rect tests. */
static int run_surface     = 1; /**< Run surface tests. */
static int run_render      = 1; /**< Run render tests. */
static int run_audio       = 1; /**< Run audio tests. */

/*
 * Prototypes.
 */
static void print_usage( const char *name );
static void parse_options( int argc, char *argv[] );


/**
 * @brief Displays program usage.
 */
static void print_usage( const char *name )
{
   printf("Usage: %s [OPTIONS]\n", name);
   printf("Options are:\n");
   printf("   -m, --manual        enables tests that require user interaction\n");
   printf("   --noplatform        do not run the platform tests\n");
   printf("   --norwops           do not run the rwops tests\n");
   printf("   --norect            do not run the rect tests\n");
   printf("   --nosurface         do not run the surface tests\n");
   printf("   --norender          do not run the render tests\n");
   printf("   --noaudio           do not run the audio tests\n");
   printf("   -v, --verbose       increases verbosity level by 1 for each -v\n");
   printf("   -q, --quiet         only displays errors\n");
   printf("   -h, --help          display this message and exit\n");
}

/**
 * @brief Handles the options.
 */
static void parse_options( int argc, char *argv[] )
{
   int i;

   for (i = 1; i < argc; ++i) {
      const char *arg = argv[i];
      if (SDL_strcmp(arg, "-m") == 0 || SDL_strcmp(arg, "--manual") == 0) {
         run_manual = 1;
         continue;
      }
      if (SDL_strcmp(arg, "-v") == 0 || SDL_strcmp(arg, "--verbose") == 0) {
         int level;
         SDL_ATgeti( SDL_AT_VERBOSE, &level );
         SDL_ATseti( SDL_AT_VERBOSE, level+1 );
         continue;
      }
      if (SDL_strcmp(arg, "-q") == 0 || SDL_strcmp(arg, "--quiet") == 0) {
         SDL_ATseti( SDL_AT_QUIET, 1 );
         SDL_setenv("SDL_ASSERT", "abort", 0);
         continue;
      }
      if (SDL_strcmp(arg, "--noplatform") == 0) {
         run_platform = 0;
         continue;
      }
      if (SDL_strcmp(arg, "--norwops") == 0) {
         run_rwops = 0;
         continue;
      }
      if (SDL_strcmp(arg, "--norect") == 0) {
         run_rect = 0;
         continue;
      }
      if (SDL_strcmp(arg, "--nosurface") == 0) {
         run_surface = 0;
         continue;
      }
      if (SDL_strcmp(arg, "--norender") == 0) {
         run_render = 0;
         continue;
      }
      if (SDL_strcmp(arg, "--noaudio") == 0) {
         run_audio = 0;
         continue;
      }

      /* Print help and exit! */
      print_usage( argv[0] );
      exit(EXIT_FAILURE);
   }
}

/**
 * @brief Main entry point.
 */
int main( int argc, char *argv[] )
{
   int failed;
   const char *rev;
   SDL_version ver;

   /* Get options. */
   parse_options( argc, argv );

   /* Defaults. */
   failed = 0;

   /* Print some text if verbose. */
   SDL_GetVersion( &ver );
   rev = SDL_GetRevision();
   SDL_ATprintVerbose( 1, "Running tests with SDL %d.%d.%d revision %s\n",
         ver.major, ver.minor, ver.patch, rev );

   /* Automatic tests. */
   if (run_platform)
      failed += test_platform();
   if (run_rwops)
      failed += test_rwops();
   if (run_rect)
      failed += test_rect();
   if (run_surface)
      failed += test_surface();
   if (run_render)
      failed += test_render();
   if (run_audio)
      failed += test_audio();

   /* Manual tests. */
   if (run_manual) {
   }

   /* Display more information if failed. */
   if (failed > 0) {
      SDL_ATprintErr( "Tests run with SDL %d.%d.%d revision %d\n",
            ver.major, ver.minor, ver.patch, rev );
      SDL_ATprintErr( "System is running %s and is %s endian\n",
            SDL_GetPlatform(),
#if SDL_BYTEORDER == SDL_LIL_ENDIAN
            "little"
#else
            "big"
#endif
            );
   }

   return failed;
}


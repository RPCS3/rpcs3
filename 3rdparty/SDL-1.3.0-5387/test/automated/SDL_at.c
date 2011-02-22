/*
 * Common code for automated test suite.
 *
 * Written by Edgar Simo "bobbens"
 *
 * Released under Public Domain.
 */


#include "SDL_at.h"
#include "SDL_stdinc.h"
#include "SDL_error.h"

#include <stdio.h> /* printf/fprintf */
#include <stdarg.h> /* va_list */


/*
 * Internal usage SDL_AT variables.
 */
static char *at_suite_msg = NULL; /**< Testsuite message. */
static char *at_test_msg = NULL; /**< Testcase message. */
static int at_success = 0; /**< Number of successful testcases. */
static int at_failure = 0; /**< Number of failed testcases. */


/*
 * Global properties.
 */
static int at_verbose = 0; /**< Verbosity. */
static int at_quiet = 0; /**< Quietness. */


/*
 * Prototypes.
 */
static void SDL_ATcleanup (void);
static void SDL_ATendWith( int success );
static void SDL_ATassertFailed( const char *msg );


/**
 * @brief Cleans up the automated testsuite state.
 */
static void SDL_ATcleanup (void)
{
   if (at_suite_msg != NULL)
      SDL_free(at_suite_msg);
   at_suite_msg   = NULL;
   if (at_test_msg != NULL)
      SDL_free(at_test_msg);
   at_test_msg    = NULL;
   at_success     = 0;
   at_failure     = 0;
}


/**
 * @brief Begin testsuite.
 */
void SDL_ATinit( const char *suite )
{
   /* Do not open twice. */
   if (at_suite_msg) {
      SDL_ATprintErr( "AT suite '%s' not closed before opening suite '%s'\n",
            at_suite_msg, suite );
   }
   /* Must have a name. */
   if (suite == NULL) {
      SDL_ATprintErr( "AT testsuite does not have a name.\n");
   }
   SDL_ATcleanup();
   at_suite_msg = SDL_strdup(suite);

   /* Verbose message. */
   SDL_ATprintVerbose( 2, "--+---> Started Test Suite '%s'\n", at_suite_msg );
}


/**
 * @brief Finish testsuite.
 */
int SDL_ATfinish (void)
{
   int failed;

   /* Make sure initialized. */
   if (at_suite_msg == NULL) {
      SDL_ATprintErr("Ended testcase without initializing.\n");
      return 1;
   }

   /* Finished without closing testcase. */
   if (at_test_msg) {
      SDL_ATprintErr( "AT suite '%s' finished without closing testcase '%s'\n",
            at_suite_msg, at_test_msg );
   }

   /* Verbose message. */
   SDL_ATprintVerbose( 2, "<-+---- Finished Test Suite '%s'\n", at_suite_msg );

   /* Display message if verbose on failed. */
   failed = at_failure;
   if (at_failure > 0) {
      SDL_ATprintErr( "%s : Failed %d out of %d testcases!\n",
            at_suite_msg, at_failure, at_failure+at_success );
   }
   else {
      SDL_ATprint( "%s : All tests successful (%d)\n",
            at_suite_msg, at_success );
   }

   /* Clean up. */
   SDL_ATcleanup();

   /* Return failed. */
   return failed;
}


/**
 * @brief Sets a property.
 */
void SDL_ATseti( int property, int value )
{
   switch (property) {
      case SDL_AT_VERBOSE:
         at_verbose = value;
         break;

      case SDL_AT_QUIET:
         at_quiet = value;
         break;
   }
}


/**
 * @brief Gets a property.
 */
void SDL_ATgeti( int property, int *value )
{
   switch (property) {
      case SDL_AT_VERBOSE:
         *value = at_verbose;
         break;

      case SDL_AT_QUIET:
         *value = at_quiet;
         break;
   }
}


/**
 * @brief Begin testcase.
 */
void SDL_ATbegin( const char *testcase )
{
   /* Do not open twice. */
   if (at_test_msg) {
      SDL_ATprintErr( "AT testcase '%s' not closed before opening testcase '%s'\n",
            at_test_msg, testcase );
   }
   /* Must have a name. */
   if (testcase == NULL) {
      SDL_ATprintErr( "AT testcase does not have a name.\n");
   }
   at_test_msg = SDL_strdup(testcase);

   /* Verbose message. */
   SDL_ATprintVerbose( 2, "  +---> StartedTest Case '%s'\n", testcase );
}


/**
 * @brief Ends the testcase with a succes or failure.
 */
static void SDL_ATendWith( int success )
{
   /* Make sure initialized. */
   if (at_test_msg == NULL) {
      SDL_ATprintErr("Ended testcase without initializing.\n");
      return;
   }

   /* Mark as success or failure. */
   if (success)
      at_success++;
   else
      at_failure++;

   /* Verbose message. */
   SDL_ATprintVerbose( 2, "  +---- Finished Test Case '%s'\n", at_test_msg );

   /* Clean up. */
   if (at_test_msg != NULL)
      SDL_free(at_test_msg);
   at_test_msg = NULL;
}


/**
 * @brief Display failed assert message.
 */
static void SDL_ATassertFailed( const char *msg )
{
   /* Print. */
   SDL_ATprintErr( "Assert Failed!\n" );
   SDL_ATprintErr( "   %s\n", msg );
   SDL_ATprintErr( "   Test Case '%s'\n", at_test_msg );
   SDL_ATprintErr( "   Test Suite '%s'\n", at_suite_msg );
   SDL_ATprintErr( "   Last SDL error '%s'\n", SDL_GetError() );
   /* End. */
   SDL_ATendWith(0);
}


/**
 * @brief Testcase test.
 */
int SDL_ATassert( const char *msg, int condition )
{
   /* Condition failed. */
   if (!condition) {
      /* Failed message. */
      SDL_ATassertFailed(msg);
   }
   return !condition;
}


/**
 * @brief Testcase test.
 */
int SDL_ATvassert( int condition, const char *msg, ... )
{
   va_list args;
   char buf[256];

   /* Condition failed. */
   if (!condition) {
      /* Get message. */
      va_start( args, msg );
      SDL_vsnprintf( buf, sizeof(buf), msg, args );
      va_end( args );
      /* Failed message. */
      SDL_ATassertFailed( buf );
   }
   return !condition;
}


/**
 * @brief End testcase.
 */
void SDL_ATend (void)
{
   SDL_ATendWith(1);
}


/**
 * @brief Displays an error.
 */
int SDL_ATprintErr( const char *msg, ... )
{
   va_list ap;
   int ret;

   /* Make sure there is something to print. */
   if (msg == NULL)
      return 0;
   else {
      va_start(ap, msg);
      ret = vfprintf( stderr, msg, ap );
      va_end(ap);
   }

   return ret;
}


/**
 * @brief Displays a message.
 */
int SDL_ATprint( const char *msg, ... )
{
   va_list ap;
   int ret;

   /* Only print if not quiet. */
   if (at_quiet)
      return 0;

   /* Make sure there is something to print. */
   if (msg == NULL)
      return 0;
   else {
      va_start(ap, msg);
      ret = vfprintf( stdout, msg, ap );
      va_end(ap);
   }

   return ret;
}


/**
 * @brief Displays a verbose message.
 */
int SDL_ATprintVerbose( int level, const char *msg, ... )
{
   va_list ap;
   int ret;

   /* Only print if not quiet. */
   if (at_quiet || (at_verbose < level))
      return 0;

   /* Make sure there is something to print. */
   if (msg == NULL)
      return 0;
   else {
      va_start(ap, msg);
      ret = vfprintf( stdout, msg, ap );
      va_end(ap);
   }

   return ret;
}

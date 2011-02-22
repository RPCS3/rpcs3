/*
 * Common code for automated test suite.
 *
 * Written by Edgar Simo "bobbens"
 *
 * Released under Public Domain.
 */


/**
 * @file SDL_at.h
 *
 * @brief Handles automatic testing functionality.
 *
 * The basic approach with SDL_AT is to divide the tests into what are called
 *  test suites and test cases. Each test suite should have multiple test
 *  cases, each test case can have multiple asserts.
 *
 * To actually test for conditions within the testcase you check asserts, if
 *  the asserts fail the failures will be logged in the testsuite and
 *  displayed.
 *
 *  Syntax is similar to OpenGL. An example would be:
 *
 * @code
 * int f; // Number failed
 * SDL_ATinit( "My testsuite" );
 *
 * SDL_ATbegin( "My first testcase" );
 * if (!SDL_ATassert( (1+1)==2, "Trying '1+1=2'."))
 *    return; // Implicitly calls SDL_ATend if assert fails
 * SDL_ATend(); // Finish testcase
 *
 * SDL_ATbegin( "My second testcase" );
 * if (!SDL_ATassert( (4/2)==2, "Trying '4/2=2'."))
 *    return; // Implicitly calls SDL_ATend if assert fails
 * SDL_ATend(); // Finish testcase
 *
 * f = SDL_ATfinish();
 * @endcode
 *
 * @author Edgar Simo "bobbens"
 */


#ifndef _SDL_AT_H
#  define _SDL_AT_H



enum {
   SDL_AT_VERBOSE, /**< Sets the verbose level. */
   SDL_AT_QUIET /**< Sets quietness. */
};


/*
 * Suite level actions.
 */
/**
 * @brief Starts the testsuite.
 *
 *    @param suite Name of the suite to start testing.
 */
void SDL_ATinit( const char *suite );
/**
 * @brief Finishes the testsuite printing out global results if verbose.
 *
 *    @return 0 if no errors occurred, otherwise number of failures.
 */
int SDL_ATfinish (void);
/**
 * @brief Sets a global property value.
 *
 *    @param property Property to set.
 *    @param value Value to set property to.
 */
void SDL_ATseti( int property, int value );
/**
 * @brief Gets a global property value.
 *
 *    @param property Property to get.
 *    @param[out] value Value of the property.
 */
void SDL_ATgeti( int property, int *value );


/*
 * Testcase level actions.
 */
/**
 * @brief Begins a testcase.
 *
 *    @param testcase Name of the testcase to begin.
 */
void SDL_ATbegin( const char *testcase );
/**
 * @brief Checks a condition in the testcase.
 *
 * Will automatically call SDL_ATend if the condition isn't met.
 *
 *    @param condition Condition to make sure is true.
 *    @param msg Message to display for failure.
 *    @return Returns 1 if the condition isn't met.
 */
int SDL_ATassert( const char *msg, int condition );
/**
 * @brief Checks a condition in the testcase.
 *
 * Will automatically call SDL_ATend if the condition isn't met.
 *
 *    @param condition Condition to make sure is true.
 *    @param msg Message to display for failure with printf style formatting.
 *    @return Returns 1 if the condition isn't met.
 */
int SDL_ATvassert( int condition, const char *msg, ... );
/**
 * @brief Ends a testcase.
 */
void SDL_ATend (void);


/*
 * Misc functions.
 */
/**
 * @brief Prints an error.
 *
 *    @param msg printf formatted string to display.
 *    @return Number of character printed.
 */
int SDL_ATprintErr( const char *msg, ... );
/**
 * @brief Prints some text.
 *
 *    @param msg printf formatted string to display.
 *    @return Number of character printed.
 */
int SDL_ATprint( const char *msg, ... );
/**
 * @brief Prints some verbose text.
 *
 * Verbosity levels are as follows:
 *
 *  - 0 standard stdout, enabled by default
 *  - 1 additional information
 *  - 2 detailed information (spammy)
 *
 *    @param level Level of verbosity to print at.
 *    @param msg printf formatted string to display.
 *    @return Number of character printed.
 */
int SDL_ATprintVerbose( int level, const char *msg, ... );


#endif /* _SDL_AT_H */



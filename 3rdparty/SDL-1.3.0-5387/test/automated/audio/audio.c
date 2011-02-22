/**
 * Automated SDL_RWops test.
 *
 * Written by Edgar Simo "bobbens"
 *
 * Released under Public Domain.
 */


#include "SDL.h"
#include "../SDL_at.h"


/**
 * @brief Prints available devices.
 */
static int audio_printDevices( int iscapture )
{
   int i, n;

   /* Get number of devices. */
   n = SDL_GetNumAudioDevices(iscapture);
   SDL_ATprintVerbose( 1, "%d %s Audio Devices\n",
         n, iscapture ? "Capture" : "Output" );

   /* List devices. */
   for (i=0; i<n; i++) {
      SDL_ATprintVerbose( 1, "   %d) %s\n", i+1, SDL_GetAudioDeviceName( i, iscapture ) );
   }

   return 0;
}


/**
 * @brief Makes sure parameters work properly.
 */
static void audio_testOpen (void)
{
   int i, n;
   int ret;

   /* Begin testcase. */
   SDL_ATbegin( "Audio Open" );

   /* List drivers. */
   n = SDL_GetNumAudioDrivers();
   SDL_ATprintVerbose( 1, "%d Audio Drivers\n", n );
   for (i=0; i<n; i++) {
      SDL_ATprintVerbose( 1, "   %s\n", SDL_GetAudioDriver(i) );
   }

   /* Start SDL. */
   ret = SDL_Init( SDL_INIT_AUDIO );
   if (SDL_ATvassert( ret==0, "SDL_Init( SDL_INIT_AUDIO ): %s", SDL_GetError()))
      return;

   /* Print devices. */
   SDL_ATprintVerbose( 1, "Using Audio Driver '%s'\n", SDL_GetCurrentAudioDriver() );
   audio_printDevices(0);
   audio_printDevices(1);

   /* Quit SDL. */
   SDL_Quit();

   /* End testcase. */
   SDL_ATend();
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
int test_audio (void)
{
#endif /* TEST_STANDALONE */

   SDL_ATinit( "SDL_Audio" );

   audio_testOpen();

   return SDL_ATfinish();
}

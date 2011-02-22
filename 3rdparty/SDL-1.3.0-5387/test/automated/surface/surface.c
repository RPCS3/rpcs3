/**
 * Automated SDL_Surface test.
 *
 * Written by Edgar Simo "bobbens"
 *
 * Released under Public Domain.
 */


#include "SDL.h"
#include "SDL_surface.h"
#include "SDL_video.h"
#include "../SDL_at.h"

#include "../common/common.h"


/*
 * Pull in images for testcases.
 */
#include "../common/images.h"


/*
 * Prototypes.
 */
/* Testcases. */
static void surface_testLoad( SDL_Surface *testsur );
static void surface_testBlit( SDL_Surface *testsur );
static int surface_testBlitBlendMode( SDL_Surface *testsur, SDL_Surface *face, int mode );
static void surface_testBlitBlend( SDL_Surface *testsur );


/**
 * @brief Tests sprite loading.
 */
static void surface_testLoad( SDL_Surface *testsur )
{
   int ret;
   SDL_Surface *face, *rface;

   SDL_ATbegin( "Load Test" );

   /* Clear surface. */
   ret = SDL_FillRect( testsur, NULL,
         SDL_MapRGB( testsur->format, 0, 0, 0 ) );
   if (SDL_ATassert( "SDL_FillRect", ret == 0))
      return;

   /* Create the blit surface. */
#ifdef __APPLE__
	face = SDL_LoadBMP("icon.bmp");
#else
	face = SDL_LoadBMP("../icon.bmp");
#endif

   if (SDL_ATassert( "SDL_CreateLoadBmp", face != NULL))
      return;

   /* Set transparent pixel as the pixel at (0,0) */
   if (face->format->palette) {
      ret = SDL_SetColorKey(face, SDL_RLEACCEL, *(Uint8 *) face->pixels);
      if (SDL_ATassert( "SDL_SetColorKey", ret == 0))
         return;
   }

   /* Convert to 32 bit to compare. */
   rface = SDL_ConvertSurface( face, testsur->format, 0 );
   if (SDL_ATassert( "SDL_ConvertSurface", rface != NULL))
      return;

   /* See if it's the same. */
   if (SDL_ATassert( "Primitives output not the same.",
            surface_compare( rface, &img_face, 0 )==0 ))
      return;

   /* Clean up. */
   SDL_FreeSurface( rface );
   SDL_FreeSurface( face );

   SDL_ATend();
}


/**
 * @brief Tests some blitting routines.
 */
static void surface_testBlit( SDL_Surface *testsur )
{
   int ret;
   SDL_Rect rect;
   SDL_Surface *face;
   int i, j, ni, nj;

   SDL_ATbegin( "Blit Tests" );

   /* Clear surface. */
   ret = SDL_FillRect( testsur, NULL,
         SDL_MapRGB( testsur->format, 0, 0, 0 ) );
   if (SDL_ATassert( "SDL_FillRect", ret == 0))
      return;

   /* Create face surface. */
   face = SDL_CreateRGBSurfaceFrom( (void*)img_face.pixel_data,
         img_face.width, img_face.height, 32, img_face.width*4,
#if (SDL_BYTEORDER == SDL_BIG_ENDIAN)
         0xff000000, /* Red bit mask. */
         0x00ff0000, /* Green bit mask. */
         0x0000ff00, /* Blue bit mask. */
         0x000000ff /* Alpha bit mask. */
#else
         0x000000ff, /* Red bit mask. */
         0x0000ff00, /* Green bit mask. */
         0x00ff0000, /* Blue bit mask. */
         0xff000000 /* Alpha bit mask. */
#endif
         );
   if (SDL_ATassert( "SDL_CreateRGBSurfaceFrom", face != NULL))
      return;

   /* Constant values. */
   rect.w = face->w;
   rect.h = face->h;
   ni     = testsur->w - face->w;
   nj     = testsur->h - face->h;

   /* Loop blit. */
   for (j=0; j <= nj; j+=4) {
      for (i=0; i <= ni; i+=4) {
         /* Blitting. */
         rect.x = i;
         rect.y = j;
         ret = SDL_BlitSurface( face, NULL, testsur, &rect );
         if (SDL_ATassert( "SDL_BlitSurface", ret == 0))
            return;
      }
   }

   /* See if it's the same. */
   if (SDL_ATassert( "Blitting output not the same (normal blit).",
            surface_compare( testsur, &img_blit, 0 )==0 ))
      return;

   /* Clear surface. */
   ret = SDL_FillRect( testsur, NULL,
         SDL_MapRGB( testsur->format, 0, 0, 0 ) );
   if (SDL_ATassert( "SDL_FillRect", ret == 0))
      return;

   /* Test blitting with colour mod. */
   for (j=0; j <= nj; j+=4) {
      for (i=0; i <= ni; i+=4) {
         /* Set colour mod. */
         ret = SDL_SetSurfaceColorMod( face, (255/nj)*j, (255/ni)*i, (255/nj)*j );
         if (SDL_ATassert( "SDL_SetSurfaceColorMod", ret == 0))
            return;

         /* Blitting. */
         rect.x = i;
         rect.y = j;
         ret = SDL_BlitSurface( face, NULL, testsur, &rect );
         if (SDL_ATassert( "SDL_BlitSurface", ret == 0))
            return;
      }
   }

   /* See if it's the same. */
   if (SDL_ATassert( "Blitting output not the same (using SDL_SetSurfaceColorMod).",
            surface_compare( testsur, &img_blitColour, 0 )==0 ))
      return;

   /* Clear surface. */
   ret = SDL_FillRect( testsur, NULL,
         SDL_MapRGB( testsur->format, 0, 0, 0 ) );
   if (SDL_ATassert( "SDL_FillRect", ret == 0))
      return;

   /* Restore colour. */
   ret = SDL_SetSurfaceColorMod( face, 255, 255, 255 );
   if (SDL_ATassert( "SDL_SetSurfaceColorMod", ret == 0))
      return;

   /* Test blitting with colour mod. */
   for (j=0; j <= nj; j+=4) {
      for (i=0; i <= ni; i+=4) {
         /* Set alpha mod. */
         ret = SDL_SetSurfaceAlphaMod( face, (255/ni)*i );
         if (SDL_ATassert( "SDL_SetSurfaceAlphaMod", ret == 0))
            return;

         /* Blitting. */
         rect.x = i;
         rect.y = j;
         ret = SDL_BlitSurface( face, NULL, testsur, &rect );
         if (SDL_ATassert( "SDL_BlitSurface", ret == 0))
            return;
      }
   }

   /* See if it's the same. */
   if (SDL_ATassert( "Blitting output not the same (using SDL_SetSurfaceAlphaMod).",
            surface_compare( testsur, &img_blitAlpha, 0 )==0 ))
      return;

   /* Clean up. */
   SDL_FreeSurface( face );

   SDL_ATend();
}


/**
 * @brief Tests a blend mode.
 */
static int surface_testBlitBlendMode( SDL_Surface *testsur, SDL_Surface *face, int mode )
{
   int ret;
   int i, j, ni, nj;
   SDL_Rect rect;

   /* Clear surface. */
   ret = SDL_FillRect( testsur, NULL,
         SDL_MapRGB( testsur->format, 0, 0, 0 ) );
   if (SDL_ATassert( "SDL_FillRect", ret == 0))
      return 1;

   /* Steps to take. */
   ni     = testsur->w - face->w;
   nj     = testsur->h - face->h;

   /* Constant values. */
   rect.w = face->w;
   rect.h = face->h;

   /* Test blend mode. */
   for (j=0; j <= nj; j+=4) {
      for (i=0; i <= ni; i+=4) {
         /* Set blend mode. */
         ret = SDL_SetSurfaceBlendMode( face, mode );
         if (SDL_ATassert( "SDL_SetSurfaceBlendMode", ret == 0))
            return 1;

         /* Blitting. */
         rect.x = i;
         rect.y = j;
         ret = SDL_BlitSurface( face, NULL, testsur, &rect );
         if (SDL_ATassert( "SDL_BlitSurface", ret == 0))
            return 1;
      }
   }

   return 0;
}


/**
 * @brief Tests some more blitting routines.
 */
static void surface_testBlitBlend( SDL_Surface *testsur )
{
   int ret;
   SDL_Rect rect;
   SDL_Surface *face;
   int i, j, ni, nj;
   int mode;

   SDL_ATbegin( "Blit Blending Tests" );

   /* Clear surface. */
   ret = SDL_FillRect( testsur, NULL,
         SDL_MapRGB( testsur->format, 0, 0, 0 ) );
   if (SDL_ATassert( "SDL_FillRect", ret == 0))
      return;

   /* Create the blit surface. */
   face = SDL_CreateRGBSurfaceFrom( (void*)img_face.pixel_data,
         img_face.width, img_face.height, 32, img_face.width*4,
#if (SDL_BYTEORDER == SDL_BIG_ENDIAN)
         0xff000000, /* Red bit mask. */
         0x00ff0000, /* Green bit mask. */
         0x0000ff00, /* Blue bit mask. */
         0x000000ff /* Alpha bit mask. */
#else
         0x000000ff, /* Red bit mask. */
         0x0000ff00, /* Green bit mask. */
         0x00ff0000, /* Blue bit mask. */
         0xff000000 /* Alpha bit mask. */
#endif
         );
   if (SDL_ATassert( "SDL_CreateRGBSurfaceFrom", face != NULL))
      return;

   /* Set alpha mod. */
   ret = SDL_SetSurfaceAlphaMod( face, 100 );
   if (SDL_ATassert( "SDL_SetSurfaceAlphaMod", ret == 0))
      return;

   /* Steps to take. */
   ni     = testsur->w - face->w;
   nj     = testsur->h - face->h;

   /* Constant values. */
   rect.w = face->w;
   rect.h = face->h;

   /* Test None. */
   if (surface_testBlitBlendMode( testsur, face, SDL_BLENDMODE_NONE ))
      return;
   if (SDL_ATassert( "Blitting blending output not the same (using SDL_BLENDMODE_NONE).",
            surface_compare( testsur, &img_blendNone, 0 )==0 ))
      return;

   /* Test Blend. */
   if (surface_testBlitBlendMode( testsur, face, SDL_BLENDMODE_BLEND ))
      return;
   if (SDL_ATassert( "Blitting blending output not the same (using SDL_BLENDMODE_BLEND).",
            surface_compare( testsur, &img_blendBlend, 0 )==0 ))
      return;

   /* Test Add. */
   if (surface_testBlitBlendMode( testsur, face, SDL_BLENDMODE_ADD ))
      return;
   if (SDL_ATassert( "Blitting blending output not the same (using SDL_BLENDMODE_ADD).",
            surface_compare( testsur, &img_blendAdd, 0 )==0 ))
      return;

   /* Test Mod. */
   if (surface_testBlitBlendMode( testsur, face, SDL_BLENDMODE_MOD ))
      return;
   if (SDL_ATassert( "Blitting blending output not the same (using SDL_BLENDMODE_MOD).",
            surface_compare( testsur, &img_blendMod, 0 )==0 ))
      return;

   /* Clear surface. */
   ret = SDL_FillRect( testsur, NULL,
         SDL_MapRGB( testsur->format, 0, 0, 0 ) );
   if (SDL_ATassert( "SDL_FillRect", ret == 0))
      return;

   /* Loop blit. */
   for (j=0; j <= nj; j+=4) {
      for (i=0; i <= ni; i+=4) {

         /* Set colour mod. */
         ret = SDL_SetSurfaceColorMod( face, (255/nj)*j, (255/ni)*i, (255/nj)*j );
         if (SDL_ATassert( "SDL_SetSurfaceColorMod", ret == 0))
            return;

         /* Set alpha mod. */
         ret = SDL_SetSurfaceAlphaMod( face, (100/ni)*i );
         if (SDL_ATassert( "SDL_SetSurfaceAlphaMod", ret == 0))
            return;

         /* Crazy blending mode magic. */
         mode = (i/4*j/4) % 4;
         if (mode==0) mode = SDL_BLENDMODE_NONE;
         else if (mode==1) mode = SDL_BLENDMODE_BLEND;
         else if (mode==2) mode = SDL_BLENDMODE_ADD;
         else if (mode==3) mode = SDL_BLENDMODE_MOD;
         ret = SDL_SetSurfaceBlendMode( face, mode );
         if (SDL_ATassert( "SDL_SetSurfaceBlendMode", ret == 0))
            return;

         /* Blitting. */
         rect.x = i;
         rect.y = j;
         ret = SDL_BlitSurface( face, NULL, testsur, &rect );
         if (SDL_ATassert( "SDL_BlitSurface", ret == 0))
            return;
      }
   }

   /* Check to see if matches. */
   if (SDL_ATassert( "Blitting blending output not the same (using SDL_BLEND_*).",
            surface_compare( testsur, &img_blendAll, 0 )==0 ))
      return;

   /* Clean up. */
   SDL_FreeSurface( face );

   SDL_ATend();
}


/**
 * @brief Runs all the tests on the surface.
 *
 *    @param testsur Surface to run tests on.
 */
void surface_runTests( SDL_Surface *testsur )
{
   /* Software surface blitting. */
   surface_testBlit( testsur );
   surface_testBlitBlend( testsur );
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
int test_surface (void)
{
#endif /* TEST_STANDALONE */
   int ret;
   SDL_Surface *testsur;

   SDL_ATinit( "SDL_Surface" );

   SDL_ATbegin( "Initializing" );
   /* Initializes the SDL subsystems. */
   ret = SDL_Init(0);
   if (SDL_ATassert( "SDL_Init(0)", ret == 0))
      goto err;

   /* Now run on the video mode. */
   ret = SDL_InitSubSystem( SDL_INIT_VIDEO );
   if (SDL_ATassert( "SDL_InitSubSystem( SDL_INIT_VIDEO )", ret == 0))
      goto err;

   /*
    * Surface on surface tests.
    */
   /* Create the test surface. */
   testsur = SDL_CreateRGBSurface( 0, 80, 60, 32, 
         RMASK, GMASK, BMASK, AMASK );
   if (SDL_ATassert( "SDL_CreateRGBSurface", testsur != NULL))
      goto err;
   SDL_ATend();
   /* Run surface on surface tests. */
   surface_testLoad( testsur );
   surface_runTests( testsur );
   /* Clean up. */
   SDL_FreeSurface( testsur );

   /* Exit SDL. */
   SDL_Quit();

   return SDL_ATfinish();

err:
   return SDL_ATfinish();
}


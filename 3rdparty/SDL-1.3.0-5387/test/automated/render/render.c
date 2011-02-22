/**
 * Automated SDL_Surface test.
 *
 * Written by Edgar Simo "bobbens"
 *
 * Released under Public Domain.
 */


#include "SDL.h"
#include "../SDL_at.h"

#include "../common/common.h"


/*
 * Pull in images for testcases.
 */
#include "../common/images.h"


#define SCREEN_W     80
#define SCREEN_H     60

#define FACE_W       img_face.width
#define FACE_H       img_face.height

static SDL_Renderer *renderer;

/*
 * Prototypes.
 */
static int render_compare( const char *msg, const SurfaceImage_t *s, int allowable_error );
static int render_isSupported( int code );
static int render_hasDrawColor (void);
static int render_hasBlendModes (void);
static int render_hasTexColor (void);
static int render_hasTexAlpha (void);
static int render_clearScreen (void);
/* Testcases. */
static int render_testPrimitives (void);
static int render_testPrimitivesBlend (void);
static int render_testBlit (void);
static int render_testBlitColour (void);
static int render_testBlitAlpha (void);
static int render_testBlitBlendMode( SDL_Texture * tface, int mode );
static int render_testBlitBlend (void);


/**
 * @brief Compares screen pixels with image pixels.
 *
 *    @param msg Message on failure.
 *    @param s Image to compare against.
 *    @return 0 on success.
 */
static int render_compare( const char *msg, const SurfaceImage_t *s, int allowable_error )
{
   int ret;
   SDL_Rect rect;
   Uint8 pix[4*80*60];
   SDL_Surface *testsur;

   /* Read pixels. */
   /* Explicitly specify the rect in case the window isn't expected size... */
   rect.x = 0;
   rect.y = 0;
   rect.w = 80;
   rect.h = 60;
   ret = SDL_RenderReadPixels(renderer, &rect, FORMAT, pix, 80*4 );
   if (SDL_ATassert( "SDL_RenderReadPixels", ret==0) )
      return 1;

   /* Create surface. */
   testsur = SDL_CreateRGBSurfaceFrom( pix, 80, 60, 32, 80*4,
                                       RMASK, GMASK, BMASK, AMASK);
   if (SDL_ATassert( "SDL_CreateRGBSurfaceFrom", testsur!=NULL ))
      return 1;

   /* Compare surface. */
   ret = surface_compare( testsur, s, allowable_error );
   if (SDL_ATassert( msg, ret==0 ))
      return 1;

   /* Clean up. */
   SDL_FreeSurface( testsur );

   return 0;
}

#if 0
static int dump_screen( int index )
{
   int ret;
   char name[1024];
   Uint8 pix[4*80*60];
   SDL_Surface *testsur;
   SDL_RendererInfo info;

   /* Read pixels. */
   ret = SDL_RenderReadPixels(renderer, NULL, FORMAT, pix, 80*4 );
   if (SDL_ATassert( "SDL_RenderReadPixels", ret==0) )
      return 1;

   /* Create surface. */
   testsur = SDL_CreateRGBSurfaceFrom( pix, 80, 60, 32, 80*4,
                                       RMASK, GMASK, BMASK, AMASK);
   if (SDL_ATassert( "SDL_CreateRGBSurfaceFrom", testsur!=NULL ))
      return 1;

   /* Dump surface. */
   SDL_GetRendererInfo(renderer,&info);
   sprintf(name, "%s-%s-%d.bmp", SDL_GetCurrentVideoDriver(), info.name, index);
   SDL_SaveBMP(testsur, name);

   /* Clean up. */
   SDL_FreeSurface( testsur );

   return 0;
}
#endif

/**
 * @brief Checks to see if functionality is supported.
 */
static int render_isSupported( int code )
{
   return (code == 0);
}


/**
 * @brief Test to see if we can vary the draw colour.
 */
static int render_hasDrawColor (void)
{
   int ret, fail;
   Uint8 r, g, b, a;

   fail = 0;

   /* Set colour. */
   ret = SDL_SetRenderDrawColor(renderer, 100, 100, 100, 100 );
   if (!render_isSupported(ret))
      fail = 1;
   ret = SDL_GetRenderDrawColor(renderer, &r, &g, &b, &a );
   if (!render_isSupported(ret))
      fail = 1;
   /* Restore natural. */
   ret = SDL_SetRenderDrawColor(renderer, 0, 0, 0, SDL_ALPHA_OPAQUE );
   if (!render_isSupported(ret))
      fail = 1;

   /* Something failed, consider not available. */
   if (fail)
      return 0;
   /* Not set properly, consider failed. */
   else if ((r != 100) || (g != 100) || (b != 100) || (a != 100))
      return 0;
   return 1;
}


/**
 * @brief Test to see if we can vary the blend mode.
 */
static int render_hasBlendModes (void)
{
   int fail;
   int ret;
   SDL_BlendMode mode;

   fail = 0;

   ret = SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND );
   if (!render_isSupported(ret))
      fail = 1;
   ret = SDL_GetRenderDrawBlendMode(renderer, &mode );
   if (!render_isSupported(ret))
      fail = 1;
   ret = (mode != SDL_BLENDMODE_BLEND);
   if (!render_isSupported(ret))
      fail = 1;
   ret = SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_ADD );
   if (!render_isSupported(ret))
      fail = 1;
   ret = SDL_GetRenderDrawBlendMode(renderer, &mode );
   if (!render_isSupported(ret))
      fail = 1;
   ret = (mode != SDL_BLENDMODE_ADD);
   if (!render_isSupported(ret))
      fail = 1;
   ret = SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_MOD );
   if (!render_isSupported(ret))
      fail = 1;
   ret = SDL_GetRenderDrawBlendMode(renderer, &mode );
   if (!render_isSupported(ret))
      fail = 1;
   ret = (mode != SDL_BLENDMODE_MOD);
   if (!render_isSupported(ret))
      fail = 1;
   ret = SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE );
   if (!render_isSupported(ret))
      fail = 1;
   ret = SDL_GetRenderDrawBlendMode(renderer, &mode );
   if (!render_isSupported(ret))
      fail = 1;
   ret = (mode != SDL_BLENDMODE_NONE);
   if (!render_isSupported(ret))
      fail = 1;

   return !fail;
}


/**
 * @brief Loads the test face.
 */
static SDL_Texture * render_loadTestFace (void)
{
   SDL_Surface *face;
   SDL_Texture *tface;

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
   if (face == NULL)
      return 0;
   tface = SDL_CreateTextureFromSurface(renderer, face);
   SDL_FreeSurface(face);

   return tface;
}


/**
 * @brief Test to see if can set texture colour mode.
 */
static int render_hasTexColor (void)
{
   int fail;
   int ret;
   SDL_Texture *tface;
   Uint8 r, g, b;

   /* Get test face. */
   tface = render_loadTestFace();
   if (tface == 0)
      return 0;

   /* See if supported. */
   fail = 0;
   ret = SDL_SetTextureColorMod( tface, 100, 100, 100 );
   if (!render_isSupported(ret))
      fail = 1;
   ret = SDL_GetTextureColorMod( tface, &r, &g, &b );
   if (!render_isSupported(ret))
      fail = 1;

   /* Clean up. */
   SDL_DestroyTexture( tface );

   if (fail)
      return 0;
   else if ((r != 100) || (g != 100) || (b != 100))
      return 0;
   return 1;
}


/**
 * @brief Test to see if we can vary the alpha of the texture.
 */
static int render_hasTexAlpha (void)
{
   int fail;
   int ret;
   SDL_Texture *tface;
   Uint8 a;

   /* Get test face. */
   tface = render_loadTestFace();
   if (tface == 0)
      return 0;

   /* See if supported. */
   fail = 0;
   ret = SDL_SetTextureAlphaMod( tface, 100 );
   if (!render_isSupported(ret))
      fail = 1;
   ret = SDL_GetTextureAlphaMod( tface, &a );
   if (!render_isSupported(ret))
      fail = 1;

   /* Clean up. */
   SDL_DestroyTexture( tface );

   if (fail)
      return 0;
   else if (a != 100)
      return 0;
   return 1;
}


/**
 * @brief Clears the screen.
 *
 * @note We don't test for errors, but they shouldn't happen.
 */
static int render_clearScreen (void)
{
   int ret;

   /* Set colour. */
   ret = SDL_SetRenderDrawColor(renderer, 0, 0, 0, SDL_ALPHA_OPAQUE );
   /*
   if (SDL_ATassert( "SDL_SetRenderDrawColor", ret == 0))
      return -1;
   */

   /* Clear screen. */
   ret = SDL_RenderFillRect(renderer, NULL );
   /*
   if (SDL_ATassert( "SDL_RenderFillRect", ret == 0))
      return -1;
   */

   /* Set defaults. */
   ret = SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE );
   /*
   if (SDL_ATassert( "SDL_SetRenderDrawBlendMode", ret == 0))
      return -1;
   */
   ret = SDL_SetRenderDrawColor(renderer, 255, 255, 255, SDL_ALPHA_OPAQUE );
   /*
   if (SDL_ATassert( "SDL_SetRenderDrawColor", ret == 0))
      return -1;
   */

   return 0;
}


/**
 * @brief Tests the SDL primitives for rendering.
 */
static int render_testPrimitives (void)
{
   int ret;
   int x, y;
   SDL_Rect rect;

   /* Clear surface. */
   if (render_clearScreen())
      return -1;

   /* Need drawcolour or just skip test. */
   if (!render_hasDrawColor())
      return 0;

   /* Draw a rectangle. */
   rect.x = 40;
   rect.y = 0;
   rect.w = 40;
   rect.h = 80;
   ret = SDL_SetRenderDrawColor(renderer, 13, 73, 200, SDL_ALPHA_OPAQUE );
   if (SDL_ATassert( "SDL_SetRenderDrawColor", ret == 0))
      return -1;
   ret = SDL_RenderFillRect(renderer, &rect );
   if (SDL_ATassert( "SDL_RenderFillRect", ret == 0))
      return -1;

   /* Draw a rectangle. */
   rect.x = 10;
   rect.y = 10;
   rect.w = 60;
   rect.h = 40;
   ret = SDL_SetRenderDrawColor(renderer, 200, 0, 100, SDL_ALPHA_OPAQUE );
   if (SDL_ATassert( "SDL_SetRenderDrawColor", ret == 0))
      return -1;
   ret = SDL_RenderFillRect(renderer, &rect );
   if (SDL_ATassert( "SDL_RenderFillRect", ret == 0))
      return -1;

   /* Draw some points like so:
    * X.X.X.X..
    * .X.X.X.X.
    * X.X.X.X.. */
   for (y=0; y<3; y++) {
      x = y % 2;
      for (; x<80; x+=2) {
         ret = SDL_SetRenderDrawColor(renderer, x*y, x*y/2, x*y/3, SDL_ALPHA_OPAQUE );
         if (SDL_ATassert( "SDL_SetRenderDrawColor", ret == 0))
            return -1;
         ret = SDL_RenderDrawPoint(renderer, x, y );
         if (SDL_ATassert( "SDL_RenderDrawPoint", ret == 0))
            return -1;
      }
   }

   /* Draw some lines. */
   ret = SDL_SetRenderDrawColor(renderer, 0, 255, 0, SDL_ALPHA_OPAQUE );
   if (SDL_ATassert( "SDL_SetRenderDrawColor", ret == 0))
      return -1;
   ret = SDL_RenderDrawLine(renderer, 0, 30, 80, 30 );
   if (SDL_ATassert( "SDL_RenderDrawLine", ret == 0))
      return -1;
   ret = SDL_SetRenderDrawColor(renderer, 55, 55, 5, SDL_ALPHA_OPAQUE );
   if (SDL_ATassert( "SDL_SetRenderDrawColor", ret == 0))
      return -1;
   ret = SDL_RenderDrawLine(renderer, 40, 30, 40, 60 );
   if (SDL_ATassert( "SDL_RenderDrawLine", ret == 0))
      return -1;
   ret = SDL_SetRenderDrawColor(renderer, 5, 105, 105, SDL_ALPHA_OPAQUE );
   if (SDL_ATassert( "SDL_SetRenderDrawColor", ret == 0))
      return -1;
   ret = SDL_RenderDrawLine(renderer, 0, 0, 29, 29 );
   if (SDL_ATassert( "SDL_RenderDrawLine", ret == 0))
      return -1;
   ret = SDL_RenderDrawLine(renderer, 29, 30, 0, 59 );
   if (SDL_ATassert( "SDL_RenderDrawLine", ret == 0))
      return -1;
   ret = SDL_RenderDrawLine(renderer, 79, 0, 50, 29 );
   if (SDL_ATassert( "SDL_RenderDrawLine", ret == 0))
      return -1;
   ret = SDL_RenderDrawLine(renderer, 79, 59, 50, 30 );
   if (SDL_ATassert( "SDL_RenderDrawLine", ret == 0))
      return -1;

   /* See if it's the same. */
   if (render_compare( "Primitives output not the same.", &img_primitives, ALLOWABLE_ERROR_OPAQUE ))
      return -1;

   return 0;
}


/**
 * @brief Tests the SDL primitives with alpha for rendering.
 */
static int render_testPrimitivesBlend (void)
{
   int ret;
   int i, j;
   SDL_Rect rect;

   /* Clear surface. */
   if (render_clearScreen())
      return -1;

   /* Need drawcolour and blendmode or just skip test. */
   if (!render_hasDrawColor() || !render_hasBlendModes())
      return 0;

   /* Create some rectangles for each blend mode. */
   ret = SDL_SetRenderDrawColor(renderer, 255, 255, 255, 0 );
   if (SDL_ATassert( "SDL_SetRenderDrawColor", ret == 0))
      return -1;
   ret = SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE );
   if (SDL_ATassert( "SDL_SetRenderDrawBlendMode", ret == 0))
      return -1;
   ret = SDL_RenderFillRect(renderer, NULL );
   if (SDL_ATassert( "SDL_RenderFillRect", ret == 0))
      return -1;
   rect.x = 10;
   rect.y = 25;
   rect.w = 40;
   rect.h = 25;
   ret = SDL_SetRenderDrawColor(renderer, 240, 10, 10, 75 );
   if (SDL_ATassert( "SDL_SetRenderDrawColor", ret == 0))
      return -1;
   ret = SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_ADD );
   if (SDL_ATassert( "SDL_SetRenderDrawBlendMode", ret == 0))
      return -1;
   ret = SDL_RenderFillRect(renderer, &rect );
   if (SDL_ATassert( "SDL_RenderFillRect", ret == 0))
      return -1;
   rect.x = 30;
   rect.y = 40;
   rect.w = 45;
   rect.h = 15;
   ret = SDL_SetRenderDrawColor(renderer, 10, 240, 10, 100 );
   if (SDL_ATassert( "SDL_SetRenderDrawColor", ret == 0))
      return -1;
   ret = SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND );
   if (SDL_ATassert( "SDL_SetRenderDrawBlendMode", ret == 0))
      return -1;
   ret = SDL_RenderFillRect(renderer, &rect );
   if (SDL_ATassert( "SDL_RenderFillRect", ret == 0))
      return -1;
   rect.x = 25;
   rect.y = 25;
   rect.w = 25;
   rect.h = 25;
   ret = SDL_SetRenderDrawColor(renderer, 10, 10, 240, 125 );
   if (SDL_ATassert( "SDL_SetRenderDrawColor", ret == 0))
      return -1;
   ret = SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE );
   if (SDL_ATassert( "SDL_SetRenderDrawBlendMode", ret == 0))
      return -1;
   ret = SDL_RenderFillRect(renderer, &rect );
   if (SDL_ATassert( "SDL_RenderFillRect", ret == 0))
      return -1;

   /* Draw blended lines, lines for everyone. */
   for (i=0; i<SCREEN_W; i+=2)  {
      ret = SDL_SetRenderDrawColor(renderer, 60+2*i, 240-2*i, 50, 3*i );
      if (SDL_ATassert( "SDL_SetRenderDrawColor", ret == 0))
         return -1;
      ret = SDL_SetRenderDrawBlendMode(renderer,(((i/2)%3)==0) ? SDL_BLENDMODE_BLEND :
            (((i/2)%3)==1) ? SDL_BLENDMODE_ADD : SDL_BLENDMODE_NONE );
      if (SDL_ATassert( "SDL_SetRenderDrawBlendMode", ret == 0))
         return -1;
      ret = SDL_RenderDrawLine(renderer, 0, 0, i, 59 );
      if (SDL_ATassert( "SDL_RenderDrawLine", ret == 0))
         return -1;
   }
   for (i=0; i<SCREEN_H; i+=2)  {
      ret = SDL_SetRenderDrawColor(renderer, 60+2*i, 240-2*i, 50, 3*i );
      if (SDL_ATassert( "SDL_SetRenderDrawColor", ret == 0))
         return -1;
      ret = SDL_SetRenderDrawBlendMode(renderer,(((i/2)%3)==0) ? SDL_BLENDMODE_BLEND :
            (((i/2)%3)==1) ? SDL_BLENDMODE_ADD : SDL_BLENDMODE_NONE );
      if (SDL_ATassert( "SDL_SetRenderDrawBlendMode", ret == 0))
         return -1;
      ret = SDL_RenderDrawLine(renderer, 0, 0, 79, i );
      if (SDL_ATassert( "SDL_RenderDrawLine", ret == 0))
         return -1;
   }

   /* Draw points. */
   for (j=0; j<SCREEN_H; j+=3) {
      for (i=0; i<SCREEN_W; i+=3) {
         ret = SDL_SetRenderDrawColor(renderer, j*4, i*3, j*4, i*3 );
         if (SDL_ATassert( "SDL_SetRenderDrawColor", ret == 0))
            return -1;
         ret = SDL_SetRenderDrawBlendMode(renderer, ((((i+j)/3)%3)==0) ? SDL_BLENDMODE_BLEND :
               ((((i+j)/3)%3)==1) ? SDL_BLENDMODE_ADD : SDL_BLENDMODE_NONE );
         if (SDL_ATassert( "SDL_SetRenderDrawBlendMode", ret == 0))
            return -1;
         ret = SDL_RenderDrawPoint(renderer, i, j );
         if (SDL_ATassert( "SDL_RenderDrawPoint", ret == 0))
            return -1;
      }
   }

   /* See if it's the same. */
   if (render_compare( "Blended primitives output not the same.", &img_blend, ALLOWABLE_ERROR_BLENDED ))
      return -1;

   return 0;
}


/**
 * @brief Tests some blitting routines.
 */
static int render_testBlit (void)
{
   int ret;
   SDL_Rect rect;
   SDL_Texture *tface;
   int i, j, ni, nj;

   /* Clear surface. */
   if (render_clearScreen())
      return -1;

   /* Need drawcolour or just skip test. */
   if (!render_hasDrawColor())
      return 0;

   /* Create face surface. */
   tface = render_loadTestFace();
   if (SDL_ATassert( "render_loadTestFace()", tface != 0))
      return -1;

   /* Constant values. */
   rect.w = img_face.width;
   rect.h = img_face.height;
   ni     = SCREEN_W - img_face.width;
   nj     = SCREEN_H - img_face.height;

   /* Loop blit. */
   for (j=0; j <= nj; j+=4) {
      for (i=0; i <= ni; i+=4) {
         /* Blitting. */
         rect.x = i;
         rect.y = j;
         ret = SDL_RenderCopy(renderer, tface, NULL, &rect );
         if (SDL_ATassert( "SDL_RenderCopy", ret == 0))
            return -1;
      }
   }

   /* Clean up. */
   SDL_DestroyTexture( tface );

   /* See if it's the same. */
   if (render_compare( "Blit output not the same.", &img_blit, ALLOWABLE_ERROR_OPAQUE ))
      return -1;

   return 0;
}


/**
 * @brief Blits doing colour tests.
 */
static int render_testBlitColour (void)
{
   int ret;
   SDL_Rect rect;
   SDL_Texture *tface;
   int i, j, ni, nj;

   /* Clear surface. */
   if (render_clearScreen())
      return -1;

   /* Need drawcolour or just skip test. */
   if (!render_hasTexColor())
      return 0;

   /* Create face surface. */
   tface = render_loadTestFace();
   if (SDL_ATassert( "render_loadTestFace()", tface != 0))
      return -1;

   /* Constant values. */
   rect.w = img_face.width;
   rect.h = img_face.height;
   ni     = SCREEN_W - img_face.width;
   nj     = SCREEN_H - img_face.height;

   /* Test blitting with colour mod. */
   for (j=0; j <= nj; j+=4) {
      for (i=0; i <= ni; i+=4) {
         /* Set colour mod. */
         ret = SDL_SetTextureColorMod( tface, (255/nj)*j, (255/ni)*i, (255/nj)*j );
         if (SDL_ATassert( "SDL_SetTextureColorMod", ret == 0))
            return -1;

         /* Blitting. */
         rect.x = i;
         rect.y = j;
         ret = SDL_RenderCopy(renderer, tface, NULL, &rect );
         if (SDL_ATassert( "SDL_RenderCopy", ret == 0))
            return -1;
      }
   }

   /* Clean up. */
   SDL_DestroyTexture( tface );

   /* See if it's the same. */
   if (render_compare( "Blit output not the same (using SDL_SetTextureColorMod).",
            &img_blitColour, ALLOWABLE_ERROR_OPAQUE ))
      return -1;

   return 0;
}


/**
 * @brief Tests blitting with alpha.
 */
static int render_testBlitAlpha (void)
{
   int ret;
   SDL_Rect rect;
   SDL_Texture *tface;
   int i, j, ni, nj;

   /* Clear surface. */
   if (render_clearScreen())
      return -1;

   /* Need alpha or just skip test. */
   if (!render_hasTexAlpha())
      return 0;

   /* Create face surface. */
   tface = render_loadTestFace();
   if (SDL_ATassert( "render_loadTestFace()", tface != 0))
      return -1;

   /* Constant values. */
   rect.w = img_face.width;
   rect.h = img_face.height;
   ni     = SCREEN_W - img_face.width;
   nj     = SCREEN_H - img_face.height;

   /* Clear surface. */
   if (render_clearScreen())
      return -1;

   /* Test blitting with alpha mod. */
   for (j=0; j <= nj; j+=4) {
      for (i=0; i <= ni; i+=4) {
         /* Set alpha mod. */
         ret = SDL_SetTextureAlphaMod( tface, (255/ni)*i );
         if (SDL_ATassert( "SDL_SetTextureAlphaMod", ret == 0))
            return -1;

         /* Blitting. */
         rect.x = i;
         rect.y = j;
         ret = SDL_RenderCopy(renderer, tface, NULL, &rect );
         if (SDL_ATassert( "SDL_RenderCopy", ret == 0))
            return -1;
      }
   }

   /* Clean up. */
   SDL_DestroyTexture( tface );

   /* See if it's the same. */
   if (render_compare( "Blit output not the same (using SDL_SetSurfaceAlphaMod).",
            &img_blitAlpha, ALLOWABLE_ERROR_BLENDED ))
      return -1;

   return 0;
}


/**
 * @brief Tests a blend mode.
 */
static int render_testBlitBlendMode( SDL_Texture * tface, int mode )
{
   int ret;
   int i, j, ni, nj;
   SDL_Rect rect;

   /* Clear surface. */
   if (render_clearScreen())
      return -1;

   /* Steps to take. */
   ni     = SCREEN_W - FACE_W;
   nj     = SCREEN_H - FACE_H;

   /* Constant values. */
   rect.w = FACE_W;
   rect.h = FACE_H;

   /* Test blend mode. */
   for (j=0; j <= nj; j+=4) {
      for (i=0; i <= ni; i+=4) {
         /* Set blend mode. */
         ret = SDL_SetTextureBlendMode( tface, mode );
         if (SDL_ATassert( "SDL_SetTextureBlendMode", ret == 0))
            return -1;

         /* Blitting. */
         rect.x = i;
         rect.y = j;
         ret = SDL_RenderCopy(renderer, tface, NULL, &rect );
         if (SDL_ATassert( "SDL_RenderCopy", ret == 0))
            return -1;
      }
   }

   return 0;
}


/**
 * @brief Tests some more blitting routines.
 */
static int render_testBlitBlend (void)
{
   int ret;
   SDL_Rect rect;
   SDL_Texture *tface;
   int i, j, ni, nj;
   int mode;

   /* Clear surface. */
   if (render_clearScreen())
      return -1;

   /* Need drawcolour and blendmode or just skip test. */
   if (!render_hasBlendModes() || !render_hasTexColor() || !render_hasTexAlpha())
      return 0;

   /* Create face surface. */
   tface = render_loadTestFace();
   if (SDL_ATassert( "render_loadTestFace()", tface != 0))
      return -1;

   /* Steps to take. */
   ni     = SCREEN_W - FACE_W;
   nj     = SCREEN_H - FACE_H;

   /* Constant values. */
   rect.w = img_face.width;
   rect.h = img_face.height;

   /* Set alpha mod. */
   ret = SDL_SetTextureAlphaMod( tface, 100 );
   if (SDL_ATassert( "SDL_SetTextureAlphaMod", ret == 0))
      return -1;

   /* Test None. */
   if (render_testBlitBlendMode( tface, SDL_BLENDMODE_NONE ))
      return -1;
   /* See if it's the same. */
   if (render_compare( "Blit blending output not the same (using SDL_BLENDMODE_NONE).",
            &img_blendNone, ALLOWABLE_ERROR_OPAQUE ))
      return -1;

   /* Test Blend. */
   if (render_testBlitBlendMode( tface, SDL_BLENDMODE_BLEND ))
      return -1;
   if (render_compare( "Blit blending output not the same (using SDL_BLENDMODE_BLEND).",
            &img_blendBlend, ALLOWABLE_ERROR_BLENDED ))
      return -1;

   /* Test Add. */
   if (render_testBlitBlendMode( tface, SDL_BLENDMODE_ADD ))
      return -1;
   if (render_compare( "Blit blending output not the same (using SDL_BLENDMODE_ADD).",
            &img_blendAdd, ALLOWABLE_ERROR_BLENDED ))
      return -1;

   /* Test Mod. */
   if (render_testBlitBlendMode( tface, SDL_BLENDMODE_MOD ))
      return -1;
   if (render_compare( "Blit blending output not the same (using SDL_BLENDMODE_MOD).",
            &img_blendMod, ALLOWABLE_ERROR_BLENDED ))
      return -1;

   /* Clear surface. */
   if (render_clearScreen())
      return -1;

   /* Loop blit. */
   for (j=0; j <= nj; j+=4) {
      for (i=0; i <= ni; i+=4) {

         /* Set colour mod. */
         ret = SDL_SetTextureColorMod( tface, (255/nj)*j, (255/ni)*i, (255/nj)*j );
         if (SDL_ATassert( "SDL_SetTextureColorMod", ret == 0))
            return -1;

         /* Set alpha mod. */
         ret = SDL_SetTextureAlphaMod( tface, (100/ni)*i );
         if (SDL_ATassert( "SDL_SetTextureAlphaMod", ret == 0))
            return -1;

         /* Crazy blending mode magic. */
         mode = (i/4*j/4) % 4;
         if (mode==0) mode = SDL_BLENDMODE_NONE;
         else if (mode==1) mode = SDL_BLENDMODE_BLEND;
         else if (mode==2) mode = SDL_BLENDMODE_ADD;
         else if (mode==3) mode = SDL_BLENDMODE_MOD;
         ret = SDL_SetTextureBlendMode( tface, mode );
         if (SDL_ATassert( "SDL_SetTextureBlendMode", ret == 0))
            return -1;

         /* Blitting. */
         rect.x = i;
         rect.y = j;
         ret = SDL_RenderCopy(renderer, tface, NULL, &rect );
         if (SDL_ATassert( "SDL_RenderCopy", ret == 0))
            return -1;
      }
   }

   /* Clean up. */
   SDL_DestroyTexture( tface );

   /* Check to see if matches. */
   if (render_compare( "Blit blending output not the same (using SDL_BLENDMODE_*).",
            &img_blendAll, ALLOWABLE_ERROR_BLENDED ))
      return -1;

   return 0;
}


/**
 * @brief Runs all the tests on the surface.
 *
 *    @return 0 on success.
 */
int render_runTests (void)
{
   int ret;

   /* No error. */
   ret = 0;

   /* Test functionality first. */
   if (render_hasDrawColor())
      SDL_ATprintVerbose( 1, "      Draw Color supported\n" );
   if (render_hasBlendModes())
      SDL_ATprintVerbose( 1, "      Blend Modes supported\n" );
   if (render_hasTexColor())
      SDL_ATprintVerbose( 1, "      Texture Color Mod supported\n" );
   if (render_hasTexAlpha())
      SDL_ATprintVerbose( 1, "      Texture Alpha Mod supported\n" );

   /* Software surface blitting. */
   ret = render_testPrimitives();
   if (ret)
      return -1;
   ret = render_testPrimitivesBlend();
   if (ret)
      return -1;
   ret = render_testBlit();
   if (ret)
      return -1;
   ret = render_testBlitColour();
   if (ret)
      return -1;
   ret = render_testBlitAlpha();
   if (ret)
      return -1;
   ret = render_testBlitBlend();

   return ret;
}


/**
 * @brief Entry point.
 *
 * This testsuite is tricky, we're creating a testsuite per driver, the thing
 *  is we do quite a of stuff outside of the actual testcase which *could*
 *  give issues. Don't like that very much, but no way around without creating
 *  superfluous testsuites.
 */
#ifdef TEST_STANDALONE
int main( int argc, const char *argv[] )
{
   (void) argc;
   (void) argv;
#else /* TEST_STANDALONE */
int test_render (void)
{
#endif /* TEST_STANDALONE */
   int failed;
   int i, j, nd, nr;
   int ret;
   const char *driver, *str;
   char msg[256];
   SDL_Window *w;
   SDL_RendererInfo info;

   /* Initializes the SDL subsystems. */
   ret = SDL_Init(0);
   if (ret != 0)
      return -1;

   /* Get number of drivers. */
   nd = SDL_GetNumVideoDrivers();
   if (nd < 0)
      goto err;
   SDL_ATprintVerbose( 1, "%d Video Drivers found\n", nd );

   /* Now run on the video mode. */
   ret = SDL_InitSubSystem( SDL_INIT_VIDEO );
   if (ret != 0)
      goto err;

   /*
    * Surface on video mode tests.
    */
   /* Run for all video modes. */
   failed = 0;
   for (i=0; i<nd; i++) {
      /* Get video mode. */
      driver = SDL_GetVideoDriver(i);
      if (driver == NULL)
         goto err;
      SDL_ATprintVerbose( 1, " %d) %s\n", i+1, driver );

      /*
       * Initialize testsuite.
       */
      SDL_snprintf( msg, sizeof(msg) , "Rendering with %s driver", driver );
      SDL_ATinit( msg );

      /*
       * Initialize.
       */
      SDL_ATbegin( "Initializing video mode" );
      /* Initialize video mode. */
      ret = SDL_VideoInit( driver );
      if (SDL_ATvassert( ret==0, "SDL_VideoInit( %s, 0 )", driver ))
         goto err_cleanup;
      /* Check to see if it's the one we want. */
      str = SDL_GetCurrentVideoDriver();
      if (SDL_ATassert( "SDL_GetCurrentVideoDriver", SDL_strcmp(driver,str)==0))
         goto err_cleanup;
      /* Create window. */
      w = SDL_CreateWindow( msg, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
            80, 60, SDL_WINDOW_SHOWN );
      if (SDL_ATassert( "SDL_CreateWindow", w!=NULL ))
         goto err_cleanup;
      /* Check title. */
      str = SDL_GetWindowTitle( w );
      if (SDL_ATassert( "SDL_GetWindowTitle", SDL_strcmp(msg,str)==0))
         goto err_cleanup;
      /* Get renderers. */
      nr = SDL_GetNumRenderDrivers();
      if (SDL_ATassert("SDL_GetNumRenderDrivers", nr>=0))
         goto err_cleanup;
      SDL_ATprintVerbose( 1, "   %d Render Drivers\n", nr );
      SDL_ATend();
      for (j=0; j<nr; j++) {

         /* We have to recreate window each time, because opengl and opengles renderers */
         /* both add SDL_WINDOW_OPENGL flag for window, that was last used              */
         SDL_DestroyWindow(w);
         w = SDL_CreateWindow( msg, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
               80, 60, SDL_WINDOW_SHOWN );
         if (SDL_ATassert( "SDL_CreateWindow", w!=NULL ))
            goto err_cleanup;

         /* Get renderer info. */
         ret = SDL_GetRenderDriverInfo( j, &info );
         if (ret != 0)
            goto err_cleanup;

         /* Set testcase name. */
         SDL_snprintf( msg, sizeof(msg), "Renderer %s", info.name );
         SDL_ATprintVerbose( 1, "    %d) %s\n", j+1, info.name );
         SDL_ATbegin( msg );

         /* Set renderer. */
         renderer = SDL_CreateRenderer( w, j, 0 );
         if (SDL_ATassert( "SDL_CreateRenderer", renderer!=0 ))
            goto err_cleanup;

         /*
          * Run tests.
          */
         ret = render_runTests();

         if (ret)
            continue;
         SDL_ATend();
      }

      /* Exit the current renderer. */
      SDL_VideoQuit();

      /*
       * Finish testsuite.
       */
      failed += SDL_ATfinish();
   }

   /* Exit SDL. */
   SDL_Quit();

   return failed;

err_cleanup:
   SDL_ATfinish();

err:
   return 1;
}


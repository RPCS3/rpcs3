/**
 * Automated SDL_Surface test.
 *
 * Written by Edgar Simo "bobbens"
 *
 * Released under Public Domain.
 */


#include "SDL.h"
#include "../SDL_at.h"

#include "common.h"


/**
 * @brief Compares a surface and a surface image for equality.
 */
int surface_compare( SDL_Surface *sur, const SurfaceImage_t *img, int allowable_error )
{
   int ret;
   int i,j;
   int bpp;
   Uint8 *p, *pd;

   /* Make sure size is the same. */
   if ((sur->w != img->width) || (sur->h != img->height))
      return -1;

   SDL_LockSurface( sur );

   ret = 0;
   bpp = sur->format->BytesPerPixel;

   /* Compare image - should be same format. */
   for (j=0; j<sur->h; j++) {
      for (i=0; i<sur->w; i++) {
         p  = (Uint8 *)sur->pixels + j * sur->pitch + i * bpp;
         pd = (Uint8 *)img->pixel_data + (j*img->width + i) * img->bytes_per_pixel;
         switch (bpp) {
            case 1:
            case 2:
            case 3:
               ret += 1;
               /*printf("%d BPP not supported yet.\n",bpp);*/
               break;

            case 4:
               {
                  int dist = 0;
                  Uint8 R, G, B, A;

                  SDL_GetRGBA(*(Uint32*)p, sur->format, &R, &G, &B, &A);

                  if (img->bytes_per_pixel == 3) {
                     dist += (R-pd[0])*(R-pd[0]);
                     dist += (G-pd[1])*(G-pd[1]);
                     dist += (B-pd[2])*(B-pd[2]);
                  } else {
                     dist += (R-pd[0])*(R-pd[0]);
                     dist += (G-pd[1])*(G-pd[1]);
                     dist += (B-pd[2])*(B-pd[2]);
                     dist += (A-pd[3])*(A-pd[3]);
                  }
                  /* Allow some difference in blending accuracy */
                  if (dist > allowable_error) {
                     /*printf("pixel %d,%d varies by %d\n", i, j, dist);*/
                     ++ret;
                  }
               }
               break;
         }
      }
   }

   SDL_UnlockSurface( sur );

   if (ret) {
      SDL_SaveBMP(sur, "fail.bmp");

      SDL_LockSurface( sur );

      bpp = sur->format->BytesPerPixel;

      if (bpp == 4) {
         for (j=0; j<sur->h; j++) {
            for (i=0; i<sur->w; i++) {
               Uint8 R, G, B, A;
               p  = (Uint8 *)sur->pixels + j * sur->pitch + i * bpp;
               pd = (Uint8 *)img->pixel_data + (j*img->width + i) * img->bytes_per_pixel;

               R = pd[0];
               G = pd[1];
               B = pd[2];
               if (img->bytes_per_pixel == 4) {
                  A = pd[3];
               } else {
                  A = 0;
               }
               *(Uint32*)p = (A << 24) | (R << 16) | (G << 8) | B;
            }
         }
      }

      SDL_UnlockSurface( sur );

      SDL_SaveBMP(sur, "good.bmp");
   }
   return ret;
}

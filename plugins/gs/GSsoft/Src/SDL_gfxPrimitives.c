/* 

  SDL_gfxPrimitives - Graphics primitives for SDL surfaces

  Note: Does not lock or update surfaces. Implement in calling routine.

*/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#include <SDL/SDL.h>

#include "SDL_gfxPrimitives.h"
#include "SDL_gfxPrimitives_font.h"

/* -===================- */

/* Define this flag to use surface blits for alpha blended drawing. */
/* This is usually slower that direct surface calculations.         */

#undef SURFACE_ALPHA_PIXEL

/* ----- Defines for pixel clipping tests */

#define clip_xmin(surface) surface->clip_rect.x
#define clip_xmax(surface) surface->clip_rect.x+surface->clip_rect.w-1
#define clip_ymin(surface) surface->clip_rect.y
#define clip_ymax(surface) surface->clip_rect.y+surface->clip_rect.h-1

/* ----- Pixel - fast, no blending, no locking, clipping */

int fastPixelColorNolock (SDL_Surface *dst, Sint16 x, Sint16 y, Uint32 color)
{
  int bpp; 
  Uint8 *p; 
      
  /* Honor clipping setup at pixel level */
  if ( (x >= clip_xmin(dst)) && 
       (x <= clip_xmax(dst)) && 
       (y >= clip_ymin(dst)) && 
       (y <= clip_ymax(dst)) ) {

    /* Get destination format */
    bpp = dst->format->BytesPerPixel;
    p = (Uint8 *)dst->pixels + y * dst->pitch + x * bpp;
    switch(bpp) {
    case 1:
        *p = color;
        break;
    case 2:
        *(Uint16 *)p = color;
        break;
    case 3:
        if(SDL_BYTEORDER == SDL_BIG_ENDIAN) {
            p[0] = (color >> 16) & 0xff;
            p[1] = (color >> 8) & 0xff;
            p[2] = color & 0xff;
        } else {
            p[0] = color & 0xff;
            p[1] = (color >> 8) & 0xff;
            p[2] = (color >> 16) & 0xff;
        }
        break;
    case 4:
        *(Uint32 *)p = color;
        break;
    } /* switch */
    

  }
  
  return(0);
}

/* ----- Pixel - fast, no blending, no locking, no clipping */

/* (faster but dangerous, make sure we stay in surface bounds) */

int fastPixelColorNolockNoclip (SDL_Surface *dst, Sint16 x, Sint16 y, Uint32 color)
{
  int bpp; 
  Uint8 *p; 
      
  /* Get destination format */
  bpp = dst->format->BytesPerPixel;
  p = (Uint8 *)dst->pixels + y * dst->pitch + x * bpp;
  switch(bpp) {
    case 1:
        *p = color;
        break;
    case 2:
        *(Uint16 *)p = color;
        break;
    case 3:
        if(SDL_BYTEORDER == SDL_BIG_ENDIAN) {
            p[0] = (color >> 16) & 0xff;
            p[1] = (color >> 8) & 0xff;
            p[2] = color & 0xff;
        } else {
            p[0] = color & 0xff;
            p[1] = (color >> 8) & 0xff;
            p[2] = (color >> 16) & 0xff;
        }
        break;
    case 4:
        *(Uint32 *)p = color;
        break;
  } /* switch */
  
  return(0);
}

/* ----- Pixel - fast, no blending, locking, clipping */

int fastPixelColor (SDL_Surface *dst, Sint16 x, Sint16 y, Uint32 color)
{
 int result;
 
 /* Lock the surface */
 if ( SDL_MUSTLOCK(dst) ) {
  if ( SDL_LockSurface(dst) < 0 ) {
   return(-1);
  }
 }
 
 result=fastPixelColorNolock (dst,x,y,color);
 
 /* Unlock surface */
 if ( SDL_MUSTLOCK(dst) ) {
  SDL_UnlockSurface (dst);
 }

 return(result);
}

/* ----- Pixel - fast, no blending, locking, RGB input */

int fastPixelRGBA (SDL_Surface *dst, Sint16 x, Sint16 y, Uint8 r, Uint8 g, Uint8 b, Uint8 a) 
{
 Uint32 color;
 
 /* Setup color */
 color=SDL_MapRGBA(dst->format, r, g, b, a);

 /* Draw */
 return(fastPixelColor(dst, x, y, color)); 

}

/* ----- Pixel - fast, no blending, no locking RGB input */

int fastPixelRGBANolock (SDL_Surface *dst, Sint16 x, Sint16 y, Uint8 r, Uint8 g, Uint8 b, Uint8 a) 
{
 Uint32 color;
 
 /* Setup color */
 color=SDL_MapRGBA(dst->format, r, g, b, a);

 /* Draw */
 return(fastPixelColorNolock(dst, x, y, color)); 
}

#ifdef SURFACE_ALPHA_PIXEL

/* ----- Pixel - using single pixel blit with blending enabled if a<255 */

static SDL_Surface *gfxPrimitivesSinglePixel=NULL;

int pixelColor (SDL_Surface *dst, Sint16 x, Sint16 y, Uint32 color) 
{ 
 SDL_Rect srect;
 SDL_Rect drect;
 int result;
  
 /* Setup source rectangle for pixel */
 srect.x=0;
 srect.y=0;
 srect.w=1;
 srect.h=1;
 
 /* Setup destination rectangle for pixel */
 drect.x=x;
 drect.y=y;
 drect.w=1;
 drect.h=1;

 /* Create single pixel in 32bit RGBA format */
 if (gfxPrimitivesSinglePixel==NULL) {
  gfxPrimitivesSinglePixel=SDL_CreateRGBSurface(SDL_SWSURFACE | SDL_HWSURFACE | SDL_SRCALPHA, 1, 1, 32, 0xFF000000, 0x00FF0000, 0x0000FF00, 0x000000FF);
 }

 /* Draw color into pixel*/
 SDL_FillRect (gfxPrimitivesSinglePixel, &srect, color);
 
 /* Draw pixel onto destination surface */
 result=SDL_BlitSurface (gfxPrimitivesSinglePixel, &srect, dst, &drect);

 return(result);
}

#else

/* PutPixel routine with alpha blending, input color in destination format */

int _putPixelAlpha(SDL_Surface *surface, Sint16 x, Sint16 y, Uint32 color, Uint8 alpha)
{
 Uint32 Rmask = surface->format->Rmask, Gmask = surface->format->Gmask, Bmask = surface->format->Bmask, Amask = surface->format->Amask;
 Uint32 R,G,B,A = 0;
	
 if (x>=clip_xmin(surface) && x<=clip_xmax(surface) && y>=clip_ymin(surface) && y<=clip_ymax(surface)) {

  switch (surface->format->BytesPerPixel) {
			case 1: { /* Assuming 8-bpp */
				if( alpha == 255 ){
					*((Uint8 *)surface->pixels + y*surface->pitch + x) = color;
				} else {
					Uint8 *pixel = (Uint8 *)surface->pixels + y*surface->pitch + x;
					
					Uint8 dR = surface->format->palette->colors[*pixel].r;
					Uint8 dG = surface->format->palette->colors[*pixel].g;
					Uint8 dB = surface->format->palette->colors[*pixel].b;
					Uint8 sR = surface->format->palette->colors[color].r;
					Uint8 sG = surface->format->palette->colors[color].g;
					Uint8 sB = surface->format->palette->colors[color].b;
					
					dR = dR + ((sR-dR)*alpha >> 8);
					dG = dG + ((sG-dG)*alpha >> 8);
					dB = dB + ((sB-dB)*alpha >> 8);
				
					*pixel = SDL_MapRGB(surface->format, dR, dG, dB);
				}
			}
			break;

			case 2: { /* Probably 15-bpp or 16-bpp */		
				if ( alpha == 255 ) {
					*((Uint16 *)surface->pixels + y*surface->pitch/2 + x) = color;
				} else {
					Uint16 *pixel = (Uint16 *)surface->pixels + y*surface->pitch/2 + x;
					Uint32 dc = *pixel;
				
					R = ((dc & Rmask) + (( (color & Rmask) - (dc & Rmask) ) * alpha >> 8)) & Rmask;
					G = ((dc & Gmask) + (( (color & Gmask) - (dc & Gmask) ) * alpha >> 8)) & Gmask;
					B = ((dc & Bmask) + (( (color & Bmask) - (dc & Bmask) ) * alpha >> 8)) & Bmask;
					if( Amask )
						A = ((dc & Amask) + (( (color & Amask) - (dc & Amask) ) * alpha >> 8)) & Amask;

					*pixel= R | G | B | A;
				}
			}
			break;

			case 3: { /* Slow 24-bpp mode, usually not used */
				Uint8 *pix = (Uint8 *)surface->pixels + y * surface->pitch + x*3;
				Uint8 rshift8=surface->format->Rshift/8;
				Uint8 gshift8=surface->format->Gshift/8;
				Uint8 bshift8=surface->format->Bshift/8;
				Uint8 ashift8=surface->format->Ashift/8;
				
				
				if ( alpha == 255 ) {
  					*(pix+rshift8) = color>>surface->format->Rshift;
  					*(pix+gshift8) = color>>surface->format->Gshift;
  					*(pix+bshift8) = color>>surface->format->Bshift;
					*(pix+ashift8) = color>>surface->format->Ashift;
				} else {
					Uint8 dR, dG, dB, dA=0;
					Uint8 sR, sG, sB, sA=0;
					
					pix = (Uint8 *)surface->pixels + y * surface->pitch + x*3;
					
					dR = *((pix)+rshift8); 
            				dG = *((pix)+gshift8);
            				dB = *((pix)+bshift8);
					dA = *((pix)+ashift8);
					
					sR = (color>>surface->format->Rshift)&0xff;
					sG = (color>>surface->format->Gshift)&0xff;
					sB = (color>>surface->format->Bshift)&0xff;
					sA = (color>>surface->format->Ashift)&0xff;
					
					dR = dR + ((sR-dR)*alpha >> 8);
					dG = dG + ((sG-dG)*alpha >> 8);
					dB = dB + ((sB-dB)*alpha >> 8);
					dA = dA + ((sA-dA)*alpha >> 8);

					*((pix)+rshift8) = dR; 
            				*((pix)+gshift8) = dG;
            				*((pix)+bshift8) = dB;
					*((pix)+ashift8) = dA;
				}
			}
			break;

			case 4: { /* Probably 32-bpp */
				if ( alpha == 255 ) {
					*((Uint32 *)surface->pixels + y*surface->pitch/4 + x) = color;
				} else {
					Uint32 *pixel = (Uint32 *)surface->pixels + y*surface->pitch/4 + x;
					Uint32 dc = *pixel;
			
					R = ((dc & Rmask) + (( (color & Rmask) - (dc & Rmask) ) * alpha >> 8)) & Rmask;
					G = ((dc & Gmask) + (( (color & Gmask) - (dc & Gmask) ) * alpha >> 8)) & Gmask;
					B = ((dc & Bmask) + (( (color & Bmask) - (dc & Bmask) ) * alpha >> 8)) & Bmask;
					if ( Amask )
						A = ((dc & Amask) + (( (color & Amask) - (dc & Amask) ) * alpha >> 8)) & Amask;
					
					*pixel = R | G | B | A;
				}
			}
			break;
  }
 }

 return(0);
}

/* ----- Pixel - pixel draw with blending enabled if a<255 */

int pixelColor (SDL_Surface *dst, Sint16 x, Sint16 y, Uint32 color) 
{
 Uint8 alpha;
 Uint32 mcolor;
 int result=0;

 /* Lock the surface */
 if ( SDL_MUSTLOCK(dst) ) {
  if ( SDL_LockSurface(dst) < 0 ) {
   return(-1);
  }
 }
 
 /* Setup color */
 alpha=color &0x000000ff;
 mcolor=SDL_MapRGBA(dst->format, (color & 0xff000000) >> 24, (color & 0x00ff0000) >> 16, (color & 0x0000ff00) >> 8, alpha);

 /* Draw */
 result = _putPixelAlpha(dst,x,y,mcolor,alpha);
	
 /* Unlock the surface */
 if (SDL_MUSTLOCK(dst) ) {
  SDL_UnlockSurface(dst);
 }

 return(result);
}


/* Filled rectangle with alpha blending, color in destination format */

int _filledRectAlpha (SDL_Surface *surface, Sint16 x1, Sint16 y1, Sint16 x2, Sint16 y2, Uint32 color, Uint8 alpha)
{
 Uint32 Rmask = surface->format->Rmask, Gmask = surface->format->Gmask, Bmask = surface->format->Bmask, Amask = surface->format->Amask;
 Uint32 R,G,B,A=0;
 Sint16 x,y;

 switch (surface->format->BytesPerPixel) {
		case 1: { /* Assuming 8-bpp */
			Uint8 *row, *pixel;
			Uint8 dR, dG, dB;
			
			Uint8 sR = surface->format->palette->colors[color].r;
			Uint8 sG = surface->format->palette->colors[color].g;
			Uint8 sB = surface->format->palette->colors[color].b;
			
			for(y = y1; y<=y2; y++){
				row = (Uint8 *)surface->pixels + y*surface->pitch;
				for(x = x1; x <= x2; x++){
					pixel = row + x;
					
					dR = surface->format->palette->colors[*pixel].r;
					dG = surface->format->palette->colors[*pixel].g;
					dB = surface->format->palette->colors[*pixel].b;
					
					dR = dR + ((sR-dR)*alpha >> 8);
					dG = dG + ((sG-dG)*alpha >> 8);
					dB = dB + ((sB-dB)*alpha >> 8);
				
					*pixel = SDL_MapRGB(surface->format, dR, dG, dB);
				}
			}
		}
		break;

		case 2: { /* Probably 15-bpp or 16-bpp */
			Uint16 *row, *pixel;
			Uint32 dR=(color & Rmask),dG=(color & Gmask),dB=(color & Bmask),dA=(color & Amask);
			
			for(y = y1; y<=y2; y++){
				row = (Uint16 *)surface->pixels + y*surface->pitch/2;
				for(x = x1; x <= x2; x++){
					pixel = row + x;

					R = ((*pixel & Rmask) + (( dR - (*pixel & Rmask) ) * alpha >> 8)) & Rmask;
					G = ((*pixel & Gmask) + (( dG - (*pixel & Gmask) ) * alpha >> 8)) & Gmask;
					B = ((*pixel & Bmask) + (( dB - (*pixel & Bmask) ) * alpha >> 8)) & Bmask;
					if( Amask )
						A = ((*pixel & Amask) + (( dA - (*pixel & Amask) ) * alpha >> 8)) & Amask;

					*pixel= R | G | B | A;
				}
			}
		}
		break;

		case 3: { /* Slow 24-bpp mode, usually not used */
			Uint8 *row,*pix;
			Uint8 dR, dG, dB, dA;
  			Uint8 rshift8=surface->format->Rshift/8; 
			Uint8 gshift8=surface->format->Gshift/8; 
			Uint8 bshift8=surface->format->Bshift/8;
			Uint8 ashift8=surface->format->Ashift/8;
			
			Uint8 sR = (color>>surface->format->Rshift)&0xff;
			Uint8 sG = (color>>surface->format->Gshift)&0xff;
			Uint8 sB = (color>>surface->format->Bshift)&0xff;
			Uint8 sA = (color>>surface->format->Ashift)&0xff;
				
			for(y = y1; y<=y2; y++){
				row = (Uint8 *)surface->pixels + y * surface->pitch;
				for(x = x1; x <= x2; x++){
					pix = row + x*3;

					dR = *((pix)+rshift8); 
            				dG = *((pix)+gshift8);
            				dB = *((pix)+bshift8);
					dA = *((pix)+ashift8);
					
					dR = dR + ((sR-dR)*alpha >> 8);
					dG = dG + ((sG-dG)*alpha >> 8);
					dB = dB + ((sB-dB)*alpha >> 8);
					dA = dA + ((sA-dA)*alpha >> 8);

					*((pix)+rshift8) = dR; 
            				*((pix)+gshift8) = dG;
            				*((pix)+bshift8) = dB;
					*((pix)+ashift8) = dA;
				}
			}
					
		}
		break;

		case 4: { /* Probably 32-bpp */
			Uint32 *row, *pixel;
			Uint32 dR=(color & Rmask),dG=(color & Gmask),dB=(color & Bmask),dA=(color & Amask);
		
			for(y = y1; y<=y2; y++){
				row = (Uint32 *)surface->pixels + y*surface->pitch/4;
				for(x = x1; x <= x2; x++){
					pixel = row + x;

					R = ((*pixel & Rmask) + (( dR - (*pixel & Rmask) ) * alpha >> 8)) & Rmask;
					G = ((*pixel & Gmask) + (( dG - (*pixel & Gmask) ) * alpha >> 8)) & Gmask;
					B = ((*pixel & Bmask) + (( dB - (*pixel & Bmask) ) * alpha >> 8)) & Bmask;
					if( Amask )
						A = ((*pixel & Amask) + (( dA - (*pixel & Amask) ) * alpha >> 8)) & Amask;

					*pixel= R | G | B | A;
				}
			}
		}
		break;
 }
	
 return(0);
}

/* Draw rectangle with alpha enabled from RGBA color. */

int filledRectAlpha (SDL_Surface *dst, Sint16 x1, Sint16 y1, Sint16 x2, Sint16 y2, Uint32 color)
{
 Uint8 alpha;
 Uint32 mcolor;
 int result=0;

 /* Lock the surface */
 if ( SDL_MUSTLOCK(dst) ) {
  if ( SDL_LockSurface(dst) < 0 ) {
   return(-1);
  }
 }
 
 /* Setup color */
 alpha = color & 0x000000ff;
 mcolor=SDL_MapRGBA(dst->format, (color & 0xff000000) >> 24, (color & 0x00ff0000) >> 16, (color & 0x0000ff00) >> 8, alpha);

 /* Draw */
 result = _filledRectAlpha(dst,x1,y1,x2,y2,mcolor,alpha);
	
 /* Unlock the surface */
 if (SDL_MUSTLOCK(dst) ) {
  SDL_UnlockSurface(dst);
 }

 return(result);
}

/* Draw horizontal line with alpha enabled from RGBA color */

int HLineAlpha(SDL_Surface *dst, Sint16 x1, Sint16 x2, Sint16 y, Uint32 color)
{
 return (filledRectAlpha(dst, x1, y, x2, y, color));
}


/* Draw vertical line with alpha enabled from RGBA color */

int VLineAlpha(SDL_Surface *dst, Sint16 x, Sint16 y1, Sint16 y2, Uint32 color)
{
 return (filledRectAlpha(dst, x, y1, x, y2, color));
}

#endif


/* Pixel - using alpha weight on color for AA-drawing */

int pixelColorWeight (SDL_Surface *dst, Sint16 x, Sint16 y, Uint32 color, Uint32 weight) 
{ 
 Uint32 a;

 /* Get alpha */
 a=(color & (Uint32)0x000000ff);

 /* Modify Alpha by weight */
 a = ((a*weight) >> 8); 
 
 return(pixelColor (dst,x,y, (color & (Uint32)0xffffff00) | (Uint32)a ));
}

int pixelRGBA (SDL_Surface *dst, Sint16 x, Sint16 y, Uint8 r, Uint8 g, Uint8 b, Uint8 a) 
{
 Uint32 color;
 
 /* Check Alpha */
 if (a==255) {
  /* No alpha blending required */
  /* Setup color */
  color=SDL_MapRGBA(dst->format, r, g, b, a);
  /* Draw */
  return(fastPixelColor (dst, x, y, color));
 } else {
  /* Alpha blending required */
  /* Draw */
  return(pixelColor (dst, x, y, ((Uint32)r << 24) | ((Uint32)g << 16) | ((Uint32)b << 8) | (Uint32)a ));
 }
}

/* ----- Horizontal line */

#ifdef SURFACE_ALPHA_PIXEL
 static SDL_Surface *gfxPrimitivesHline=NULL;
#endif

int hlineColor (SDL_Surface *dst, Sint16 x1, Sint16 x2, Sint16 y, Uint32 color) 
{
 Sint16 left,right,top,bottom;
 Uint8 *pixel,*pixellast;
 int dx;
 int pixx, pixy;
 Sint16 w;
 Sint16 xtmp;
 int result=-1;
 Uint8 *colorptr;
#ifdef SURFACE_ALPHA_PIXEL
 Uint32 a;
 SDL_Rect srect;
 SDL_Rect drect;
#endif
 
 /* Get clipping boundary */
 left = dst->clip_rect.x;
 right = dst->clip_rect.x+dst->clip_rect.w-1;
 top = dst->clip_rect.y;
 bottom = dst->clip_rect.y+dst->clip_rect.h-1;

 /* Swap x1, x2 if required */
 if (x1>x2) {
  xtmp=x1; x1=x2; x2=xtmp;
 }

 /* Visible */
 if ((x1>right) || (x2<left) || (y<top) || (y>bottom)) {
  return(0);
 }
 
 /* Clip x */
 if (x1<left) { 
  x1=left;
 } 
 if (x2>right) {
  x2=right;
 }
  
 /* Calculate width */
 w=x2-x1;
 
 /* Sanity check on width */
 if (w<0) {
  return(0);
 }
 
 /* Alpha check */
 if ((color & 255)==255) {

  /* No alpha-blending required */

  /* Setup color */
  colorptr=(Uint8 *)&color;
  if (SDL_BYTEORDER == SDL_BIG_ENDIAN) {
   color=SDL_MapRGBA(dst->format, colorptr[0], colorptr[1], colorptr[2], colorptr[3]);
  } else {
   color=SDL_MapRGBA(dst->format, colorptr[3], colorptr[2], colorptr[1], colorptr[0]);
  }
  
  /* Lock surface */
  SDL_LockSurface(dst);

  /* More variable setup */
  dx=w;
  pixx = dst->format->BytesPerPixel;
  pixy = dst->pitch;
  pixel = ((Uint8*)dst->pixels) + pixx * (int)x1 + pixy * (int)y;
  
  /* Draw */
  switch(dst->format->BytesPerPixel) {
   case 1:
    memset (pixel, color, dx);
    break;
   case 2:
    pixellast = pixel + dx + dx;
    for (; pixel<=pixellast; pixel += pixx) {
     *(Uint16*)pixel = color;
    }
    break;
   case 3:
    pixellast = pixel + dx + dx + dx;
    for (; pixel<=pixellast; pixel += pixx) {
     if (SDL_BYTEORDER == SDL_BIG_ENDIAN) {
      pixel[0] = (color >> 16) & 0xff;
      pixel[1] = (color >> 8) & 0xff;
      pixel[2] = color & 0xff;
     } else {
      pixel[0] = color & 0xff;
      pixel[1] = (color >> 8) & 0xff;
      pixel[2] = (color >> 16) & 0xff;
     }
    }
    break;
   default: /* case 4*/
    dx = dx + dx;
    pixellast = pixel + dx + dx;
    for (; pixel<=pixellast; pixel += pixx) {
     *(Uint32*)pixel = color;
    }
    break;
  }

  /* Unlock surface */
  SDL_UnlockSurface(dst);

  /* Set result code */
  result=0;

 } else {

  /* Alpha blending blit */

#ifdef SURFACE_ALPHA_PIXEL

  /* Adjust width for Rect setup */
  w++;
 
  /* Setup source rectangle for pixel */
  srect.x=0;
  srect.y=0;
  srect.w=w;
  srect.h=1;
 
  /* Setup rectangle for destination line */
  drect.x=x1;
  drect.y=y;
  drect.w=w;
  drect.h=1;
 
  /* Maybe deallocate existing surface if size is too small */
  if ((gfxPrimitivesHline!=NULL) && (gfxPrimitivesHline->w<w) ) {
   SDL_FreeSurface(gfxPrimitivesHline);
   gfxPrimitivesHline=NULL;
  }
 
  /* Create horizontal line surface in destination format if necessary */
  if (gfxPrimitivesHline==NULL) {
   gfxPrimitivesHline=SDL_CreateRGBSurface(SDL_SWSURFACE | SDL_HWSURFACE | SDL_SRCALPHA, w, 1, 32, 0xFF000000, 0x00FF0000, 0x0000FF00, 0x000000FF);
  }

  /* Get alpha */
  a=(color & (Uint32)0x000000ff);

  /* Toggle alpha blending if necessary, reset otherwise */
  if (a != 255) { 
   SDL_SetAlpha (gfxPrimitivesHline, SDL_SRCALPHA, 255); 
  } else {
   SDL_SetAlpha (gfxPrimitivesHline, 0, 255);
  }
 
  /* Draw color into pixel*/
  SDL_FillRect (gfxPrimitivesHline, &srect, color);
 
  /* Draw pixel onto destination surface */
  result=SDL_BlitSurface (gfxPrimitivesHline, &srect, dst, &drect);

#else

 result=HLineAlpha (dst, x1, x1+w, y, color);

#endif

 }
  
 return(result);
}

int hlineRGBA (SDL_Surface *dst, Sint16 x1, Sint16 x2, Sint16 y, Uint8 r, Uint8 g, Uint8 b, Uint8 a) 
{
 /* Draw */
 return(hlineColor(dst, x1, x2, y, ((Uint32)r << 24) | ((Uint32)g << 16) | ((Uint32)b << 8) | (Uint32)a)); 
}

/* ----- Vertical line */

#ifdef SURFACE_ALPHA_PIXEL
 static SDL_Surface *gfxPrimitivesVline=NULL;
#endif

int vlineColor (SDL_Surface *dst, Sint16 x, Sint16 y1, Sint16 y2, Uint32 color) 
{
 Sint16 left,right,top,bottom;
 Uint8 *pixel, *pixellast;
 int dy;
 int pixx, pixy;
 Sint16 h;
 Sint16 ytmp;
 int result=-1;
 Uint8 *colorptr;
#ifdef SURFACE_ALPHA_PIXEL
 SDL_Rect srect;
 SDL_Rect drect;
 Uint32 a;
#endif
 
 /* Get clipping boundary */
 left = dst->clip_rect.x;
 right = dst->clip_rect.x+dst->clip_rect.w-1;
 top = dst->clip_rect.y;
 bottom = dst->clip_rect.y+dst->clip_rect.h-1;

 /* Swap y1, y2 if required */
 if (y1>y2) {
  ytmp=y1; y1=y2; y2=ytmp;
 }

 /* Visible */
 if ((y2<top) || (y1>bottom) || (x<left) || (x>right)) {
  return(0);
 }

 /* Clip y */
 if (y1<top) { 
  y1=top;
 } 
 if (y2>bottom) {
  y2=bottom;
 }
   
 /* Calculate height */
 h=y2-y1;
 
 /* Sanity check on height */
 if (h<0) {
  return(0);
 }

 /* Alpha check */
 if ((color & 255)==255) {

  /* No alpha-blending required */
  
  /* Setup color */
  colorptr=(Uint8 *)&color;
  if (SDL_BYTEORDER == SDL_BIG_ENDIAN) {
   color=SDL_MapRGBA(dst->format, colorptr[0], colorptr[1], colorptr[2], colorptr[3]);
  } else {
   color=SDL_MapRGBA(dst->format, colorptr[3], colorptr[2], colorptr[1], colorptr[0]);
  }

  /* Lock surface */
  SDL_LockSurface(dst);

  /* More variable setup */
  dy=h;
  pixx = dst->format->BytesPerPixel;
  pixy = dst->pitch;
  pixel = ((Uint8*)dst->pixels) + pixx * (int)x + pixy * (int)y1;
  pixellast = pixel + pixy*dy;
  
  /* Draw */
  switch(dst->format->BytesPerPixel) {
   case 1:
    for (; pixel<=pixellast; pixel += pixy) {
     *(Uint8*)pixel = color;
    }
    break;
   case 2:
    for (; pixel<=pixellast; pixel += pixy) {
     *(Uint16*)pixel = color;
    }
    break;
   case 3:
    for (; pixel<=pixellast; pixel += pixy) {
     if(SDL_BYTEORDER == SDL_BIG_ENDIAN) {
      pixel[0] = (color >> 16) & 0xff;
      pixel[1] = (color >> 8) & 0xff;
      pixel[2] = color & 0xff;
     } else {
      pixel[0] = color & 0xff;
      pixel[1] = (color >> 8) & 0xff;
      pixel[2] = (color >> 16) & 0xff;
     }
    }
    break;
   default: /* case 4*/
    for (; pixel<=pixellast; pixel += pixy) {
     *(Uint32*)pixel = color;
    }
    break;
  }

  /* Unlock surface */
  SDL_UnlockSurface(dst);

  /* Set result code */
  result=0;

 } else {

  /* Alpha blending blit */

#ifdef SURFACE_ALPHA_PIXEL

  /* Adjust height for Rect setup */
  h++;
  
  /* Setup source rectangle for pixel */
  srect.x=0;
  srect.y=0;
  srect.w=1;
  srect.h=h;
 
  /* Setup rectangle for line */
  drect.x=x;
  drect.y=y1;
  drect.w=1;
  drect.h=h;

  /* Maybe deallocate existing surface if size is too small */
  if ( (gfxPrimitivesVline!=NULL) && (gfxPrimitivesVline->h<h) ) {
   SDL_FreeSurface(gfxPrimitivesVline);
   gfxPrimitivesVline=NULL;
  }
 
  /* Create horizontal line surface in destination format if necessary */
  if (gfxPrimitivesVline==NULL) {
   gfxPrimitivesVline=SDL_CreateRGBSurface(SDL_SWSURFACE | SDL_HWSURFACE | SDL_SRCALPHA, 1, h, 32, 0xFF000000, 0x00FF0000, 0x0000FF00, 0x000000FF);
  }

  /* Get alpha */
  a=(color & (Uint32)0x000000ff);

  /* Toggle alpha blending if necessary, reset otherwise */
  if (a != 255) {
   SDL_SetAlpha (gfxPrimitivesVline, SDL_SRCALPHA, 255);
  } else {
   SDL_SetAlpha (gfxPrimitivesVline, 0, 255);
  }
 
  /* Draw color into pixel*/
  SDL_FillRect (gfxPrimitivesVline, &srect, color);
 
  /* Draw Vline onto destination surface */
  result=SDL_BlitSurface (gfxPrimitivesVline, &srect, dst, &drect);

#else

 result=VLineAlpha(dst,x, y1, y1+h, color);

#endif
 
 } 

 return(result);
}

int vlineRGBA (SDL_Surface *dst, Sint16 x, Sint16 y1, Sint16 y2, Uint8 r, Uint8 g, Uint8 b, Uint8 a) 
{
 /* Draw */
 return(vlineColor(dst, x, y1, y2, ((Uint32)r << 24) | ((Uint32)g << 16) | ((Uint32)b << 8) | (Uint32)a)); 
}

/* ----- Rectangle */

int rectangleColor (SDL_Surface *dst, Sint16 x1, Sint16 y1, Sint16 x2, Sint16 y2, Uint32 color) 
{
 int result;
 Sint16 w,h, xtmp, ytmp;


 /* Swap x1, x2 if required */
 if (x1>x2) {
  xtmp=x1; x1=x2; x2=xtmp;
 }

 /* Swap y1, y2 if required */
 if (y1>y2) {
  ytmp=y1; y1=y2; y2=ytmp;
 }
 
 /* Calculate width&height */
 w=x2-x1;
 h=y2-y1;

 /* Sanity check */
 if ((w<0) || (h<0)) {
  return(0);
 }

 /* Test for special cases of straight lines or single point */
 if (x1==x2) {
  if (y1==y2) {
   return(pixelColor(dst, x1, y1, color));
  } else {
   return(vlineColor(dst, x1, y1, y2, color));
  }
 } else {
  if (y1==y2) { 
   return(hlineColor(dst, x1, x2, y1, color));
  }
 }
 
 /* Draw rectangle */
 result=0;
 result |= vlineColor(dst, x1, y1, y2, color);
 result |= vlineColor(dst, x2, y1, y2, color);
 result |= hlineColor(dst, x1, x2, y1, color);
 result |= hlineColor(dst, x1, x2, y2, color);
 
 return(result);

}

int rectangleRGBA (SDL_Surface *dst, Sint16 x1, Sint16 y1, Sint16 x2, Sint16 y2, Uint8 r, Uint8 g, Uint8 b, Uint8 a) 
{
 /* Draw */
 return(rectangleColor(dst, x1, y1, x2, y2, ((Uint32)r << 24) | ((Uint32)g << 16) | ((Uint32)b << 8) | (Uint32)a)); 
}

/* --------- Clipping routines for box/line */

/* Clipping based heavily on code from                       */
/* http://www.ncsa.uiuc.edu/Vis/Graphics/src/clipCohSuth.c   */

#define CLIP_LEFT_EDGE   0x1
#define CLIP_RIGHT_EDGE  0x2
#define CLIP_BOTTOM_EDGE 0x4
#define CLIP_TOP_EDGE    0x8
#define CLIP_INSIDE(a)   (!a)
#define CLIP_REJECT(a,b) (a&b)
#define CLIP_ACCEPT(a,b) (!(a|b))

static int clipEncode (Sint16 x, Sint16 y, Sint16 left, Sint16 top, Sint16 right, Sint16 bottom)
{
 int code = 0;
 if (x < left) {
  code |= CLIP_LEFT_EDGE;
 } else if (x > right) {
  code |= CLIP_RIGHT_EDGE;
 }
 if (y < top) {   
  code |= CLIP_TOP_EDGE;
 } else if (y > bottom) {
  code |= CLIP_BOTTOM_EDGE;
 }
 return code;
}

static int clipLine(SDL_Surface *dst, Sint16 *x1, Sint16 *y1, Sint16 *x2, Sint16 *y2)
{
 Sint16 left,right,top,bottom;
 int code1, code2;
 int draw = 0;
 Sint16 swaptmp;
 float m;

 /* Get clipping boundary */
 left = dst->clip_rect.x;
 right = dst->clip_rect.x+dst->clip_rect.w-1;
 top = dst->clip_rect.y;
 bottom = dst->clip_rect.y+dst->clip_rect.h-1;

 while (1) {
  code1 = clipEncode (*x1, *y1, left, top, right, bottom);
  code2 = clipEncode (*x2, *y2, left, top, right, bottom);
  if (CLIP_ACCEPT(code1, code2)) {
   draw = 1;
   break;
  } else if (CLIP_REJECT(code1, code2))
   break;
  else {
   if(CLIP_INSIDE (code1)) {
    swaptmp = *x2; *x2 = *x1; *x1 = swaptmp;
    swaptmp = *y2; *y2 = *y1; *y1 = swaptmp;
    swaptmp = code2; code2 = code1; code1 = swaptmp;
   }
   if (*x2 != *x1) {      
    m = (*y2 - *y1) / (float)(*x2 - *x1);
   } else {
    m = 1.0f;
   }
   if (code1 & CLIP_LEFT_EDGE) {
    *y1 += (Sint16)((left - *x1) * m);
    *x1 = left; 
   } else if (code1 & CLIP_RIGHT_EDGE) {
    *y1 += (Sint16)((right - *x1) * m);
    *x1 = right; 
   } else if (code1 & CLIP_BOTTOM_EDGE) {
    if (*x2 != *x1) {
     *x1 += (Sint16)((bottom - *y1) / m);
    }
    *y1 = bottom;
   } else if (code1 & CLIP_TOP_EDGE) {
    if (*x2 != *x1) {
     *x1 += (Sint16)((top - *y1) / m);
    }
    *y1 = top;
   } 
  }
 }
 
 return draw;
}

/* ----- Filled rectangle (Box) */

#ifdef SURFACE_ALPHA_PIXEL
 static SDL_Surface *gfxPrimitivesBox=NULL;
#endif

int boxColor (SDL_Surface *dst, Sint16 x1, Sint16 y1, Sint16 x2, Sint16 y2, Uint32 color) 
{
 Uint8 *pixel, *pixellast;
 int x, dx;
 int dy;
 int pixx, pixy;
 Sint16 w,h,tmp;
 int result;
 Uint8 *colorptr;
#ifdef SURFACE_ALPHA_PIXEL
 Uint32 a;
 SDL_Rect srect;
 SDL_Rect drect;
#endif

 /* Clip diagonal and test if we have to draw */
 if (!(clipLine(dst,&x1,&y1,&x2,&y2))) {
  return(0);
 }

 /* Test for special cases of straight lines or single point */
 if (x1==x2) {
  if (y1<y2) {
   return(vlineColor(dst, x1, y1, y2, color));
  } else if (y1>y2) {
   return(vlineColor(dst, x1, y2, y1, color));
  } else {
   return(pixelColor(dst, x1, y1, color));
  }
 }
 if (y1==y2) { 
  if (x1<x2) {
   return(hlineColor(dst, x1, x2, y1, color));
  } else if (x1>x2) { 
   return(hlineColor(dst, x2, x1, y1, color));
  }
 }
  
 /* Order coordinates */
 if (x1>x2) {
  tmp=x1;
  x1=x2;
  x2=tmp;
 }
 if (y1>y2) {
  tmp=y1;
  y1=y2;
  y2=tmp;
 }
 
 /* Calculate width&height */
 w=x2-x1;
 h=y2-y1;
 
 /* Alpha check */
 if ((color & 255)==255) {

  /* No alpha-blending required */
  
  /* Setup color */
  colorptr=(Uint8 *)&color;
  if (SDL_BYTEORDER == SDL_BIG_ENDIAN) {
   color=SDL_MapRGBA(dst->format, colorptr[0], colorptr[1], colorptr[2], colorptr[3]);
  } else {
   color=SDL_MapRGBA(dst->format, colorptr[3], colorptr[2], colorptr[1], colorptr[0]);
  }

  /* Lock surface */
  SDL_LockSurface(dst);

  /* More variable setup */
  dx=w;
  dy=h;
  pixx = dst->format->BytesPerPixel;
  pixy = dst->pitch;
  pixel = ((Uint8*)dst->pixels) + pixx * (int)x1 + pixy * (int)y1;
  pixellast = pixel + pixx*dx + pixy*dy;
  
  /* Draw */
  switch(dst->format->BytesPerPixel) {
   case 1:
    for (; pixel<=pixellast; pixel += pixy) {
     memset(pixel,(Uint8)color,dx);
    }
    break;
   case 2:
    pixy -= (pixx*dx);
    for (; pixel<=pixellast; pixel += pixy) {
     for (x=0; x<dx; x++) {
      *(Uint16*)pixel = color;
      pixel += pixx;
     }
    }
    break;
   case 3:
    pixy -= (pixx*dx);
    for (; pixel<=pixellast; pixel += pixy) {
     for (x=0; x<dx; x++) {
      if(SDL_BYTEORDER == SDL_BIG_ENDIAN) {
       pixel[0] = (color >> 16) & 0xff;
       pixel[1] = (color >> 8) & 0xff;
       pixel[2] = color & 0xff;
      } else {
       pixel[0] = color & 0xff;
       pixel[1] = (color >> 8) & 0xff;
       pixel[2] = (color >> 16) & 0xff;
      }
      pixel += pixx;
     }
    }
    break;
   default: /* case 4*/
    pixy -= (pixx*dx);
    for (; pixel<=pixellast; pixel += pixy) {
     for (x=0; x<dx; x++) {
      *(Uint32*)pixel = color;
      pixel += pixx;
     }
    }
    break;
  }

  /* Unlock surface */
  SDL_UnlockSurface(dst);
  
  result=0;

 } else {

#ifdef SURFACE_ALPHA_PIXEL
 
  /* Setup source rectangle for pixel */
  srect.x=0;
  srect.y=0;
  srect.w=w;
  srect.h=h;
 
  /* Setup rectangle for line */
  drect.x=x1;
  drect.y=y1;
  drect.w=w;
  drect.h=h;

  /* Maybe deallocate existing surface if size is too small */
  if ((gfxPrimitivesBox!=NULL) && ((gfxPrimitivesBox->w<w) || (gfxPrimitivesBox->h<h))) {
   SDL_FreeSurface(gfxPrimitivesBox);
   gfxPrimitivesBox=NULL;
  }
 
  /* Create box surface in destination format if necessary */
  if (gfxPrimitivesBox==NULL) {
   gfxPrimitivesBox=SDL_CreateRGBSurface(SDL_SWSURFACE | SDL_HWSURFACE | SDL_SRCALPHA, w, h, 32, 0xFF000000, 0x00FF0000, 0x0000FF00, 0x000000FF);
  }

  /* Get alpha */
  a=(color & (Uint32)0x000000ff);

  /* Toggle alpha blending if necessary, reset otherwise */
  if (a != 255) {
   SDL_SetAlpha (gfxPrimitivesBox, SDL_SRCALPHA, 255);
  } else {
   SDL_SetAlpha (gfxPrimitivesBox, 0, 255);
  }
 
  /* Draw color into pixel*/
  SDL_FillRect (gfxPrimitivesBox, &srect, color);
 
  /* Draw pixel onto destination surface */
  result=SDL_BlitSurface (gfxPrimitivesBox, &srect, dst, &drect);

#else
 
 result=filledRectAlpha(dst,x1,y1,x1+w,y1+h,color);

#endif

 }
 
 return(result);
}

int boxRGBA (SDL_Surface *dst, Sint16 x1, Sint16 y1,  Sint16 x2, Sint16 y2, Uint8 r, Uint8 g, Uint8 b, Uint8 a) 
{
 /* Draw */
 return(boxColor(dst, x1, y1, x2, y2, ((Uint32)r << 24) | ((Uint32)g << 16) | ((Uint32)b << 8) | (Uint32)a)); 
}

/* ----- Line */

/* Non-alpha line drawing code adapted from routine          */
/* by Pete Shinners, pete@shinners.org                       */
/* Originally from pygame, http://pygame.seul.org            */

#define ABS(a) (((a)<0) ? -(a) : (a))

int lineColor(SDL_Surface *dst, Sint16 x1, Sint16 y1, Sint16 x2, Sint16 y2, Uint32 color)
{
 int pixx, pixy;
 int x,y;
 int dx,dy;
 int ax,ay;
 int sx,sy;
 int swaptmp;
 Uint8 *pixel;
 Uint8 *colorptr;

 /* Clip line and test if we have to draw */
 if (!(clipLine(dst,&x1,&y1,&x2,&y2))) {
  return(0);
 }

 /* Test for special cases of straight lines or single point */
 if (x1==x2) {
  if (y1<y2) {
   return(vlineColor(dst, x1, y1, y2, color));
  } else if (y1>y2) {
   return(vlineColor(dst, x1, y2, y1, color));
  } else {
   return(pixelColor(dst, x1, y1, color));
  }
 }
 if (y1==y2) { 
  if (x1<x2) {
   return(hlineColor(dst, x1, x2, y1, color));
  } else if (x1>x2) { 
   return(hlineColor(dst, x2, x1, y1, color));
  }
 }

 /* Variable setup */
 dx = x2 - x1;
 dy = y2 - y1;
 sx = (dx >= 0) ? 1 : -1;
 sy = (dy >= 0) ? 1 : -1;

 /* Check for alpha blending */
 if ((color & 255)==255) { 

  /* No alpha blending - use fast pixel routines */

  /* Setup color */
  colorptr=(Uint8 *)&color;
  if (SDL_BYTEORDER == SDL_BIG_ENDIAN) {
   color=SDL_MapRGBA(dst->format, colorptr[0], colorptr[1], colorptr[2], colorptr[3]);
  } else {
   color=SDL_MapRGBA(dst->format, colorptr[3], colorptr[2], colorptr[1], colorptr[0]);
  }
  
  /* Lock surface */
  SDL_LockSurface(dst);

  /* More variable setup */
  dx = sx * dx + 1;
  dy = sy * dy + 1;
  pixx = dst->format->BytesPerPixel;
  pixy = dst->pitch;
  pixel = ((Uint8*)dst->pixels) + pixx * (int)x1 + pixy * (int)y1;
  pixx *= sx;
  pixy *= sy;
  if (dx < dy) {
   swaptmp = dx; dx = dy; dy = swaptmp;
   swaptmp = pixx; pixx = pixy; pixy = swaptmp;
  }

  /* Draw */
  x=0;
  y=0;
  switch(dst->format->BytesPerPixel) {
   case 1:
    for(; x < dx; x++, pixel += pixx) {
     *pixel = color;
     y += dy; 
     if (y >= dx) {
      y -= dx; pixel += pixy;
     }
    }
    break;
   case 2:
    for (; x < dx; x++, pixel += pixx) {
     *(Uint16*)pixel = color;
     y += dy; 
     if (y >= dx) {
      y -= dx; 
      pixel += pixy;
     }
    }
    break;
   case 3:
    for(; x < dx; x++, pixel += pixx) {
     if(SDL_BYTEORDER == SDL_BIG_ENDIAN) {
      pixel[0] = (color >> 16) & 0xff;
      pixel[1] = (color >> 8) & 0xff;
      pixel[2] = color & 0xff;
     } else {
      pixel[0] = color & 0xff;
      pixel[1] = (color >> 8) & 0xff;
      pixel[2] = (color >> 16) & 0xff;
     }
     y += dy; 
     if (y >= dx) {
      y -= dx; 
      pixel += pixy;
     }
    }
    break;
   default: /* case 4 */
     for(; x < dx; x++, pixel += pixx) {
      *(Uint32*)pixel = color;
      y += dy; 
      if (y >= dx) {
       y -= dx; 
       pixel += pixy;
      }
     }
     break;
  }

  /* Unlock surface */
  SDL_UnlockSurface(dst);

 } else {

  /* Alpha blending required - use single-pixel blits */

  ax = ABS(dx) << 1;
  ay = ABS(dy) << 1;
  x = x1;
  y = y1;
  if (ax > ay) {
    int d = ay - (ax >> 1);
    while (x != x2) {
     pixelColor(dst, x, y, color);
     if (d > 0 || (d == 0 && sx == 1)) {
      y += sy;
      d -= ax;
     } 
     x += sx;
     d += ay;
    }
  } else {
    int d = ax - (ay >> 1);
    while (y != y2) {
     pixelColor(dst, x, y, color);
     if (d > 0 || ((d == 0) && (sy == 1))) {
      x += sx;
      d -= ay;
     }
     y += sy;
     d += ax;
    }
  }
  pixelColor(dst, x, y, color);

 }
 
 return(0);
}

int lineRGBA (SDL_Surface *dst, Sint16 x1, Sint16 y1, Sint16 x2, Sint16 y2, Uint8 r, Uint8 g, Uint8 b, Uint8 a) 
{
 /* Draw */
 return(lineColor(dst, x1, y1, x2, y2, ((Uint32)r << 24) | ((Uint32)g << 16) | ((Uint32)b << 8) | (Uint32)a)); 
}

/* AA Line */

#define AAlevels 256
#define AAbits 8

/* 

This implementation of the Wu antialiasing code is based on Mike Abrash's
DDJ article which was reprinted as Chapter 42 of his Graphics Programming
Black Book, but has been optimized to work with SDL and utilizes 32-bit
fixed-point arithmetic. (A. Schiffler).

*/

int aalineColorInt (SDL_Surface *dst, Sint16 x1, Sint16 y1, Sint16 x2, Sint16 y2, Uint32 color, int draw_endpoint)
{
 Sint32 xx0,yy0,xx1,yy1;
 int result;
 Uint32 intshift, erracc, erradj;
 Uint32 erracctmp, wgt, wgtcompmask;
 int dx, dy, tmp, xdir, y0p1, x0pxdir;

 /* Clip line and test if we have to draw */
 if (!(clipLine(dst,&x1,&y1,&x2,&y2))) {
  return(0);
 }

 /* Keep on working with 32bit numbers */
 xx0=x1;
 yy0=y1;
 xx1=x2;
 yy1=y2;
 
 /* Reorder points if required */ 
 if (yy0 > yy1) {
  tmp = yy0; yy0 = yy1; yy1 = tmp;
  tmp = xx0; xx0 = xx1; xx1 = tmp;
 }

 /* Calculate distance */
 dx = xx1 - xx0;
 dy = yy1 - yy0;

 /* Adjust for negative dx and set xdir */
 if (dx >= 0) {
   xdir=1;
 } else {
   xdir=-1;
   dx=(-dx);
 }

 /* Check for special cases */
 if (dx==0) {
   /* Vertical line */
   return (vlineColor(dst,x1,y1,y2,color));
 } else if (dy==0) {
   /* Horizontal line */
   return (hlineColor(dst,x1,x2,y1,color));
 } else if (dx==dy) {
   /* Diagonal line */
   return (lineColor(dst,x1,y1,x2,y2,color));
 } 
  
  /* Line is not horizontal, vertical or diagonal */
  result=0;
  /* Zero accumulator */
  erracc = 0;			
  /* # of bits by which to shift erracc to get intensity level */
  intshift = 32 - AAbits;
  /* Mask used to flip all bits in an intensity weighting */
  wgtcompmask = AAlevels - 1;

  /* Draw the initial pixel in the foreground color */
  result |= pixelColor (dst, x1, y1, color);

  /* x-major or y-major? */
  if (dy > dx) {

    /* y-major.  Calculate 16-bit fixed point fractional part of a pixel that
       X advances every time Y advances 1 pixel, truncating the result so that
       we won't overrun the endpoint along the X axis */
    /* Not-so-portable version: erradj = ((Uint64)dx << 32) / (Uint64)dy; */
    erradj = ((dx << 16) / dy)<<16;

    /* draw all pixels other than the first and last */
    x0pxdir=xx0+xdir;
    while (--dy) {
      erracctmp = erracc;
      erracc += erradj;
      if (erracc <= erracctmp) {
	/* rollover in error accumulator, x coord advances */
	xx0=x0pxdir;
	x0pxdir += xdir;
      }
      yy0++;			/* y-major so always advance Y */

      /* the AAbits most significant bits of erracc give us the intensity
	 weighting for this pixel, and the complement of the weighting for
	 the paired pixel. */
      wgt = (erracc >> intshift) & 255;      
      result |= pixelColorWeight (dst, xx0, yy0, color, 255-wgt);
      result |= pixelColorWeight (dst, x0pxdir, yy0, color, wgt);      
    }

  } else {

   /* x-major line.  Calculate 16-bit fixed-point fractional part of a pixel
      that Y advances each time X advances 1 pixel, truncating the result so
      that we won't overrun the endpoint along the X axis. */
   /* Not-so-portable version: erradj = ((Uint64)dy << 32) / (Uint64)dx; */
   erradj = ((dy << 16) / dx)<<16;
 
   /* draw all pixels other than the first and last */
   y0p1=yy0+1;
   while (--dx) {

    erracctmp = erracc;
    erracc += erradj;
    if (erracc <= erracctmp) {
      /* Accumulator turned over, advance y */
      yy0=y0p1;
      y0p1++;
    }
    xx0 += xdir;			/* x-major so always advance X */
    /* the AAbits most significant bits of erracc give us the intensity
       weighting for this pixel, and the complement of the weighting for
       the paired pixel. */
    wgt = (erracc >> intshift) & 255;
    result |= pixelColorWeight (dst, xx0, yy0, color, 255-wgt);
    result |= pixelColorWeight (dst, xx0, y0p1, color, wgt);
  }
 }

 /* Do we have to draw the endpoint */ 
 if (draw_endpoint) {
  /* Draw final pixel, always exactly intersected by the line and doesn't
     need to be weighted. */
  result |= pixelColor (dst, x2, y2, color);
 }

 return(result);
}

int aalineColor (SDL_Surface *dst, Sint16 x1, Sint16 y1, Sint16 x2, Sint16 y2, Uint32 color)
{
 return(aalineColorInt(dst,x1,y1,x2,y1,color,1));
}

int aalineRGBA (SDL_Surface *dst, Sint16 x1, Sint16 y1, Sint16 x2, Sint16 y2, Uint8 r, Uint8 g, Uint8 b, Uint8 a) 
{
 return(aalineColorInt(dst, x1, y1, x2, y2, ((Uint32)r << 24) | ((Uint32)g << 16) | ((Uint32)b << 8) | (Uint32)a, 1)); 
}


/* ----- Circle */

/* Note: Based on algorithm from sge library, modified by A. Schiffler */
/* with multiple pixel-draw removal and other minor speedup changes.   */

int circleColor(SDL_Surface *dst, Sint16 x, Sint16 y, Sint16 r, Uint32 color)
{
 int result;
 Sint16 x1,y1,x2,y2;
 Sint16 cx = 0;
 Sint16 cy = r;
 Sint16 ocx = (Sint16)0xffff;
 Sint16 ocy = (Sint16)0xffff;
 Sint16 df = 1 - r;
 Sint16 d_e = 3;
 Sint16 d_se = -2 * r + 5;
 Sint16 xpcx, xmcx, xpcy, xmcy;
 Sint16 ypcy, ymcy, ypcx, ymcx;
 Uint8 *colorptr;
  
 /* Sanity check radius */
 if (r<0) {
  return(-1);
 }

 /* Special case for r=0 - draw a point */
 if (r==0) {
  return(pixelColor (dst, x, y, color));  
 }

 /* Test if bounding box of circle is visible */
 x1=x-r;
 y1=y-r;
 x2=x+r;
 y2=y+r;
 if (!(clipLine(dst,&x1,&y1,&x2,&y2))) {
  return(0);
 }

 /* Draw circle */
 result=0; 
 /* Alpha Check */
 if ((color & 255) ==255) {

  /* No Alpha - direct memory writes */

  /* Setup color */
  colorptr=(Uint8 *)&color;
  if (SDL_BYTEORDER == SDL_BIG_ENDIAN) {
   color=SDL_MapRGBA(dst->format, colorptr[0], colorptr[1], colorptr[2], colorptr[3]);
  } else {
   color=SDL_MapRGBA(dst->format, colorptr[3], colorptr[2], colorptr[1], colorptr[0]);
  }

  /* Lock surface */
  SDL_LockSurface(dst);
  
  /* Draw */
  do {
   if ((ocy!=cy) || (ocx!=cx)) {
    xpcx=x+cx;
    xmcx=x-cx;
    if (cy>0) {
     ypcy=y+cy;
     ymcy=y-cy;
     result |= fastPixelColorNolock(dst,xmcx,ypcy,color);
     result |= fastPixelColorNolock(dst,xpcx,ypcy,color);
     result |= fastPixelColorNolock(dst,xmcx,ymcy,color);
     result |= fastPixelColorNolock(dst,xpcx,ymcy,color);
    } else {
     result |= fastPixelColorNolock(dst,xmcx,y,color);
     result |= fastPixelColorNolock(dst,xpcx,y,color);
    }
    ocy=cy;
    xpcy=x+cy;
    xmcy=x-cy;
    if (cx>0) {
     ypcx=y+cx;
     ymcx=y-cx;
     result |= fastPixelColorNolock(dst,xmcy,ypcx,color);
     result |= fastPixelColorNolock(dst,xpcy,ypcx,color);
     result |= fastPixelColorNolock(dst,xmcy,ymcx,color);
     result |= fastPixelColorNolock(dst,xpcy,ymcx,color);
    } else {
     result |= fastPixelColorNolock(dst,xmcy,y,color);
     result |= fastPixelColorNolock(dst,xpcy,y,color);
    }
    ocx=cx;
   }
   /* Update */
   if (df < 0)  {
    df += d_e;
    d_e += 2;
    d_se += 2;
   } else {
    df += d_se;
    d_e += 2;
    d_se += 4;
    cy--;
   }
   cx++;
  } while(cx <= cy);

  /* Unlock surface */
  SDL_UnlockSurface(dst);

 } else {
 
  /* Using Alpha - blended pixel blits */
  
  do {
   /* Draw */
   if ((ocy!=cy) || (ocx!=cx)) {
    xpcx=x+cx;
    xmcx=x-cx;
    if (cy>0) {
     ypcy=y+cy;
     ymcy=y-cy;
     result |= pixelColor(dst,xmcx,ypcy,color);
     result |= pixelColor(dst,xpcx,ypcy,color);
     result |= pixelColor(dst,xmcx,ymcy,color);
     result |= pixelColor(dst,xpcx,ymcy,color);
    } else {
     result |= pixelColor(dst,xmcx,y,color);
     result |= pixelColor(dst,xpcx,y,color);
    }
    ocy=cy;
    xpcy=x+cy;
    xmcy=x-cy;
    if (cx>0) {
     ypcx=y+cx;
     ymcx=y-cx;
     result |= pixelColor(dst,xmcy,ypcx,color);
     result |= pixelColor(dst,xpcy,ypcx,color);
     result |= pixelColor(dst,xmcy,ymcx,color);
     result |= pixelColor(dst,xpcy,ymcx,color);
    } else {
     result |= pixelColor(dst,xmcy,y,color);
     result |= pixelColor(dst,xpcy,y,color);
    }
    ocx=cx;
   }
   /* Update */
   if (df < 0)  {
    df += d_e;
    d_e += 2;
    d_se += 2;
   } else {
    df += d_se;
    d_e += 2;
    d_se += 4;
    cy--;
   }
   cx++;
  } while(cx <= cy);

 } /* Alpha check */
 
 return(result);
}

int circleRGBA (SDL_Surface *dst, Sint16 x, Sint16 y, Sint16 rad, Uint8 r, Uint8 g, Uint8 b, Uint8 a) 
{
 /* Draw */
 return(circleColor(dst, x, y, rad, ((Uint32)r << 24) | ((Uint32)g << 16) | ((Uint32)b << 8) | (Uint32)a)); 
}

/* ----- AA Circle */

/* Low-speed antialiased circle implementation by drawing aalines. */
/* Works best for larger radii. */

int aacircleColor(SDL_Surface *dst, Sint16 x, Sint16 y, Sint16 r, Uint32 color)
{
 return (aaellipseColor(dst, x, y, r, r, color));
}

int aacircleRGBA (SDL_Surface *dst, Sint16 x, Sint16 y, Sint16 rad, Uint8 r, Uint8 g, Uint8 b, Uint8 a)
{
 /* Draw */
 return(aaellipseColor(dst, x, y, rad, rad, ((Uint32)r << 24) | ((Uint32)g << 16) | ((Uint32)b << 8) | (Uint32)a));
}

/* ----- Filled Circle */

/* Note: Based on algorithm from sge library with multiple-hline draw removal */
/* and other speedup changes. */

int filledCircleColor(SDL_Surface *dst, Sint16 x, Sint16 y, Sint16 r, Uint32 color)
{
 int result;
 Sint16 x1,y1,x2,y2;
 Sint16 cx = 0;
 Sint16 cy = r;
 Sint16 ocx = (Sint16)0xffff;
 Sint16 ocy = (Sint16)0xffff;
 Sint16 df = 1 - r;
 Sint16 d_e = 3;
 Sint16 d_se = -2 * r + 5;
 Sint16 xpcx, xmcx, xpcy, xmcy;
 Sint16 ypcy, ymcy, ypcx, ymcx;

 /* Sanity check radius */
 if (r<0) {
  return(-1);
 }

 /* Special case for r=0 - draw a point */
 if (r==0) {
  return(pixelColor (dst, x, y, color));  
 }

 /* Test bounding box */
 x1=x-r;
 y1=y-r;
 x2=x+r;
 y2=y+r;
 if (!(clipLine(dst,&x1,&y1,&x2,&y2))) {
  return(0);
 }

 /* Draw */
 result=0;
 do {
   xpcx=x+cx;
   xmcx=x-cx;
   xpcy=x+cy;
   xmcy=x-cy;
   if (ocy!=cy) {
    if (cy>0) {
     ypcy=y+cy;
     ymcy=y-cy;
     result |= hlineColor(dst,xmcx,xpcx,ypcy,color);
     result |= hlineColor(dst,xmcx,xpcx,ymcy,color);
    } else {
     result |= hlineColor(dst,xmcx,xpcx,y,color);
    }
    ocy=cy;
   }
   if (ocx!=cx) {
    if (cx!=cy) {
     if (cx>0) {
      ypcx=y+cx;
      ymcx=y-cx;
      result |= hlineColor(dst,xmcy,xpcy,ymcx,color);
      result |= hlineColor(dst,xmcy,xpcy,ypcx,color);
     } else {
      result |= hlineColor(dst,xmcy,xpcy,y,color);
     }
    }
    ocx=cx;
   }
   /* Update */
   if (df < 0)  {
    df += d_e;
    d_e += 2;
    d_se += 2;
   } else {
    df += d_se;
    d_e += 2;
    d_se += 4;
    cy--;
   }
   cx++;
 } while(cx <= cy);

 return(result);
}

int filledCircleRGBA (SDL_Surface *dst, Sint16 x, Sint16 y, Sint16 rad, Uint8 r, Uint8 g, Uint8 b, Uint8 a) 
{
 /* Draw */
 return(filledCircleColor(dst, x, y, rad, ((Uint32)r << 24) | ((Uint32)g << 16) | ((Uint32)b << 8) | (Uint32)a)); 
}


/* ----- Ellipse */

/* Note: Based on algorithm from sge library with multiple-hline draw removal */
/* and other speedup changes. */

int ellipseColor (SDL_Surface *dst, Sint16 x, Sint16 y, Sint16 rx, Sint16 ry, Uint32 color)
{
 int result;
 Sint16 x1, y1, x2, y2;
 int ix, iy;
 int h, i, j, k;
 int oh, oi, oj, ok;
 int xmh, xph, ypk, ymk;
 int xmi, xpi, ymj, ypj;
 int xmj, xpj, ymi, ypi;
 int xmk, xpk, ymh, yph;
 Uint8 *colorptr;
  
 /* Sanity check radii */  
 if ((rx<0) || (ry<0)) {
  return(-1);
 }  

 /* Special case for rx=0 - draw a vline */
 if (rx==0) {
  return(vlineColor (dst, x, y-ry, y+ry, color));  
 }
 /* Special case for ry=0 - draw a hline */
 if (ry==0) {
  return(hlineColor (dst, x-rx, x+rx, y, color));  
 }

 /* Test bounding box */
 x1=x-rx;
 y1=y-ry;
 x2=x+rx;
 y2=y+ry;
 if (!(clipLine(dst,&x1,&y1,&x2,&y2))) {
  return(0);
 }

 /* Init vars */
 oh = oi = oj = ok = 0xFFFF;
 
 /* Draw */
 result=0;
 /* Check alpha */
 if ((color & 255)==255) {

  /* No Alpha - direct memory writes */

  /* Setup color */
  colorptr=(Uint8 *)&color;
  if (SDL_BYTEORDER == SDL_BIG_ENDIAN) {
   color=SDL_MapRGBA(dst->format, colorptr[0], colorptr[1], colorptr[2], colorptr[3]);
  } else {
   color=SDL_MapRGBA(dst->format, colorptr[3], colorptr[2], colorptr[1], colorptr[0]);
  }

  /* Lock surface */
  SDL_LockSurface(dst);
  
  if (rx > ry) {
  	ix = 0;
   	iy = rx * 64;

 		do {
	 		h = (ix + 32) >> 6;
	 		i = (iy + 32) >> 6;
	 		j = (h * ry) / rx;
	 		k = (i * ry) / rx;

			if (((ok!=k) && (oj!=k)) || ((oj!=j) && (ok!=j)) || (k!=j)) {
			 xph=x+h;
			 xmh=x-h;
 			 if (k>0) {
 			  ypk=y+k;
 			  ymk=y-k;
 			  result |= fastPixelColorNolock (dst,xmh,ypk, color);
 			  result |= fastPixelColorNolock (dst,xph,ypk, color);
 			  result |= fastPixelColorNolock (dst,xmh,ymk, color);
 			  result |= fastPixelColorNolock (dst,xph,ymk, color);
			 } else {
 			  result |= fastPixelColorNolock (dst,xmh,y, color);
 			  result |= fastPixelColorNolock (dst,xph,y, color);
			 }
 			 ok=k;
			 xpi=x+i;
			 xmi=x-i;
			 if (j>0) {
			  ypj=y+j;
			  ymj=y-j;
 			  result |= fastPixelColorNolock (dst,xmi,ypj, color);
 			  result |= fastPixelColorNolock (dst,xpi,ypj, color);
 			  result |= fastPixelColorNolock (dst,xmi,ymj, color);
 			  result |= fastPixelColorNolock (dst,xpi,ymj, color);
			 } else {
 			  result |= fastPixelColorNolock (dst,xmi,y, color);
 			  result |= fastPixelColorNolock (dst,xpi,y, color);
			 }
			 oj=j;
			}

			ix = ix + iy / rx;
	 		iy = iy - ix / rx;

		} while (i > h);
  } else {
  	ix = 0;
   	iy = ry * 64;

  	do {
	 		h = (ix + 32) >> 6;
	 		i = (iy + 32) >> 6;
	 		j = (h * rx) / ry;
	 		k = (i * rx) / ry;

			if (((oi!=i) && (oh!=i)) || ((oh!=h) && (oi!=h) && (i!=h))) {
			 xmj=x-j;
			 xpj=x+j;
 			 if (i>0) {
 			  ypi=y+i;
 			  ymi=y-i;
	 		  result |= fastPixelColorNolock (dst,xmj,ypi,color);
	 		  result |= fastPixelColorNolock (dst,xpj,ypi,color);
	 		  result |= fastPixelColorNolock (dst,xmj,ymi,color);
	 		  result |= fastPixelColorNolock (dst,xpj,ymi,color);
		  	 } else {
	 		  result |= fastPixelColorNolock (dst,xmj,y,color);
	 		  result |= fastPixelColorNolock (dst,xpj,y,color);
			 }
			 oi=i;
			 xmk=x-k;
			 xpk=x+k;
			 if (h>0) {
			  yph=y+h;
			  ymh=y-h;
 	 		  result |= fastPixelColorNolock (dst,xmk,yph,color);
 	 		  result |= fastPixelColorNolock (dst,xpk,yph,color);
 	 		  result |= fastPixelColorNolock (dst,xmk,ymh,color);
 	 		  result |= fastPixelColorNolock (dst,xpk,ymh,color);
			 } else {
 	 		  result |= fastPixelColorNolock (dst,xmk,y,color);
 	 		  result |= fastPixelColorNolock (dst,xpk,y,color);			 
			 }
			 oh=h;
			}
			
	 		ix = ix + iy / ry;
	 		iy = iy - ix / ry;

  	} while(i > h);
  }

  /* Unlock surface */
  SDL_UnlockSurface(dst);

 } else {
  
 if (rx > ry) {
  	ix = 0;
   	iy = rx * 64;

 		do {
	 		h = (ix + 32) >> 6;
	 		i = (iy + 32) >> 6;
	 		j = (h * ry) / rx;
	 		k = (i * ry) / rx;

			if (((ok!=k) && (oj!=k)) || ((oj!=j) && (ok!=j)) || (k!=j)) {
			 xph=x+h;
			 xmh=x-h;
 			 if (k>0) {
 			  ypk=y+k;
 			  ymk=y-k;
 			  result |= pixelColor (dst,xmh,ypk, color);
 			  result |= pixelColor (dst,xph,ypk, color);
 			  result |= pixelColor (dst,xmh,ymk, color);
 			  result |= pixelColor (dst,xph,ymk, color);
			 } else {
 			  result |= pixelColor (dst,xmh,y, color);
 			  result |= pixelColor (dst,xph,y, color);
			 }
 			 ok=k;
			 xpi=x+i;
			 xmi=x-i;
			 if (j>0) {
			  ypj=y+j;
			  ymj=y-j;
 			  result |= pixelColor (dst,xmi,ypj, color);
 			  result |= pixelColor (dst,xpi,ypj, color);
 			  result |= pixelColor (dst,xmi,ymj, color);
 			  result |= pixelColor (dst,xpi,ymj, color);
			 } else {
 			  result |= pixelColor (dst,xmi,y, color);
 			  result |= pixelColor (dst,xpi,y, color);
			 }
			 oj=j;
			}

			ix = ix + iy / rx;
	 		iy = iy - ix / rx;

		} while (i > h);
  } else {
  	ix = 0;
   	iy = ry * 64;

  	do {
	 		h = (ix + 32) >> 6;
	 		i = (iy + 32) >> 6;
	 		j = (h * rx) / ry;
	 		k = (i * rx) / ry;

			if (((oi!=i) && (oh!=i)) || ((oh!=h) && (oi!=h) && (i!=h))) {
			 xmj=x-j;
			 xpj=x+j;
 			 if (i>0) {
 			  ypi=y+i;
 			  ymi=y-i;
	 		  result |= pixelColor (dst,xmj,ypi,color);
	 		  result |= pixelColor (dst,xpj,ypi,color);
	 		  result |= pixelColor (dst,xmj,ymi,color);
	 		  result |= pixelColor (dst,xpj,ymi,color);
		  	 } else {
	 		  result |= pixelColor (dst,xmj,y,color);
	 		  result |= pixelColor (dst,xpj,y,color);
			 }
			 oi=i;
			 xmk=x-k;
			 xpk=x+k;
			 if (h>0) {
			  yph=y+h;
			  ymh=y-h;
 	 		  result |= pixelColor (dst,xmk,yph,color);
 	 		  result |= pixelColor (dst,xpk,yph,color);
 	 		  result |= pixelColor (dst,xmk,ymh,color);
 	 		  result |= pixelColor (dst,xpk,ymh,color);
			 } else {
 	 		  result |= pixelColor (dst,xmk,y,color);
 	 		  result |= pixelColor (dst,xpk,y,color);			 
			 }
			 oh=h;
			}
			
	 		ix = ix + iy / ry;
	 		iy = iy - ix / ry;

  	} while(i > h);
 }

 } /* Alpha check */
 
 return(result);
}

int ellipseRGBA (SDL_Surface *dst, Sint16 x, Sint16 y, Sint16 rx, Sint16 ry, Uint8 r, Uint8 g, Uint8 b, Uint8 a) 
{
 /* Draw */
 return(ellipseColor(dst, x, y, rx, ry, ((Uint32)r << 24) | ((Uint32)g << 16) | ((Uint32)b << 8) | (Uint32)a)); 
}

/* ----- AA Ellipse */

/* Low-speed antialiased ellipse implementation by drawing aalines. */
/* Works best for larger radii. */

int aaellipseColor(SDL_Surface *dst, Sint16 x, Sint16 y, Sint16 rx, Sint16 ry, Uint32 color)
{
 int result;
 Sint16 x1, y1, x2, y2;
 double angle;
 double deltaAngle;
 double drx, dry, dr;
 int posX, posY, oldPosX, oldPosY;
 int i, r;

 /* Sanity check radii */  
 if ((rx<0) || (ry<0)) {
  return(-1);
 }  

 /* Special case for rx=0 - draw a vline */
 if (rx==0) {
  return(vlineColor (dst, x, y-ry, y+ry, color));  
 }
 /* Special case for ry=0 - draw a hline */
 if (ry==0) {
  return(hlineColor (dst, x-rx, x+rx, y, color));  
 }

 /* Test bounding box */
 x1=x-rx;
 y1=y-ry;
 x2=x+rx;
 y2=y+ry;
 if (!(clipLine(dst,&x1,&y1,&x2,&y2))) {
  return(0);
 }

 /* Draw */
 r=(rx+ry)>>1;
 dr=(double)r;
 drx=(double)rx;
 dry=(double)ry;
 deltaAngle=(2*M_PI)/dr;
 angle=deltaAngle;
 oldPosX=x+rx;
 oldPosY=y;

 result=0;
 for(i=0; i<r; i++) {
  posX=x+(int)(drx*cos(angle));
  posY=y+(int)(dry*sin(angle));
  result |= aalineColorInt (dst, oldPosX, oldPosY, posX, posY, color, 1);
  oldPosX=posX;
  oldPosY=posY;
  angle += deltaAngle;
 }

 return (result);
}

int aaellipseRGBA (SDL_Surface *dst, Sint16 x, Sint16 y, Sint16 rx, Sint16 ry, Uint8 r, Uint8 g, Uint8 b, Uint8 a)
{
 /* Draw */
 return(aaellipseColor(dst, x, y, rx, ry, ((Uint32)r << 24) | ((Uint32)g << 16) | ((Uint32)b << 8) | (Uint32)a));
}

/* ---- Filled Ellipse */

/* Note: Based on algorithm from sge library with multiple-hline draw removal */
/* and other speedup changes. */

int filledEllipseColor (SDL_Surface *dst, Sint16 x, Sint16 y, Sint16 rx, Sint16 ry, Uint32 color)
{
 int result;
 Sint16 x1,y1,x2,y2;
 int ix, iy;
 int h, i, j, k;
 int oh, oi, oj, ok;
 int xmh, xph;
 int xmi, xpi;
 int xmj, xpj;
 int xmk, xpk;
 
 /* Sanity check radii */  
 if ((rx<0) || (ry<0)) {
  return(-1);
 }  

 /* Special case for rx=0 - draw a vline */
 if (rx==0) {
  return(vlineColor (dst, x, y-ry, y+ry, color));  
 }
 /* Special case for ry=0 - draw a hline */
 if (ry==0) {
  return(hlineColor (dst, x-rx, x+rx, y, color));  
 }

 /* Test bounding box */
 x1=x-rx;
 y1=y-ry;
 x2=x+rx;
 y2=y+ry;
 if (!(clipLine(dst,&x1,&y1,&x2,&y2))) {
  return(0);
 }

 /* Init vars */
 oh = oi = oj = ok = 0xFFFF;

 /* Draw */
 result=0;  
 if (rx > ry) {
  	ix = 0;
   	iy = rx * 64;

 		do {
	 		h = (ix + 32) >> 6;
	 		i = (iy + 32) >> 6;
	 		j = (h * ry) / rx;
	 		k = (i * ry) / rx;

			if ((ok!=k) && (oj!=k)) {
			 xph=x+h;
			 xmh=x-h;
 			 if (k>0) {
	 		  result |= hlineColor (dst,xmh,xph,y+k,color);
	   		  result |= hlineColor (dst,xmh,xph,y-k,color);
			 } else {
	   		  result |= hlineColor (dst,xmh,xph,y,color);
			 }
 			 ok=k;
			}
			if ((oj!=j) && (ok!=j) && (k!=j))  {
			 xmi=x-i;
			 xpi=x+i;
			 if (j>0) {
 	 		  result |= hlineColor (dst,xmi,xpi,y+j,color);
	  		  result |= hlineColor (dst,xmi,xpi,y-j,color);
			 } else {
	  		  result |= hlineColor (dst,xmi,xpi,y  ,color);
			 }
			 oj=j;
			}

			ix = ix + iy / rx;
	 		iy = iy - ix / rx;

		} while (i > h);
  } else {
  	ix = 0;
   	iy = ry * 64;

  	do {
	 		h = (ix + 32) >> 6;
	 		i = (iy + 32) >> 6;
	 		j = (h * rx) / ry;
	 		k = (i * rx) / ry;

			if ((oi!=i) && (oh!=i)) {
			 xmj=x-j;
			 xpj=x+j; 
 			 if (i>0) {
	 		  result |= hlineColor (dst,xmj,xpj,y+i,color);
	   		  result |= hlineColor (dst,xmj,xpj,y-i,color);
		  	 } else {
	   		  result |= hlineColor (dst,xmj,xpj,y,color);
			 }
			 oi=i;
			}
			if ((oh!=h) && (oi!=h) && (i!=h)) {
			 xmk=x-k;
			 xpk=x+k;
			 if (h>0) {
 	 		  result |= hlineColor (dst,xmk,xpk,y+h,color);
	  		  result |= hlineColor (dst,xmk,xpk,y-h,color);
			 } else {
	  		  result |= hlineColor (dst,xmk,xpk,y  ,color);
			 }
			 oh=h;
			}
			
	 		ix = ix + iy / ry;
	 		iy = iy - ix / ry;

  	} while(i > h);
 }

 return(result);
}


int filledEllipseRGBA (SDL_Surface *dst, Sint16 x, Sint16 y, Sint16 rx, Sint16 ry, Uint8 r, Uint8 g, Uint8 b, Uint8 a) 
{
 /* Draw */
 return(filledEllipseColor(dst, x, y, rx, ry, ((Uint32)r << 24) | ((Uint32)g << 16) | ((Uint32)b << 8) | (Uint32)a)); 
}

/* ---- Polygon */

int polygonColor (SDL_Surface *dst, Sint16 *vx, Sint16 *vy, int n, Uint32 color)
{
 int result;
 int i;
 Sint16 *x1, *y1, *x2, *y2;

 /* Sanity check */
 if (n<3) {
  return(-1);
 }

 /* Pointer setup */ 
 x1=x2=vx;
 y1=y2=vy;
 x2++;
 y2++;
 
 /* Draw */
 result=0;
 for (i=1; i<n; i++) {
  result |= lineColor (dst, *x1, *y1, *x2, *y2, color);
  x1=x2;
  y1=y2;
  x2++; 
  y2++; 
 }
 result |= lineColor (dst, *x1, *y1, *vx, *vy, color);

 return(result);
}

int polygonRGBA (SDL_Surface *dst, Sint16 *vx, Sint16 *vy, int n, Uint8 r, Uint8 g, Uint8 b, Uint8 a) 
{
 /* Draw */
 return(polygonColor(dst, vx, vy, n, ((Uint32)r << 24) | ((Uint32)g << 16) | ((Uint32)b << 8) | (Uint32)a)); 
}

/* ---- Filled Polygon */

int gfxPrimitivesCompareInt(const void *a, const void *b);

static int *gfxPrimitivesPolyInts=NULL;
static int gfxPrimitivesPolyAllocated=0;

int filledPolygonColor (SDL_Surface *dst, Sint16 *vx, Sint16 *vy, int n, int color)
{
	int result;
	int i;
	int y;
	int miny, maxy;
	int x1, y1;
	int x2, y2;
	int ind1, ind2;
	int ints;
	
	/* Sanity check */
	if (n<3) {
	 return -1;
	}
	
	/* Allocate temp array, only grow array */
	if (!gfxPrimitivesPolyAllocated) {
	 gfxPrimitivesPolyInts = (int *) malloc(sizeof(int) * n);
	 gfxPrimitivesPolyAllocated = n;
	} else {
	 if (gfxPrimitivesPolyAllocated<n) {
 	  gfxPrimitivesPolyInts = (int *) realloc(gfxPrimitivesPolyInts, sizeof(int) * n);
	  gfxPrimitivesPolyAllocated = n;
	 }
	}		

	/* Determine Y maxima */
	miny = vy[0];
	maxy = vy[0];
	for (i=1; (i < n); i++) {
		if (vy[i] < miny) {
		 miny = vy[i];
		} else if (vy[i] > maxy) {
		 maxy = vy[i];
		}
	}
	
	/* Draw, scanning y */
	result=0;
	for (y=miny; (y <= maxy); y++) {
		ints = 0;
		for (i=0; (i < n); i++) {
			if (!i) {
				ind1 = n-1;
				ind2 = 0;
			} else {
				ind1 = i-1;
				ind2 = i;
			}
			y1 = vy[ind1];
			y2 = vy[ind2];
			if (y1 < y2) {
				x1 = vx[ind1];
				x2 = vx[ind2];
			} else if (y1 > y2) {
				y2 = vy[ind1];
				y1 = vy[ind2];
				x2 = vx[ind1];
				x1 = vx[ind2];
			} else {
				continue;
			}
			if ((y >= y1) && (y < y2)) {
				gfxPrimitivesPolyInts[ints++] = (y-y1) * (x2-x1) / (y2-y1) + x1;
			} else if ((y == maxy) && (y > y1) && (y <= y2)) {
				gfxPrimitivesPolyInts[ints++] = (y-y1) * (x2-x1) / (y2-y1) + x1;
			}
		}
		qsort(gfxPrimitivesPolyInts, ints, sizeof(int), gfxPrimitivesCompareInt);

		for (i=0; (i<ints); i+=2) {
			result |= hlineColor(dst, gfxPrimitivesPolyInts[i], gfxPrimitivesPolyInts[i+1], y, color);
		}
	}
	
 return (result);
}

int filledPolygonRGBA (SDL_Surface *dst, Sint16 *vx, Sint16 *vy, int n, Uint8 r, Uint8 g, Uint8 b, Uint8 a) 
{
 /* Draw */
 return(filledPolygonColor(dst, vx, vy, n, ((Uint32)r << 24) | ((Uint32)g << 16) | ((Uint32)b << 8) | (Uint32)a)); 
}

int gfxPrimitivesCompareInt(const void *a, const void *b)
{
 return (*(const int *)a) - (*(const int *)b);
}

/* ---- Character (8x8 internal font) */

static SDL_Surface *gfxPrimitivesFont[256];
static Uint32 gfxPrimitivesFontColor[256];

int characterColor (SDL_Surface *dst, Sint16 x, Sint16 y, char c, Uint32 color)
{
 SDL_Rect srect;
 SDL_Rect drect;
 int result;
 int ix, iy, k;
 unsigned char *charpos;
 unsigned char bits[8]={128,64,32,16,8,4,2,1};
 unsigned char *bitpos;
 Uint8 *curpos;
 int forced_redraw;

 /* Setup source rectangle for 8x8 bitmap */
 srect.x=0;
 srect.y=0;
 srect.w=8;
 srect.h=8;
 
 /* Setup destination rectangle for 8x8 bitmap */
 drect.x=x;
 drect.y=y;
 drect.w=8;
 drect.h=8;

 /* Create new 8x8 bitmap surface if not already present */
 if (gfxPrimitivesFont[(unsigned char)c]==NULL) {
  gfxPrimitivesFont[(unsigned char)c]=SDL_CreateRGBSurface(SDL_SWSURFACE | SDL_HWSURFACE | SDL_SRCALPHA, 8, 8, 32, 0xFF000000, 0x00FF0000, 0x0000FF00, 0x000000FF);
  /* Check pointer */
  if (gfxPrimitivesFont[(unsigned char)c]==NULL) {
   return (-1);
  }
  /* Definitely redraw */
  forced_redraw=1;
 } else {
  forced_redraw=0;
 }

 /* Check if color has changed */
 if ((gfxPrimitivesFontColor[(unsigned char)c]!=color) || (forced_redraw)) {
   /* Redraw character */
   SDL_SetAlpha (gfxPrimitivesFont[(unsigned char)c], SDL_SRCALPHA, 255);
   gfxPrimitivesFontColor[(unsigned char)c]=color;

   /* Variable setup */
   k = (unsigned char)c;
   k *= 8;
   charpos=gfxPrimitivesFontdata;
   charpos += k;   
  
   /* Clear bitmap */
   curpos=(Uint8 *)gfxPrimitivesFont[(unsigned char)c]->pixels;
   memset (curpos, 0, 8*8*4);
 
   /* Drawing loop */
   for (iy=0; iy<8; iy++) {
    bitpos=bits;
    for (ix=0; ix<8; ix++) {
     if ((*charpos & *bitpos)==*bitpos) {
      memcpy(curpos,&color,4);
     } 
     bitpos++;
     curpos += 4;;
    }
    charpos++;
   }
 }

 /* Draw bitmap onto destination surface */
 result=SDL_BlitSurface (gfxPrimitivesFont[(unsigned char)c], &srect, dst, &drect);
 
 return(result);
}

int characterRGBA (SDL_Surface *dst, Sint16 x, Sint16 y, char c, Uint8 r, Uint8 g, Uint8 b, Uint8 a) 
{
 /* Draw */
 return(characterColor(dst, x, y, c, ((Uint32)r << 24) | ((Uint32)g << 16) | ((Uint32)b << 8) | (Uint32)a)); 
}

int stringColor (SDL_Surface *dst, Sint16 x, Sint16 y, char *c, Uint32 color)
{
 int result;
 int i,length;
 char *curchar;
 int curx;
   
 length=strlen(c);
 curchar=c;
 curx=x;   
 result=0;
 for (i=0; i<length; i++) {
  result |= characterColor(dst,curx,y,*curchar,color);
  curchar++;
  curx += 8;
 } 
 
 return(result);
}  

int stringRGBA (SDL_Surface *dst, Sint16 x, Sint16 y, char *c, Uint8 r, Uint8 g, Uint8 b, Uint8 a) 
{
 /* Draw */
 return(stringColor(dst, x, y, c, ((Uint32)r << 24) | ((Uint32)g << 16) | ((Uint32)b << 8) | (Uint32)a)); 
}


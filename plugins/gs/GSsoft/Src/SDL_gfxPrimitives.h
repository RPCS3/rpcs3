/* 

 SDL_gfxPrimitives.h
 
*/

#ifndef _SDL_gfxPrimitives_h
#define _SDL_gfxPrimitives_h

#include <math.h>
#ifndef M_PI
 #define M_PI	3.141592654
#endif

#include <SDL/SDL.h>

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

/* ----- Versioning */

#define SDL_GFXPRIMITIVES_MAJOR	1
#define SDL_GFXPRIMITIVES_MINOR	5

/* ----- W32 DLL interface */

#ifdef WIN32
 #define DLLINTERFACE DECLSPEC
#else
 #define DLLINTERFACE
#endif

/* ----- Prototypes */

/* Note: all ___Color routines expect the color to be in format 0xRRGGBBAA */

/* Pixel */

DLLINTERFACE int pixelColor (SDL_Surface *dst, Sint16 x, Sint16 y, Uint32 color);
DLLINTERFACE int pixelRGBA (SDL_Surface *dst, Sint16 x, Sint16 y, Uint8 r, Uint8 g, Uint8 b, Uint8 a);

/* Horizontal line */

DLLINTERFACE int hlineColor (SDL_Surface *dst, Sint16 x1, Sint16 x2, Sint16 y, Uint32 color);
DLLINTERFACE int hlineRGBA (SDL_Surface *dst, Sint16 x1, Sint16 x2, Sint16 y, Uint8 r, Uint8 g, Uint8 b, Uint8 a);

/* Vertical line */

DLLINTERFACE int vlineColor (SDL_Surface *dst, Sint16 x, Sint16 y1, Sint16 y2, Uint32 color);
DLLINTERFACE int vlineRGBA (SDL_Surface *dst, Sint16 x, Sint16 y1, Sint16 y2, Uint8 r, Uint8 g, Uint8 b, Uint8 a);

/* Rectangle */

DLLINTERFACE int rectangleColor (SDL_Surface *dst, Sint16 x1, Sint16 y1, Sint16 x2, Sint16 y2, Uint32 color);
DLLINTERFACE int rectangleRGBA (SDL_Surface *dst, Sint16 x1, Sint16 y1, Sint16 x2, Sint16 y2, Uint8 r, Uint8 g, Uint8 b, Uint8 a);

/* Filled rectangle (Box) */

DLLINTERFACE int boxColor (SDL_Surface *dst, Sint16 x1, Sint16 y1, Sint16 x2, Sint16 y2, Uint32 color);
DLLINTERFACE int boxRGBA (SDL_Surface *dst, Sint16 x1, Sint16 y1, Sint16 x2, Sint16 y2, Uint8 r, Uint8 g, Uint8 b, Uint8 a);

/* Line */

DLLINTERFACE int lineColor(SDL_Surface *dst, Sint16 x1, Sint16 y1, Sint16 x2, Sint16 y2, Uint32 color);
DLLINTERFACE int lineRGBA (SDL_Surface *dst, Sint16 x1, Sint16 y1, Sint16 x2, Sint16 y2, Uint8 r, Uint8 g, Uint8 b, Uint8 a);

/* AA Line */
DLLINTERFACE int aalineColor (SDL_Surface *dst, Sint16 x1, Sint16 y1, Sint16 x2, Sint16 y2, Uint32 color);
DLLINTERFACE int aalineRGBA (SDL_Surface *dst, Sint16 x1, Sint16 y1, Sint16 x2, Sint16 y2, Uint8 r, Uint8 g, Uint8 b, Uint8 a);

/* Circle */

DLLINTERFACE int circleColor(SDL_Surface *dst, Sint16 x, Sint16 y, Sint16 r, Uint32 color);
DLLINTERFACE int circleRGBA (SDL_Surface *dst, Sint16 x, Sint16 y, Sint16 rad, Uint8 r, Uint8 g, Uint8 b, Uint8 a);

/* AA Circle */

DLLINTERFACE int aacircleColor(SDL_Surface *dst, Sint16 x, Sint16 y, Sint16 r, Uint32 color);
DLLINTERFACE int aacircleRGBA (SDL_Surface *dst, Sint16 x, Sint16 y, Sint16 rad, Uint8 r, Uint8 g, Uint8 b, Uint8 a);

/* Filled Circle */

DLLINTERFACE int filledCircleColor(SDL_Surface *dst, Sint16 x, Sint16 y, Sint16 r, Uint32 color);
DLLINTERFACE int filledCircleRGBA (SDL_Surface *dst, Sint16 x, Sint16 y, Sint16 rad, Uint8 r, Uint8 g, Uint8 b, Uint8 a);

/* Ellipse */

DLLINTERFACE int ellipseColor (SDL_Surface *dst, Sint16 x, Sint16 y, Sint16 rx, Sint16 ry, Uint32 color);
DLLINTERFACE int ellipseRGBA (SDL_Surface *dst, Sint16 x, Sint16 y, Sint16 rx, Sint16 ry, Uint8 r, Uint8 g, Uint8 b, Uint8 a);

/* AA Ellipse */

DLLINTERFACE int aaellipseColor (SDL_Surface *dst, Sint16 x, Sint16 y, Sint16 rx, Sint16 ry, Uint32 color);
DLLINTERFACE int aaellipseRGBA (SDL_Surface *dst, Sint16 x, Sint16 y, Sint16 rx, Sint16 ry, Uint8 r, Uint8 g, Uint8 b, Uint8 a);

/* Filled Ellipse */

DLLINTERFACE int filledEllipseColor (SDL_Surface *dst, Sint16 x, Sint16 y, Sint16 rx, Sint16 ry, Uint32 color);
DLLINTERFACE int filledEllipseRGBA (SDL_Surface *dst, Sint16 x, Sint16 y, Sint16 rx, Sint16 ry, Uint8 r, Uint8 g, Uint8 b, Uint8 a);

/* Polygon */

DLLINTERFACE int polygonColor (SDL_Surface *dst, Sint16 *vx, Sint16 *vy, int n, Uint32 color);
DLLINTERFACE int polygonRGBA (SDL_Surface *dst, Sint16 *vx, Sint16 *vy, int n, Uint8 r, Uint8 g, Uint8 b, Uint8 a);

/* Filled Polygon */

DLLINTERFACE int filledPolygonColor (SDL_Surface *dst, Sint16 *vx, Sint16 *vy, int n, int color);
DLLINTERFACE int filledPolygonRGBA (SDL_Surface *dst, Sint16 *vx, Sint16 *vy, int n, Uint8 r, Uint8 g, Uint8 b, Uint8 a);

/* 8x8 Characters/Strings */

DLLINTERFACE int characterColor (SDL_Surface *dst, Sint16 x, Sint16 y, char c, Uint32 color);
DLLINTERFACE int characterRGBA (SDL_Surface *dst, Sint16 x, Sint16 y, char c, Uint8 r, Uint8 g, Uint8 b, Uint8 a);
DLLINTERFACE int stringColor (SDL_Surface *dst, Sint16 x, Sint16 y, char *c, Uint32 color);
DLLINTERFACE int stringRGBA (SDL_Surface *dst, Sint16 x, Sint16 y, char *c, Uint8 r, Uint8 g, Uint8 b, Uint8 a);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
};
#endif

#endif /* _SDL_gfxPrimitives_h */

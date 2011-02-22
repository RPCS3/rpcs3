/**
 * Automated SDL test common framework.
 *
 * Written by Edgar Simo "bobbens"
 *
 * Released under Public Domain.
 */


#ifndef COMMON_H
#  define COMMON_H


#  define FORMAT  SDL_PIXELFORMAT_ARGB8888
#  define AMASK   0xff000000 /**< Alpha bit mask. */
#  define RMASK   0x00ff0000 /**< Red bit mask. */
#  define GMASK   0x0000ff00 /**< Green bit mask. */
#  define BMASK   0x000000ff /**< Blue bit mask. */


typedef struct SurfaceImage_s {
   int width;
   int height;
   unsigned int  bytes_per_pixel; /* 3:RGB, 4:RGBA */ 
   const unsigned char pixel_data[];
} SurfaceImage_t;

#define ALLOWABLE_ERROR_OPAQUE	0
#define ALLOWABLE_ERROR_BLENDED	64

/**
 * @brief Compares a surface and a surface image for equality.
 *
 *    @param sur Surface to compare.
 *    @param img Image to compare against.
 *    @return 0 if they are the same, -1 on error and positive if different.
 */
int surface_compare( SDL_Surface *sur, const SurfaceImage_t *img, int allowable_error );


#endif /* COMMON_H */


/*
   This implements the AdvanceMAME Scale2x feature found on this page,
   http://scale2x.sourceforge.net/
   
   It is an incredibly simple and powerful image doubling routine that does
   an astonishing job of doubling game graphic data while interpolating out
   the jaggies. Congrats to the AdvanceMAME team, I'm very impressed and
   surprised with this code!
*/


#ifndef __SCALE2X_H__
#define __SCALE2X_H__

void scale2x(SDL_Surface *src, SDL_Surface *dst);

#endif /* __SCALE2X_H__ */

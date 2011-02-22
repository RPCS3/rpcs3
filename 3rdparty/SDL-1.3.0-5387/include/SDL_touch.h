/*
    SDL - Simple DirectMedia Layer
    Copyright (C) 1997-2011 Sam Lantinga

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

    Sam Lantinga
    slouken@libsdl.org
*/

/**
 *  \file SDL_touch.h
 *  
 *  Include file for SDL touch event handling.
 */

#ifndef _SDL_touch_h
#define _SDL_touch_h

#include "SDL_stdinc.h"
#include "SDL_error.h"
#include "SDL_video.h"

#include "begin_code.h"
/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
/* *INDENT-OFF* */
extern "C" {
/* *INDENT-ON* */
#endif


typedef Sint64 SDL_TouchID;
typedef Sint64 SDL_FingerID;


struct SDL_Finger {
  SDL_FingerID id;
  Uint16 x;
  Uint16 y;
  Uint16 pressure;
  Uint16 xdelta;
  Uint16 ydelta;
  Uint16 last_x, last_y,last_pressure;  /* the last reported coordinates */
  SDL_bool down;
};

typedef struct SDL_Touch SDL_Touch;
typedef struct SDL_Finger SDL_Finger;


struct SDL_Touch {
  
  /* Free the touch when it's time */
  void (*FreeTouch) (SDL_Touch * touch);
  
  /* data common for tablets */
  float pressure_max, pressure_min;
  float x_max,x_min;
  float y_max,y_min;
  Uint16 xres,yres,pressureres;
  float native_xres,native_yres,native_pressureres;
  float tilt;                   /* for future use */
  float rotation;               /* for future use */
  
  /* Data common to all touch */
  SDL_TouchID id;
  SDL_Window *focus;
  
  char *name;
  Uint8 buttonstate;
  SDL_bool relative_mode;
  SDL_bool flush_motion;

  int num_fingers;
  int max_fingers;
  SDL_Finger** fingers;
    
  void *driverdata;
};



/* Function prototypes */

/**
 *  \brief Get the touch object at the given id.
 *
 *
 */
  extern DECLSPEC SDL_Touch* SDLCALL SDL_GetTouch(SDL_TouchID id);



/**
 *  \brief Get the finger object of the given touch, at the given id.
 *
 *
 */
  extern 
  DECLSPEC SDL_Finger* SDLCALL SDL_GetFinger(SDL_Touch *touch, SDL_FingerID id);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
/* *INDENT-OFF* */
}
/* *INDENT-ON* */
#endif
#include "close_code.h"

#endif /* _SDL_mouse_h */

/* vi: set ts=4 sw=4 expandtab: */

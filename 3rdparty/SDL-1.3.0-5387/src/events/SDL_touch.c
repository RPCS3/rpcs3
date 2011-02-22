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
#include "SDL_config.h"

/* General touch handling code for SDL */

#include "SDL_events.h"
#include "SDL_events_c.h"
#include "../video/SDL_sysvideo.h"

#include <stdio.h>


static int SDL_num_touch = 0;
static SDL_Touch **SDL_touchPads = NULL;


/* Public functions */
int
SDL_TouchInit(void)
{
  return (0);
}

SDL_Touch *
SDL_GetTouch(SDL_TouchID id)
{
    int index = SDL_GetTouchIndexId(id);
    if (index < 0 || index >= SDL_num_touch) {
        return NULL;
    }
    return SDL_touchPads[index];
}

SDL_Touch *
SDL_GetTouchIndex(int index)
{
    if (index < 0 || index >= SDL_num_touch) {
        return NULL;
    }
    return SDL_touchPads[index];
}

int
SDL_GetFingerIndexId(SDL_Touch* touch,SDL_FingerID fingerid)
{
    int i;
    for(i = 0;i < touch->num_fingers;i++)
        if(touch->fingers[i]->id == fingerid)
            return i;
    return -1;
}


SDL_Finger *
SDL_GetFinger(SDL_Touch* touch,SDL_FingerID id)
{
    int index = SDL_GetFingerIndexId(touch,id);
    if(index < 0 || index >= touch->num_fingers)
        return NULL;
    return touch->fingers[index];
}


int
SDL_GetTouchIndexId(SDL_TouchID id)
{
    int index;
    SDL_Touch *touch;

    for (index = 0; index < SDL_num_touch; ++index) {
        touch = SDL_touchPads[index];
        if (touch->id == id) {
            return index;
        }
    }
    return -1;
}

int
SDL_AddTouch(const SDL_Touch * touch, char *name)
{
    SDL_Touch **touchPads;
    int index;
    size_t length;

    if (SDL_GetTouchIndexId(touch->id) != -1) {
        SDL_SetError("Touch ID already in use");
    }

    /* Add the touch to the list of touch */
    touchPads = (SDL_Touch **) SDL_realloc(SDL_touchPads,
                                      (SDL_num_touch + 1) * sizeof(*touch));
    if (!touchPads) {
        SDL_OutOfMemory();
        return -1;
    }

    SDL_touchPads = touchPads;
    index = SDL_num_touch++;

    SDL_touchPads[index] = (SDL_Touch *) SDL_malloc(sizeof(*SDL_touchPads[index]));
    if (!SDL_touchPads[index]) {
        SDL_OutOfMemory();
        return -1;
    }
    *SDL_touchPads[index] = *touch;

    /* we're setting the touch properties */
    length = 0;
    length = SDL_strlen(name);
    SDL_touchPads[index]->focus = 0;
    SDL_touchPads[index]->name = SDL_malloc((length + 2) * sizeof(char));
    SDL_strlcpy(SDL_touchPads[index]->name, name, length + 1);   

    SDL_touchPads[index]->num_fingers = 0;
    SDL_touchPads[index]->max_fingers = 1;
    SDL_touchPads[index]->fingers = (SDL_Finger **) SDL_malloc(sizeof(SDL_Finger*));
    SDL_touchPads[index]->fingers[0] = NULL;
    SDL_touchPads[index]->buttonstate = 0;
    SDL_touchPads[index]->relative_mode = SDL_FALSE;
    SDL_touchPads[index]->flush_motion = SDL_FALSE;
    
    SDL_touchPads[index]->xres = (1<<(16-1));
    SDL_touchPads[index]->yres = (1<<(16-1));
    //Do I want this here? Probably
    SDL_GestureAddTouch(SDL_touchPads[index]);

    return index;
}

void
SDL_DelTouch(SDL_TouchID id)
{
    int index = SDL_GetTouchIndexId(id);
    SDL_Touch *touch = SDL_GetTouch(id);

    if (!touch) {
        return;
    }

    
    SDL_free(touch->name);
 
    if (touch->FreeTouch) {
        touch->FreeTouch(touch);
    }
    SDL_free(touch);

    SDL_num_touch--;
    SDL_touchPads[index] = SDL_touchPads[SDL_num_touch];
}

void
SDL_TouchQuit(void)
{
    int i;

    for (i = SDL_num_touch-1; i > 0 ; --i) {
        SDL_DelTouch(i);
    }
    SDL_num_touch = 0;

    if (SDL_touchPads) {
        SDL_free(SDL_touchPads);
        SDL_touchPads = NULL;
    }
}

int
SDL_GetNumTouch(void)
{
    return SDL_num_touch;
}
SDL_Window *
SDL_GetTouchFocusWindow(SDL_TouchID id)
{
    SDL_Touch *touch = SDL_GetTouch(id);

    if (!touch) {
        return 0;
    }
    return touch->focus;
}

void
SDL_SetTouchFocus(SDL_TouchID id, SDL_Window * window)
{
    int index = SDL_GetTouchIndexId(id);
    SDL_Touch *touch = SDL_GetTouch(id);
    int i;
    SDL_bool focus;

    if (!touch || (touch->focus == window)) {
        return;
    }

    /* See if the current window has lost focus */
    if (touch->focus) {
        focus = SDL_FALSE;
        for (i = 0; i < SDL_num_touch; ++i) {
            SDL_Touch *check;
            if (i != index) {
                check = SDL_touchPads[i];
                if (check && check->focus == touch->focus) {
                    focus = SDL_TRUE;
                    break;
                }
            }
        }
        if (!focus) {
            SDL_SendWindowEvent(touch->focus, SDL_WINDOWEVENT_LEAVE, 0, 0);
        }
    }

    touch->focus = window;

    if (touch->focus) {
        focus = SDL_FALSE;
        for (i = 0; i < SDL_num_touch; ++i) {
            SDL_Touch *check;
            if (i != index) {
                check = SDL_touchPads[i];
                if (check && check->focus == touch->focus) {
                    focus = SDL_TRUE;
                    break;
                }
            }
        }
        if (!focus) {
            SDL_SendWindowEvent(touch->focus, SDL_WINDOWEVENT_ENTER, 0, 0);
        }
    }
}

int 
SDL_AddFinger(SDL_Touch* touch,SDL_Finger *finger)
{
    int index;
    SDL_Finger **fingers;
    //printf("Adding Finger...\n");
    if (SDL_GetFingerIndexId(touch,finger->id) != -1) {
        SDL_SetError("Finger ID already in use");
        }

    /* Add the touch to the list of touch */
    if(touch->num_fingers  >= touch->max_fingers){
                //printf("Making room for it!\n");
                fingers = (SDL_Finger **) SDL_realloc(touch->fingers,
                                                     (touch->num_fingers + 1) * sizeof(SDL_Finger *));
                touch->max_fingers = touch->num_fingers+1;
                if (!fingers) {
                        SDL_OutOfMemory();
                        return -1;
                } else {
                        touch->max_fingers = touch->num_fingers+1;
                        touch->fingers = fingers;
                }
        }

    index = touch->num_fingers;
    //printf("Max_Fingers: %i Index: %i\n",touch->max_fingers,index);

    touch->fingers[index] = (SDL_Finger *) SDL_malloc(sizeof(SDL_Finger));
    if (!touch->fingers[index]) {
        SDL_OutOfMemory();
        return -1;
    }
    *(touch->fingers[index]) = *finger;
    touch->num_fingers++;

    return index;
}

int
SDL_DelFinger(SDL_Touch* touch,SDL_FingerID fingerid)
{
    int index = SDL_GetFingerIndexId(touch,fingerid);
    SDL_Finger* finger = SDL_GetFinger(touch,fingerid);

    if (!finger) {
        return -1;
    }
 

    SDL_free(finger);
    touch->num_fingers--;
    touch->fingers[index] = touch->fingers[touch->num_fingers];
    return 0;
}


int
SDL_SendFingerDown(SDL_TouchID id, SDL_FingerID fingerid, SDL_bool down, 
                   float xin, float yin, float pressurein)
{
    int posted;
        Uint16 x;
        Uint16 y;
        Uint16 pressure;
        SDL_Finger *finger;

    SDL_Touch* touch = SDL_GetTouch(id);

    if(!touch) {
      return SDL_TouchNotFoundError(id);
    }

    
    //scale to Integer coordinates
    x = (Uint16)((xin+touch->x_min)*(touch->xres)/(touch->native_xres));
    y = (Uint16)((yin+touch->y_min)*(touch->yres)/(touch->native_yres));
    pressure = (Uint16)((yin+touch->pressure_min)*(touch->pressureres)/(touch->native_pressureres));
    
    finger = SDL_GetFinger(touch,fingerid);
    if(down) {
        if(finger == NULL) {
            SDL_Finger nf;
            nf.id = fingerid;
            nf.x = x;
            nf.y = y;
            nf.pressure = pressure;
            nf.xdelta = 0;
            nf.ydelta = 0;
            nf.last_x = x;
            nf.last_y = y;
            nf.last_pressure = pressure;
            nf.down = SDL_FALSE;
            if(SDL_AddFinger(touch,&nf) < 0) return 0;
            finger = SDL_GetFinger(touch,fingerid);
        }
        else if(finger->down) return 0;
        if(xin < touch->x_min || yin < touch->y_min) return 0; //should defer if only a partial input
        posted = 0;
        if (SDL_GetEventState(SDL_FINGERDOWN) == SDL_ENABLE) {
            SDL_Event event;
            event.tfinger.type = SDL_FINGERDOWN;
            event.tfinger.touchId = id;
            event.tfinger.x = x;
            event.tfinger.y = y;
            event.tfinger.pressure = pressure;
            event.tfinger.state = touch->buttonstate;
            event.tfinger.windowID = touch->focus ? touch->focus->id : 0;
            event.tfinger.fingerId = fingerid;
            posted = (SDL_PushEvent(&event) > 0);
        }
        if(posted) finger->down = SDL_TRUE;
        return posted;
    }
    else {
        if(finger == NULL) {
            SDL_SetError("Finger not found.");
            return 0;
        }      
        posted = 0;
        if (SDL_GetEventState(SDL_FINGERUP) == SDL_ENABLE) {
            SDL_Event event;
            event.tfinger.type = SDL_FINGERUP;
            event.tfinger.touchId =  id;
            event.tfinger.state = touch->buttonstate;
            event.tfinger.windowID = touch->focus ? touch->focus->id : 0;
            event.tfinger.fingerId = fingerid;
            //I don't trust the coordinates passed on fingerUp
            event.tfinger.x = finger->x; 
            event.tfinger.y = finger->y;
            event.tfinger.dx = 0;
            event.tfinger.dy = 0;

            if(SDL_DelFinger(touch,fingerid) < 0) return 0;
            posted = (SDL_PushEvent(&event) > 0);
        }        
        return posted;
    }
}

int
SDL_SendTouchMotion(SDL_TouchID id, SDL_FingerID fingerid, int relative, 
                    float xin, float yin, float pressurein)
{
    int index = SDL_GetTouchIndexId(id);
    SDL_Touch *touch;
    SDL_Finger *finger;
    int posted;
    Sint16 xrel, yrel;
    Uint16 x;
    Uint16 y;
    Uint16 pressure;
    
    touch = SDL_GetTouch(id);
    if (!touch) {
      return SDL_TouchNotFoundError(id);
    }

    //scale to Integer coordinates
    x = (Uint16)((xin+touch->x_min)*(touch->xres)/(touch->native_xres));
    y = (Uint16)((yin+touch->y_min)*(touch->yres)/(touch->native_yres));
    pressure = (Uint16)((yin+touch->pressure_min)*(touch->pressureres)/(touch->native_pressureres));
    if(touch->flush_motion) {
        return 0;
    }
    
    finger = SDL_GetFinger(touch,fingerid);
    if(finger == NULL || !finger->down) {
        return SDL_SendFingerDown(id,fingerid,SDL_TRUE,xin,yin,pressurein);        
    } else {
        /* the relative motion is calculated regarding the last position */
        if (relative) {
            xrel = x;
            yrel = y;
            x = (finger->last_x + x);
            y = (finger->last_y + y);
        } else {
            if(xin < touch->x_min) x = finger->last_x; /*If movement is only in one axis,*/
            if(yin < touch->y_min) y = finger->last_y; /*The other is marked as -1*/
            if(pressurein < touch->pressure_min) pressure = finger->last_pressure;
            xrel = x - finger->last_x;
            yrel = y - finger->last_y;
            //printf("xrel,yrel (%i,%i)\n",(int)xrel,(int)yrel);
        }
        
        /* Drop events that don't change state */
        if (!xrel && !yrel) {
#if 0
            printf("Touch event didn't change state - dropped!\n");
#endif
            return 0;
        }
        
        /* Update internal touch coordinates */
        
        finger->x = x;
        finger->y = y;
        
        /*Should scale to window? Normalize? Maintain Aspect?*/
        //SDL_GetWindowSize(touch->focus, &x_max, &y_max);
        
        /* make sure that the pointers find themselves inside the windows */
        /* only check if touch->xmax is set ! */
        /*
          if (x_max && touch->x > x_max) {
          touch->x = x_max;
          } else if (touch->x < 0) {
          touch->x = 0;
          }
          
          if (y_max && touch->y > y_max) {
          touch->y = y_max;
          } else if (touch->y < 0) {
          touch->y = 0;
          }
        */
        finger->xdelta = xrel;
        finger->ydelta = yrel;
        finger->pressure = pressure;
        
        
        
        /* Post the event, if desired */
        posted = 0;
        if (SDL_GetEventState(SDL_FINGERMOTION) == SDL_ENABLE) {
            SDL_Event event;
            event.tfinger.type = SDL_FINGERMOTION;
            event.tfinger.touchId = id;
            event.tfinger.fingerId = fingerid;
            event.tfinger.x = x;
            event.tfinger.y = y;
            event.tfinger.dx = xrel;
            event.tfinger.dy = yrel;            
                
            event.tfinger.pressure = pressure;
            event.tfinger.state = touch->buttonstate;
            event.tfinger.windowID = touch->focus ? touch->focus->id : 0;
            posted = (SDL_PushEvent(&event) > 0);
        }
        finger->last_x = finger->x;
        finger->last_y = finger->y;
        finger->last_pressure = finger->pressure;
        return posted;
    }
}

int
SDL_SendTouchButton(SDL_TouchID id, Uint8 state, Uint8 button)
{
    SDL_Touch *touch;
    int posted;
    Uint32 type;

    
    touch = SDL_GetTouch(id);
    if (!touch) {
      return SDL_TouchNotFoundError(id);
    }

    /* Figure out which event to perform */
    switch (state) {
    case SDL_PRESSED:
        if (touch->buttonstate & SDL_BUTTON(button)) {
            /* Ignore this event, no state change */
            return 0;
        }
        type = SDL_TOUCHBUTTONDOWN;
        touch->buttonstate |= SDL_BUTTON(button);
        break;
    case SDL_RELEASED:
        if (!(touch->buttonstate & SDL_BUTTON(button))) {
            /* Ignore this event, no state change */
            return 0;
        }
        type = SDL_TOUCHBUTTONUP;
        touch->buttonstate &= ~SDL_BUTTON(button);
        break;
    default:
        /* Invalid state -- bail */
        return 0;
    }

    /* Post the event, if desired */
    posted = 0;
    if (SDL_GetEventState(type) == SDL_ENABLE) {
        SDL_Event event;
        event.type = type;
        event.tbutton.touchId = touch->id;
        event.tbutton.state = state;
        event.tbutton.button = button;
        event.tbutton.windowID = touch->focus ? touch->focus->id : 0;
        posted = (SDL_PushEvent(&event) > 0);
    }
    return posted;
}

char *
SDL_GetTouchName(SDL_TouchID id)
{
    SDL_Touch *touch = SDL_GetTouch(id);
    if (!touch) {
        return NULL;
    }
    return touch->name;
}

int SDL_TouchNotFoundError(SDL_TouchID id) {
  //int i;
  SDL_SetError("ERROR: Cannot send touch on non-existent device with id: %li make sure SDL_AddTouch has been called\n",id);
#if 0
  printf("ERROR: There are %i touches installed with Id's:\n",SDL_num_touch);
  for(i=0;i < SDL_num_touch;i++) {
    printf("ERROR: %li\n",SDL_touchPads[i]->id);
  }
#endif
  return 0;
}
/* vi: set ts=4 sw=4 expandtab: */

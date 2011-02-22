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
#include "SDL_x11video.h"
#include "SDL_x11touch.h"
#include "../../events/SDL_touch_c.h"


#ifdef SDL_INPUT_LINUXEV
#include <linux/input.h>
#include <fcntl.h>
#endif

void
X11_InitTouch(_THIS)
{
#ifdef SDL_INPUT_LINUXEV
  FILE *fd;
  fd = fopen("/proc/bus/input/devices","r");
  
  char c;
  int i = 0;
  char line[256];
  char tstr[256];
  int vendor = -1,product = -1,event = -1;
  while(!feof(fd)) {
    if(fgets(line,256,fd) <=0) continue;
    if(line[0] == '\n') {
      if(vendor == 1386){
	/*printf("Wacom... Assuming it is a touch device\n");*/
	/*sprintf(tstr,"/dev/input/event%i",event);*/
	/*printf("At location: %s\n",tstr);*/

	SDL_Touch touch;
	touch.pressure_max = 0;
	touch.pressure_min = 0;
	touch.id = event; 
	

	touch.driverdata = SDL_malloc(sizeof(EventTouchData));
	EventTouchData* data = (EventTouchData*)(touch.driverdata);

	data->x = -1;
	data->y = -1;
	data->pressure = -1;
	data->finger = 0;
	data->up = SDL_FALSE;
	

	data->eventStream = open(tstr, 
				 O_RDONLY | O_NONBLOCK);
	ioctl (data->eventStream, EVIOCGNAME (sizeof (tstr)), tstr);

	int abs[5];
	ioctl(data->eventStream,EVIOCGABS(0),abs);	
	touch.x_min = abs[1];
	touch.x_max = abs[2];
	touch.native_xres = touch.x_max - touch.x_min;
	ioctl(data->eventStream,EVIOCGABS(ABS_Y),abs);	
	touch.y_min = abs[1];
	touch.y_max = abs[2];
	touch.native_yres = touch.y_max - touch.y_min;
	ioctl(data->eventStream,EVIOCGABS(ABS_PRESSURE),abs);	
	touch.pressure_min = abs[1];
	touch.pressure_max = abs[2];
	touch.native_pressureres = touch.pressure_max - touch.pressure_min;

	SDL_AddTouch(&touch, tstr);
      }
      vendor = -1;
      product = -1;
      event = -1;      
    }
    else if(line[0] == 'I') {
      i = 1;
      while(line[i]) {
	sscanf(&line[i],"Vendor=%x",&vendor);
	sscanf(&line[i],"Product=%x",&product);
	i++;
      }
    }
    else if(line[0] == 'H') {
      i = 1;
      while(line[i]) {
	sscanf(&line[i],"event%d",&event);
	i++;
      }
    }
  }
  
  close(fd);
#endif
}

void
X11_QuitTouch(_THIS)
{
    SDL_TouchQuit();
}

/* vi: set ts=4 sw=4 expandtab: */

/*  ecma119.c

 *  Copyright (C) 2002-2005  PCSX2 Team

 *

 *  This program is free software; you can redistribute it and/or modify

 *  it under the terms of the GNU General Public License as published by

 *  the Free Software Foundation; either version 2 of the License, or

 *  (at your option) any later version.

 *

 *  This program is distributed in the hope that it will be useful,

 *  but WITHOUT ANY WARRANTY; without even the implied warranty of

 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the

 *  GNU General Public License for more details.

 *

 *  You should have received a copy of the GNU General Public License

 *  along with this program; if not, write to the Free Software

 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

 *

 *  PCSX2 members can be contacted through their website at www.pcsx2.net.

 */





#include <stddef.h>



// #ifndef __LINUX__

// #ifdef __linux__

// #define __LINUX__

// #endif /* __linux__ */

// #endif /* No __LINUX__ */



// #define CDVDdefs

// #include "PS2Edefs.h"



#include "ecma119.h"





const char ECMA119VolumeIDstdid[] = "CD001\0";





int ValidateECMA119PrimaryVolume(struct ECMA119PrimaryVolume *volume) {

  int i;



  if(volume == NULL)  return(-1);



  // Volume ID

  if(volume->id.voltype != 1)  return(-1); // Incorrect volume type

  if(volume->id.version != 1)  return(-1); // Not a Standard Version?

  i = 0;

  while((ECMA119VolumeIDstdid[i] != 0) &&

        (ECMA119VolumeIDstdid[i] == volume->id.stdid[i]))  i++;

  if(ECMA119VolumeIDstdid[i] != 0)  return(-1); // "CD001" did not match?



  // Looks like numblocksle might give us maximum sector count...

  // Looks like blocksizele can be compared to blocksize stored in isofile...



  return(0);

} // END ValidateECMA119PrimaryVolume()





// Not sure the Partition Volume will be much help...


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

#import "SDLUIAccelerationDelegate.h"
/* needed for SDL_IPHONE_MAX_GFORCE macro */
#import "../../../include/SDL_config_iphoneos.h"

static SDLUIAccelerationDelegate *sharedDelegate=nil;

@implementation SDLUIAccelerationDelegate

/*
	Returns a shared instance of the SDLUIAccelerationDelegate, creating the shared delegate if it doesn't exist yet.
*/
+(SDLUIAccelerationDelegate *)sharedDelegate {
	if (sharedDelegate == nil) {
		sharedDelegate = [[SDLUIAccelerationDelegate alloc] init];
	}
	return sharedDelegate;
}
/*
	UIAccelerometerDelegate delegate method.  Invoked by the UIAccelerometer instance when it has new data for us.
	We just take the data and mark that we have new data available so that the joystick system will pump it to the
	events system when SDL_SYS_JoystickUpdate is called.
*/	
-(void)accelerometer:(UIAccelerometer *)accelerometer didAccelerate:(UIAcceleration *)acceleration {
		
	x = acceleration.x;
	y = acceleration.y;
	z = acceleration.z;
	
	hasNewData = YES;	
}
/* 
	getLastOrientation -- put last obtained accelerometer data into Sint16 array
 
	Called from the joystick system when it needs the accelerometer data.
	Function grabs the last data sent to the accelerometer and converts it 
	from floating point to Sint16, which is what the joystick system expects.
 
	To do the conversion, the data is first clamped onto the interval
	[-SDL_IPHONE_MAX_G_FORCE, SDL_IPHONE_MAX_G_FORCE], then the data is multiplied
	by MAX_SINT16 so that it is mapped to the full range of an Sint16.
 
	You can customize the clamped range of this function by modifying the 
	SDL_IPHONE_MAX_GFORCE macro in SDL_config_iphoneos.h.
 
	Once converted to Sint16, the accelerometer data no longer has coherent units.
	You can convert the data back to units of g-force by multiplying it 
	in your application's code by SDL_IPHONE_MAX_GFORCE / 0x7FFF.
 */
-(void)getLastOrientation:(Sint16 *)data {

	#define MAX_SINT16 0x7FFF	

	/* clamp the data */
	if (x > SDL_IPHONE_MAX_GFORCE) x = SDL_IPHONE_MAX_GFORCE;
	else if (x < -SDL_IPHONE_MAX_GFORCE) x = -SDL_IPHONE_MAX_GFORCE;
	if (y > SDL_IPHONE_MAX_GFORCE) y = SDL_IPHONE_MAX_GFORCE;
	else if (y < -SDL_IPHONE_MAX_GFORCE) y = -SDL_IPHONE_MAX_GFORCE;
	if (z > SDL_IPHONE_MAX_GFORCE) z = SDL_IPHONE_MAX_GFORCE;
	else if (z < -SDL_IPHONE_MAX_GFORCE) z = -SDL_IPHONE_MAX_GFORCE;
	
	/* pass in data mapped to range of SInt16 */
	data[0] = (x / SDL_IPHONE_MAX_GFORCE) * MAX_SINT16;
	data[1] = (y / SDL_IPHONE_MAX_GFORCE) * MAX_SINT16;
	data[2] = (z / SDL_IPHONE_MAX_GFORCE) * MAX_SINT16;

}

/*
	Initialize SDLUIAccelerationDelegate.  Since we don't have any data yet,
	just set our last received data to zero, and indicate we don't have any;
*/
-(id)init {
	self = [super init];
	x = y = z = 0.0;
	hasNewData = NO;
	return self;
}

-(void)dealloc {
	sharedDelegate = nil;
	[self shutdown];
	[super dealloc];
}

/*
	Lets our delegate start receiving accelerometer updates.
*/
-(void)startup {
	[UIAccelerometer sharedAccelerometer].delegate = self;
	isRunning = YES;
}
/*
	Stops our delegate from receiving accelerometer updates.
*/
-(void)shutdown {
	if ([UIAccelerometer sharedAccelerometer].delegate == self) {
		[UIAccelerometer sharedAccelerometer].delegate = nil;
	}
	isRunning = NO;
}
/*
	Our we currently receiving accelerometer updates?
*/
-(BOOL)isRunning {
	return isRunning;
}
/*
	Do we have any data that hasn't been pumped into SDL's event system?
*/
-(BOOL)hasNewData {
	return hasNewData;
}
/*
	When the joystick system grabs the new data, it sets this to NO.
*/
-(void)setHasNewData:(BOOL)value {
	hasNewData = value;
}

@end

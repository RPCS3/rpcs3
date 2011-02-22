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

#import <UIKit/UIKit.h>
#import <OpenGLES/EAGL.h>
#import <OpenGLES/ES1/gl.h>
#import <OpenGLES/ES1/glext.h>
#import "SDL_uikitview.h"
/*
    This class wraps the CAEAGLLayer from CoreAnimation into a convenient UIView subclass.
    The view content is basically an EAGL surface you render your OpenGL scene into.
    Note that setting the view non-opaque will only work if the EAGL surface has an alpha channel.
 */
/* *INDENT-OFF* */
@interface SDL_uikitopenglview : SDL_uikitview {
    
@private
    /* The pixel dimensions of the backbuffer */
    GLint backingWidth;
    GLint backingHeight;
    
    EAGLContext *context;
    
    /* OpenGL names for the renderbuffer and framebuffers used to render to this view */
    GLuint viewRenderbuffer, viewFramebuffer;
    
    /* OpenGL name for the depth buffer that is attached to viewFramebuffer, if it exists (0 if it does not exist) */
    GLuint depthRenderbuffer;
    
}

@property (nonatomic, retain, readonly) EAGLContext *context;

- (void)swapBuffers;
- (void)setCurrentContext;

- (id)initWithFrame:(CGRect)frame
    retainBacking:(BOOL)retained \
    rBits:(int)rBits \
    gBits:(int)gBits \
    bBits:(int)bBits \
    aBits:(int)aBits \
    depthBits:(int)depthBits \
    majorVersion:(int)majorVersion;

@end
/* *INDENT-ON* */

/* vi: set ts=4 sw=4 expandtab: */

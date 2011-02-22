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

    SDL1.3 DirectFB driver by couriersud@arcor.de
	
*/

#include "SDL_DirectFB_video.h"

#if SDL_DIRECTFB_OPENGL
#include "SDL_DirectFB_opengl.h"
#include "SDL_DirectFB_window.h"

#include <directfbgl.h>
#include "SDL_loadso.h"
#endif

#if SDL_DIRECTFB_OPENGL

struct SDL_GLDriverData
{
    int gl_active;              /* to stop switching drivers while we have a valid context */
    int initialized;
    DirectFB_GLContext *firstgl;        /* linked list */
    
    /* OpenGL */
    void (*glFinish) (void);
    void (*glFlush) (void);
};

#define OPENGL_REQUIRS_DLOPEN
#if defined(OPENGL_REQUIRS_DLOPEN) && defined(SDL_LOADSO_DLOPEN)
#include <dlfcn.h>
#define GL_LoadObject(X)	dlopen(X, (RTLD_NOW|RTLD_GLOBAL))
#define GL_LoadFunction		dlsym
#define GL_UnloadObject		dlclose
#else
#define GL_LoadObject	SDL_LoadObject
#define GL_LoadFunction	SDL_LoadFunction
#define GL_UnloadObject	SDL_UnloadObject
#endif

static void DirectFB_GL_UnloadLibrary(_THIS);

int
DirectFB_GL_Initialize(_THIS)
{
    if (_this->gl_data) {
        return 0;
    }

    _this->gl_data =
        (struct SDL_GLDriverData *) SDL_calloc(1,
                                               sizeof(struct
                                                      SDL_GLDriverData));
    if (!_this->gl_data) {
        SDL_OutOfMemory();
        return -1;
    }
    _this->gl_data->initialized = 0;

    ++_this->gl_data->initialized;
    _this->gl_data->firstgl = NULL;

    if (DirectFB_GL_LoadLibrary(_this, NULL) < 0) {
        return -1;
    }

    /* Initialize extensions */
    /* FIXME needed?
     * X11_GL_InitExtensions(_this);
     */

    return 0;
}

void
DirectFB_GL_Shutdown(_THIS)
{
    if (!_this->gl_data || (--_this->gl_data->initialized > 0)) {
        return;
    }

    DirectFB_GL_UnloadLibrary(_this);

    SDL_free(_this->gl_data);
    _this->gl_data = NULL;
}

int
DirectFB_GL_LoadLibrary(_THIS, const char *path)
{
    //SDL_DFB_DEVICEDATA(_this);

    void *handle = NULL;

    SDL_DFB_DEBUG("Loadlibrary : %s\n", path);

    if (_this->gl_data->gl_active) {
        SDL_SetError("OpenGL context already created");
        return -1;
    }


    if (path == NULL) {
        path = SDL_getenv("SDL_VIDEO_GL_DRIVER");
        if (path == NULL) {
            path = "libGL.so";
        }
    }

    handle = GL_LoadObject(path);
    if (handle == NULL) {
        SDL_DFB_ERR("Library not found: %s\n", path);
        /* SDL_LoadObject() will call SDL_SetError() for us. */
        return -1;
    }

    SDL_DFB_DEBUG("Loaded library: %s\n", path);

    _this->gl_config.dll_handle = handle;
    _this->gl_config.driver_loaded = 1;
    if (path) {
        SDL_strlcpy(_this->gl_config.driver_path, path,
                    SDL_arraysize(_this->gl_config.driver_path));
    } else {
        *_this->gl_config.driver_path = '\0';
    }

    _this->gl_data->glFinish = DirectFB_GL_GetProcAddress(_this, "glFinish");
    _this->gl_data->glFlush = DirectFB_GL_GetProcAddress(_this, "glFlush");

    return 0;
}

static void
DirectFB_GL_UnloadLibrary(_THIS)
{
 #if 0
    int ret;

    if (_this->gl_config.driver_loaded) {

        ret = GL_UnloadObject(_this->gl_config.dll_handle);
        if (ret)
            SDL_DFB_ERR("Error #%d trying to unload library.\n", ret);
        _this->gl_config.dll_handle = NULL;
        _this->gl_config.driver_loaded = 0;
    }
#endif
    /* Free OpenGL memory */
    SDL_free(_this->gl_data);
    _this->gl_data = NULL;
}

void *
DirectFB_GL_GetProcAddress(_THIS, const char *proc)
{
    void *handle;

    handle = _this->gl_config.dll_handle;
    return GL_LoadFunction(handle, proc);
}

SDL_GLContext
DirectFB_GL_CreateContext(_THIS, SDL_Window * window)
{
    //SDL_DFB_DEVICEDATA(_this);
    SDL_DFB_WINDOWDATA(window);
    DirectFB_GLContext *context;

    SDL_DFB_ALLOC_CLEAR(context, sizeof(DirectFB_GLContext));

    SDL_DFB_CHECKERR(windata->surface->GetGL(windata->surface,
                                             &context->context));

    if (!context->context)
        return NULL;

    context->is_locked = 0;
    context->sdl_window = window;
    
    context->next = _this->gl_data->firstgl;
    _this->gl_data->firstgl = context;

    SDL_DFB_CHECK(context->context->Unlock(context->context));

    if (DirectFB_GL_MakeCurrent(_this, window, context) < 0) {
        DirectFB_GL_DeleteContext(_this, context);
        return NULL;
    }

    return context;

  error:
    return NULL;
}

int
DirectFB_GL_MakeCurrent(_THIS, SDL_Window * window, SDL_GLContext context)
{
    //SDL_DFB_WINDOWDATA(window);
    DirectFB_GLContext *ctx = (DirectFB_GLContext *) context;
    DirectFB_GLContext *p;

    for (p = _this->gl_data->firstgl; p; p = p->next)
    {
       if (p->is_locked) {
         SDL_DFB_CHECKERR(p->context->Unlock(p->context));
         p->is_locked = 0;
       }
        
    }

    if (ctx != NULL) {
        SDL_DFB_CHECKERR(ctx->context->Lock(ctx->context));
        ctx->is_locked = 1;
    }

    return 0;
  error:
    return -1;
}

int
DirectFB_GL_SetSwapInterval(_THIS, int interval)
{
    SDL_Unsupported();
    return -1;
}

int
DirectFB_GL_GetSwapInterval(_THIS)
{
    SDL_Unsupported();
    return -1;
}

void
DirectFB_GL_SwapWindow(_THIS, SDL_Window * window)
{
    //SDL_DFB_DEVICEDATA(_this);
    SDL_DFB_WINDOWDATA(window);
    DFBRegion region;
    DirectFB_GLContext *p;

    region.x1 = 0;
    region.y1 = 0;
    region.x2 = window->w;
    region.y2 = window->h;

#if 0
    if (devdata->glFinish)
        devdata->glFinish();
    else if (devdata->glFlush)
        devdata->glFlush();
#endif

  	for (p = _this->gl_data->firstgl; p != NULL; p = p->next)
        if (p->sdl_window == window && p->is_locked)
        {
            SDL_DFB_CHECKERR(p->context->Unlock(p->context));
            p->is_locked = 0;
        }            

    SDL_DFB_CHECKERR(windata->window_surface->Flip(windata->window_surface,NULL,  DSFLIP_PIPELINE |DSFLIP_BLIT | DSFLIP_ONSYNC ));

    //if (windata->gl_context) {
        //SDL_DFB_CHECKERR(windata->surface->Flip(windata->surface,NULL, DSFLIP_ONSYNC)); 
        //SDL_DFB_CHECKERR(windata->gl_context->context->Lock(windata->gl_context->context));
    //}

    return;
  error:
    return;
}

void
DirectFB_GL_DeleteContext(_THIS, SDL_GLContext context)
{
    DirectFB_GLContext *ctx = (DirectFB_GLContext *) context;
    DirectFB_GLContext *p;

    if (ctx->is_locked)
        SDL_DFB_CHECK(ctx->context->Unlock(ctx->context));
    SDL_DFB_RELEASE(ctx->context);

    for (p = _this->gl_data->firstgl; p && p->next != ctx; p = p->next)
        ;
    if (p)
        p->next = ctx->next;
    else
        _this->gl_data->firstgl = ctx->next;

    SDL_DFB_FREE(ctx);
}

void
DirectFB_GL_FreeWindowContexts(_THIS, SDL_Window * window)
{
    DirectFB_GLContext *p;
    
	for (p = _this->gl_data->firstgl; p != NULL; p = p->next)
	    if (p->sdl_window == window)
	    {
	    	if (p->is_locked)
	        	SDL_DFB_CHECK(p->context->Unlock(p->context));
	        SDL_DFB_RELEASE(p->context);
	    }
}

void
DirectFB_GL_ReAllocWindowContexts(_THIS, SDL_Window * window)
{
    DirectFB_GLContext *p;

	for (p = _this->gl_data->firstgl; p != NULL; p = p->next)
	    if (p->sdl_window == window)
	    {
            SDL_DFB_WINDOWDATA(window);
			SDL_DFB_CHECK(windata->surface->GetGL(windata->surface,
	                                         &p->context));
    		if (p->is_locked)
            	SDL_DFB_CHECK(p->context->Lock(p->context));
	        }
}

void
DirectFB_GL_DestroyWindowContexts(_THIS, SDL_Window * window)
{
    DirectFB_GLContext *p;

	for (p = _this->gl_data->firstgl; p != NULL; p = p->next)
		if (p->sdl_window == window)
			DirectFB_GL_DeleteContext(_this, p);
}

#endif

/* vi: set ts=4 sw=4 expandtab: */

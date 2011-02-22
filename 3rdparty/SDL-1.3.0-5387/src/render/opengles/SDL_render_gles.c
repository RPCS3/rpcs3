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

#if SDL_VIDEO_RENDER_OGL_ES && !SDL_RENDER_DISABLED

#include "SDL_opengles.h"
#include "../SDL_sysrender.h"

#if defined(SDL_VIDEO_DRIVER_PANDORA)

/* Empty function stub to get OpenGL ES 1.x support without  */
/* OpenGL ES extension GL_OES_draw_texture supported         */
GL_API void GL_APIENTRY
glDrawTexiOES(GLint x, GLint y, GLint z, GLint width, GLint height)
{
    return;
}

#endif /* PANDORA */

/* OpenGL ES 1.1 renderer implementation, based on the OpenGL renderer */

static const float inv255f = 1.0f / 255.0f;

static SDL_Renderer *GLES_CreateRenderer(SDL_Window * window, Uint32 flags);
static void GLES_WindowEvent(SDL_Renderer * renderer,
                             const SDL_WindowEvent *event);
static int GLES_CreateTexture(SDL_Renderer * renderer, SDL_Texture * texture);
static int GLES_UpdateTexture(SDL_Renderer * renderer, SDL_Texture * texture,
                              const SDL_Rect * rect, const void *pixels,
                              int pitch);
static int GLES_LockTexture(SDL_Renderer * renderer, SDL_Texture * texture,
                            const SDL_Rect * rect, void **pixels, int *pitch);
static void GLES_UnlockTexture(SDL_Renderer * renderer,
                               SDL_Texture * texture);
static int GLES_UpdateViewport(SDL_Renderer * renderer);
static int GLES_RenderClear(SDL_Renderer * renderer);
static int GLES_RenderDrawPoints(SDL_Renderer * renderer,
                                 const SDL_Point * points, int count);
static int GLES_RenderDrawLines(SDL_Renderer * renderer,
                                const SDL_Point * points, int count);
static int GLES_RenderFillRects(SDL_Renderer * renderer,
                                const SDL_Rect * rects, int count);
static int GLES_RenderCopy(SDL_Renderer * renderer, SDL_Texture * texture,
                           const SDL_Rect * srcrect,
                           const SDL_Rect * dstrect);
static void GLES_RenderPresent(SDL_Renderer * renderer);
static void GLES_DestroyTexture(SDL_Renderer * renderer,
                                SDL_Texture * texture);
static void GLES_DestroyRenderer(SDL_Renderer * renderer);


SDL_RenderDriver GLES_RenderDriver = {
    GLES_CreateRenderer,
    {
     "opengles",
     (SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC),
     1,
     {SDL_PIXELFORMAT_ABGR8888},
     0,
     0}
};

typedef struct
{
    SDL_GLContext context;
    struct {
        Uint32 color;
        int blendMode;
        GLenum scaleMode;
        SDL_bool tex_coords;
    } current;

    SDL_bool useDrawTexture;
    SDL_bool GL_OES_draw_texture_supported;
} GLES_RenderData;

typedef struct
{
    GLuint texture;
    GLenum type;
    GLfloat texw;
    GLfloat texh;
    GLenum format;
    GLenum formattype;
    void *pixels;
    int pitch;
    GLenum scaleMode;
} GLES_TextureData;

static void
GLES_SetError(const char *prefix, GLenum result)
{
    const char *error;

    switch (result) {
    case GL_NO_ERROR:
        error = "GL_NO_ERROR";
        break;
    case GL_INVALID_ENUM:
        error = "GL_INVALID_ENUM";
        break;
    case GL_INVALID_VALUE:
        error = "GL_INVALID_VALUE";
        break;
    case GL_INVALID_OPERATION:
        error = "GL_INVALID_OPERATION";
        break;
    case GL_STACK_OVERFLOW:
        error = "GL_STACK_OVERFLOW";
        break;
    case GL_STACK_UNDERFLOW:
        error = "GL_STACK_UNDERFLOW";
        break;
    case GL_OUT_OF_MEMORY:
        error = "GL_OUT_OF_MEMORY";
        break;
    default:
        error = "UNKNOWN";
        break;
    }
    SDL_SetError("%s: %s", prefix, error);
}

static SDL_GLContext SDL_CurrentContext = NULL;

static int
GLES_ActivateRenderer(SDL_Renderer * renderer)
{
    GLES_RenderData *data = (GLES_RenderData *) renderer->driverdata;

    if (SDL_CurrentContext != data->context) {
        if (SDL_GL_MakeCurrent(renderer->window, data->context) < 0) {
            return -1;
        }
        SDL_CurrentContext = data->context;

        GLES_UpdateViewport(renderer);
    }
    return 0;
}

/* This is called if we need to invalidate all of the SDL OpenGL state */
static void
GLES_ResetState(SDL_Renderer *renderer)
{
    GLES_RenderData *data = (GLES_RenderData *) renderer->driverdata;

    if (SDL_CurrentContext == data->context) {
        GLES_UpdateViewport(renderer);
    } else {
        GLES_ActivateRenderer(renderer);
    }

    data->current.color = 0;
    data->current.blendMode = -1;
    data->current.scaleMode = 0;
    data->current.tex_coords = SDL_FALSE;

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glEnableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
}

SDL_Renderer *
GLES_CreateRenderer(SDL_Window * window, Uint32 flags)
{

    SDL_Renderer *renderer;
    GLES_RenderData *data;
    GLint value;

    renderer = (SDL_Renderer *) SDL_calloc(1, sizeof(*renderer));
    if (!renderer) {
        SDL_OutOfMemory();
        return NULL;
    }

    data = (GLES_RenderData *) SDL_calloc(1, sizeof(*data));
    if (!data) {
        GLES_DestroyRenderer(renderer);
        SDL_OutOfMemory();
        return NULL;
    }

    renderer->WindowEvent = GLES_WindowEvent;
    renderer->CreateTexture = GLES_CreateTexture;
    renderer->UpdateTexture = GLES_UpdateTexture;
    renderer->LockTexture = GLES_LockTexture;
    renderer->UnlockTexture = GLES_UnlockTexture;
    renderer->UpdateViewport = GLES_UpdateViewport;
    renderer->RenderClear = GLES_RenderClear;
    renderer->RenderDrawPoints = GLES_RenderDrawPoints;
    renderer->RenderDrawLines = GLES_RenderDrawLines;
    renderer->RenderFillRects = GLES_RenderFillRects;
    renderer->RenderCopy = GLES_RenderCopy;
    renderer->RenderPresent = GLES_RenderPresent;
    renderer->DestroyTexture = GLES_DestroyTexture;
    renderer->DestroyRenderer = GLES_DestroyRenderer;
    renderer->info = GLES_RenderDriver.info;
    renderer->info.flags = SDL_RENDERER_ACCELERATED;
    renderer->driverdata = data;

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 1);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);

    data->context = SDL_GL_CreateContext(window);
    if (!data->context) {
        GLES_DestroyRenderer(renderer);
        return NULL;
    }
    if (SDL_GL_MakeCurrent(window, data->context) < 0) {
        GLES_DestroyRenderer(renderer);
        return NULL;
    }

    if (flags & SDL_RENDERER_PRESENTVSYNC) {
        SDL_GL_SetSwapInterval(1);
    } else {
        SDL_GL_SetSwapInterval(0);
    }
    if (SDL_GL_GetSwapInterval() > 0) {
        renderer->info.flags |= SDL_RENDERER_PRESENTVSYNC;
    }

#if SDL_VIDEO_DRIVER_PANDORA
    data->GL_OES_draw_texture_supported = SDL_FALSE;
    data->useDrawTexture = SDL_FALSE;
#else
    if (SDL_GL_ExtensionSupported("GL_OES_draw_texture")) {
        data->GL_OES_draw_texture_supported = SDL_TRUE;
        data->useDrawTexture = SDL_TRUE;
    } else {
        data->GL_OES_draw_texture_supported = SDL_FALSE;
        data->useDrawTexture = SDL_FALSE;
    }
#endif

    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &value);
    renderer->info.max_texture_width = value;
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &value);
    renderer->info.max_texture_height = value;

    /* Set up parameters for rendering */
    GLES_ResetState(renderer);

    return renderer;
}

static void
GLES_WindowEvent(SDL_Renderer * renderer, const SDL_WindowEvent *event)
{
    if (event->event == SDL_WINDOWEVENT_SIZE_CHANGED) {
        /* Rebind the context to the window area and update matrices */
        SDL_CurrentContext = NULL;
    }
}

static __inline__ int
power_of_2(int input)
{
    int value = 1;

    while (value < input) {
        value <<= 1;
    }
    return value;
}

static int
GLES_CreateTexture(SDL_Renderer * renderer, SDL_Texture * texture)
{
    GLES_TextureData *data;
    GLint internalFormat;
    GLenum format, type;
    int texture_w, texture_h;
    GLenum result;

    GLES_ActivateRenderer(renderer);

    switch (texture->format) {
    case SDL_PIXELFORMAT_ABGR8888:
        internalFormat = GL_RGBA;
        format = GL_RGBA;
        type = GL_UNSIGNED_BYTE;
        break;
    default:
        SDL_SetError("Texture format not supported");
        return -1;
    }

    data = (GLES_TextureData *) SDL_calloc(1, sizeof(*data));
    if (!data) {
        SDL_OutOfMemory();
        return -1;
    }

    if (texture->access == SDL_TEXTUREACCESS_STREAMING) {
        data->pitch = texture->w * SDL_BYTESPERPIXEL(texture->format);
        data->pixels = SDL_malloc(texture->h * data->pitch);
        if (!data->pixels) {
            SDL_OutOfMemory();
            SDL_free(data);
            return -1;
        }
    }

    texture->driverdata = data;

    glGetError();
    glEnable(GL_TEXTURE_2D);
    glGenTextures(1, &data->texture);

    data->type = GL_TEXTURE_2D;
    /* no NPOV textures allowed in OpenGL ES (yet) */
    texture_w = power_of_2(texture->w);
    texture_h = power_of_2(texture->h);
    data->texw = (GLfloat) texture->w / texture_w;
    data->texh = (GLfloat) texture->h / texture_h;

    data->format = format;
    data->formattype = type;
    data->scaleMode = GL_LINEAR;
    glBindTexture(data->type, data->texture);
    glTexParameteri(data->type, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(data->type, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glTexImage2D(data->type, 0, internalFormat, texture_w,
                             texture_h, 0, format, type, NULL);
    glDisable(GL_TEXTURE_2D);

    result = glGetError();
    if (result != GL_NO_ERROR) {
        GLES_SetError("glTexImage2D()", result);
        return -1;
    }
    return 0;
}

static int
GLES_UpdateTexture(SDL_Renderer * renderer, SDL_Texture * texture,
                   const SDL_Rect * rect, const void *pixels, int pitch)
{
    GLES_TextureData *data = (GLES_TextureData *) texture->driverdata;
    Uint8 *blob = NULL;
    Uint8 *src;
    int srcPitch;
    int y;

    GLES_ActivateRenderer(renderer);

    /* Bail out if we're supposed to update an empty rectangle */
    if (rect->w <= 0 || rect->h <= 0)
        return 0;

    /* Reformat the texture data into a tightly packed array */
    srcPitch = rect->w * SDL_BYTESPERPIXEL(texture->format);
    src = (Uint8 *)pixels;
    if (pitch != srcPitch)
    {
        blob = (Uint8 *)SDL_malloc(srcPitch * rect->h);
        if (!blob)
        {
            SDL_OutOfMemory();
            return -1;
        }
        src = blob;
        for (y = 0; y < rect->h; ++y)
        {
            SDL_memcpy(src, pixels, srcPitch);
            src += srcPitch;
            pixels = (Uint8 *)pixels + pitch;
        }
        src = blob;
    }

    /* Create a texture subimage with the supplied data */
    glGetError();
    glEnable(data->type);
    glBindTexture(data->type, data->texture);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexSubImage2D(data->type,
                    0,
                    rect->x,
                    rect->y,
                    rect->w,
                    rect->h,
                    data->format,
                    data->formattype,
                    src);
    if (blob) {
        SDL_free(blob);
    }

    if (glGetError() != GL_NO_ERROR)
    {
        SDL_SetError("Failed to update texture");
        return -1;
    }
    return 0;
}

static int
GLES_LockTexture(SDL_Renderer * renderer, SDL_Texture * texture,
                 const SDL_Rect * rect, void **pixels, int *pitch)
{
    GLES_TextureData *data = (GLES_TextureData *) texture->driverdata;

    *pixels =
        (void *) ((Uint8 *) data->pixels + rect->y * data->pitch +
                  rect->x * SDL_BYTESPERPIXEL(texture->format));
    *pitch = data->pitch;
    return 0;
}

static void
GLES_UnlockTexture(SDL_Renderer * renderer, SDL_Texture * texture)
{
    GLES_TextureData *data = (GLES_TextureData *) texture->driverdata;
    SDL_Rect rect;

    /* We do whole texture updates, at least for now */
    rect.x = 0;
    rect.y = 0;
    rect.w = texture->w;
    rect.h = texture->h;
    GLES_UpdateTexture(renderer, texture, &rect, data->pixels, data->pitch);
}

static int
GLES_UpdateViewport(SDL_Renderer * renderer)
{
    GLES_RenderData *data = (GLES_RenderData *) renderer->driverdata;

    if (SDL_CurrentContext != data->context) {
        /* We'll update the viewport after we rebind the context */
        return 0;
    }

    glViewport(renderer->viewport.x, renderer->viewport.y,
               renderer->viewport.w, renderer->viewport.h);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrthof((GLfloat) 0,
			 (GLfloat) renderer->viewport.w,
             (GLfloat) renderer->viewport.h,
             (GLfloat) 0, 0.0, 1.0);
    return 0;
}

static void
GLES_SetColor(GLES_RenderData * data, Uint8 r, Uint8 g, Uint8 b, Uint8 a)
{
    Uint32 color = ((a << 24) | (r << 16) | (g << 8) | b);

    if (color != data->current.color) {
        glColor4f((GLfloat) r * inv255f,
                        (GLfloat) g * inv255f,
                        (GLfloat) b * inv255f,
                        (GLfloat) a * inv255f);
        data->current.color = color;
    }
}

static void
GLES_SetBlendMode(GLES_RenderData * data, int blendMode)
{
    if (blendMode != data->current.blendMode) {
        switch (blendMode) {
        case SDL_BLENDMODE_NONE:
            glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
            glDisable(GL_BLEND);
            break;
        case SDL_BLENDMODE_BLEND:
            glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            break;
        case SDL_BLENDMODE_ADD:
            glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE);
            break;
        case SDL_BLENDMODE_MOD:
            glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
            glEnable(GL_BLEND);
            glBlendFunc(GL_ZERO, GL_SRC_COLOR);
            break;
        }
        data->current.blendMode = blendMode;
    }
}

static void
GLES_SetTexCoords(GLES_RenderData * data, SDL_bool enabled)
{
    if (enabled != data->current.tex_coords) {
        if (enabled) {
            glEnableClientState(GL_TEXTURE_COORD_ARRAY);
        } else {
            glDisableClientState(GL_TEXTURE_COORD_ARRAY);
        }
        data->current.tex_coords = enabled;
    }
}

static void
GLES_SetDrawingState(SDL_Renderer * renderer)
{
    GLES_RenderData *data = (GLES_RenderData *) renderer->driverdata;

    GLES_ActivateRenderer(renderer);

    GLES_SetColor(data, (GLfloat) renderer->r,
                        (GLfloat) renderer->g,
                        (GLfloat) renderer->b,
                        (GLfloat) renderer->a);

    GLES_SetBlendMode(data, renderer->blendMode);

    GLES_SetTexCoords(data, SDL_FALSE);
}

static int
GLES_RenderClear(SDL_Renderer * renderer)
{
    GLES_ActivateRenderer(renderer);

    glClearColor((GLfloat) renderer->r * inv255f,
                 (GLfloat) renderer->g * inv255f,
                 (GLfloat) renderer->b * inv255f,
                 (GLfloat) renderer->a * inv255f);

    glClear(GL_COLOR_BUFFER_BIT);

    return 0;
}

static int
GLES_RenderDrawPoints(SDL_Renderer * renderer, const SDL_Point * points,
                      int count)
{
    GLES_RenderData *data = (GLES_RenderData *) renderer->driverdata;
    int i;
    GLshort *vertices;

    GLES_SetDrawingState(renderer);

    vertices = SDL_stack_alloc(GLshort, count*2);
    for (i = 0; i < count; ++i) {
        vertices[2*i+0] = (GLshort)points[i].x;
        vertices[2*i+1] = (GLshort)points[i].y;
    }
    glVertexPointer(2, GL_SHORT, 0, vertices);
    glDrawArrays(GL_POINTS, 0, count);
    SDL_stack_free(vertices);

    return 0;
}

static int
GLES_RenderDrawLines(SDL_Renderer * renderer, const SDL_Point * points,
                     int count)
{
    GLES_RenderData *data = (GLES_RenderData *) renderer->driverdata;
    int i;
    GLshort *vertices;

    GLES_SetDrawingState(renderer);

    vertices = SDL_stack_alloc(GLshort, count*2);
    for (i = 0; i < count; ++i) {
        vertices[2*i+0] = (GLshort)points[i].x;
        vertices[2*i+1] = (GLshort)points[i].y;
    }
    glVertexPointer(2, GL_SHORT, 0, vertices);
    if (count > 2 && 
        points[0].x == points[count-1].x && points[0].y == points[count-1].y) {
        /* GL_LINE_LOOP takes care of the final segment */
        --count;
        glDrawArrays(GL_LINE_LOOP, 0, count);
    } else {
        glDrawArrays(GL_LINE_STRIP, 0, count);
    }
    SDL_stack_free(vertices);

    return 0;
}

static int
GLES_RenderFillRects(SDL_Renderer * renderer, const SDL_Rect * rects,
                     int count)
{
    GLES_RenderData *data = (GLES_RenderData *) renderer->driverdata;
    int i;

    GLES_SetDrawingState(renderer);

    for (i = 0; i < count; ++i) {
        const SDL_Rect *rect = &rects[i];
        GLshort minx = rect->x;
        GLshort maxx = rect->x + rect->w;
        GLshort miny = rect->y;
        GLshort maxy = rect->y + rect->h;
        GLshort vertices[8];
        vertices[0] = minx;
        vertices[1] = miny;
        vertices[2] = maxx;
        vertices[3] = miny;
        vertices[4] = minx;
        vertices[5] = maxy;
        vertices[6] = maxx;
        vertices[7] = maxy;

        glVertexPointer(2, GL_SHORT, 0, vertices);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    }

    return 0;
}

static int
GLES_RenderCopy(SDL_Renderer * renderer, SDL_Texture * texture,
                const SDL_Rect * srcrect, const SDL_Rect * dstrect)
{

    GLES_RenderData *data = (GLES_RenderData *) renderer->driverdata;
    GLES_TextureData *texturedata = (GLES_TextureData *) texture->driverdata;
    int minx, miny, maxx, maxy;
    GLfloat minu, maxu, minv, maxv;

    GLES_ActivateRenderer(renderer);

    glEnable(GL_TEXTURE_2D);

    glBindTexture(texturedata->type, texturedata->texture);

    if (texturedata->scaleMode != data->current.scaleMode) {
        glTexParameteri(texturedata->type, GL_TEXTURE_MIN_FILTER,
                        texturedata->scaleMode);
        glTexParameteri(texturedata->type, GL_TEXTURE_MAG_FILTER,
                        texturedata->scaleMode);
        data->current.scaleMode = texturedata->scaleMode;
    }

    if (texture->modMode) {
        GLES_SetColor(data, texture->r, texture->g, texture->b, texture->a);
    } else {
        GLES_SetColor(data, 255, 255, 255, 255);
    }

    GLES_SetBlendMode(data, texture->blendMode);

    GLES_SetTexCoords(data, SDL_TRUE);

    if (data->GL_OES_draw_texture_supported && data->useDrawTexture) {
        /* this code is a little funny because the viewport is upside down vs SDL's coordinate system */
        GLint cropRect[4];
        int w, h;
        SDL_Window *window = renderer->window;

        SDL_GetWindowSize(window, &w, &h);
        cropRect[0] = srcrect->x;
        cropRect[1] = srcrect->y + srcrect->h;
        cropRect[2] = srcrect->w;
        cropRect[3] = -srcrect->h;
        glTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_CROP_RECT_OES,
                               cropRect);
        glDrawTexiOES(dstrect->x, h - dstrect->y - dstrect->h, 0,
                            dstrect->w, dstrect->h);
    } else {

        minx = dstrect->x;
        miny = dstrect->y;
        maxx = dstrect->x + dstrect->w;
        maxy = dstrect->y + dstrect->h;

        minu = (GLfloat) srcrect->x / texture->w;
        minu *= texturedata->texw;
        maxu = (GLfloat) (srcrect->x + srcrect->w) / texture->w;
        maxu *= texturedata->texw;
        minv = (GLfloat) srcrect->y / texture->h;
        minv *= texturedata->texh;
        maxv = (GLfloat) (srcrect->y + srcrect->h) / texture->h;
        maxv *= texturedata->texh;

        GLshort vertices[8];
        GLfloat texCoords[8];

        vertices[0] = minx;
        vertices[1] = miny;
        vertices[2] = maxx;
        vertices[3] = miny;
        vertices[4] = minx;
        vertices[5] = maxy;
        vertices[6] = maxx;
        vertices[7] = maxy;

        texCoords[0] = minu;
        texCoords[1] = minv;
        texCoords[2] = maxu;
        texCoords[3] = minv;
        texCoords[4] = minu;
        texCoords[5] = maxv;
        texCoords[6] = maxu;
        texCoords[7] = maxv;

        glVertexPointer(2, GL_SHORT, 0, vertices);
        glTexCoordPointer(2, GL_FLOAT, 0, texCoords);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    }
    glDisable(GL_TEXTURE_2D);

    return 0;
}

static void
GLES_RenderPresent(SDL_Renderer * renderer)
{
    GLES_ActivateRenderer(renderer);

    SDL_GL_SwapWindow(renderer->window);
}

static void
GLES_DestroyTexture(SDL_Renderer * renderer, SDL_Texture * texture)
{
    GLES_TextureData *data = (GLES_TextureData *) texture->driverdata;

    GLES_ActivateRenderer(renderer);

    if (!data) {
        return;
    }
    if (data->texture) {
        glDeleteTextures(1, &data->texture);
    }
    if (data->pixels) {
        SDL_free(data->pixels);
    }
    SDL_free(data);
    texture->driverdata = NULL;
}

static void
GLES_DestroyRenderer(SDL_Renderer * renderer)
{
    GLES_RenderData *data = (GLES_RenderData *) renderer->driverdata;

    if (data) {
        if (data->context) {
            SDL_GL_DeleteContext(data->context);
        }
        SDL_free(data);
    }
    SDL_free(renderer);
}

#endif /* SDL_VIDEO_RENDER_OGL_ES && !SDL_RENDER_DISABLED */

/* vi: set ts=4 sw=4 expandtab: */

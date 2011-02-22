/*
 * Small SDL example to demonstrate dynamically loading 
 * OpenGL lib and functions
 *
 * (FYI it was supposed to look like snow in the wind or something...)
 *
 * Compile with :
 * gcc testdyngl.c `sdl-config --libs --cflags` -o testdyngl -DHAVE_OPENGL
 *
 * You can specify a different OpenGL lib on the command line, i.e. :
 * ./testdyngl  /usr/X11R6/lib/libGL.so.1.2
 * or
 * ./testdyngl  /usr/lib/libGL.so.1.0.4496
 *
 */

#include <stdio.h>
#include <stdlib.h>

#include "SDL.h"

#ifdef __MACOS__
#define HAVE_OPENGL
#endif

#ifdef HAVE_OPENGL

#include "SDL_opengl.h"

/* Call this instead of exit(), so we can clean up SDL: atexit() is evil. */
static void
quit(int rc)
{
    SDL_Quit();
    exit(rc);
}

void *
get_funcaddr(const char *p)
{
    void *f = SDL_GL_GetProcAddress(p);
    if (f) {
        return f;
    } else {
        printf("Unable to get function pointer for %s\n", p);
        quit(1);
    }
    return NULL;
}

typedef struct
{
    void (APIENTRY * glBegin) (GLenum);
    void (APIENTRY * glEnd) ();
    void (APIENTRY * glVertex3f) (GLfloat, GLfloat, GLfloat);
    void (APIENTRY * glClearColor) (GLfloat, GLfloat, GLfloat, GLfloat);
    void (APIENTRY * glClear) (GLbitfield);
    void (APIENTRY * glDisable) (GLenum);
    void (APIENTRY * glEnable) (GLenum);
    void (APIENTRY * glColor4ub) (GLubyte, GLubyte, GLubyte, GLubyte);
    void (APIENTRY * glPointSize) (GLfloat);
    void (APIENTRY * glHint) (GLenum, GLenum);
    void (APIENTRY * glBlendFunc) (GLenum, GLenum);
    void (APIENTRY * glMatrixMode) (GLenum);
    void (APIENTRY * glLoadIdentity) ();
    void (APIENTRY * glOrtho) (GLdouble, GLdouble, GLdouble, GLdouble,
                               GLdouble, GLdouble);
    void (APIENTRY * glRotatef) (GLfloat, GLfloat, GLfloat, GLfloat);
    void (APIENTRY * glViewport) (GLint, GLint, GLsizei, GLsizei);
    void (APIENTRY * glFogf) (GLenum, GLfloat);
    const GLubyte* (APIENTRY * glGetString) (GLenum);
}
glfuncs;

void
init_glfuncs(glfuncs * f)
{
    f->glBegin = get_funcaddr("glBegin");
    f->glEnd = get_funcaddr("glEnd");
    f->glVertex3f = get_funcaddr("glVertex3f");
    f->glClearColor = get_funcaddr("glClearColor");
    f->glClear = get_funcaddr("glClear");
    f->glDisable = get_funcaddr("glDisable");
    f->glEnable = get_funcaddr("glEnable");
    f->glColor4ub = get_funcaddr("glColor4ub");
    f->glPointSize = get_funcaddr("glPointSize");
    f->glHint = get_funcaddr("glHint");
    f->glBlendFunc = get_funcaddr("glBlendFunc");
    f->glMatrixMode = get_funcaddr("glMatrixMode");
    f->glLoadIdentity = get_funcaddr("glLoadIdentity");
    f->glOrtho = get_funcaddr("glOrtho");
    f->glRotatef = get_funcaddr("glRotatef");
    f->glViewport = get_funcaddr("glViewport");
    f->glFogf = get_funcaddr("glFogf");
    f->glGetString = get_funcaddr("glGetString");

    printf("Vendor     : %s\n", f->glGetString(GL_VENDOR));
    printf("Renderer   : %s\n", f->glGetString(GL_RENDERER));
    printf("Version    : %s\n", f->glGetString(GL_VERSION));
    printf("Extensions : %s\n", f->glGetString(GL_EXTENSIONS));
}

#define NB_PIXELS 1000

int
main(int argc, char *argv[])
{
    glfuncs f;
    int i;
    SDL_Event event;
    int done = 0;
    GLfloat pixels[NB_PIXELS * 3];
    const char *gl_library = NULL;      /* Use the default GL library */

    if (argv[1]) {
        gl_library = argv[1];
    }

    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        printf("Unable to init SDL : %s\n", SDL_GetError());
        return (1);
    }

    if (SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1) < 0) {
        printf("Unable to set GL attribute : %s\n", SDL_GetError());
        quit(1);
    }

    if (SDL_GL_LoadLibrary(gl_library) < 0) {
        printf("Unable to dynamically open GL lib : %s\n", SDL_GetError());
        quit(1);
    }

    if (SDL_SetVideoMode(640, 480, 0, SDL_OPENGL) == NULL) {
        printf("Unable to open video mode : %s\n", SDL_GetError());
        quit(1);
    }

    /* Set the window manager title bar */
    SDL_WM_SetCaption("SDL Dynamic OpenGL Loading Test", "testdyngl");

    init_glfuncs(&f);

    for (i = 0; i < NB_PIXELS; i++) {
        pixels[3 * i] = rand() % 250 - 125;
        pixels[3 * i + 1] = rand() % 250 - 125;
        pixels[3 * i + 2] = rand() % 250 - 125;
    }

    f.glViewport(0, 0, 640, 480);

    f.glMatrixMode(GL_PROJECTION);
    f.glLoadIdentity();
    f.glOrtho(-100, 100, -100, 100, -500, 500);

    f.glMatrixMode(GL_MODELVIEW);
    f.glLoadIdentity();

    f.glEnable(GL_DEPTH_TEST);
    f.glDisable(GL_TEXTURE_2D);
    f.glEnable(GL_BLEND);
    f.glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    f.glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    f.glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    f.glEnable(GL_POINT_SMOOTH);
    f.glHint(GL_POINT_SMOOTH_HINT, GL_NICEST);
    f.glPointSize(5.0f);
    f.glEnable(GL_FOG);
    f.glFogf(GL_FOG_START, -500);
    f.glFogf(GL_FOG_END, 500);
    f.glFogf(GL_FOG_DENSITY, 0.005);

    do {
        f.glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        f.glRotatef(2.0, 1.0, 1.0, 1.0);
        f.glRotatef(1.0, 0.0, 1.0, 1.0);

        f.glColor4ub(255, 255, 255, 255);
        f.glBegin(GL_POINTS);
        for (i = 0; i < NB_PIXELS; i++) {
            f.glVertex3f(pixels[3 * i], pixels[3 * i + 1], pixels[3 * i + 2]);
        }
        f.glEnd();
        SDL_GL_SwapBuffers();

        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_KEYDOWN)
                done = 1;
        }

        SDL_Delay(20);
    } while (!done);

    SDL_Quit();
    return 0;
}

#else /* HAVE_OPENGL */

int
main(int argc, char *argv[])
{
    printf("No OpenGL support on this system\n");
    return 1;
}

#endif /* HAVE_OPENGL */

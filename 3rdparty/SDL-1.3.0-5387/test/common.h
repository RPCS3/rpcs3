
/* A simple test program framework */

#define SDL_NO_COMPAT
#include "SDL.h"

#define DEFAULT_WINDOW_WIDTH  640
#define DEFAULT_WINDOW_HEIGHT 480

#define VERBOSE_VIDEO   0x00000001
#define VERBOSE_MODES   0x00000002
#define VERBOSE_RENDER  0x00000004
#define VERBOSE_EVENT   0x00000008
#define VERBOSE_AUDIO   0x00000010

typedef struct
{
    /* SDL init flags */
    char **argv;
    Uint32 flags;
    Uint32 verbose;

    /* Video info */
    const char *videodriver;
    int display;
    const char *window_title;
    const char *window_icon;
    Uint32 window_flags;
    int window_x;
    int window_y;
    int window_w;
    int window_h;
    int depth;
    int refresh_rate;
    int num_windows;
    SDL_Window **windows;

    /* Renderer info */
    const char *renderdriver;
    Uint32 render_flags;
    SDL_bool skip_renderer;
    SDL_Renderer **renderers;

    /* Audio info */
    const char *audiodriver;
    SDL_AudioSpec audiospec;

    /* GL settings */
    int gl_red_size;
    int gl_green_size;
    int gl_blue_size;
    int gl_alpha_size;
    int gl_buffer_size;
    int gl_depth_size;
    int gl_stencil_size;
    int gl_double_buffer;
    int gl_accum_red_size;
    int gl_accum_green_size;
    int gl_accum_blue_size;
    int gl_accum_alpha_size;
    int gl_stereo;
    int gl_multisamplebuffers;
    int gl_multisamplesamples;
    int gl_retained_backing;
    int gl_accelerated;
    int gl_major_version;
    int gl_minor_version;
} CommonState;

extern CommonState *CommonCreateState(char **argv, Uint32 flags);
extern int CommonArg(CommonState * state, int index);
extern const char *CommonUsage(CommonState * state);
extern SDL_bool CommonInit(CommonState * state);
extern void CommonEvent(CommonState * state, SDL_Event * event, int *done);
extern void CommonQuit(CommonState * state);

/* vi: set ts=4 sw=4 expandtab: */

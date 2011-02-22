
/* A simple test program framework */

#include <SDL/SDL.h>

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
    Uint32 window_flags;
    int window_x;
    int window_y;
    int window_w;
    int window_h;
    int depth;
    int refresh_rate;
    int num_windows;
    SDL_WindowID *windows;

    /* Renderer info */
    const char *renderdriver;
    Uint32 render_flags;
    SDL_bool skip_renderer;

    /* Audio info */
    const char *audiodriver;
    SDL_AudioSpec audiospec;
} CommonState;

extern CommonState *CommonCreateState(char **argv, Uint32 flags);
extern int CommonArg(CommonState * state, int index);
extern const char *CommonUsage(CommonState * state);
extern SDL_bool CommonInit(CommonState * state);
extern void CommonEvent(CommonState * state, SDL_Event * event, int *done);
extern void CommonQuit(CommonState * state);

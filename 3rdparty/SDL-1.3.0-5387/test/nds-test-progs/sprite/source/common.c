
/* A simple test program framework */

#include <stdio.h>

#include "common.h"

#define VIDEO_USAGE \
"[--video driver] [--renderer driver] [--info all|video|modes|render|event] [--display %d] [--fullscreen | --windows N] [--title title] [--center | --position X,Y] [--geometry WxH] [--depth N] [--refresh R] [--vsync] [--noframe] [--resize] [--minimize] [--maximize] [--grab]"

#define AUDIO_USAGE \
"[--rate N] [--format U8|S8|U16|U16LE|U16BE|S16|S16LE|S16BE] [--channels N] [--samples N]"

CommonState *
CommonCreateState(char **argv, Uint32 flags)
{
    CommonState *state = SDL_calloc(1, sizeof(*state));
    if (!state) {
        SDL_OutOfMemory();
        return NULL;
    }

    /* Initialize some defaults */
    state->argv = argv;
    state->flags = flags;
    state->window_title = argv[0];
    state->window_flags = SDL_WINDOW_SHOWN;
    state->window_x = SDL_WINDOWPOS_UNDEFINED;
    state->window_y = SDL_WINDOWPOS_UNDEFINED;
    state->window_w = 256;
    state->window_h = 192;
    state->num_windows = 1;
    state->audiospec.freq = 22050;
    state->audiospec.format = AUDIO_S16;
    state->audiospec.channels = 2;
    state->audiospec.samples = 2048;
    return state;
}

int
CommonArg(CommonState * state, int index)
{
    char **argv = state->argv;

    if (SDL_strcasecmp(argv[index], "--video") == 0) {
        ++index;
        if (!argv[index]) {
            return -1;
        }
        state->videodriver = argv[index];
        return 2;
    }
    if (SDL_strcasecmp(argv[index], "--renderer") == 0) {
        ++index;
        if (!argv[index]) {
            return -1;
        }
        state->renderdriver = argv[index];
        return 2;
    }
    if (SDL_strcasecmp(argv[index], "--info") == 0) {
        ++index;
        if (!argv[index]) {
            return -1;
        }
        if (SDL_strcasecmp(argv[index], "all") == 0) {
            state->verbose |=
                (VERBOSE_VIDEO | VERBOSE_MODES | VERBOSE_RENDER |
                 VERBOSE_EVENT);
            return 2;
        }
        if (SDL_strcasecmp(argv[index], "video") == 0) {
            state->verbose |= VERBOSE_VIDEO;
            return 2;
        }
        if (SDL_strcasecmp(argv[index], "modes") == 0) {
            state->verbose |= VERBOSE_MODES;
            return 2;
        }
        if (SDL_strcasecmp(argv[index], "render") == 0) {
            state->verbose |= VERBOSE_RENDER;
            return 2;
        }
        if (SDL_strcasecmp(argv[index], "event") == 0) {
            state->verbose |= VERBOSE_EVENT;
            return 2;
        }
        return -1;
    }
    if (SDL_strcasecmp(argv[index], "--display") == 0) {
        ++index;
        if (!argv[index]) {
            return -1;
        }
        state->display = SDL_atoi(argv[index]);
        return 2;
    }
    if (SDL_strcasecmp(argv[index], "--fullscreen") == 0) {
        state->window_flags |= SDL_WINDOW_FULLSCREEN;
        state->num_windows = 1;
        return 1;
    }
    if (SDL_strcasecmp(argv[index], "--windows") == 0) {
        ++index;
        if (!argv[index] || !SDL_isdigit(*argv[index])) {
            return -1;
        }
        if (!(state->window_flags & SDL_WINDOW_FULLSCREEN)) {
            state->num_windows = SDL_atoi(argv[index]);
        }
        return 2;
    }
    if (SDL_strcasecmp(argv[index], "--title") == 0) {
        ++index;
        if (!argv[index]) {
            return -1;
        }
        state->window_title = argv[index];
        return 2;
    }
    if (SDL_strcasecmp(argv[index], "--center") == 0) {
        state->window_x = SDL_WINDOWPOS_CENTERED;
        state->window_y = SDL_WINDOWPOS_CENTERED;
        return 1;
    }
    if (SDL_strcasecmp(argv[index], "--position") == 0) {
        char *x, *y;
        ++index;
        if (!argv[index]) {
            return -1;
        }
        x = argv[index];
        y = argv[index];
        while (*y && *y != ',') {
            ++y;
        }
        if (!*y) {
            return -1;
        }
        *y++ = '\0';
        state->window_x = SDL_atoi(x);
        state->window_y = SDL_atoi(y);
        return 2;
    }
    if (SDL_strcasecmp(argv[index], "--geometry") == 0) {
        char *w, *h;
        ++index;
        if (!argv[index]) {
            return -1;
        }
        w = argv[index];
        h = argv[index];
        while (*h && *h != 'x') {
            ++h;
        }
        if (!*h) {
            return -1;
        }
        *h++ = '\0';
        state->window_w = SDL_atoi(w);
        state->window_h = SDL_atoi(h);
        return 2;
    }
    if (SDL_strcasecmp(argv[index], "--depth") == 0) {
        ++index;
        if (!argv[index]) {
            return -1;
        }
        state->depth = SDL_atoi(argv[index]);
        return 2;
    }
    if (SDL_strcasecmp(argv[index], "--refresh") == 0) {
        ++index;
        if (!argv[index]) {
            return -1;
        }
        state->refresh_rate = SDL_atoi(argv[index]);
        return 2;
    }
    if (SDL_strcasecmp(argv[index], "--vsync") == 0) {
        state->render_flags |= SDL_RENDERER_PRESENTVSYNC;
        return 1;
    }
    if (SDL_strcasecmp(argv[index], "--noframe") == 0) {
        state->window_flags |= SDL_WINDOW_BORDERLESS;
        return 1;
    }
    if (SDL_strcasecmp(argv[index], "--resize") == 0) {
        state->window_flags |= SDL_WINDOW_RESIZABLE;
        return 1;
    }
    if (SDL_strcasecmp(argv[index], "--minimize") == 0) {
        state->window_flags |= SDL_WINDOW_MINIMIZED;
        return 1;
    }
    if (SDL_strcasecmp(argv[index], "--maximize") == 0) {
        state->window_flags |= SDL_WINDOW_MAXIMIZED;
        return 1;
    }
    if (SDL_strcasecmp(argv[index], "--grab") == 0) {
        state->window_flags |= SDL_WINDOW_INPUT_GRABBED;
        return 1;
    }
    if (SDL_strcasecmp(argv[index], "--rate") == 0) {
        ++index;
        if (!argv[index]) {
            return -1;
        }
        state->audiospec.freq = SDL_atoi(argv[index]);
        return 2;
    }
    if (SDL_strcasecmp(argv[index], "--format") == 0) {
        ++index;
        if (!argv[index]) {
            return -1;
        }
        if (SDL_strcasecmp(argv[index], "U8") == 0) {
            state->audiospec.format = AUDIO_U8;
            return 2;
        }
        if (SDL_strcasecmp(argv[index], "S8") == 0) {
            state->audiospec.format = AUDIO_S8;
            return 2;
        }
        if (SDL_strcasecmp(argv[index], "U16") == 0) {
            state->audiospec.format = AUDIO_U16;
            return 2;
        }
        if (SDL_strcasecmp(argv[index], "U16LE") == 0) {
            state->audiospec.format = AUDIO_U16LSB;
            return 2;
        }
        if (SDL_strcasecmp(argv[index], "U16BE") == 0) {
            state->audiospec.format = AUDIO_U16MSB;
            return 2;
        }
        if (SDL_strcasecmp(argv[index], "S16") == 0) {
            state->audiospec.format = AUDIO_S16;
            return 2;
        }
        if (SDL_strcasecmp(argv[index], "S16LE") == 0) {
            state->audiospec.format = AUDIO_S16LSB;
            return 2;
        }
        if (SDL_strcasecmp(argv[index], "S16BE") == 0) {
            state->audiospec.format = AUDIO_S16MSB;
            return 2;
        }
        return -1;
    }
    if (SDL_strcasecmp(argv[index], "--channels") == 0) {
        ++index;
        if (!argv[index]) {
            return -1;
        }
        state->audiospec.channels = (Uint8) SDL_atoi(argv[index]);
        return 2;
    }
    if (SDL_strcasecmp(argv[index], "--samples") == 0) {
        ++index;
        if (!argv[index]) {
            return -1;
        }
        state->audiospec.samples = (Uint16) SDL_atoi(argv[index]);
        return 2;
    }
    if ((SDL_strcasecmp(argv[index], "-h") == 0)
        || (SDL_strcasecmp(argv[index], "--help") == 0)) {
        /* Print the usage message */
        return -1;
    }
    return 0;
}

const char *
CommonUsage(CommonState * state)
{
    switch (state->flags & (SDL_INIT_VIDEO | SDL_INIT_AUDIO)) {
    case SDL_INIT_VIDEO:
        return VIDEO_USAGE;
    case SDL_INIT_AUDIO:
        return AUDIO_USAGE;
    case (SDL_INIT_VIDEO | SDL_INIT_AUDIO):
        return VIDEO_USAGE " " AUDIO_USAGE;
    default:
        return "";
    }
}

static void
PrintRendererFlag(Uint32 flag)
{
    switch (flag) {
    case SDL_RENDERER_SINGLEBUFFER:
        fprintf(stderr, "SingleBuffer");
        break;
    case SDL_RENDERER_PRESENTCOPY:
        fprintf(stderr, "PresentCopy");
        break;
    case SDL_RENDERER_PRESENTFLIP2:
        fprintf(stderr, "PresentFlip2");
        break;
    case SDL_RENDERER_PRESENTFLIP3:
        fprintf(stderr, "PresentFlip3");
        break;
    case SDL_RENDERER_PRESENTDISCARD:
        fprintf(stderr, "PresentDiscard");
        break;
    case SDL_RENDERER_PRESENTVSYNC:
        fprintf(stderr, "PresentVSync");
        break;
    case SDL_RENDERER_ACCELERATED:
        fprintf(stderr, "Accelerated");
        break;
    default:
        fprintf(stderr, "0x%8.8x", flag);
        break;
    }
}

static void
PrintBlendMode(Uint32 flag)
{
    switch (flag) {
    case SDL_BLENDMODE_NONE:
        fprintf(stderr, "None");
        break;
    case SDL_BLENDMODE_MASK:
        fprintf(stderr, "Mask");
        break;
    case SDL_BLENDMODE_BLEND:
        fprintf(stderr, "Blend");
        break;
    case SDL_BLENDMODE_ADD:
        fprintf(stderr, "Add");
        break;
    case SDL_BLENDMODE_MOD:
        fprintf(stderr, "Mod");
        break;
    default:
        fprintf(stderr, "0x%8.8x", flag);
        break;
    }
}

static void
PrintScaleMode(Uint32 flag)
{
    switch (flag) {
    case SDL_TEXTURESCALEMODE_NONE:
        fprintf(stderr, "None");
        break;
    case SDL_TEXTURESCALEMODE_FAST:
        fprintf(stderr, "Fast");
        break;
    case SDL_TEXTURESCALEMODE_SLOW:
        fprintf(stderr, "Slow");
        break;
    case SDL_TEXTURESCALEMODE_BEST:
        fprintf(stderr, "Best");
        break;
    default:
        fprintf(stderr, "0x%8.8x", flag);
        break;
    }
}

static void
PrintPixelFormat(Uint32 format)
{
    switch (format) {
    case SDL_PIXELFORMAT_UNKNOWN:
        fprintf(stderr, "Unknwon");
        break;
    case SDL_PIXELFORMAT_INDEX1LSB:
        fprintf(stderr, "Index1LSB");
        break;
    case SDL_PIXELFORMAT_INDEX1MSB:
        fprintf(stderr, "Index1MSB");
        break;
    case SDL_PIXELFORMAT_INDEX4LSB:
        fprintf(stderr, "Index4LSB");
        break;
    case SDL_PIXELFORMAT_INDEX4MSB:
        fprintf(stderr, "Index4MSB");
        break;
    case SDL_PIXELFORMAT_INDEX8:
        fprintf(stderr, "Index8");
        break;
    case SDL_PIXELFORMAT_RGB332:
        fprintf(stderr, "RGB332");
        break;
    case SDL_PIXELFORMAT_RGB444:
        fprintf(stderr, "RGB444");
        break;
    case SDL_PIXELFORMAT_RGB555:
        fprintf(stderr, "RGB555");
        break;
    case SDL_PIXELFORMAT_ARGB4444:
        fprintf(stderr, "ARGB4444");
        break;
    case SDL_PIXELFORMAT_ARGB1555:
        fprintf(stderr, "ARGB1555");
        break;
    case SDL_PIXELFORMAT_RGB565:
        fprintf(stderr, "RGB565");
        break;
    case SDL_PIXELFORMAT_RGB24:
        fprintf(stderr, "RGB24");
        break;
    case SDL_PIXELFORMAT_BGR24:
        fprintf(stderr, "BGR24");
        break;
    case SDL_PIXELFORMAT_RGB888:
        fprintf(stderr, "RGB888");
        break;
    case SDL_PIXELFORMAT_BGR888:
        fprintf(stderr, "BGR888");
        break;
    case SDL_PIXELFORMAT_ARGB8888:
        fprintf(stderr, "ARGB8888");
        break;
    case SDL_PIXELFORMAT_RGBA8888:
        fprintf(stderr, "RGBA8888");
        break;
    case SDL_PIXELFORMAT_ABGR8888:
        fprintf(stderr, "ABGR8888");
        break;
    case SDL_PIXELFORMAT_BGRA8888:
        fprintf(stderr, "BGRA8888");
        break;
    case SDL_PIXELFORMAT_ARGB2101010:
        fprintf(stderr, "ARGB2101010");
        break;
    case SDL_PIXELFORMAT_YV12:
        fprintf(stderr, "YV12");
        break;
    case SDL_PIXELFORMAT_IYUV:
        fprintf(stderr, "IYUV");
        break;
    case SDL_PIXELFORMAT_YUY2:
        fprintf(stderr, "YUY2");
        break;
    case SDL_PIXELFORMAT_UYVY:
        fprintf(stderr, "UYVY");
        break;
    case SDL_PIXELFORMAT_YVYU:
        fprintf(stderr, "YVYU");
        break;
    case SDL_PIXELFORMAT_ABGR1555:
        fprintf(stderr, "ABGR1555");
        break;
    case SDL_PIXELFORMAT_BGR555:
        fprintf(stderr, "BGR555");
        break;
    default:
        fprintf(stderr, "0x%8.8x", format);
        break;
    }
}

static void
PrintRenderer(SDL_RendererInfo * info)
{
    int i, count;

    fprintf(stderr, "  Renderer %s:\n", info->name);

    fprintf(stderr, "    Flags: 0x%8.8X", info->flags);
    fprintf(stderr, " (");
    count = 0;
    for (i = 0; i < sizeof(info->flags) * 8; ++i) {
        Uint32 flag = (1 << i);
        if (info->flags & flag) {
            if (count > 0) {
                fprintf(stderr, " | ");
            }
            PrintRendererFlag(flag);
            ++count;
        }
    }
    fprintf(stderr, ")\n");

    fprintf(stderr, "    Blend: 0x%8.8X", info->blend_modes);
    fprintf(stderr, " (");
    count = 0;
    for (i = 0; i < sizeof(info->blend_modes) * 8; ++i) {
        Uint32 flag = (1 << i);
        if (info->blend_modes & flag) {
            if (count > 0) {
                fprintf(stderr, " | ");
            }
            PrintBlendMode(flag);
            ++count;
        }
    }
    fprintf(stderr, ")\n");

    fprintf(stderr, "    Scale: 0x%8.8X", info->scale_modes);
    fprintf(stderr, " (");
    count = 0;
    for (i = 0; i < sizeof(info->scale_modes) * 8; ++i) {
        Uint32 flag = (1 << i);
        if (info->scale_modes & flag) {
            if (count > 0) {
                fprintf(stderr, " | ");
            }
            PrintScaleMode(flag);
            ++count;
        }
    }
    fprintf(stderr, ")\n");

    fprintf(stderr, "    Texture formats (%d): ", info->num_texture_formats);
    for (i = 0; i < (int) info->num_texture_formats; ++i) {
        if (i > 0) {
            fprintf(stderr, ", ");
        }
        PrintPixelFormat(info->texture_formats[i]);
    }
    fprintf(stderr, "\n");

    if (info->max_texture_width || info->max_texture_height) {
        fprintf(stderr, "    Max Texture Size: %dx%d\n",
                info->max_texture_width, info->max_texture_height);
    }
}

SDL_bool
CommonInit(CommonState * state)
{
    int i, j, m, n;
    SDL_DisplayMode fullscreen_mode;

    if (state->flags & SDL_INIT_VIDEO) {
        if (state->verbose & VERBOSE_VIDEO) {
            n = SDL_GetNumVideoDrivers();
            if (n == 0) {
                fprintf(stderr, "No built-in video drivers\n");
            } else {
                fprintf(stderr, "Built-in video drivers:");
                for (i = 0; i < n; ++i) {
                    if (i > 0) {
                        fprintf(stderr, ",");
                    }
                    fprintf(stderr, " %s", SDL_GetVideoDriver(i));
                }
                fprintf(stderr, "\n");
            }
        }
        if (SDL_VideoInit(state->videodriver, 0) < 0) {
            fprintf(stderr, "Couldn't initialize video driver: %s\n",
                    SDL_GetError());
            return SDL_FALSE;
        }
        if (state->verbose & VERBOSE_VIDEO) {
            fprintf(stderr, "Video driver: %s\n",
                    SDL_GetCurrentVideoDriver());
        }

        if (state->verbose & VERBOSE_MODES) {
            SDL_DisplayMode mode;
            int bpp;
            Uint32 Rmask, Gmask, Bmask, Amask;

            n = SDL_GetNumVideoDisplays();
            fprintf(stderr, "Number of displays: %d\n", n);
            for (i = 0; i < n; ++i) {
                fprintf(stderr, "Display %d:\n", i);
                SDL_SelectVideoDisplay(i);

                SDL_GetDesktopDisplayMode(&mode);
                SDL_PixelFormatEnumToMasks(mode.format, &bpp, &Rmask, &Gmask,
                                           &Bmask, &Amask);
                fprintf(stderr,
                        "  Current mode: %dx%d@%dHz, %d bits-per-pixel\n",
                        mode.w, mode.h, mode.refresh_rate, bpp);
                if (Rmask || Gmask || Bmask) {
                    fprintf(stderr, "      Red Mask = 0x%.8x\n", Rmask);
                    fprintf(stderr, "      Green Mask = 0x%.8x\n", Gmask);
                    fprintf(stderr, "      Blue Mask = 0x%.8x\n", Bmask);
                    if (Amask)
                        fprintf(stderr, "      Alpha Mask = 0x%.8x\n", Amask);
                }

                /* Print available fullscreen video modes */
                m = SDL_GetNumDisplayModes();
                if (m == 0) {
                    fprintf(stderr, "No available fullscreen video modes\n");
                } else {
                    fprintf(stderr, "  Fullscreen video modes:\n");
                    for (j = 0; j < m; ++j) {
                        SDL_GetDisplayMode(j, &mode);
                        SDL_PixelFormatEnumToMasks(mode.format, &bpp, &Rmask,
                                                   &Gmask, &Bmask, &Amask);
                        fprintf(stderr,
                                "    Mode %d: %dx%d@%dHz, %d bits-per-pixel\n",
                                j, mode.w, mode.h, mode.refresh_rate, bpp);
                        if (Rmask || Gmask || Bmask) {
                            fprintf(stderr, "        Red Mask = 0x%.8x\n",
                                    Rmask);
                            fprintf(stderr, "        Green Mask = 0x%.8x\n",
                                    Gmask);
                            fprintf(stderr, "        Blue Mask = 0x%.8x\n",
                                    Bmask);
                            if (Amask)
                                fprintf(stderr,
                                        "        Alpha Mask = 0x%.8x\n",
                                        Amask);
                        }
                    }
                }
            }
        }

        SDL_SelectVideoDisplay(state->display);
        if (state->verbose & VERBOSE_RENDER) {
            SDL_RendererInfo info;

            n = SDL_GetNumRenderDrivers();
            if (n == 0) {
                fprintf(stderr, "No built-in render drivers\n");
            } else {
                fprintf(stderr, "Built-in render drivers:\n");
                for (i = 0; i < n; ++i) {
                    SDL_GetRenderDriverInfo(i, &info);
                    PrintRenderer(&info);
                }
            }
        }

        switch (state->depth) {
        case 8:
            fullscreen_mode.format = SDL_PIXELFORMAT_INDEX8;
            break;
        case 15:
            fullscreen_mode.format = SDL_PIXELFORMAT_BGR555;
            break;
        case 16:
        default:
            fullscreen_mode.format = SDL_PIXELFORMAT_ABGR1555;
            break;
/* NDS       default:
            fullscreen_mode.format = SDL_PIXELFORMAT_RGB888;
            break;*/
        }
        fullscreen_mode.w = state->window_w;
        fullscreen_mode.h = state->window_h;
        fullscreen_mode.refresh_rate = state->refresh_rate;
        SDL_SetFullscreenDisplayMode(&fullscreen_mode);

        state->windows =
            (SDL_WindowID *) SDL_malloc(state->num_windows *
                                        sizeof(*state->windows));
        if (!state->windows) {
            fprintf(stderr, "Out of memory!\n");
            return SDL_FALSE;
        }
        for (i = 0; i < state->num_windows; ++i) {
            char title[1024];

            if (state->num_windows > 1) {
                SDL_snprintf(title, SDL_arraysize(title), "%s %d",
                             state->window_title, i + 1);
            } else {
                SDL_strlcpy(title, state->window_title, SDL_arraysize(title));
            }
            state->windows[i] =
                SDL_CreateWindow(title, state->window_x, state->window_y,
                                 state->window_w, state->window_h,
                                 state->window_flags);
            if (!state->windows[i]) {
                fprintf(stderr, "Couldn't create window: %s\n",
                        SDL_GetError());
                return SDL_FALSE;
            }

            if (!state->skip_renderer
                && (state->renderdriver
                    || !(state->window_flags & SDL_WINDOW_OPENGL))) {
                m = -1;
                if (state->renderdriver) {
                    SDL_RendererInfo info;
                    n = SDL_GetNumRenderDrivers();
                    for (j = 0; j < n; ++j) {
                        SDL_GetRenderDriverInfo(j, &info);
                        if (SDL_strcasecmp(info.name, state->renderdriver) ==
                            0) {
                            m = j;
                            break;
                        }
                    }
                    if (m == n) {
                        fprintf(stderr,
                                "Couldn't find render driver named %s",
                                state->renderdriver);
                        return SDL_FALSE;
                    }
                }
                if (SDL_CreateRenderer
                    (state->windows[i], m, state->render_flags) < 0) {
                    fprintf(stderr, "Couldn't create renderer: %s\n",
                            SDL_GetError());
                    return SDL_FALSE;
                }
                if (state->verbose & VERBOSE_RENDER) {
                    SDL_RendererInfo info;

                    fprintf(stderr, "Current renderer:\n");
                    SDL_GetRendererInfo(&info);
                    PrintRenderer(&info);
                }
            }
        }
        SDL_SelectRenderer(state->windows[0]);
    }

    if (state->flags & SDL_INIT_AUDIO) {
        if (state->verbose & VERBOSE_AUDIO) {
            n = SDL_GetNumAudioDrivers();
            if (n == 0) {
                fprintf(stderr, "No built-in audio drivers\n");
            } else {
                fprintf(stderr, "Built-in audio drivers:");
                for (i = 0; i < n; ++i) {
                    if (i > 0) {
                        fprintf(stderr, ",");
                    }
                    fprintf(stderr, " %s", SDL_GetAudioDriver(i));
                }
                fprintf(stderr, "\n");
            }
        }
        if (SDL_AudioInit(state->audiodriver) < 0) {
            fprintf(stderr, "Couldn't initialize audio driver: %s\n",
                    SDL_GetError());
            return SDL_FALSE;
        }
        if (state->verbose & VERBOSE_VIDEO) {
            fprintf(stderr, "Audio driver: %s\n",
                    SDL_GetCurrentAudioDriver());
        }

        if (SDL_OpenAudio(&state->audiospec, NULL) < 0) {
            fprintf(stderr, "Couldn't open audio: %s\n", SDL_GetError());
            return SDL_FALSE;
        }
    }

    return SDL_TRUE;
}

static void
PrintEvent(SDL_Event * event)
{
    fprintf(stderr, "SDL EVENT: ");
    switch (event->type) {
    case SDL_WINDOWEVENT:
        switch (event->window.event) {
        case SDL_WINDOWEVENT_SHOWN:
            fprintf(stderr, "Window %d shown", event->window.windowID);
            break;
        case SDL_WINDOWEVENT_HIDDEN:
            fprintf(stderr, "Window %d hidden", event->window.windowID);
            break;
        case SDL_WINDOWEVENT_EXPOSED:
            fprintf(stderr, "Window %d exposed", event->window.windowID);
            break;
        case SDL_WINDOWEVENT_MOVED:
            fprintf(stderr, "Window %d moved to %d,%d",
                    event->window.windowID, event->window.data1,
                    event->window.data2);
            break;
        case SDL_WINDOWEVENT_RESIZED:
            fprintf(stderr, "Window %d resized to %dx%d",
                    event->window.windowID, event->window.data1,
                    event->window.data2);
            break;
        case SDL_WINDOWEVENT_MINIMIZED:
            fprintf(stderr, "Window %d minimized", event->window.windowID);
            break;
        case SDL_WINDOWEVENT_MAXIMIZED:
            fprintf(stderr, "Window %d maximized", event->window.windowID);
            break;
        case SDL_WINDOWEVENT_RESTORED:
            fprintf(stderr, "Window %d restored", event->window.windowID);
            break;
        case SDL_WINDOWEVENT_ENTER:
            fprintf(stderr, "Mouse entered window %d",
                    event->window.windowID);
            break;
        case SDL_WINDOWEVENT_LEAVE:
            fprintf(stderr, "Mouse left window %d", event->window.windowID);
            break;
        case SDL_WINDOWEVENT_FOCUS_GAINED:
            fprintf(stderr, "Window %d gained keyboard focus",
                    event->window.windowID);
            break;
        case SDL_WINDOWEVENT_FOCUS_LOST:
            fprintf(stderr, "Window %d lost keyboard focus",
                    event->window.windowID);
            break;
        case SDL_WINDOWEVENT_CLOSE:
            fprintf(stderr, "Window %d closed", event->window.windowID);
            break;
        default:
            fprintf(stderr, "Window %d got unknown event %d",
                    event->window.windowID, event->window.event);
            break;
        }
        break;
    case SDL_KEYDOWN:
        fprintf(stderr,
                "Keyboard %d: key pressed  in window %d: scancode 0x%08X = %s, keycode 0x%08X = %s",
                event->key.which, event->key.windowID,
                event->key.keysym.scancode,
                SDL_GetScancodeName(event->key.keysym.scancode),
                event->key.keysym.sym, SDL_GetKeyName(event->key.keysym.sym));
        break;
    case SDL_KEYUP:
        fprintf(stderr,
                "Keyboard %d: key released in window %d: scancode 0x%08X = %s, keycode 0x%08X = %s",
                event->key.which, event->key.windowID,
                event->key.keysym.scancode,
                SDL_GetScancodeName(event->key.keysym.scancode),
                event->key.keysym.sym, SDL_GetKeyName(event->key.keysym.sym));
        break;
    case SDL_TEXTINPUT:
        fprintf(stderr, "Keyboard %d: text input \"%s\" in window %d",
                event->text.which, event->text.text, event->text.windowID);
        break;
    case SDL_MOUSEMOTION:
        fprintf(stderr, "Mouse %d: moved to %d,%d (%d,%d) in window %d",
                event->motion.which, event->motion.x, event->motion.y,
                event->motion.xrel, event->motion.yrel,
                event->motion.windowID);
        break;
    case SDL_MOUSEBUTTONDOWN:
        fprintf(stderr, "Mouse %d: button %d pressed at %d,%d in window %d",
                event->button.which, event->button.button, event->button.x,
                event->button.y, event->button.windowID);
        break;
    case SDL_MOUSEBUTTONUP:
        fprintf(stderr, "Mouse %d: button %d released at %d,%d in window %d",
                event->button.which, event->button.button, event->button.x,
                event->button.y, event->button.windowID);
        break;
    case SDL_MOUSEWHEEL:
        fprintf(stderr,
                "Mouse %d: wheel scrolled %d in x and %d in y in window %d",
                event->wheel.which, event->wheel.x, event->wheel.y,
                event->wheel.windowID);
        break;
    case SDL_JOYBALLMOTION:
        fprintf(stderr, "Joystick %d: ball %d moved by %d,%d",
                event->jball.which, event->jball.ball, event->jball.xrel,
                event->jball.yrel);
        break;
    case SDL_JOYHATMOTION:
        fprintf(stderr, "Joystick %d: hat %d moved to ", event->jhat.which,
                event->jhat.hat);
        switch (event->jhat.value) {
        case SDL_HAT_CENTERED:
            fprintf(stderr, "CENTER");
            break;
        case SDL_HAT_UP:
            fprintf(stderr, "UP");
            break;
        case SDL_HAT_RIGHTUP:
            fprintf(stderr, "RIGHTUP");
            break;
        case SDL_HAT_RIGHT:
            fprintf(stderr, "RIGHT");
            break;
        case SDL_HAT_RIGHTDOWN:
            fprintf(stderr, "RIGHTDOWN");
            break;
        case SDL_HAT_DOWN:
            fprintf(stderr, "DOWN");
            break;
        case SDL_HAT_LEFTDOWN:
            fprintf(stderr, "LEFTDOWN");
            break;
        case SDL_HAT_LEFT:
            fprintf(stderr, "LEFT");
            break;
        case SDL_HAT_LEFTUP:
            fprintf(stderr, "LEFTUP");
            break;
        default:
            fprintf(stderr, "UNKNOWN");
            break;
        }
        break;
    case SDL_JOYBUTTONDOWN:
        fprintf(stderr, "Joystick %d: button %d pressed",
                event->jbutton.which, event->jbutton.button);
        break;
    case SDL_JOYBUTTONUP:
        fprintf(stderr, "Joystick %d: button %d released",
                event->jbutton.which, event->jbutton.button);
        break;
    case SDL_QUIT:
        fprintf(stderr, "Quit requested");
        break;
    case SDL_USEREVENT:
        fprintf(stderr, "User event %d", event->user.code);
        break;
    default:
        fprintf(stderr, "Unknown event %d", event->type);
        break;
    }
    fprintf(stderr, "\n");
}

void
CommonEvent(CommonState * state, SDL_Event * event, int *done)
{
    if (state->verbose & VERBOSE_EVENT) {
        PrintEvent(event);
    }

    switch (event->type) {
    case SDL_WINDOWEVENT:
        switch (event->window.event) {
        case SDL_WINDOWEVENT_CLOSE:
            *done = 1;
            break;
        }
        break;
    case SDL_KEYDOWN:
        switch (event->key.keysym.sym) {
            /* Add hotkeys here */
        case SDLK_g:
            if (event->key.keysym.mod & KMOD_CTRL) {
                /* Ctrl-G toggle grab */
            }
            break;
        case SDLK_ESCAPE:
            *done = 1;
            break;
        default:
            break;
        }
        break;
    case SDL_QUIT:
        *done = 1;
        break;
    }
}

void
CommonQuit(CommonState * state)
{
    if (state->flags & SDL_INIT_VIDEO) {
        SDL_VideoQuit();
    }
    if (state->flags & SDL_INIT_AUDIO) {
        SDL_AudioQuit();
    }
    if (state->windows) {
        SDL_free(state->windows);
    }
    SDL_free(state);
}


/* Simple program:  draw as many random objects on the screen as possible */

#include <stdlib.h>
#include <stdio.h>
#include <time.h>

#include "common.h"

#define NUM_OBJECTS	100

static CommonState *state;
static int num_objects;
static SDL_bool cycle_color;
static SDL_bool cycle_alpha;
static int cycle_direction = 1;
static int current_alpha = 255;
static int current_color = 255;
static SDL_BlendMode blendMode = SDL_BLENDMODE_NONE;

void
DrawPoints(SDL_Renderer * renderer)
{
    int i;
    int x, y;
    SDL_Rect viewport;

    /* Query the sizes */
    SDL_RenderGetViewport(renderer, &viewport);

    for (i = 0; i < num_objects * 4; ++i) {
        /* Cycle the color and alpha, if desired */
        if (cycle_color) {
            current_color += cycle_direction;
            if (current_color < 0) {
                current_color = 0;
                cycle_direction = -cycle_direction;
            }
            if (current_color > 255) {
                current_color = 255;
                cycle_direction = -cycle_direction;
            }
        }
        if (cycle_alpha) {
            current_alpha += cycle_direction;
            if (current_alpha < 0) {
                current_alpha = 0;
                cycle_direction = -cycle_direction;
            }
            if (current_alpha > 255) {
                current_alpha = 255;
                cycle_direction = -cycle_direction;
            }
        }
        SDL_SetRenderDrawColor(renderer, 255, (Uint8) current_color,
                               (Uint8) current_color, (Uint8) current_alpha);

        x = rand() % viewport.w;
        y = rand() % viewport.h;
        SDL_RenderDrawPoint(renderer, x, y);
    }
}

void
DrawLines(SDL_Renderer * renderer)
{
    int i;
    int x1, y1, x2, y2;
    SDL_Rect viewport;

    /* Query the sizes */
    SDL_RenderGetViewport(renderer, &viewport);

    for (i = 0; i < num_objects; ++i) {
        /* Cycle the color and alpha, if desired */
        if (cycle_color) {
            current_color += cycle_direction;
            if (current_color < 0) {
                current_color = 0;
                cycle_direction = -cycle_direction;
            }
            if (current_color > 255) {
                current_color = 255;
                cycle_direction = -cycle_direction;
            }
        }
        if (cycle_alpha) {
            current_alpha += cycle_direction;
            if (current_alpha < 0) {
                current_alpha = 0;
                cycle_direction = -cycle_direction;
            }
            if (current_alpha > 255) {
                current_alpha = 255;
                cycle_direction = -cycle_direction;
            }
        }
        SDL_SetRenderDrawColor(renderer, 255, (Uint8) current_color,
                               (Uint8) current_color, (Uint8) current_alpha);

        if (i == 0) {
            SDL_RenderDrawLine(renderer, 0, 0, viewport.w - 1, viewport.h - 1);
            SDL_RenderDrawLine(renderer, 0, viewport.h - 1, viewport.w - 1, 0);
            SDL_RenderDrawLine(renderer, 0, viewport.h / 2, viewport.w - 1, viewport.h / 2);
            SDL_RenderDrawLine(renderer, viewport.w / 2, 0, viewport.w / 2, viewport.h - 1);
        } else {
            x1 = (rand() % (viewport.w*2)) - viewport.w;
            x2 = (rand() % (viewport.w*2)) - viewport.w;
            y1 = (rand() % (viewport.h*2)) - viewport.h;
            y2 = (rand() % (viewport.h*2)) - viewport.h;
            SDL_RenderDrawLine(renderer, x1, y1, x2, y2);
        }
    }
}

void
DrawRects(SDL_Renderer * renderer)
{
    int i;
    SDL_Rect rect;
    SDL_Rect viewport;

    /* Query the sizes */
    SDL_RenderGetViewport(renderer, &viewport);

    for (i = 0; i < num_objects / 4; ++i) {
        /* Cycle the color and alpha, if desired */
        if (cycle_color) {
            current_color += cycle_direction;
            if (current_color < 0) {
                current_color = 0;
                cycle_direction = -cycle_direction;
            }
            if (current_color > 255) {
                current_color = 255;
                cycle_direction = -cycle_direction;
            }
        }
        if (cycle_alpha) {
            current_alpha += cycle_direction;
            if (current_alpha < 0) {
                current_alpha = 0;
                cycle_direction = -cycle_direction;
            }
            if (current_alpha > 255) {
                current_alpha = 255;
                cycle_direction = -cycle_direction;
            }
        }
        SDL_SetRenderDrawColor(renderer, 255, (Uint8) current_color,
                               (Uint8) current_color, (Uint8) current_alpha);

        rect.w = rand() % (viewport.h / 2);
        rect.h = rand() % (viewport.h / 2);
        rect.x = (rand() % (viewport.w*2) - viewport.w) - (rect.w / 2);
        rect.y = (rand() % (viewport.h*2) - viewport.h) - (rect.h / 2);
        SDL_RenderFillRect(renderer, &rect);
    }
}

int
main(int argc, char *argv[])
{
    int i, done;
    SDL_Event event;
    Uint32 then, now, frames;

    /* Initialize parameters */
    num_objects = NUM_OBJECTS;

    /* Initialize test framework */
    state = CommonCreateState(argv, SDL_INIT_VIDEO);
    if (!state) {
        return 1;
    }
    for (i = 1; i < argc;) {
        int consumed;

        consumed = CommonArg(state, i);
        if (consumed == 0) {
            consumed = -1;
            if (SDL_strcasecmp(argv[i], "--blend") == 0) {
                if (argv[i + 1]) {
                    if (SDL_strcasecmp(argv[i + 1], "none") == 0) {
                        blendMode = SDL_BLENDMODE_NONE;
                        consumed = 2;
                    } else if (SDL_strcasecmp(argv[i + 1], "blend") == 0) {
                        blendMode = SDL_BLENDMODE_BLEND;
                        consumed = 2;
                    } else if (SDL_strcasecmp(argv[i + 1], "add") == 0) {
                        blendMode = SDL_BLENDMODE_ADD;
                        consumed = 2;
                    } else if (SDL_strcasecmp(argv[i + 1], "mod") == 0) {
                        blendMode = SDL_BLENDMODE_MOD;
                        consumed = 2;
                    }
                }
            } else if (SDL_strcasecmp(argv[i], "--cyclecolor") == 0) {
                cycle_color = SDL_TRUE;
                consumed = 1;
            } else if (SDL_strcasecmp(argv[i], "--cyclealpha") == 0) {
                cycle_alpha = SDL_TRUE;
                consumed = 1;
            } else if (SDL_isdigit(*argv[i])) {
                num_objects = SDL_atoi(argv[i]);
                consumed = 1;
            }
        }
        if (consumed < 0) {
            fprintf(stderr,
                    "Usage: %s %s [--blend none|blend|add|mod] [--cyclecolor] [--cyclealpha]\n",
                    argv[0], CommonUsage(state));
            return 1;
        }
        i += consumed;
    }
    if (!CommonInit(state)) {
        return 2;
    }

    /* Create the windows and initialize the renderers */
    for (i = 0; i < state->num_windows; ++i) {
        SDL_Renderer *renderer = state->renderers[i];
        SDL_SetRenderDrawBlendMode(renderer, blendMode);
        SDL_SetRenderDrawColor(renderer, 0xA0, 0xA0, 0xA0, 0xFF);
        SDL_RenderClear(renderer);
    }

    srand((unsigned int)time(NULL));

    /* Main render loop */
    frames = 0;
    then = SDL_GetTicks();
    done = 0;
    while (!done) {
        /* Check for events */
        ++frames;
        while (SDL_PollEvent(&event)) {
            CommonEvent(state, &event, &done);
        }
        for (i = 0; i < state->num_windows; ++i) {
            SDL_Renderer *renderer = state->renderers[i];
            SDL_SetRenderDrawColor(renderer, 0xA0, 0xA0, 0xA0, 0xFF);
            SDL_RenderClear(renderer);

            DrawRects(renderer);
            DrawLines(renderer);
            DrawPoints(renderer);

            SDL_RenderPresent(renderer);
        }
    }

    CommonQuit(state);

    /* Print out some timing information */
    now = SDL_GetTicks();
    if (now > then) {
        double fps = ((double) frames * 1000) / (now - then);
        printf("%2.2f frames per second\n", fps);
    }
    return 0;
}

/* vi: set ts=4 sw=4 expandtab: */

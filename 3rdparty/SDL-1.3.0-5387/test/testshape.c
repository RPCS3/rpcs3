#include <stdlib.h>
#include <math.h>
#include <stdio.h>
#include "SDL.h"
#include "SDL_shape.h"

#define SHAPED_WINDOW_X 150
#define SHAPED_WINDOW_Y 150
#define SHAPED_WINDOW_DIMENSION 640

#define TICK_INTERVAL 1000/10

typedef struct LoadedPicture {
    SDL_Surface *surface;
    SDL_Texture *texture;
    SDL_WindowShapeMode mode;
} LoadedPicture;

void render(SDL_Renderer *renderer,SDL_Texture *texture,SDL_Rect texture_dimensions)
{
    //Clear render-target to blue.
    SDL_SetRenderDrawColor(renderer,0x00,0x00,0xff,0xff);
    SDL_RenderClear(renderer);
    
    //Render the texture.
    SDL_RenderCopy(renderer,texture,&texture_dimensions,&texture_dimensions);
    
    SDL_RenderPresent(renderer);
}

static Uint32 next_time;

Uint32 time_left()
{
    Uint32 now = SDL_GetTicks();
    if(next_time <= now)
        return 0;
    else
        return next_time - now;
}

int main(int argc,char** argv)
{
    Uint8 num_pictures;
    LoadedPicture* pictures;
    int i, j;
    SDL_PixelFormat* format = NULL;
    SDL_Window *window;
    SDL_Renderer *renderer;
    SDL_Color black = {0,0,0,0xff};
    SDL_Event event;
    int event_pending = 0;
    int should_exit = 0;
    unsigned int current_picture;
    int button_down;
    Uint32 pixelFormat = 0;
    int access = 0;
    SDL_Rect texture_dimensions;;

    if(argc < 2) {
        printf("SDL_Shape requires at least one bitmap file as argument.\n");
        exit(-1);
    }
    
    if(SDL_VideoInit(NULL) == -1) {
        printf("Could not initialize SDL video.\n");
        exit(-2);
    }
    
    num_pictures = argc - 1;
    pictures = (LoadedPicture *)malloc(sizeof(LoadedPicture)*num_pictures);
    for(i=0;i<num_pictures;i++)
        pictures[i].surface = NULL;
    for(i=0;i<num_pictures;i++) {
        pictures[i].surface = SDL_LoadBMP(argv[i+1]);
        if(pictures[i].surface == NULL) {
            j = 0;
            for(j=0;j<num_pictures;j++)
                if(pictures[j].surface != NULL)
                    SDL_FreeSurface(pictures[j].surface);
            free(pictures);
            SDL_VideoQuit();
            printf("Could not load surface from named bitmap file.\n");
            exit(-3);
        }

        format = pictures[i].surface->format;
        if(format->Amask != 0) {
            pictures[i].mode.mode = ShapeModeBinarizeAlpha;
            pictures[i].mode.parameters.binarizationCutoff = 255;
        }
        else {
            pictures[i].mode.mode = ShapeModeColorKey;
            pictures[i].mode.parameters.colorKey = black;
        }
    }
    
    window = SDL_CreateShapedWindow("SDL_Shape test",SHAPED_WINDOW_X,SHAPED_WINDOW_Y,SHAPED_WINDOW_DIMENSION,SHAPED_WINDOW_DIMENSION,SDL_WINDOW_RESIZABLE | SDL_WINDOW_SHOWN);
    if(window == NULL) {
        for(i=0;i<num_pictures;i++)
            SDL_FreeSurface(pictures[i].surface);
        free(pictures);
        SDL_VideoQuit();
        printf("Could not create shaped window for SDL_Shape.\n");
        exit(-4);
    }
    renderer = SDL_CreateRenderer(window,-1,0);
    if (!renderer) {
        SDL_DestroyWindow(window);
        for(i=0;i<num_pictures;i++)
            SDL_FreeSurface(pictures[i].surface);
        free(pictures);
        SDL_VideoQuit();
        printf("Could not create rendering context for SDL_Shape window.\n");
        exit(-5);
    }
    
    for(i=0;i<num_pictures;i++)
        pictures[i].texture = NULL;
    for(i=0;i<num_pictures;i++) {
        pictures[i].texture = SDL_CreateTextureFromSurface(renderer,pictures[i].surface);
        if(pictures[i].texture == NULL) {
            j = 0;
            for(j=0;j<num_pictures;i++)
                if(pictures[i].texture != NULL)
                    SDL_DestroyTexture(pictures[i].texture);
            for(i=0;i<num_pictures;i++)
                SDL_FreeSurface(pictures[i].surface);
            free(pictures);
            SDL_DestroyRenderer(renderer);
            SDL_DestroyWindow(window);
            SDL_VideoQuit();
            printf("Could not create texture for SDL_shape.\n");
            exit(-6);
        }
    }
    
    event_pending = 0;
    should_exit = 0;
    event_pending = SDL_PollEvent(&event);
    current_picture = 0;
    button_down = 0;
    texture_dimensions.h = 0;
    texture_dimensions.w = 0;
    texture_dimensions.x = 0;
    texture_dimensions.y = 0;
    SDL_QueryTexture(pictures[current_picture].texture,(Uint32 *)&pixelFormat,(int *)&access,&texture_dimensions.w,&texture_dimensions.h);
    SDL_SetWindowSize(window,texture_dimensions.w,texture_dimensions.h);
    SDL_SetWindowShape(window,pictures[current_picture].surface,&pictures[current_picture].mode);
    next_time = SDL_GetTicks() + TICK_INTERVAL;
    while(should_exit == 0) {
        event_pending = SDL_PollEvent(&event);
        if(event_pending == 1) {
            if(event.type == SDL_KEYDOWN) {
                button_down = 1;
                if(event.key.keysym.sym == SDLK_ESCAPE)
                    should_exit = 1;
            }
            if(button_down && event.type == SDL_KEYUP) {
                button_down = 0;
                current_picture += 1;
                if(current_picture >= num_pictures)
                    current_picture = 0;
                SDL_QueryTexture(pictures[current_picture].texture,(Uint32 *)&pixelFormat,(int *)&access,&texture_dimensions.w,&texture_dimensions.h);
                SDL_SetWindowSize(window,texture_dimensions.w,texture_dimensions.h);
                SDL_SetWindowShape(window,pictures[current_picture].surface,&pictures[current_picture].mode);
            }
            if(event.type == SDL_QUIT)
                should_exit = 1;
            event_pending = 0;
        }
        render(renderer,pictures[current_picture].texture,texture_dimensions);
        SDL_Delay(time_left());
        next_time += TICK_INTERVAL;
    }
    
    //Free the textures.
    for(i=0;i<num_pictures;i++)
        SDL_DestroyTexture(pictures[i].texture);
    SDL_DestroyRenderer(renderer);
    //Destroy the window.
    SDL_DestroyWindow(window);
    //Free the original surfaces backing the textures.
    for(i=0;i<num_pictures;i++)
        SDL_FreeSurface(pictures[i].surface);
    free(pictures);
    //Call SDL_VideoQuit() before quitting.
    SDL_VideoQuit();

    return 0;
}

/* vi: set ts=4 sw=4 expandtab: */

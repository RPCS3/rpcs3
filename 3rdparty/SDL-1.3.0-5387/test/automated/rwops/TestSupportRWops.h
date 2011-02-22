
#include <stdio.h>
#include "SDL.h"

#ifndef TestSupportRWops_h
#define TestSupportRWops_h

FILE* TestSupportRWops_OpenFPFromReadDir(const char *file, const char *mode);
FILE* TestSupportRWops_OpenFPFromWriteDir(const char *file, const char *mode);
SDL_RWops* TestSupportRWops_OpenRWopsFromReadDir(const char *file, const char *mode);
SDL_RWops* TestSupportRWops_OpenRWopsFromWriteDir(const char *file, const char *mode);


#endif

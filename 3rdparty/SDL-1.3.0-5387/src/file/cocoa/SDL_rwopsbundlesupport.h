#ifdef __APPLE__

#include <stdio.h>

#ifndef SDL_rwopsbundlesupport_h
#define SDL_rwopsbundlesupport_h
FILE* SDL_OpenFPFromBundleOrFallback(const char *file, const char *mode);
#endif
#endif

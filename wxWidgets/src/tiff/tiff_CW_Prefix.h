#define NEED_STRDUP
#define WINVER 0x0400

/*
    We need to make sure some symbols required for correct use of
    Win32 SDK headers (and that some of these headers are included
    to begin with).  The best way is to simply include one of the
    ANSI headers.
*/

#include <stdlib.h>


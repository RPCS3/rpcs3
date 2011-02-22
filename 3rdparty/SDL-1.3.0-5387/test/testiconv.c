
#include <stdio.h>

#include "SDL.h"

static size_t
widelen(char *data)
{
    size_t len = 0;
    Uint32 *p = (Uint32 *) data;
    while (*p++) {
        ++len;
    }
    return len;
}

int
main(int argc, char *argv[])
{
    const char *formats[] = {
        "UTF8",
        "UTF-8",
        "UTF16BE",
        "UTF-16BE",
        "UTF16LE",
        "UTF-16LE",
        "UTF32BE",
        "UTF-32BE",
        "UTF32LE",
        "UTF-32LE",
        "UCS4",
        "UCS-4",
    };
    char buffer[BUFSIZ];
    char *ucs4;
    char *test[2];
    int i, index = 0;
    FILE *file;
    int errors = 0;

    if (!argv[1]) {
        argv[1] = "utf8.txt";
    }
    file = fopen(argv[1], "rb");
    if (!file) {
        fprintf(stderr, "Unable to open %s\n", argv[1]);
        return (1);
    }

    while (fgets(buffer, sizeof(buffer), file)) {
        /* Convert to UCS-4 */
        size_t len;
        ucs4 =
            SDL_iconv_string("UCS-4", "UTF-8", buffer,
                             SDL_strlen(buffer) + 1);
        len = (widelen(ucs4) + 1) * 4;
        for (i = 0; i < SDL_arraysize(formats); ++i) {
            test[0] = SDL_iconv_string(formats[i], "UCS-4", ucs4, len);
            test[1] = SDL_iconv_string("UCS-4", formats[i], test[0], len);
            if (!test[1] || SDL_memcmp(test[1], ucs4, len) != 0) {
                fprintf(stderr, "FAIL: %s\n", formats[i]);
                ++errors;
            }
            if (test[0]) {
                SDL_free(test[0]);
            }
            if (test[1]) {
                SDL_free(test[1]);
            }
        }
        test[0] = SDL_iconv_string("UTF-8", "UCS-4", ucs4, len);
        SDL_free(ucs4);
        fputs(test[0], stdout);
        SDL_free(test[0]);
    }
    return (errors ? errors + 1 : 0);
}

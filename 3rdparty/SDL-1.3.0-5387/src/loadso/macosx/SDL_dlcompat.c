/*
    SDL - Simple DirectMedia Layer
    Copyright (C) 1997-2011 Sam Lantinga

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

    Sam Lantinga
    slouken@libsdl.org
*/
#include "SDL_config.h"

#ifdef SDL_LOADSO_DLCOMPAT

/* Please note that dlcompat apparently ships in current Mac OS X versions
 *  as a system library that provides compatibility with the Unix "dlopen"
 *  interface. In order to allow SDL to work on older OS X releases and also
 *  not conflict with the system lib on newer versions, we include dlcompat
 *  in SDL and change the symbols to prevent symbol clash with any existing
 *  system libraries.  --ryan.
 */

/* here is the dlcompat license: */

/*
Copyright (c) 2002 Jorge Acereda  <jacereda@users.sourceforge.net> &
                   Peter O'Gorman <ogorman@users.sourceforge.net>
                   
Portions may be copyright others, see the AUTHORS file included with this
distribution.

Maintained by Peter O'Gorman <ogorman@users.sourceforge.net>

Bug Reports and other queries should go to <ogorman@users.sourceforge.net>

Permission is hereby granted, free of charge, to any person obtaining
a copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be
included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include <pthread.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdarg.h>
#include <limits.h>
#include <mach-o/dyld.h>
#include <mach-o/nlist.h>
#include <mach-o/getsect.h>

#include "SDL_stdinc.h"

/* Just playing to see if it would compile with the freebsd headers, it does,
 * but because of the different values for RTLD_LOCAL etc, it would break binary
 * compat... oh well
 */
#ifndef __BSD_VISIBLE
#define __BSD_VISIBLE 1
#endif

/*include "dlfcn.h"*/
#ifdef __cplusplus
extern "C"
{
#endif

#if defined (__GNUC__) && __GNUC__ > 3
#define dl_restrict __restrict
#else
#define dl_restrict
#endif

#if 0
#ifndef _POSIX_SOURCE
/*
 * Structure filled in by dladdr().
 */
    typedef struct SDL_OSX_dl_info
    {
        const char *dli_fname;  /* Pathname of shared object */
        void *dli_fbase;        /* Base address of shared object */
        const char *dli_sname;  /* Name of nearest symbol */
        void *dli_saddr;        /* Address of nearest symbol */
    } SDL_OSX_Dl_info;

    static int SDL_OSX_dladdr(const void *dl_restrict,
                              SDL_OSX_Dl_info * dl_restrict);
#endif                          /* ! _POSIX_SOURCE */
#endif                          /* 0 */

    static int SDL_OSX_dlclose(void *handle);
    static const char *SDL_OSX_dlerror(void);
    static void *SDL_OSX_dlopen(const char *path, int mode);
    static void *SDL_OSX_dlsym(void *dl_restrict handle,
                               const char *dl_restrict symbol);

#define RTLD_LAZY	0x1
#define RTLD_NOW	0x2
#define RTLD_LOCAL	0x4
#define RTLD_GLOBAL	0x8

#ifndef _POSIX_SOURCE
#define RTLD_NOLOAD	0x10
#define RTLD_NODELETE	0x80

/*
 * Special handle arguments for SDL_OSX_dlsym().
 */
#define	RTLD_NEXT		((void *) -1)   /* Search subsequent objects. */
#define	RTLD_DEFAULT	((void *) -2)   /* Use default search algorithm. */
#endif                          /* ! _POSIX_SOURCE */

#ifdef __cplusplus
}
#endif

#ifndef dl_restrict
#define dl_restrict __restrict
#endif
/* This is not available on 10.1 */
#ifndef LC_LOAD_WEAK_DYLIB
#define	LC_LOAD_WEAK_DYLIB (0x18 | LC_REQ_DYLD)
#endif

/* With this stuff here, this thing may actually compile/run on 10.0 systems
 * Not that I have a 10.0 system to test it on anylonger
 */
#ifndef LC_REQ_DYLD
#define LC_REQ_DYLD 0x80000000
#endif
#ifndef NSADDIMAGE_OPTION_RETURN_ONLY_IF_LOADED
#define NSADDIMAGE_OPTION_RETURN_ONLY_IF_LOADED 0x4
#endif
#ifndef NSADDIMAGE_OPTION_RETURN_ON_ERROR
#define NSADDIMAGE_OPTION_RETURN_ON_ERROR 0x1
#endif
#ifndef NSLOOKUPSYMBOLINIMAGE_OPTION_BIND
#define NSLOOKUPSYMBOLINIMAGE_OPTION_BIND 0x0
#endif
#ifndef NSLOOKUPSYMBOLINIMAGE_OPTION_RETURN_ON_ERROR
#define NSLOOKUPSYMBOLINIMAGE_OPTION_RETURN_ON_ERROR 0x4
#endif
/* These symbols will be looked for in dyld */
static const struct mach_header *(*dyld_NSAddImage) (const char *,
                                                     unsigned long) = 0;
static int (*dyld_NSIsSymbolNameDefinedInImage) (const struct mach_header *,
                                                 const char *) = 0;
static NSSymbol(*dyld_NSLookupSymbolInImage) (const struct mach_header *,
                                              const char *, unsigned long) =
    0;

/* Define this to make dlcompat reuse data block. This way in theory we save
 * a little bit of overhead. However we then couldn't correctly catch excess
 * calls to SDL_OSX_dlclose(). Hence we don't use this feature
 */
#undef REUSE_STATUS

/* Size of the internal error message buffer (used by dlerror()) */
#define ERR_STR_LEN			251

/* Maximum number of search paths supported by getSearchPath */
#define MAX_SEARCH_PATHS	32


#define MAGIC_DYLIB_OFI ((NSObjectFileImage) 'DYOF')
#define MAGIC_DYLIB_MOD ((NSModule) 'DYMO')

/* internal flags */
#define DL_IN_LIST 0x01

/* our mutex */
static pthread_mutex_t dlcompat_mutex;
/* Our thread specific storage
 */
static pthread_key_t dlerror_key;

struct dlthread
{
    int lockcnt;
    unsigned char errset;
    char errstr[ERR_STR_LEN];
};

/* This is our central data structure. Whenever a module is loaded via
 * SDL_OSX_dlopen(), we create such a struct.
 */
struct dlstatus
{
    struct dlstatus *next;      /* pointer to next element in the linked list */
    NSModule module;
    const struct mach_header *lib;
    int refs;                   /* reference count */
    int mode;                   /* mode in which this module was loaded */
    dev_t device;
    ino_t inode;
    int flags;                  /* Any internal flags we may need */
};

/* Head node of the dlstatus list */
static struct dlstatus mainStatus =
    { 0, MAGIC_DYLIB_MOD, NULL, -1, RTLD_GLOBAL, 0, 0, 0 };
static struct dlstatus *stqueue = &mainStatus;


/* Storage for the last error message (used by dlerror()) */
/* static char err_str[ERR_STR_LEN]; */
/* static int err_filled = 0; */

/* Prototypes to internal functions */
static void debug(const char *fmt, ...);
static void error(const char *str, ...);
static const char *safegetenv(const char *s);
static const char *searchList(void);
static const char *getSearchPath(int i);
static const char *getFullPath(int i, const char *file);
static const struct stat *findFile(const char *file, const char **fullPath);
static int isValidStatus(struct dlstatus *status);
static inline int isFlagSet(int mode, int flag);
static struct dlstatus *lookupStatus(const struct stat *sbuf);
static void insertStatus(struct dlstatus *dls, const struct stat *sbuf);
static int promoteLocalToGlobal(struct dlstatus *dls);
static void *reference(struct dlstatus *dls, int mode);
static void *dlsymIntern(struct dlstatus *dls, const char *symbol,
                         int canSetError);
static struct dlstatus *allocStatus(void);
static struct dlstatus *loadModule(const char *path, const struct stat *sbuf,
                                   int mode);
static NSSymbol search_linked_libs(const struct mach_header *mh,
                                   const char *symbol);
static const char *get_lib_name(const struct mach_header *mh);
static const struct mach_header *get_mach_header_from_NSModule(NSModule mod);
static void dlcompat_init_func(void);
static inline void dlcompat_init_check(void);
static inline void dolock(void);
static inline void dounlock(void);
static void dlerrorfree(void *data);
static void resetdlerror(void);
static const struct mach_header *my_find_image(const char *name);
static const struct mach_header *image_for_address(const void *address);
static inline char *dyld_error_str(void);

#if FINK_BUILD
/* Two Global Functions */
static void *dlsym_prepend_underscore(void *handle, const char *symbol);
static void *dlsym_auto_underscore(void *handle, const char *symbol);

/* And their _intern counterparts */
static void *dlsym_prepend_underscore_intern(void *handle,
                                             const char *symbol);
static void *dlsym_auto_underscore_intern(void *handle, const char *symbol);
#endif

/* Functions */

static void
debug(const char *fmt, ...)
{
#if DEBUG > 1
    va_list arg;
    va_start(arg, fmt);
    fprintf(stderr, "DLDEBUG: ");
    vfprintf(stderr, fmt, arg);
    fprintf(stderr, "\n");
    fflush(stderr);
    va_end(arg);
#endif
}

static void
error(const char *str, ...)
{
    va_list arg;
    struct dlthread *tss;
    char *err_str;
    va_start(arg, str);
    tss = pthread_getspecific(dlerror_key);
    err_str = tss->errstr;
    SDL_strlcpy(err_str, "dlcompat: ", ERR_STR_LEN);
    vsnprintf(err_str + 10, ERR_STR_LEN - 10, str, arg);
    va_end(arg);
    debug("ERROR: %s\n", err_str);
    tss->errset = 1;
}

static void
warning(const char *str)
{
#if DEBUG > 0
    fprintf(stderr, "WARNING: dlcompat: %s\n", str);
#endif
}

static const char *
safegetenv(const char *s)
{
    const char *ss = SDL_getenv(s);
    return ss ? ss : "";
}

/* because this is only used for debugging and error reporting functions, we
 * don't really care about how elegant it is... it could use the load
 * commands to find the install name of the library, but...
 */
static const char *
get_lib_name(const struct mach_header *mh)
{
    unsigned long count = _dyld_image_count();
    unsigned long i;
    const char *val = NULL;
    if (mh) {
        for (i = 0; i < count; i++) {
            if (mh == _dyld_get_image_header(i)) {
                val = _dyld_get_image_name(i);
                break;
            }
        }
    }
    return val;
}

/* Returns the mach_header for the module bu going through all the loaded images
 * and finding the one with the same name as the module. There really ought to be
 * an api for doing this, would be faster, but there isn't one right now
 */
static const struct mach_header *
get_mach_header_from_NSModule(NSModule mod)
{
    const char *mod_name = NSNameOfModule(mod);
    const struct mach_header *mh = NULL;
    unsigned long count = _dyld_image_count();
    unsigned long i;
    debug("Module name: %s", mod_name);
    for (i = 0; i < count; i++) {
        if (!SDL_strcmp(mod_name, _dyld_get_image_name(i))) {
            mh = _dyld_get_image_header(i);
            break;
        }
    }
    return mh;
}


/* Compute and return a list of all directories that we should search when
 * trying to locate a module. We first look at the values of LD_LIBRARY_PATH
 * and DYLD_LIBRARY_PATH, and then finally fall back to looking into
 * /usr/lib and /lib. Since both of the environments variables can contain a
 * list of colon seperated paths, we simply concat them and the two other paths
 * into one big string, which we then can easily parse.
 * Splitting this string into the actual path list is done by getSearchPath()
 */
static const char *
searchList()
{
    size_t buf_size;
    static char *buf = NULL;
    const char *ldlp = safegetenv("LD_LIBRARY_PATH");
    const char *dyldlp = safegetenv("DYLD_LIBRARY_PATH");
    const char *stdpath = SDL_getenv("DYLD_FALLBACK_LIBRARY_PATH");
    if (!stdpath)
        stdpath = "/usr/local/lib:/lib:/usr/lib";
    if (!buf) {
        buf_size =
            SDL_strlen(ldlp) + SDL_strlen(dyldlp) + SDL_strlen(stdpath) + 4;
        buf = SDL_malloc(buf_size);
        SDL_snprintf(buf, buf_size, "%s%s%s%s%s%c", dyldlp,
                     (dyldlp[0] ? ":" : ""), ldlp, (ldlp[0] ? ":" : ""),
                     stdpath, '\0');
    }
    return buf;
}

/* Returns the ith search path from the list as computed by searchList() */
static const char *
getSearchPath(int i)
{
    static const char *list = 0;
    static char **path = (char **) 0;
    static int end = 0;
    static int numsize = MAX_SEARCH_PATHS;
    static char **tmp;
    /* So we can call SDL_free() in the "destructor" we use i=-1 to return the alloc'd array */
    if (i == -1) {
        return (const char *) path;
    }
    if (!path) {
        path = (char **) SDL_calloc(MAX_SEARCH_PATHS, sizeof(char **));
    }
    if (!list && !end)
        list = searchList();
    if (i >= (numsize)) {
        debug("Increasing size for long PATH");
        tmp =
            (char **) SDL_calloc((MAX_SEARCH_PATHS + numsize),
                                 sizeof(char **));
        if (tmp) {
            SDL_memcpy(tmp, path, sizeof(char **) * numsize);
            SDL_free(path);
            path = tmp;
            numsize += MAX_SEARCH_PATHS;
        } else {
            return 0;
        }
    }

    while (!path[i] && !end) {
        path[i] = strsep((char **) &list, ":");

        if (path[i][0] == 0)
            path[i] = 0;
        end = (list == 0);
    }
    return path[i];
}

static const char *
getFullPath(int i, const char *file)
{
    static char buf[PATH_MAX];
    const char *path = getSearchPath(i);
    if (path) {
        SDL_snprintf(buf, PATH_MAX, "%s/%s", path, file);
    }
    return path ? buf : 0;
}

/* Given a file name, try to determine the full path for that file. Starts
 * its search in the current directory, and then tries all paths in the 
 * search list in the order they are specified there.
 */
static const struct stat *
findFile(const char *file, const char **fullPath)
{
    int i = 0;
    static struct stat sbuf;
    char *fileName;
    debug("finding file %s", file);
    *fullPath = file;
    if (0 == stat(file, &sbuf))
        return &sbuf;
    if (SDL_strchr(file, '/'))
        return 0;               /* If the path had a / we don't look in env var places */
    fileName = NULL;
    if (!fileName)
        fileName = (char *) file;
    while ((*fullPath = getFullPath(i++, fileName))) {
        if (0 == stat(*fullPath, &sbuf))
            return &sbuf;
    }
    ;
    return 0;
}

/* Determine whether a given dlstatus is valid or not */
static int
isValidStatus(struct dlstatus *status)
{
    /* Walk the list to verify status is contained in it */
    struct dlstatus *dls = stqueue;
    while (dls && status != dls)
        dls = dls->next;
    if (dls == 0)
        error("invalid handle");
    else if ((dls->module == 0) || (dls->refs == 0))
        error("handle to closed library");
    else
        return TRUE;
    return FALSE;
}

static inline int
isFlagSet(int mode, int flag)
{
    return (mode & flag) == flag;
}

static struct dlstatus *
lookupStatus(const struct stat *sbuf)
{
    struct dlstatus *dls = stqueue;
    debug("looking for status");
    while (dls && ( /* isFlagSet(dls->mode, RTLD_UNSHARED) */ 0
                   || sbuf->st_dev != dls->device
                   || sbuf->st_ino != dls->inode))
        dls = dls->next;
    return dls;
}

static void
insertStatus(struct dlstatus *dls, const struct stat *sbuf)
{
    debug("inserting status");
    dls->inode = sbuf->st_ino;
    dls->device = sbuf->st_dev;
    dls->refs = 0;
    dls->mode = 0;
    if ((dls->flags & DL_IN_LIST) == 0) {
        dls->next = stqueue;
        stqueue = dls;
        dls->flags |= DL_IN_LIST;
    }
}

static struct dlstatus *
allocStatus()
{
    struct dlstatus *dls;
#ifdef REUSE_STATUS
    dls = stqueue;
    while (dls && dls->module)
        dls = dls->next;
    if (!dls)
#endif
        dls = SDL_calloc(sizeof(*dls), 1);
    return dls;
}

static int
promoteLocalToGlobal(struct dlstatus *dls)
{
    static int (*p) (NSModule module) = 0;
    debug("promoting");
    if (!p)
        _dyld_func_lookup("__dyld_NSMakePrivateModulePublic", (void **) &p);
    return (dls->module == MAGIC_DYLIB_MOD) || (p && p(dls->module));
}

static void *
reference(struct dlstatus *dls, int mode)
{
    if (dls) {
        if (dls->module == MAGIC_DYLIB_MOD && isFlagSet(mode, RTLD_LOCAL)) {
            warning("trying to open a .dylib with RTLD_LOCAL");
            error("unable to open a .dylib with RTLD_LOCAL");
            return NULL;
        }
        if (isFlagSet(mode, RTLD_GLOBAL) && !isFlagSet(dls->mode, RTLD_GLOBAL)
            && !promoteLocalToGlobal(dls)) {
            error("unable to promote local module to global");
            return NULL;
        }
        dls->mode |= mode;
        dls->refs++;
    } else
        debug("reference called with NULL argument");

    return dls;
}

static const struct mach_header *
my_find_image(const char *name)
{
    const struct mach_header *mh = 0;
    const char *id = NULL;
    int i = _dyld_image_count();
    int j;
    mh = (struct mach_header *)
        dyld_NSAddImage(name, NSADDIMAGE_OPTION_RETURN_ONLY_IF_LOADED |
                        NSADDIMAGE_OPTION_RETURN_ON_ERROR);
    if (!mh) {
        for (j = 0; j < i; j++) {
            id = _dyld_get_image_name(j);
            if (!SDL_strcmp(id, name)) {
                mh = _dyld_get_image_header(j);
                break;
            }
        }
    }
    return mh;
}

/*
 * dyld adds libraries by first adding the directly dependant libraries in link order, and
 * then adding the dependencies for those libraries, so we should do the same... but we don't
 * bother adding the extra dependencies, if the symbols are neither in the loaded image nor
 * any of it's direct dependencies, then it probably isn't there.
 */
static NSSymbol
search_linked_libs(const struct mach_header *mh, const char *symbol)
{
    unsigned int n;
    struct load_command *lc = 0;
    struct mach_header *wh;
    NSSymbol nssym = 0;
    if (dyld_NSAddImage && dyld_NSIsSymbolNameDefinedInImage
        && dyld_NSLookupSymbolInImage) {
        lc = (struct load_command *) ((char *) mh +
                                      sizeof(struct mach_header));
        for (n = 0; n < mh->ncmds;
             n++, lc = (struct load_command *) ((char *) lc + lc->cmdsize)) {
            if ((LC_LOAD_DYLIB == lc->cmd)
                || (LC_LOAD_WEAK_DYLIB == lc->cmd)) {
                if ((wh = (struct mach_header *)
                     my_find_image((char
                                    *) (((struct dylib_command *) lc)->dylib.
                                        name.offset + (char *) lc)))) {
                    if (dyld_NSIsSymbolNameDefinedInImage(wh, symbol)) {
                        nssym = dyld_NSLookupSymbolInImage(wh,
                                                           symbol,
                                                           NSLOOKUPSYMBOLINIMAGE_OPTION_BIND
                                                           |
                                                           NSLOOKUPSYMBOLINIMAGE_OPTION_RETURN_ON_ERROR);
                        break;
                    }
                }
            }
        }
        if ((!nssym) && NSIsSymbolNameDefined(symbol)) {
            /* I've never seen this debug message... */
            debug("Symbol \"%s\" is defined but was not found", symbol);
        }
    }
    return nssym;
}

/* Up to the caller to SDL_free() returned string */
static inline char *
dyld_error_str()
{
    NSLinkEditErrors dylder;
    int dylderno;
    const char *dylderrstr;
    const char *dyldfile;
    char *retStr = NULL;
    NSLinkEditError(&dylder, &dylderno, &dyldfile, &dylderrstr);
    if (dylderrstr && *dylderrstr) {
        retStr = SDL_strdup(dylderrstr);
    }
    return retStr;
}

static void *
dlsymIntern(struct dlstatus *dls, const char *symbol, int canSetError)
{
    NSSymbol nssym = 0;
#ifdef __GCC__
    void *caller = __builtin_return_address(1); /* Be *very* careful about inlining */
#else
    void *caller = NULL;
#endif
    const struct mach_header *caller_mh = 0;
    char *savedErrorStr = NULL;
    resetdlerror();
#ifndef RTLD_SELF
#define	RTLD_SELF		((void *) -3)
#endif
    if (NULL == dls)
        dls = RTLD_SELF;
    if ((RTLD_NEXT == dls) || (RTLD_SELF == dls)) {
        if (dyld_NSIsSymbolNameDefinedInImage && dyld_NSLookupSymbolInImage
            && caller) {
            caller_mh = image_for_address(caller);
            if (RTLD_SELF == dls) {
                /* FIXME: We should be using the NSModule api, if SELF is an MH_BUNDLE
                 * But it appears to work anyway, and looking at the code in dyld_libfuncs.c
                 * this is acceptable.
                 */
                if (dyld_NSIsSymbolNameDefinedInImage(caller_mh, symbol)) {
                    nssym = dyld_NSLookupSymbolInImage(caller_mh,
                                                       symbol,
                                                       NSLOOKUPSYMBOLINIMAGE_OPTION_BIND
                                                       |
                                                       NSLOOKUPSYMBOLINIMAGE_OPTION_RETURN_ON_ERROR);
                }
            }
            if (!nssym) {
                if (RTLD_SELF == dls)
                    savedErrorStr = dyld_error_str();
                nssym = search_linked_libs(caller_mh, symbol);
            }
        } else {
            if (canSetError)
                error("RTLD_SELF and RTLD_NEXT are not supported");
            return NULL;
        }
    }
    if (!nssym) {

        if (RTLD_DEFAULT == dls) {
            dls = &mainStatus;
        }
        if (!isValidStatus(dls))
            return NULL;

        if (dls->module != MAGIC_DYLIB_MOD) {
            nssym = NSLookupSymbolInModule(dls->module, symbol);
            if (!nssym && NSIsSymbolNameDefined(symbol)) {
                debug("Searching dependencies");
                savedErrorStr = dyld_error_str();
                nssym =
                    search_linked_libs(get_mach_header_from_NSModule
                                       (dls->module), symbol);
            }
        } else if (dls->lib && dyld_NSIsSymbolNameDefinedInImage
                   && dyld_NSLookupSymbolInImage) {
            if (dyld_NSIsSymbolNameDefinedInImage(dls->lib, symbol)) {
                nssym = dyld_NSLookupSymbolInImage(dls->lib,
                                                   symbol,
                                                   NSLOOKUPSYMBOLINIMAGE_OPTION_BIND
                                                   |
                                                   NSLOOKUPSYMBOLINIMAGE_OPTION_RETURN_ON_ERROR);
            } else if (NSIsSymbolNameDefined(symbol)) {
                debug("Searching dependencies");
                savedErrorStr = dyld_error_str();
                nssym = search_linked_libs(dls->lib, symbol);
            }
        } else if (dls->module == MAGIC_DYLIB_MOD) {
            /* Global context, use NSLookupAndBindSymbol */
            if (NSIsSymbolNameDefined(symbol)) {
                /* There doesn't seem to be a return on error option for this call???
                   this is potentially broken, if binding fails, it will improperly
                   exit the application. */
                nssym = NSLookupAndBindSymbol(symbol);
            } else {
                if (savedErrorStr)
                    SDL_free(savedErrorStr);
                savedErrorStr = SDL_malloc(256);
                SDL_snprintf(savedErrorStr, 256,
                             "Symbol \"%s\" not in global context", symbol);
            }
        }
    }
    /* Error reporting */
    if (!nssym) {
        if (!savedErrorStr || !SDL_strlen(savedErrorStr)) {
            if (savedErrorStr)
                SDL_free(savedErrorStr);
            savedErrorStr = SDL_malloc(256);
            SDL_snprintf(savedErrorStr, 256, "Symbol \"%s\" not found",
                         symbol);
        }
        if (canSetError) {
            error(savedErrorStr);
        } else {
            debug(savedErrorStr);
        }
        if (savedErrorStr)
            SDL_free(savedErrorStr);
        return NULL;
    }
    return NSAddressOfSymbol(nssym);
}

static struct dlstatus *
loadModule(const char *path, const struct stat *sbuf, int mode)
{
    NSObjectFileImage ofi = 0;
    NSObjectFileImageReturnCode ofirc;
    struct dlstatus *dls;
    NSLinkEditErrors ler;
    int lerno;
    const char *errstr;
    const char *file;
    void (*init) (void);

    ofirc = NSCreateObjectFileImageFromFile(path, &ofi);
    switch (ofirc) {
    case NSObjectFileImageSuccess:
        break;
    case NSObjectFileImageInappropriateFile:
        if (dyld_NSAddImage && dyld_NSIsSymbolNameDefinedInImage
            && dyld_NSLookupSymbolInImage) {
            if (isFlagSet(mode, RTLD_LOCAL)) {
                warning("trying to open a .dylib with RTLD_LOCAL");
                error("unable to open this file with RTLD_LOCAL");
                return NULL;
            }
        } else {
            error("opening this file is unsupported on this system");
            return NULL;
        }
        break;
    case NSObjectFileImageFailure:
        error("object file setup failure");
        return NULL;
    case NSObjectFileImageArch:
        error("no object for this architecture");
        return NULL;
    case NSObjectFileImageFormat:
        error("bad object file format");
        return NULL;
    case NSObjectFileImageAccess:
        error("can't read object file");
        return NULL;
    default:
        error("unknown error from NSCreateObjectFileImageFromFile()");
        return NULL;
    }
    dls = lookupStatus(sbuf);
    if (!dls) {
        dls = allocStatus();
    }
    if (!dls) {
        error("unable to allocate memory");
        return NULL;
    }
    //      dls->lib = 0;
    if (ofirc == NSObjectFileImageInappropriateFile) {
        if ((dls->lib =
             dyld_NSAddImage(path, NSADDIMAGE_OPTION_RETURN_ON_ERROR))) {
            debug("Dynamic lib loaded at %ld", dls->lib);
            ofi = MAGIC_DYLIB_OFI;
            dls->module = MAGIC_DYLIB_MOD;
            ofirc = NSObjectFileImageSuccess;
            /* Although it is possible with a bit of work to modify this so it works and
               functions with RTLD_NOW, I don't deem it necessary at the moment */
        }
        if (!(dls->module)) {
            NSLinkEditError(&ler, &lerno, &file, &errstr);
            if (!errstr || (!SDL_strlen(errstr)))
                error("Can't open this file type");
            else
                error(errstr);
            if ((dls->flags & DL_IN_LIST) == 0) {
                SDL_free(dls);
            }
            return NULL;
        }
    } else {
        dls->module = NSLinkModule(ofi, path,
                                   NSLINKMODULE_OPTION_RETURN_ON_ERROR |
                                   NSLINKMODULE_OPTION_PRIVATE |
                                   (isFlagSet(mode, RTLD_NOW) ?
                                    NSLINKMODULE_OPTION_BINDNOW : 0));
        NSDestroyObjectFileImage(ofi);
        if (dls->module) {
            dls->lib = get_mach_header_from_NSModule(dls->module);
        }
    }
    if (!dls->module) {
        NSLinkEditError(&ler, &lerno, &file, &errstr);
        if ((dls->flags & DL_IN_LIST) == 0) {
            SDL_free(dls);
        }
        error(errstr);
        return NULL;
    }

    insertStatus(dls, sbuf);
    dls = reference(dls, mode);
    if ((init = dlsymIntern(dls, "__init", 0))) {
        debug("calling _init()");
        init();
    }
    return dls;
}

inline static void
dlcompat_init_check(void)
{
    static pthread_mutex_t l = PTHREAD_MUTEX_INITIALIZER;
    static int init_done = 0;

    pthread_mutex_lock(&l);
    if (!init_done) {
        dlcompat_init_func();
        init_done = 1;
    }
    pthread_mutex_unlock(&l);
}

static void
dlcompat_init_func(void)
{
    _dyld_func_lookup("__dyld_NSAddImage", (void **) &dyld_NSAddImage);
    _dyld_func_lookup("__dyld_NSIsSymbolNameDefinedInImage",
                      (void **) &dyld_NSIsSymbolNameDefinedInImage);
    _dyld_func_lookup("__dyld_NSLookupSymbolInImage",
                      (void **) &dyld_NSLookupSymbolInImage);
    if (pthread_mutex_init(&dlcompat_mutex, NULL))
        exit(1);
    if (pthread_key_create(&dlerror_key, &dlerrorfree))
        exit(1);
}

static void
resetdlerror()
{
    struct dlthread *tss;
    tss = pthread_getspecific(dlerror_key);
    tss->errset = 0;
}

static void
dlerrorfree(void *data)
{
    SDL_free(data);
}

/* We kind of want a recursive lock here, but meet a little trouble
 * because they are not available pre OS X 10.2, so we fake it
 * using thread specific storage to keep a lock count
 */
static inline void
dolock(void)
{
    int err = 0;
    struct dlthread *tss;
    dlcompat_init_check();
    tss = pthread_getspecific(dlerror_key);
    if (!tss) {
        tss = SDL_malloc(sizeof(struct dlthread));
        tss->lockcnt = 0;
        tss->errset = 0;
        if (pthread_setspecific(dlerror_key, tss)) {
            fprintf(stderr, "dlcompat: pthread_setspecific failed\n");
            exit(1);
        }
    }
    if (!tss->lockcnt)
        err = pthread_mutex_lock(&dlcompat_mutex);
    tss->lockcnt = tss->lockcnt + 1;
    if (err)
        exit(err);
}

static inline void
dounlock(void)
{
    int err = 0;
    struct dlthread *tss;
    tss = pthread_getspecific(dlerror_key);
    tss->lockcnt = tss->lockcnt - 1;
    if (!tss->lockcnt)
        err = pthread_mutex_unlock(&dlcompat_mutex);
    if (err)
        exit(err);
}

static void *
SDL_OSX_dlopen(const char *path, int mode)
{
    const struct stat *sbuf;
    struct dlstatus *dls;
    const char *fullPath;

    dolock();
    resetdlerror();
    if (!path) {
        dls = &mainStatus;
        goto dlopenok;
    }
    if (!(sbuf = findFile(path, &fullPath))) {
        error("file \"%s\" not found", path);
        goto dlopenerror;
    }
    /* Now checks that it hasn't been closed already */
    if ((dls = lookupStatus(sbuf)) && (dls->refs > 0)) {
        /* debug("status found"); */
        dls = reference(dls, mode);
        goto dlopenok;
    }
#ifdef 	RTLD_NOLOAD
    if (isFlagSet(mode, RTLD_NOLOAD)) {
        error("no existing handle and RTLD_NOLOAD specified");
        goto dlopenerror;
    }
#endif
    if (isFlagSet(mode, RTLD_LAZY) && isFlagSet(mode, RTLD_NOW)) {
        error("how can I load something both RTLD_LAZY and RTLD_NOW?");
        goto dlopenerror;
    }
    dls = loadModule(fullPath, sbuf, mode);

  dlopenok:
    dounlock();
    return (void *) dls;
  dlopenerror:
    dounlock();
    return NULL;
}

#if !FINK_BUILD
static void *
SDL_OSX_dlsym(void *dl_restrict handle, const char *dl_restrict symbol)
{
    int sym_len = SDL_strlen(symbol);
    void *value = NULL;
    char *malloc_sym = NULL;
    dolock();
    malloc_sym = SDL_malloc(sym_len + 2);
    if (malloc_sym) {
        SDL_snprintf(malloc_sym, sym_len + 2, "_%s", symbol);
        value = dlsymIntern(handle, malloc_sym, 1);
        SDL_free(malloc_sym);
    } else {
        error("Unable to allocate memory");
        goto dlsymerror;
    }
    dounlock();
    return value;
  dlsymerror:
    dounlock();
    return NULL;
}
#endif

#if FINK_BUILD

static void *
dlsym_prepend_underscore(void *handle, const char *symbol)
{
    void *answer;
    dolock();
    answer = dlsym_prepend_underscore_intern(handle, symbol);
    dounlock();
    return answer;
}

static void *
dlsym_prepend_underscore_intern(void *handle, const char *symbol)
{
/*
 *	A quick and easy way for porting packages which call dlsym(handle,"sym")
 *	If the porter adds -Ddlsym=dlsym_prepend_underscore to the CFLAGS then
 *	this function will be called, and will add the required underscore.
 *	
 *	Note that I haven't figured out yet which should be "standard", prepend
 *	the underscore always, or not at all. These global functions need to go away
 *	for opendarwin.
 */
    int sym_len = SDL_strlen(symbol);
    void *value = NULL;
    char *malloc_sym = NULL;
    malloc_sym = SDL_malloc(sym_len + 2);
    if (malloc_sym) {
        SDL_snprintf(malloc_sym, sym_len + 2, "_%s", symbol);
        value = dlsymIntern(handle, malloc_sym, 1);
        SDL_free(malloc_sym);
    } else {
        error("Unable to allocate memory");
    }
    return value;
}

static void *
dlsym_auto_underscore(void *handle, const char *symbol)
{
    void *answer;
    dolock();
    answer = dlsym_auto_underscore_intern(handle, symbol);
    dounlock();
    return answer;

}

static void *
dlsym_auto_underscore_intern(void *handle, const char *symbol)
{
    struct dlstatus *dls = handle;
    void *addr = 0;
    addr = dlsymIntern(dls, symbol, 0);
    if (!addr)
        addr = dlsym_prepend_underscore_intern(handle, symbol);
    return addr;
}


static void *
SDL_OSX_dlsym(void *dl_restrict handle, const char *dl_restrict symbol)
{
    struct dlstatus *dls = handle;
    void *addr = 0;
    dolock();
    addr = dlsymIntern(dls, symbol, 1);
    dounlock();
    return addr;
}
#endif

static int
SDL_OSX_dlclose(void *handle)
{
    struct dlstatus *dls = handle;
    dolock();
    resetdlerror();
    if (!isValidStatus(dls)) {
        goto dlcloseerror;
    }
    if (dls->module == MAGIC_DYLIB_MOD) {
        const char *name;
        if (!dls->lib) {
            name = "global context";
        } else {
            name = get_lib_name(dls->lib);
        }
        warning("trying to close a .dylib!");
        error("Not closing \"%s\" - dynamic libraries cannot be closed",
              name);
        goto dlcloseerror;
    }
    if (!dls->module) {
        error("module already closed");
        goto dlcloseerror;
    }

    if (dls->refs == 1) {
        unsigned long options = 0;
        void (*fini) (void);
        if ((fini = dlsymIntern(dls, "__fini", 0))) {
            debug("calling _fini()");
            fini();
        }
        options |= NSUNLINKMODULE_OPTION_RESET_LAZY_REFERENCES;
#ifdef RTLD_NODELETE
        if (isFlagSet(dls->mode, RTLD_NODELETE))
            options |= NSUNLINKMODULE_OPTION_KEEP_MEMORY_MAPPED;
#endif
        if (!NSUnLinkModule(dls->module, options)) {
            error("unable to unlink module");
            goto dlcloseerror;
        }
        dls->refs--;
        dls->module = 0;
        /* Note: the dlstatus struct dls is neither removed from the list
         * nor is the memory it occupies freed. This shouldn't pose a 
         * problem in mostly all cases, though.
         */
    }
    dounlock();
    return 0;
  dlcloseerror:
    dounlock();
    return 1;
}

static const char *
SDL_OSX_dlerror(void)
{
    struct dlthread *tss;
    const char *err_str = NULL;
    dlcompat_init_check();
    tss = pthread_getspecific(dlerror_key);
    if (tss != NULL && tss->errset != 0) {
        tss->errset = 0;
        err_str = tss->errstr;
    }
    return (err_str);
}

/* Given an address, return the mach_header for the image containing it
 * or zero if the given address is not contained in any loaded images.
 */
static const struct mach_header *
image_for_address(const void *address)
{
    unsigned long i;
    unsigned long j;
    unsigned long count = _dyld_image_count();
    const struct mach_header *mh = 0;
    struct load_command *lc = 0;
    unsigned long addr = 0;
    for (i = 0; i < count; i++) {
        addr = (unsigned long) address - _dyld_get_image_vmaddr_slide(i);
        mh = _dyld_get_image_header(i);
        if (mh) {
            lc = (struct load_command *) ((char *) mh +
                                          sizeof(struct mach_header));
            for (j = 0; j < mh->ncmds;
                 j++, lc =
                 (struct load_command *) ((char *) lc + lc->cmdsize)) {
                if (LC_SEGMENT == lc->cmd &&
                    addr >= ((struct segment_command *) lc)->vmaddr &&
                    addr <
                    ((struct segment_command *) lc)->vmaddr +
                    ((struct segment_command *) lc)->vmsize) {
                    goto image_found;
                }
            }
        }
        mh = 0;
    }
  image_found:
    return mh;
}

#if 0                           /* unused */
static int
SDL_OSX_dladdr(const void *dl_restrict p, SDL_OSX_Dl_info * dl_restrict info)
{
/*
	FIXME: USe the routine image_for_address.
*/
    unsigned long i;
    unsigned long j;
    unsigned long count = _dyld_image_count();
    struct mach_header *mh = 0;
    struct load_command *lc = 0;
    unsigned long addr = NULL;
    unsigned long table_off = (unsigned long) 0;
    int found = 0;
    if (!info)
        return 0;
    dolock();
    resetdlerror();
    info->dli_fname = 0;
    info->dli_fbase = 0;
    info->dli_sname = 0;
    info->dli_saddr = 0;
/* Some of this was swiped from code posted by Douglas Davidson <ddavidso AT apple DOT com>
 * to darwin-development AT lists DOT apple DOT com and slightly modified
 */
    for (i = 0; i < count; i++) {
        addr = (unsigned long) p - _dyld_get_image_vmaddr_slide(i);
        mh = _dyld_get_image_header(i);
        if (mh) {
            lc = (struct load_command *) ((char *) mh +
                                          sizeof(struct mach_header));
            for (j = 0; j < mh->ncmds;
                 j++, lc =
                 (struct load_command *) ((char *) lc + lc->cmdsize)) {
                if (LC_SEGMENT == lc->cmd &&
                    addr >= ((struct segment_command *) lc)->vmaddr &&
                    addr <
                    ((struct segment_command *) lc)->vmaddr +
                    ((struct segment_command *) lc)->vmsize) {
                    info->dli_fname = _dyld_get_image_name(i);
                    info->dli_fbase = (void *) mh;
                    found = 1;
                    break;
                }
            }
            if (found)
                break;
        }
    }
    if (!found) {
        dounlock();
        return 0;
    }
    lc = (struct load_command *) ((char *) mh + sizeof(struct mach_header));
    for (j = 0; j < mh->ncmds;
         j++, lc = (struct load_command *) ((char *) lc + lc->cmdsize)) {
        if (LC_SEGMENT == lc->cmd) {
            if (!SDL_strcmp
                (((struct segment_command *) lc)->segname, "__LINKEDIT"))
                break;
        }
    }
    table_off =
        ((unsigned long) ((struct segment_command *) lc)->vmaddr) -
        ((unsigned long) ((struct segment_command *) lc)->fileoff) +
        _dyld_get_image_vmaddr_slide(i);
    debug("table off %x", table_off);

    lc = (struct load_command *) ((char *) mh + sizeof(struct mach_header));
    for (j = 0; j < mh->ncmds;
         j++, lc = (struct load_command *) ((char *) lc + lc->cmdsize)) {
        if (LC_SYMTAB == lc->cmd) {

            struct nlist *symtable =
                (struct nlist *) (((struct symtab_command *) lc)->symoff +
                                  table_off);
            unsigned long numsyms = ((struct symtab_command *) lc)->nsyms;
            struct nlist *nearest = NULL;
            unsigned long diff = 0xffffffff;
            unsigned long strtable =
                (unsigned long) (((struct symtab_command *) lc)->stroff +
                                 table_off);
            debug("symtable %x", symtable);
            for (i = 0; i < numsyms; i++) {
                /* Ignore the following kinds of Symbols */
                if ((!symtable->n_value)        /* Undefined */
                    ||(symtable->n_type >= N_PEXT)      /* Debug symbol */
                    ||(!(symtable->n_type & N_EXT))     /* Local Symbol */
                    ) {
                    symtable++;
                    continue;
                }
                if ((addr >= symtable->n_value)
                    && (diff >= (symtable->n_value - addr))) {
                    diff = (unsigned long) symtable->n_value - addr;
                    nearest = symtable;
                }
                symtable++;
            }
            if (nearest) {
                info->dli_saddr = nearest->n_value + ((void *) p - addr);
                info->dli_sname = (char *) (strtable + nearest->n_un.n_strx);
            }
        }
    }
    dounlock();
    return 1;
}
#endif

/*
 * Implement the dlfunc() interface, which behaves exactly the same as
 * dlsym() except that it returns a function pointer instead of a data
 * pointer.  This can be used by applications to avoid compiler warnings
 * about undefined behavior, and is intended as prior art for future
 * POSIX standardization.  This function requires that all pointer types
 * have the same representation, which is true on all platforms FreeBSD
 * runs on, but is not guaranteed by the C standard.
 */
#if 0
static dlfunc_t
SDL_OSX_dlfunc(void *dl_restrict handle, const char *dl_restrict symbol)
{
    union
    {
        void *d;
        dlfunc_t f;
    } rv;
    int sym_len = SDL_strlen(symbol);
    char *malloc_sym = NULL;
    dolock();
    malloc_sym = SDL_malloc(sym_len + 2);
    if (malloc_sym) {
        SDL_snprintf(malloc_sym, sym_len + 2, "_%s", symbol);
        rv.d = dlsymIntern(handle, malloc_sym, 1);
        SDL_free(malloc_sym);
    } else {
        error("Unable to allocate memory");
        goto dlfuncerror;
    }
    dounlock();
    return rv.f;
  dlfuncerror:
    dounlock();
    return NULL;
}
#endif



/* dlcompat ends, here's the SDL interface...  --ryan.  */


/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/* System dependent library loading routines                           */

#include "SDL_loadso.h"

void *
SDL_LoadObject(const char *sofile)
{
    void *handle = SDL_OSX_dlopen(sofile, RTLD_NOW);
    const char *loaderror = SDL_OSX_dlerror();
    if (handle == NULL) {
        SDL_SetError("Failed loading %s: %s", sofile, loaderror);
    }
    return (handle);
}

void *
SDL_LoadFunction(void *handle, const char *name)
{
    void *symbol = SDL_OSX_dlsym(handle, name);
    if (symbol == NULL) {
        SDL_SetError("Failed loading %s: %s", name, SDL_OSX_dlerror());
    }
    return (symbol);
}

void
SDL_UnloadObject(void *handle)
{
    if (handle != NULL) {
        SDL_OSX_dlclose(handle);
    }
}

#endif /* SDL_LOADSO_DLCOMPAT */

/* vi: set ts=4 sw=4 expandtab: */

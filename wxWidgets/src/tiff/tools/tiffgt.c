
/*
 * Copyright (c) 1988-1997 Sam Leffler
 * Copyright (c) 1991-1997 Silicon Graphics, Inc.
 * Copyright (c) 2003, Andrey Kiselev <dron@ak4719.spb.edu>
 *
 * Permission to use, copy, modify, distribute, and sell this software and 
 * its documentation for any purpose is hereby granted without fee, provided
 * that (i) the above copyright notices and this permission notice appear in
 * all copies of the software and related documentation, and (ii) the names of
 * Sam Leffler and Silicon Graphics may not be used in any advertising or
 * publicity relating to the software without the specific, prior written
 * permission of Sam Leffler and Silicon Graphics.
 * 
 * THE SOFTWARE IS PROVIDED "AS-IS" AND WITHOUT WARRANTY OF ANY KIND, 
 * EXPRESS, IMPLIED OR OTHERWISE, INCLUDING WITHOUT LIMITATION, ANY 
 * WARRANTY OF MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE.  
 * 
 * IN NO EVENT SHALL SAM LEFFLER OR SILICON GRAPHICS BE LIABLE FOR
 * ANY SPECIAL, INCIDENTAL, INDIRECT OR CONSEQUENTIAL DAMAGES OF ANY KIND,
 * OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
 * WHETHER OR NOT ADVISED OF THE POSSIBILITY OF DAMAGE, AND ON ANY THEORY OF 
 * LIABILITY, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE 
 * OF THIS SOFTWARE.
 */

#include "tif_config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#if HAVE_APPLE_OPENGL_FRAMEWORK
# include <OpenGL/gl.h>
# include <GLUT/glut.h>
#else
# include <GL/gl.h>
# include <GL/glut.h>
#endif

#include "tiffio.h"
#include "tiffiop.h"

#ifndef HAVE_GETOPT
extern int getopt(int, char**, char*);
#endif

static  uint32  width = 0, height = 0;          /* window width & height */
static  uint32* raster = NULL;                  /* displayable image */
static TIFFRGBAImage img;
static int      order0 = 0, order;
static uint16   photo0 = (uint16) -1, photo;
static int      stoponerr = 0;                  /* stop on read error */
static int      verbose = 0;
#define TITLE_LENGTH    1024
static char     title[TITLE_LENGTH];            /* window title line */
static uint32   xmax, ymax;
static char**   filelist = NULL;
static int      fileindex;
static int      filenum;
static TIFFErrorHandler oerror;
static TIFFErrorHandler owarning;

static void	cleanup_and_exit(void);
static int	initImage(void);
static int	prevImage(void);
static int	nextImage(void);
static void	setWindowSize(void);
static void	usage(void);
static uint16	photoArg(const char*);
static void	raster_draw(void);
static void	raster_reshape(int, int);
static void	raster_keys(unsigned char, int, int);
static void	raster_special(int, int, int);

extern  char* optarg;
extern  int optind;
static TIFF* tif = NULL;

int
main(int argc, char* argv[])
{
        int c;
        int dirnum = -1;
        uint32 diroff = 0;

        oerror = TIFFSetErrorHandler(NULL);
        owarning = TIFFSetWarningHandler(NULL);
        while ((c = getopt(argc, argv, "d:o:p:eflmsvw?")) != -1)
            switch (c) {
            case 'd':
                dirnum = atoi(optarg);
                break;
            case 'e':
                oerror = TIFFSetErrorHandler(oerror);
                break;
            case 'l':
                order0 = FILLORDER_LSB2MSB;
                break;
            case 'm':
                order0 = FILLORDER_MSB2LSB;
                break;
            case 'o':
                diroff = strtoul(optarg, NULL, 0);
                break;
            case 'p':
                photo0 = photoArg(optarg);
                break;
            case 's':
                stoponerr = 1;
                break;
            case 'w':
                owarning = TIFFSetWarningHandler(owarning);
                break;
            case 'v':
                verbose = 1;
                break;
            case '?':
                usage();
                /*NOTREACHED*/
            }
        filenum = argc - optind;
        if ( filenum < 1)
                usage();

        glutInit(&argc, argv);
        glutInitDisplayMode(GLUT_SINGLE | GLUT_RGB);

        /*
         * Get the screen size
         */
        xmax = glutGet(GLUT_SCREEN_WIDTH);
        ymax = glutGet(GLUT_SCREEN_HEIGHT);

        /*
         * Use 90% of the screen size
         */
        xmax = xmax - xmax / 10.0;
        ymax = ymax - ymax / 10.0;

        filelist = (char **) _TIFFmalloc(filenum * sizeof(char*));
        if (!filelist) {
                TIFFError(argv[0], "Can not allocate space for the file list.");
                return 1;
        }
        _TIFFmemcpy(filelist, argv + optind, filenum * sizeof(char*));
        fileindex = -1;
        if (nextImage() < 0) {
                _TIFFfree(filelist);
                return 2;
        }
        /*
         * Set initial directory if user-specified
         * file was opened successfully.
         */
        if (dirnum != -1 && !TIFFSetDirectory(tif, dirnum))
            TIFFError(argv[0], "Error, seeking to directory %d", dirnum);
        if (diroff != 0 && !TIFFSetSubDirectory(tif, diroff))
            TIFFError(argv[0], "Error, setting subdirectory at %#x", diroff);
        order = order0;
        photo = photo0;
	if (initImage() < 0){
                _TIFFfree(filelist);
                return 3;
        }
        /*
         * Create a new window or reconfigure an existing
         * one to suit the image to be displayed.
         */
        glutInitWindowSize(width, height);
        snprintf(title, TITLE_LENGTH - 1, "%s [%u]", filelist[fileindex],
                (unsigned int) TIFFCurrentDirectory(tif));
        glutCreateWindow(title);
        glutDisplayFunc(raster_draw);
        glutReshapeFunc(raster_reshape);
        glutKeyboardFunc(raster_keys);
        glutSpecialFunc(raster_special);
        glutMainLoop();

        cleanup_and_exit();
        return 0;
}

static void 
cleanup_and_exit(void)
{
        TIFFRGBAImageEnd(&img);
        if (filelist != NULL)
                _TIFFfree(filelist);
        if (raster != NULL)
                _TIFFfree(raster);
        if (tif != NULL)
                TIFFClose(tif);
        exit(0);
}

static int
initImage(void)
{
        uint32 w, h;

        if (order)
                TIFFSetField(tif, TIFFTAG_FILLORDER, order);
        if (photo != (uint16) -1)
                TIFFSetField(tif, TIFFTAG_PHOTOMETRIC, photo);
        if (!TIFFRGBAImageBegin(&img, tif, stoponerr, title)) {
                TIFFError(filelist[fileindex], "%s", title);
                TIFFClose(tif);
                tif = NULL;
                return -1;
        }

        /*
         * Setup the image raster as required.
         */
        h = img.height;
        w = img.width;
        if (h > ymax) {
                w = (int)(w * ((float)ymax / h));
                h = ymax;
        }
        if (w > xmax) {
                h = (int)(h * ((float)xmax / w));
                w = xmax;
        }

	if (w != width || h != height) {
		uint32 rastersize =
			_TIFFMultiply32(tif, img.width, img.height, "allocating raster buffer");
		if (raster != NULL)
			_TIFFfree(raster), raster = NULL;
		raster = (uint32*) _TIFFCheckMalloc(tif, rastersize, sizeof (uint32),
						    "allocating raster buffer");
		if (raster == NULL) {
			width = height = 0;
			TIFFError(filelist[fileindex], "No space for raster buffer");
			cleanup_and_exit();
		}
		width = w;
		height = h;
	}
	TIFFRGBAImageGet(&img, raster, img.width, img.height);
#if HOST_BIGENDIAN
	TIFFSwabArrayOfLong(raster,img.width*img.height);
#endif
	return 0;
}

static int
prevImage(void)
{
        if (fileindex > 0)
                fileindex--;
        else if (tif)
                return fileindex;
        if (tif)
                TIFFClose(tif);
        tif = TIFFOpen(filelist[fileindex], "r");
        if (tif == NULL)
                return -1;
        return fileindex;
}

static int
nextImage(void)
{
        if (fileindex < filenum - 1)
                fileindex++;
        else if (tif)
                return fileindex;
        if (tif)
                TIFFClose(tif);
        tif = TIFFOpen(filelist[fileindex], "r");
        if (tif == NULL)
                return -1;
        return fileindex;
}

static void
setWindowSize(void)
{
        glutReshapeWindow(width, height);
}

static void
raster_draw(void)
{
  glDrawPixels(img.width, img.height, GL_RGBA, GL_UNSIGNED_BYTE, (const GLvoid *) raster);
}

static void
raster_reshape(int win_w, int win_h)
{
        GLfloat xratio = (GLfloat)win_w/img.width;
        GLfloat yratio = (GLfloat)win_h/img.height;
        int     ratio = (int)(((xratio > yratio)?xratio:yratio) * 100);

        glPixelZoom(xratio, yratio);
        glViewport(0, 0, win_w, win_h);
        snprintf(title, 1024, "%s [%u] %d%%", filelist[fileindex],
                (unsigned int) TIFFCurrentDirectory(tif), ratio);
        glutSetWindowTitle(title);
}

static void
raster_keys(unsigned char key, int x, int y)
{
        switch (key) {
                case 'b':                       /* photometric MinIsBlack */
                    photo = PHOTOMETRIC_MINISBLACK;
                    initImage();
                    break;
                case 'l':                       /* lsb-to-msb FillOrder */
                    order = FILLORDER_LSB2MSB;
                    initImage();
                    break;
                case 'm':                       /* msb-to-lsb FillOrder */
                    order = FILLORDER_MSB2LSB;
                    initImage();
                    break;
                case 'w':                       /* photometric MinIsWhite */
                    photo = PHOTOMETRIC_MINISWHITE;
                    initImage();
                    break;
                case 'W':                       /* toggle warnings */
                    owarning = TIFFSetWarningHandler(owarning);
                    initImage();
                    break;
                case 'E':                       /* toggle errors */
                    oerror = TIFFSetErrorHandler(oerror);
                    initImage();
                    break;
                case 'z':                       /* reset to defaults */
                case 'Z':
                    order = order0;
                    photo = photo0;
                    if (owarning == NULL)
                        owarning = TIFFSetWarningHandler(NULL);
                    if (oerror == NULL)
                        oerror = TIFFSetErrorHandler(NULL);
                    initImage();
                    break;
                case 'q':                       /* exit */
                case '\033':
                    cleanup_and_exit();
        }
        glutPostRedisplay();
}

static void
raster_special(int key, int x, int y)
{
        switch (key) {
                case GLUT_KEY_PAGE_UP:          /* previous logical image */
                    if (TIFFCurrentDirectory(tif) > 0) {
                            if (TIFFSetDirectory(tif,
                                                 TIFFCurrentDirectory(tif)-1)) {
                                    initImage();
                                    setWindowSize();
                        }
                    } else {
                            TIFFRGBAImageEnd(&img);
                            prevImage();
                            initImage();
                            setWindowSize();
                    }
                break;
                case GLUT_KEY_PAGE_DOWN:        /* next logical image */
                    if (!TIFFLastDirectory(tif)) {
                            if (TIFFReadDirectory(tif)) {
                                    initImage();
                                    setWindowSize();
                            }
                    } else {
                            TIFFRGBAImageEnd(&img);
                            nextImage();
                            initImage();
                            setWindowSize();
                    }
                break;
                case GLUT_KEY_HOME:             /* 1st image in current file */
                        if (TIFFSetDirectory(tif, 0)) {
                                TIFFRGBAImageEnd(&img);
                                initImage();
                                setWindowSize();
                        }
                break;
                case GLUT_KEY_END:              /* last image in current file */
                        TIFFRGBAImageEnd(&img);
                        while (!TIFFLastDirectory(tif))
                                TIFFReadDirectory(tif);
                        initImage();
                        setWindowSize();
                break;
        }
        glutPostRedisplay();
}



char* stuff[] = {
"usage: tiffgt [options] file.tif",
"where options are:",
" -c            use colormap visual",
" -d dirnum     set initial directory (default is 0)",
" -e            enable display of TIFF error messages",
" -l            force lsb-to-msb FillOrder",
" -m            force msb-to-lsb FillOrder",
" -o offset     set initial directory offset",
" -p photo      override photometric interpretation",
" -r            use fullcolor visual",
" -s            stop decoding on first error (default is ignore errors)",
" -v            enable verbose mode",
" -w            enable display of TIFF warning messages",
NULL
};

static void
usage(void)
{
        char buf[BUFSIZ];
        int i;

        setbuf(stderr, buf);
                fprintf(stderr, "%s\n\n", TIFFGetVersion());
        for (i = 0; stuff[i] != NULL; i++)
                fprintf(stderr, "%s\n", stuff[i]);
        exit(-1);
}

static uint16
photoArg(const char* arg)
{
        if (strcmp(arg, "miniswhite") == 0)
            return (PHOTOMETRIC_MINISWHITE);
        else if (strcmp(arg, "minisblack") == 0)
            return (PHOTOMETRIC_MINISBLACK);
        else if (strcmp(arg, "rgb") == 0)
            return (PHOTOMETRIC_RGB);
        else if (strcmp(arg, "palette") == 0)
            return (PHOTOMETRIC_PALETTE);
        else if (strcmp(arg, "mask") == 0)
            return (PHOTOMETRIC_MASK);
        else if (strcmp(arg, "separated") == 0)
            return (PHOTOMETRIC_SEPARATED);
        else if (strcmp(arg, "ycbcr") == 0)
            return (PHOTOMETRIC_YCBCR);
        else if (strcmp(arg, "cielab") == 0)
            return (PHOTOMETRIC_CIELAB);
        else if (strcmp(arg, "logl") == 0)
            return (PHOTOMETRIC_LOGL);
        else if (strcmp(arg, "logluv") == 0)
            return (PHOTOMETRIC_LOGLUV);
        else
            return ((uint16) -1);
}

/* vim: set ts=8 sts=8 sw=8 noet: */
/*
 * Local Variables:
 * mode: c
 * c-basic-offset: 8
 * fill-column: 78
 * End:
 */

/*
 *
 * xtiff - view a TIFF file in an X window
 *
 * Dan Sears
 * Chris Sears
 *
 * Copyright 1991 by Digital Equipment Corporation, Maynard, Massachusetts.
 *
 *                      All Rights Reserved
 *
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose and without fee is hereby granted,
 * provided that the above copyright notice appear in all copies and that
 * both that copyright notice and this permission notice appear in
 * supporting documentation, and that the name of Digital not be
 * used in advertising or publicity pertaining to distribution of the
 * software without specific, written prior permission.
 *
 * DIGITAL DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
 * ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL
 * DIGITAL BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR
 * ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
 * WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
 * ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
 * SOFTWARE.
 *
 * Revision 1.0  90/05/07
 *      Initial release.
 * Revision 2.0  90/12/20
 *      Converted to use the Athena Widgets and the Xt Intrinsics.
 *
 * Notes:
 *
 * According to the TIFF 5.0 Specification, it is possible to have
 * both a TIFFTAG_COLORMAP and a TIFFTAG_COLORRESPONSECURVE.  This
 * doesn't make sense since a TIFFTAG_COLORMAP is 16 bits wide and
 * a TIFFTAG_COLORRESPONSECURVE is tfBitsPerSample bits wide for each
 * channel.  This is probably a bug in the specification.
 * In this case, TIFFTAG_COLORRESPONSECURVE is ignored.
 * This might make sense if TIFFTAG_COLORMAP was 8 bits wide.
 *
 * TIFFTAG_COLORMAP is often incorrectly written as ranging from
 * 0 to 255 rather than from 0 to 65535.  CheckAndCorrectColormap()
 * takes care of this.
 *
 * Only ORIENTATION_TOPLEFT is supported correctly.  This is the
 * default TIFF and X orientation.  Other orientations will be
 * displayed incorrectly.
 *
 * There is no support for or use of 3/3/2 DirectColor visuals.
 * TIFFTAG_MINSAMPLEVALUE and TIFFTAG_MAXSAMPLEVALUE are not supported.
 *
 * Only TIFFTAG_BITSPERSAMPLE values that are 1, 2, 4 or 8 are supported.
 */
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <tiffio.h>
#include <X11/Xatom.h>
#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <X11/Xproto.h>
#include <X11/Shell.h>
#include <X11/Xaw/Form.h>
#include <X11/Xaw/List.h>
#include <X11/Xaw/Label.h>
#include <X11/cursorfont.h>
#define XK_MISCELLANY
#include <X11/keysymdef.h>
#include "xtifficon.h"

#define TIFF_GAMMA      "2.2"     /* default gamma from the TIFF 5.0 spec */
#define ROUND(x)        (uint16) ((x) + 0.5)
#define SCALE(x, s)     (((x) * 65535L) / (s))
#define MCHECK(m)       if (!m) { fprintf(stderr, "malloc failed\n"); exit(0); }
#define MIN(a, b)       (((a) < (b)) ? (a) : (b))
#define MAX(a, b)       (((a) > (b)) ? (a) : (b))
#define VIEWPORT_WIDTH  700
#define VIEWPORT_HEIGHT 500
#define KEY_TRANSLATE   20

#ifdef __STDC__
#define PP(args)    args
#else
#define PP(args)    ()
#endif

int main PP((int argc, char **argv));
void OpenTIFFFile PP((void));
void GetTIFFHeader PP((void));
void SetNameLabel PP((void));
void CheckAndCorrectColormap PP((void));
void SimpleGammaCorrection PP((void));
void GetVisual PP((void));
Boolean SearchVisualList PP((int image_depth,
    int visual_class, Visual **visual));
void GetTIFFImage PP((void));
void CreateXImage PP((void));
XtCallbackProc SelectProc PP((Widget w, caddr_t unused_1, caddr_t unused_2));
void QuitProc PP((void));
void NextProc PP((void));
void PreviousProc PP((void));
void PageProc PP((int direction));
void EventProc PP((Widget widget, caddr_t unused, XEvent *event));
void ResizeProc PP((void));
int XTiffErrorHandler PP((Display *display, XErrorEvent *error_event));
void Usage PP((void));

int xtVersion = XtSpecificationRelease;     /* xtiff depends on R4 or higher */

/*
 * Xt data structures
 */
Widget shellWidget, formWidget, listWidget, labelWidget, imageWidget;

enum { ButtonQuit = 0, ButtonPreviousPage = 1, ButtonNextPage = 2 };

String buttonStrings[] = { "Quit", "Previous", "Next" };

static XrmOptionDescRec shellOptions[] = {
    { "-help", "*help", XrmoptionNoArg, (caddr_t) "True" },
    { "-gamma", "*gamma", XrmoptionSepArg, NULL },
    { "-usePixmap", "*usePixmap", XrmoptionSepArg, NULL },
    { "-viewportWidth", "*viewportWidth", XrmoptionSepArg, NULL },
    { "-viewportHeight", "*viewportHeight", XrmoptionSepArg, NULL },
    { "-translate", "*translate", XrmoptionSepArg, NULL },
    { "-verbose", "*verbose", XrmoptionSepArg, NULL }
};

typedef struct {
    Boolean help;
    float gamma;
    Boolean usePixmap;
    uint32 viewportWidth;
    uint32 viewportHeight;
    int translate;
    Boolean verbose;
} AppData, *AppDataPtr;

AppData appData;

XtResource clientResources[] = {
    {
        "help", XtCBoolean, XtRBoolean, sizeof(Boolean),
        XtOffset(AppDataPtr, help), XtRImmediate, (XtPointer) False
    }, {
        "gamma", "Gamma", XtRFloat, sizeof(float),
        XtOffset(AppDataPtr, gamma), XtRString, (XtPointer) TIFF_GAMMA
    }, {
        "usePixmap", "UsePixmap", XtRBoolean, sizeof(Boolean),
        XtOffset(AppDataPtr, usePixmap), XtRImmediate, (XtPointer) True
    }, {
        "viewportWidth", "ViewportWidth", XtRInt, sizeof(int),
        XtOffset(AppDataPtr, viewportWidth), XtRImmediate,
        (XtPointer) VIEWPORT_WIDTH
    }, {
        "viewportHeight", "ViewportHeight", XtRInt, sizeof(int),
        XtOffset(AppDataPtr, viewportHeight), XtRImmediate,
        (XtPointer) VIEWPORT_HEIGHT
    }, {
        "translate", "Translate", XtRInt, sizeof(int),
        XtOffset(AppDataPtr, translate), XtRImmediate, (XtPointer) KEY_TRANSLATE
    }, {
        "verbose", "Verbose", XtRBoolean, sizeof(Boolean),
        XtOffset(AppDataPtr, verbose), XtRImmediate, (XtPointer) True
    }
};

Arg formArgs[] = {
    { XtNresizable, True }
};

Arg listArgs[] = {
    { XtNresizable, False },
    { XtNborderWidth, 0 },
    { XtNdefaultColumns, 3 },
    { XtNforceColumns, True },
    { XtNlist, (int) buttonStrings },
    { XtNnumberStrings, XtNumber(buttonStrings) },
    { XtNtop, XtChainTop },
    { XtNleft, XtChainLeft },
    { XtNbottom, XtChainTop },
    { XtNright, XtChainLeft }
};

Arg labelArgs[] = {
    { XtNresizable, False },
    { XtNwidth, 200 },
    { XtNborderWidth, 0 },
    { XtNjustify, XtJustifyLeft },
    { XtNtop, XtChainTop },
    { XtNleft, XtChainLeft },
    { XtNbottom, XtChainTop },
    { XtNright, XtChainLeft }
};

Arg imageArgs[] = {
    { XtNresizable, True },
    { XtNborderWidth, 0 },
    { XtNtop, XtChainTop },
    { XtNleft, XtChainLeft },
    { XtNbottom, XtChainTop },
    { XtNright, XtChainLeft }
};

XtActionsRec actionsTable[] = {
    { "quit", QuitProc },
    { "next", NextProc },
    { "previous", PreviousProc },
    { "notifyresize", ResizeProc }
};

char translationsTable[] = "<Key>q:      quit() \n \
                            <Key>Q:      quit() \n \
                            <Message>WM_PROTOCOLS: quit()\n \
                            <Key>p:      previous() \n \
                            <Key>P:      previous() \n \
                            <Key>n:      next() \n \
                            <Key>N:      next() \n \
                            <Configure>: notifyresize()";

/*
 * X data structures
 */
Colormap            xColormap;
Display *           xDisplay;
Pixmap              xImagePixmap;
Visual *            xVisual;
XImage *            xImage;
GC                  xWinGc;
int                 xImageDepth, xScreen, xRedMask, xGreenMask, xBlueMask,
                    xOffset = 0, yOffset = 0, grabX = -1, grabY = -1;
unsigned char       basePixel = 0;

/*
 * TIFF data structures
 */
TIFF *              tfFile = NULL;
uint32              tfImageWidth, tfImageHeight;
uint16              tfBitsPerSample, tfSamplesPerPixel, tfPlanarConfiguration,
                    tfPhotometricInterpretation, tfGrayResponseUnit,
                    tfImageDepth, tfBytesPerRow;
int                 tfDirectory = 0, tfMultiPage = False;
double              tfUnitMap, tfGrayResponseUnitMap[] = {
                        -1, -10, -100, -1000, -10000, -100000
                    };

/*
 * display data structures
 */
double              *dRed, *dGreen, *dBlue;

/*
 * shared data structures
 */
uint16 *            redMap = NULL, *greenMap = NULL, *blueMap = NULL,
                    *grayMap = NULL, colormapSize;
char *             imageMemory;
char *              fileName;

int
main(int argc, char **argv)
{
    XSetWindowAttributes window_attributes;
    Widget widget_list[3];
    Arg args[5];

    setbuf(stdout, NULL); setbuf(stderr, NULL);

    shellWidget = XtInitialize(argv[0], "XTiff", shellOptions,
        XtNumber(shellOptions), &argc, argv);

    XSetErrorHandler(XTiffErrorHandler);

    XtGetApplicationResources(shellWidget, &appData,
        (XtResourceList) clientResources, (Cardinal) XtNumber(clientResources),
        (ArgList) NULL, (Cardinal) 0);

    if ((argc <= 1) || (argc > 2) || appData.help)
        Usage();

    if (appData.verbose == False) {
        TIFFSetErrorHandler(0);
        TIFFSetWarningHandler(0);
    }

    fileName = argv[1];

    xDisplay = XtDisplay(shellWidget);
    xScreen = DefaultScreen(xDisplay);

    OpenTIFFFile();
    GetTIFFHeader();
    SimpleGammaCorrection();
    GetVisual();
    GetTIFFImage();

    /*
     * Send visual, colormap, depth and iconPixmap to shellWidget.
     * Sending the visual to the shell is only possible with the advent of R4.
     */
    XtSetArg(args[0], XtNvisual, xVisual);
    XtSetArg(args[1], XtNcolormap, xColormap);
    XtSetArg(args[2], XtNdepth,
        xImageDepth == 1 ? DefaultDepth(xDisplay, xScreen) : xImageDepth);
    XtSetArg(args[3], XtNiconPixmap,
        XCreateBitmapFromData(xDisplay, RootWindow(xDisplay, xScreen),
            xtifficon_bits, xtifficon_width, xtifficon_height));
    XtSetArg(args[4], XtNallowShellResize, True);
    XtSetValues(shellWidget, args, 5);

    /*
     * widget instance hierarchy
     */
    formWidget = XtCreateManagedWidget("form", formWidgetClass,
        shellWidget, formArgs, XtNumber(formArgs));

        widget_list[0] = listWidget = XtCreateWidget("list",
            listWidgetClass, formWidget, listArgs, XtNumber(listArgs));

        widget_list[1] = labelWidget = XtCreateWidget("label",
            labelWidgetClass, formWidget, labelArgs, XtNumber(labelArgs));

        widget_list[2] = imageWidget = XtCreateWidget("image",
            widgetClass, formWidget, imageArgs, XtNumber(imageArgs));

    XtManageChildren(widget_list, XtNumber(widget_list));

    /*
     * initial widget sizes - for small images let xtiff size itself
     */
    if (tfImageWidth >= appData.viewportWidth) {
        XtSetArg(args[0], XtNwidth, appData.viewportWidth);
        XtSetValues(shellWidget, args, 1);
    }
    if (tfImageHeight >= appData.viewportHeight) {
        XtSetArg(args[0], XtNheight, appData.viewportHeight);
        XtSetValues(shellWidget, args, 1);
    }

    XtSetArg(args[0], XtNwidth, tfImageWidth);
    XtSetArg(args[1], XtNheight, tfImageHeight);
    XtSetValues(imageWidget, args, 2);

    /*
     * formWidget uses these constraints but they are stored in the children.
     */
    XtSetArg(args[0], XtNfromVert, listWidget);
    XtSetValues(imageWidget, args, 1);
    XtSetArg(args[0], XtNfromHoriz, listWidget);
    XtSetValues(labelWidget, args, 1);

    SetNameLabel();

    XtAddCallback(listWidget, XtNcallback, (XtCallbackProc) SelectProc,
        (XtPointer) NULL);

    XtAddActions(actionsTable, XtNumber(actionsTable));
    XtSetArg(args[0], XtNtranslations,
        XtParseTranslationTable(translationsTable));
    XtSetValues(formWidget, &args[0], 1);
    XtSetValues(imageWidget, &args[0], 1);

    /*
     * This is intended to be a little faster than going through
     * the translation manager.
     */
    XtAddEventHandler(imageWidget, ExposureMask | ButtonPressMask
        | ButtonReleaseMask | Button1MotionMask | KeyPressMask,
        False, EventProc, NULL);

    XtRealizeWidget(shellWidget);

    window_attributes.cursor = XCreateFontCursor(xDisplay, XC_fleur);
    XChangeWindowAttributes(xDisplay, XtWindow(imageWidget),
        CWCursor, &window_attributes);

    CreateXImage();

    XtMainLoop();

    return 0;
}

void
OpenTIFFFile()
{
    if (tfFile != NULL)
        TIFFClose(tfFile);

    if ((tfFile = TIFFOpen(fileName, "r")) == NULL) {
	fprintf(appData.verbose ? stderr : stdout,
	    "xtiff: can't open %s as a TIFF file\n", fileName);
        exit(0);
    }

    tfMultiPage = (TIFFLastDirectory(tfFile) ? False : True);
}

void
GetTIFFHeader()
{
    register int i;

    if (!TIFFSetDirectory(tfFile, tfDirectory)) {
        fprintf(stderr, "xtiff: can't seek to directory %d in %s\n",
            tfDirectory, fileName);
        exit(0);
    }

    TIFFGetField(tfFile, TIFFTAG_IMAGEWIDTH, &tfImageWidth);
    TIFFGetField(tfFile, TIFFTAG_IMAGELENGTH, &tfImageHeight);

    /*
     * If the following tags aren't present then use the TIFF defaults.
     */
    TIFFGetFieldDefaulted(tfFile, TIFFTAG_BITSPERSAMPLE, &tfBitsPerSample);
    TIFFGetFieldDefaulted(tfFile, TIFFTAG_SAMPLESPERPIXEL, &tfSamplesPerPixel);
    TIFFGetFieldDefaulted(tfFile, TIFFTAG_PLANARCONFIG, &tfPlanarConfiguration);
    TIFFGetFieldDefaulted(tfFile, TIFFTAG_GRAYRESPONSEUNIT, &tfGrayResponseUnit);

    tfUnitMap = tfGrayResponseUnitMap[tfGrayResponseUnit];
    colormapSize = 1 << tfBitsPerSample;
    tfImageDepth = tfBitsPerSample * tfSamplesPerPixel;

    dRed = (double *) malloc(colormapSize * sizeof(double));
    dGreen = (double *) malloc(colormapSize * sizeof(double));
    dBlue = (double *) malloc(colormapSize * sizeof(double));
    MCHECK(dRed); MCHECK(dGreen); MCHECK(dBlue);

    /*
     * If TIFFTAG_PHOTOMETRIC is not present then assign a reasonable default.
     * The TIFF 5.0 specification doesn't give a default.
     */
    if (!TIFFGetField(tfFile, TIFFTAG_PHOTOMETRIC,
            &tfPhotometricInterpretation)) {
        if (tfSamplesPerPixel != 1)
            tfPhotometricInterpretation = PHOTOMETRIC_RGB;
        else if (tfBitsPerSample == 1)
            tfPhotometricInterpretation = PHOTOMETRIC_MINISBLACK;
        else if (TIFFGetField(tfFile, TIFFTAG_COLORMAP,
                &redMap, &greenMap, &blueMap)) {
            tfPhotometricInterpretation = PHOTOMETRIC_PALETTE;
            redMap = greenMap = blueMap = NULL;
        } else
            tfPhotometricInterpretation = PHOTOMETRIC_MINISBLACK;
    }

    /*
     * Given TIFFTAG_PHOTOMETRIC extract or create the response curves.
     */
    switch (tfPhotometricInterpretation) {
    case PHOTOMETRIC_RGB:
	redMap = (uint16 *) malloc(colormapSize * sizeof(uint16));
	greenMap = (uint16 *) malloc(colormapSize * sizeof(uint16));
	blueMap = (uint16 *) malloc(colormapSize * sizeof(uint16));
	MCHECK(redMap); MCHECK(greenMap); MCHECK(blueMap);
	for (i = 0; i < colormapSize; i++)
	    dRed[i] = dGreen[i] = dBlue[i]
		= (double) SCALE(i, colormapSize - 1);
        break;
    case PHOTOMETRIC_PALETTE:
        if (!TIFFGetField(tfFile, TIFFTAG_COLORMAP,
                &redMap, &greenMap, &blueMap)) {
            redMap = (uint16 *) malloc(colormapSize * sizeof(uint16));
            greenMap = (uint16 *) malloc(colormapSize * sizeof(uint16));
            blueMap = (uint16 *) malloc(colormapSize * sizeof(uint16));
            MCHECK(redMap); MCHECK(greenMap); MCHECK(blueMap);
            for (i = 0; i < colormapSize; i++)
                dRed[i] = dGreen[i] = dBlue[i]
                    = (double) SCALE(i, colormapSize - 1);
        } else {
            CheckAndCorrectColormap();
            for (i = 0; i < colormapSize; i++) {
                dRed[i] = (double) redMap[i];
                dGreen[i] = (double) greenMap[i];
                dBlue[i] = (double) blueMap[i];
            }
        }
        break;
    case PHOTOMETRIC_MINISWHITE:
        redMap = (uint16 *) malloc(colormapSize * sizeof(uint16));
        greenMap = (uint16 *) malloc(colormapSize * sizeof(uint16));
        blueMap = (uint16 *) malloc(colormapSize * sizeof(uint16));
        MCHECK(redMap); MCHECK(greenMap); MCHECK(blueMap);
	for (i = 0; i < colormapSize; i++)
	    dRed[i] = dGreen[i] = dBlue[i] = (double)
		 SCALE(colormapSize-1-i, colormapSize-1);
        break;
    case PHOTOMETRIC_MINISBLACK:
        redMap = (uint16 *) malloc(colormapSize * sizeof(uint16));
        greenMap = (uint16 *) malloc(colormapSize * sizeof(uint16));
        blueMap = (uint16 *) malloc(colormapSize * sizeof(uint16));
        MCHECK(redMap); MCHECK(greenMap); MCHECK(blueMap);
	for (i = 0; i < colormapSize; i++)
	    dRed[i] = dGreen[i] = dBlue[i] = (double) SCALE(i, colormapSize-1);
        break;
    default:
        fprintf(stderr,
            "xtiff: can't display photometric interpretation type %d\n",
            tfPhotometricInterpretation);
        exit(0);
    }
}

void
SetNameLabel()
{
    char buffer[BUFSIZ];
    Arg args[1];

    if (tfMultiPage)
        sprintf(buffer, "%s - page %d", fileName, tfDirectory);
    else
        strcpy(buffer, fileName);
    XtSetArg(args[0], XtNlabel, buffer);
    XtSetValues(labelWidget, args, 1);
}

/*
 * Many programs get TIFF colormaps wrong.  They use 8-bit colormaps instead of
 * 16-bit colormaps.  This function is a heuristic to detect and correct this.
 */
void
CheckAndCorrectColormap()
{
    register int i;

    for (i = 0; i < colormapSize; i++)
        if ((redMap[i] > 255) || (greenMap[i] > 255) || (blueMap[i] > 255))
            return;

    for (i = 0; i < colormapSize; i++) {
        redMap[i] = SCALE(redMap[i], 255);
        greenMap[i] = SCALE(greenMap[i], 255);
        blueMap[i] = SCALE(blueMap[i], 255);
    }
    TIFFWarning(fileName, "Assuming 8-bit colormap");
}

void
SimpleGammaCorrection()
{
    register int i;
    register double i_gamma = 1.0 / appData.gamma;

    for (i = 0; i < colormapSize; i++) {
        if (((tfPhotometricInterpretation == PHOTOMETRIC_MINISWHITE)
            && (i == colormapSize - 1))
            || ((tfPhotometricInterpretation == PHOTOMETRIC_MINISBLACK)
            && (i == 0)))
            redMap[i] = greenMap[i] = blueMap[i] = 0;
        else {
            redMap[i] = ROUND((pow(dRed[i] / 65535.0, i_gamma) * 65535.0));
            greenMap[i] = ROUND((pow(dGreen[i] / 65535.0, i_gamma) * 65535.0));
            blueMap[i] = ROUND((pow(dBlue[i] / 65535.0, i_gamma) * 65535.0));
        }
    }

    free(dRed); free(dGreen); free(dBlue);
}

static char* classNames[] = {
    "StaticGray",
    "GrayScale",
    "StaticColor",
    "PseudoColor",
    "TrueColor",
    "DirectColor"
};

/*
 * Current limitation: the visual is set initially by the first file.
 * It cannot be changed.
 */
void
GetVisual()
{
    XColor *colors = NULL;
    unsigned long *pixels = NULL;
    unsigned long i;

    switch (tfImageDepth) {
    /*
     * X really wants a 32-bit image with the fourth channel unused,
     * but the visual structure thinks it's 24-bit.  bitmap_unit is 32.
     */
    case 32:
    case 24:
        if (SearchVisualList(24, DirectColor, &xVisual) == False) {
            fprintf(stderr, "xtiff: 24-bit DirectColor visual not available\n");
            exit(0);
        }

        colors = (XColor *) malloc(3 * colormapSize * sizeof(XColor));
        MCHECK(colors);

        for (i = 0; i < colormapSize; i++) {
            colors[i].pixel = (i << 16) + (i << 8) + i;
            colors[i].red = redMap[i];
            colors[i].green = greenMap[i];
            colors[i].blue = blueMap[i];
            colors[i].flags = DoRed | DoGreen | DoBlue;
        }

        xColormap = XCreateColormap(xDisplay, RootWindow(xDisplay, xScreen),
            xVisual, AllocAll);
        XStoreColors(xDisplay, xColormap, colors, colormapSize);
        break;
    case 8:
    case 4:
    case 2:
        /*
         * We assume that systems with 24-bit visuals also have 8-bit visuals.
         * We don't promote from 8-bit PseudoColor to 24/32 bit DirectColor.
         */
        switch (tfPhotometricInterpretation) {
        case PHOTOMETRIC_MINISWHITE:
        case PHOTOMETRIC_MINISBLACK:
            if (SearchVisualList((int) tfImageDepth, GrayScale, &xVisual) == True)
                break;
        case PHOTOMETRIC_PALETTE:
            if (SearchVisualList((int) tfImageDepth, PseudoColor, &xVisual) == True)
                break;
        default:
            fprintf(stderr, "xtiff: Unsupported TIFF/X configuration\n");
            exit(0);
        }

        colors = (XColor *) malloc(colormapSize * sizeof(XColor));
        MCHECK(colors);

        for (i = 0; i < colormapSize; i++) {
            colors[i].pixel = i;
            colors[i].red = redMap[i];
            colors[i].green = greenMap[i];
            colors[i].blue = blueMap[i];
            colors[i].flags = DoRed | DoGreen | DoBlue;
        }

        /*
         * xtiff's colormap allocation is private.  It does not attempt
         * to detect whether any existing colormap entries are suitable
         * for its use.  This will cause colormap flashing.  Furthermore,
         * background and foreground are taken from the environment.
         * For example, the foreground color may be red when the visual
         * is GrayScale.  If the colormap is completely populated,
         * Xt will not be able to allocate fg and bg.
         */
        if (tfImageDepth == 8)
            xColormap = XCreateColormap(xDisplay, RootWindow(xDisplay, xScreen),
                xVisual, AllocAll);
        else {
            xColormap = XCreateColormap(xDisplay, RootWindow(xDisplay, xScreen),
                xVisual, AllocNone);
            pixels = (unsigned long *)
                malloc(colormapSize * sizeof(unsigned long));
            MCHECK(pixels);
            (void) XAllocColorCells(xDisplay, xColormap, True,
                NULL, 0, pixels, colormapSize);
            basePixel = (unsigned char) pixels[0];
            free(pixels);
        }
        XStoreColors(xDisplay, xColormap, colors, colormapSize);
        break;
    case 1:
        xImageDepth = 1;
        xVisual = DefaultVisual(xDisplay, xScreen);
        xColormap = DefaultColormap(xDisplay, xScreen);
        break;
    default:
        fprintf(stderr, "xtiff: unsupported image depth %d\n", tfImageDepth);
        exit(0);
    }

    if (appData.verbose == True)
	fprintf(stderr, "%s: Using %d-bit %s visual.\n",
	    fileName, xImageDepth, classNames[xVisual->class]);

    if (colors != NULL)
        free(colors);
    if (grayMap != NULL)
        free(grayMap);
    if (redMap != NULL)
        free(redMap);
    if (greenMap != NULL)
        free(greenMap);
    if (blueMap != NULL)
        free(blueMap);

    colors = NULL; grayMap = redMap = greenMap = blueMap = NULL;
}

/*
 * Search for an appropriate visual.  Promote where necessary.
 * Check to make sure that ENOUGH colormap entries are writeable.
 * basePixel was determined when XAllocColorCells() contiguously
 * allocated enough entries.  basePixel is used below in GetTIFFImage.
 */
Boolean
SearchVisualList(image_depth, visual_class, visual)
    int image_depth, visual_class;
    Visual **visual;
{
    XVisualInfo template_visual, *visual_list, *vl;
    int i, n_visuals;

    template_visual.screen = xScreen;
    vl = visual_list = XGetVisualInfo(xDisplay, VisualScreenMask,
        &template_visual, &n_visuals);

    if (n_visuals == 0) {
        fprintf(stderr, "xtiff: visual list not available\n");
        exit(0);
    }

    for (i = 0; i < n_visuals; vl++, i++) {
        if ((vl->class == visual_class) && (vl->depth >= image_depth)
            && (vl->visual->map_entries >= (1 << vl->depth))) {
            *visual = vl->visual;
            xImageDepth = vl->depth;
            xRedMask = vl->red_mask;
            xGreenMask = vl->green_mask;
            xBlueMask = vl->blue_mask;
            XFree((char *) visual_list);
            return True;
        }
    }

    XFree((char *) visual_list);
    return False;
}

void
GetTIFFImage()
{
    int pixel_map[3], red_shift, green_shift, blue_shift;
    char *scan_line, *output_p, *input_p;
    uint32 i, j;
    uint16 s;

    scan_line = (char *) malloc(tfBytesPerRow = TIFFScanlineSize(tfFile));
    MCHECK(scan_line);

    if ((tfImageDepth == 32) || (tfImageDepth == 24)) {
        output_p = imageMemory = (char *)
            malloc(tfImageWidth * tfImageHeight * 4);
        MCHECK(imageMemory);

        /*
         * Handle different color masks for different frame buffers.
         */
        if (ImageByteOrder(xDisplay) == LSBFirst) { /* DECstation 5000 */
            red_shift = pixel_map[0] = xRedMask == 0xFF000000 ? 3
                : (xRedMask == 0xFF0000 ? 2 : (xRedMask == 0xFF00 ? 1 : 0));
            green_shift = pixel_map[1] = xGreenMask == 0xFF000000 ? 3
                : (xGreenMask == 0xFF0000 ? 2 : (xGreenMask == 0xFF00 ? 1 : 0));
            blue_shift = pixel_map[2] = xBlueMask == 0xFF000000 ? 3
                : (xBlueMask == 0xFF0000 ? 2 : (xBlueMask == 0xFF00 ? 1 : 0));
        } else { /* Ardent */
            red_shift = pixel_map[0] = xRedMask == 0xFF000000 ? 0
                : (xRedMask == 0xFF0000 ? 1 : (xRedMask == 0xFF00 ? 2 : 3));
            green_shift = pixel_map[0] = xGreenMask == 0xFF000000 ? 0
                : (xGreenMask == 0xFF0000 ? 1 : (xGreenMask == 0xFF00 ? 2 : 3));
            blue_shift = pixel_map[0] = xBlueMask == 0xFF000000 ? 0
                : (xBlueMask == 0xFF0000 ? 1 : (xBlueMask == 0xFF00 ? 2 : 3));
        }

        if (tfPlanarConfiguration == PLANARCONFIG_CONTIG) {
            for (i = 0; i < tfImageHeight; i++) {
                if (TIFFReadScanline(tfFile, scan_line, i, 0) < 0)
                    break;
                for (input_p = scan_line, j = 0; j < tfImageWidth; j++) {
                    *(output_p + red_shift) = *input_p++;
                    *(output_p + green_shift) = *input_p++;
                    *(output_p + blue_shift) = *input_p++;
                    output_p += 4;
                    if (tfSamplesPerPixel == 4) /* skip the fourth channel */
                        input_p++;
                }
            }
        } else {
            for (s = 0; s < tfSamplesPerPixel; s++) {
                if (s == 3)             /* skip the fourth channel */
                    continue;
                for (i = 0; i < tfImageHeight; i++) {
                    if (TIFFReadScanline(tfFile, scan_line, i, s) < 0)
                        break;
                    input_p = scan_line;
                    output_p = imageMemory + (i*tfImageWidth*4) + pixel_map[s];
                    for (j = 0; j < tfImageWidth; j++, output_p += 4)
                        *output_p = *input_p++;
                }
            }
        }
    } else {
        if (xImageDepth == tfImageDepth) {
            output_p = imageMemory = (char *)
                malloc(tfBytesPerRow * tfImageHeight);
            MCHECK(imageMemory);

            for (i = 0; i < tfImageHeight; i++, output_p += tfBytesPerRow)
                if (TIFFReadScanline(tfFile, output_p, i, 0) < 0)
                    break;
        } else if ((xImageDepth == 8) && (tfImageDepth == 4)) {
            output_p = imageMemory = (char *)
                malloc(tfBytesPerRow * 2 * tfImageHeight + 2);
            MCHECK(imageMemory);

            /*
             * If a scanline is of odd size the inner loop below will overshoot.
             * This is handled very simply by recalculating the start point at
             * each scanline and padding imageMemory a little at the end.
             */
            for (i = 0; i < tfImageHeight; i++) {
                if (TIFFReadScanline(tfFile, scan_line, i, 0) < 0)
                    break;
                output_p = &imageMemory[i * tfImageWidth];
                input_p = scan_line;
                for (j = 0; j < tfImageWidth; j += 2, input_p++) {
                    *output_p++ = (*input_p >> 4) + basePixel;
                    *output_p++ = (*input_p & 0xf) + basePixel;
                }
            }
        } else if ((xImageDepth == 8) && (tfImageDepth == 2)) {
            output_p = imageMemory = (char *)
                malloc(tfBytesPerRow * 4 * tfImageHeight + 4);
            MCHECK(imageMemory);

            for (i = 0; i < tfImageHeight; i++) {
                if (TIFFReadScanline(tfFile, scan_line, i, 0) < 0)
                    break;
                output_p = &imageMemory[i * tfImageWidth];
                input_p = scan_line;
                for (j = 0; j < tfImageWidth; j += 4, input_p++) {
                    *output_p++ = (*input_p >> 6) + basePixel;
                    *output_p++ = ((*input_p >> 4) & 3) + basePixel;
                    *output_p++ = ((*input_p >> 2) & 3) + basePixel;
                    *output_p++ = (*input_p & 3) + basePixel;
                }
            }
        } else if ((xImageDepth == 4) && (tfImageDepth == 2)) {
            output_p = imageMemory = (char *)
                malloc(tfBytesPerRow * 2 * tfImageHeight + 2);
            MCHECK(imageMemory);

            for (i = 0; i < tfImageHeight; i++) {
                if (TIFFReadScanline(tfFile, scan_line, i, 0) < 0)
                    break;
                output_p = &imageMemory[i * tfBytesPerRow * 2];
                input_p = scan_line;
                for (j = 0; j < tfImageWidth; j += 4, input_p++) {
                    *output_p++ = (((*input_p>>6) << 4)
                        | ((*input_p >> 4) & 3)) + basePixel;
                    *output_p++ = ((((*input_p>>2) & 3) << 4)
                        | (*input_p & 3)) + basePixel;
                }
            }
        } else {
            fprintf(stderr,
                "xtiff: can't handle %d-bit TIFF file on an %d-bit display\n",
                tfImageDepth, xImageDepth);
            exit(0);
        }
    }

    free(scan_line);
}

void
CreateXImage()
{
    XGCValues gc_values;
    GC bitmap_gc;

    xOffset = yOffset = 0;
    grabX = grabY = -1;

    xImage = XCreateImage(xDisplay, xVisual, xImageDepth,
        xImageDepth == 1 ? XYBitmap : ZPixmap, /* offset */ 0,
        (char *) imageMemory, tfImageWidth, tfImageHeight,
        /* bitmap_pad */ 8, /* bytes_per_line */ 0);

    /*
     * libtiff converts LSB data into MSB but doesn't change the FillOrder tag.
     */
    if (xImageDepth == 1)
        xImage->bitmap_bit_order = MSBFirst;
    if (xImageDepth <= 8)
        xImage->byte_order = MSBFirst;

    /*
     * create an appropriate GC
     */
    gc_values.function = GXcopy;
    gc_values.plane_mask = AllPlanes;
    if (tfPhotometricInterpretation == PHOTOMETRIC_MINISBLACK) {
        gc_values.foreground = XWhitePixel(xDisplay, xScreen);
        gc_values.background = XBlackPixel(xDisplay, xScreen);
    } else {
        gc_values.foreground = XBlackPixel(xDisplay, xScreen);
        gc_values.background = XWhitePixel(xDisplay, xScreen);
    }
    xWinGc = XCreateGC(xDisplay, XtWindow(shellWidget),
        GCFunction | GCPlaneMask | GCForeground | GCBackground, &gc_values);

    /*
     * create the pixmap and load the image
     */
    if (appData.usePixmap == True) {
        xImagePixmap = XCreatePixmap(xDisplay, RootWindow(xDisplay, xScreen),
            xImage->width, xImage->height, xImageDepth);

        /*
         * According to the O'Reilly X Protocol Reference Manual, page 53,
         * "A pixmap depth of one is always supported and listed, but windows
         * of depth one might not be supported."  Therefore we create a pixmap
         * of depth one and use XCopyPlane().  This is idiomatic.
         */
        if (xImageDepth == 1) {         /* just pass the bits through */
            gc_values.foreground = 1;   /* foreground describes set bits */
            gc_values.background = 0;   /* background describes clear bits */
            bitmap_gc = XCreateGC(xDisplay, xImagePixmap,
                GCForeground | GCBackground, &gc_values);
            XPutImage(xDisplay, xImagePixmap, bitmap_gc, xImage,
                0, 0, 0, 0, xImage->width, xImage->height);
        } else
            XPutImage(xDisplay, xImagePixmap, xWinGc, xImage,
                0, 0, 0, 0, xImage->width, xImage->height);
        XDestroyImage(xImage);
        free(imageMemory);
    }
}

XtCallbackProc
SelectProc(w, unused_1, unused_2)
    Widget w;
    caddr_t unused_1;
    caddr_t unused_2;
{
    XawListReturnStruct *list_return;

    list_return = XawListShowCurrent(w);

    switch (list_return->list_index) {
    case ButtonQuit:
        QuitProc();
        break;
    case ButtonPreviousPage:
        PreviousProc();
        break;
    case ButtonNextPage:
        NextProc();
        break;
    default:
        fprintf(stderr, "error in SelectProc\n");
        exit(0);
    }
    XawListUnhighlight(w);
}

void
QuitProc(void)
{
    exit(0);
}

void
NextProc()
{
    PageProc(ButtonNextPage);
}

void
PreviousProc()
{
    PageProc(ButtonPreviousPage);
}

void
PageProc(direction)
    int direction;
{
    XEvent fake_event;
    Arg args[4];

    switch (direction) {
    case ButtonPreviousPage:
        if (tfDirectory > 0)
            TIFFSetDirectory(tfFile, --tfDirectory);
        else
            return;
        break;
    case ButtonNextPage:
        if (TIFFReadDirectory(tfFile) == True)
            tfDirectory++;
        else
            return;
        break;
    default:
        fprintf(stderr, "error in PageProc\n");
        exit(0);
    }

    xOffset = yOffset = 0;
    grabX = grabY = -1;

    GetTIFFHeader();
    SetNameLabel();
    GetTIFFImage();

    if (appData.usePixmap == True)
        XFreePixmap(xDisplay, xImagePixmap);
    else
        XDestroyImage(xImage);

    CreateXImage();

    /*
     * Using XtSetValues() to set the widget size causes a resize.
     * This resize gets propagated up to the parent shell.
     * In order to disable this visually disconcerting effect,
     * shell resizing is temporarily disabled.
     */
    XtSetArg(args[0], XtNallowShellResize, False);
    XtSetValues(shellWidget, args, 1);

    XtSetArg(args[0], XtNwidth, tfImageWidth);
    XtSetArg(args[1], XtNheight, tfImageHeight);
    XtSetValues(imageWidget, args, 2);

    XtSetArg(args[0], XtNallowShellResize, True);
    XtSetValues(shellWidget, args, 1);

    XClearWindow(xDisplay, XtWindow(imageWidget));

    fake_event.type = Expose;
    fake_event.xexpose.x = fake_event.xexpose.y = 0;
    fake_event.xexpose.width = tfImageWidth;    /* the window will clip */
    fake_event.xexpose.height = tfImageHeight;
    EventProc(imageWidget, NULL, &fake_event);
}

void
EventProc(widget, unused, event)
    Widget widget;
    caddr_t unused;
    XEvent *event;
{
    int ih, iw, ww, wh, sx, sy, w, h, dx, dy;
    Dimension w_width, w_height;
    XEvent next_event;
    Arg args[2];

    if (event->type == MappingNotify) {
        XRefreshKeyboardMapping((XMappingEvent *) event);
        return;
    }

    if (!XtIsRealized(widget))
        return;

    if ((event->type == ButtonPress) || (event->type == ButtonRelease))
        if (event->xbutton.button != Button1)
            return;

    iw = tfImageWidth;  /* avoid sign problems */
    ih = tfImageHeight;

    /*
     * The grabX and grabY variables record where the user grabbed the image.
     * They also record whether the mouse button is down or not.
     */
    if (event->type == ButtonPress) {
        grabX = event->xbutton.x;
        grabY = event->xbutton.y;
        return;
    }

    /*
     * imageWidget is a Core widget and doesn't get resized.
     * So we calculate the size of its viewport here.
     */
    XtSetArg(args[0], XtNwidth, &w_width);
    XtSetArg(args[1], XtNheight, &w_height);
    XtGetValues(shellWidget, args, 2);
    ww = w_width;
    wh = w_height;
    XtGetValues(listWidget, args, 2);
    wh -= w_height;

    switch (event->type) {
    case Expose:
        dx = event->xexpose.x;
        dy = event->xexpose.y;
        sx = dx + xOffset;
        sy = dy + yOffset;
        w = MIN(event->xexpose.width, iw);
        h = MIN(event->xexpose.height, ih);
        break;
    case KeyPress:
        if ((grabX >= 0) || (grabY >= 0)) /* Mouse button is still down */
            return;
        switch (XLookupKeysym((XKeyEvent *) event, /* KeySyms index */ 0)) {
        case XK_Up:
            if (ih < wh)    /* Don't scroll if the window fits the image. */
                return;
            sy = yOffset + appData.translate;
            sy = MIN(ih - wh, sy);
            if (sy == yOffset)  /* Filter redundant stationary refreshes. */
                return;
            yOffset = sy;
            sx = xOffset;
            dx = dy = 0;
            w = ww; h = wh;
            break;
        case XK_Down:
            if (ih < wh)
                return;
            sy = yOffset - appData.translate;
            sy = MAX(sy, 0);
            if (sy == yOffset)
                return;
            yOffset = sy;
            sx = xOffset;
            dx = dy = 0;
            w = ww; h = wh;
            break;
        case XK_Left:
            if (iw < ww)
                return;
            sx = xOffset + appData.translate;
            sx = MIN(iw - ww, sx);
            if (sx == xOffset)
                return;
            xOffset = sx;
            sy = yOffset;
            dx = dy = 0;
            w = ww; h = wh;
            break;
        case XK_Right:
            if (iw < ww)
                return;
            sx = xOffset - appData.translate;
            sx = MAX(sx, 0);
            if (sx == xOffset)
                return;
            xOffset = sx;
            sy = yOffset;
            dx = dy = 0;
            w = ww; h = wh;
            break;
        default:
            return;
        }
        break;
    case MotionNotify:
        /*
         * MotionEvent compression.  Ignore multiple motion events.
         * Ignore motion events if the mouse button is up.
         */
        if (XPending(xDisplay)) /* Xlib doesn't flush the output buffer */
            if (XtPeekEvent(&next_event))
                if (next_event.type == MotionNotify)
                    return;
        if ((grabX < 0) || (grabY < 0))
            return;
        sx = xOffset + grabX - (int) event->xmotion.x;
        if (sx >= (iw - ww))    /* clamp x motion but allow y motion */
            sx = iw - ww;
        sx = MAX(sx, 0);
        sy = yOffset + grabY - (int) event->xmotion.y;
        if (sy >= (ih - wh)) /* clamp y motion but allow x motion */
            sy = ih - wh;
        sy = MAX(sy, 0);
        if ((sx == xOffset) && (sy == yOffset))
            return;
        dx = dy = 0;
        w = ww; h = wh;
        break;
    case ButtonRelease:
        xOffset = xOffset + grabX - (int) event->xbutton.x;
        xOffset = MIN(iw - ww, xOffset);
        xOffset = MAX(xOffset, 0);
        yOffset = yOffset + grabY - (int) event->xbutton.y;
        yOffset = MIN(ih - wh, yOffset);
        yOffset = MAX(yOffset, 0);
        grabX = grabY = -1;
    default:
        return;
    }

    if (appData.usePixmap == True) {
        if (xImageDepth == 1)
            XCopyPlane(xDisplay, xImagePixmap, XtWindow(widget),
                xWinGc, sx, sy, w, h, dx, dy, 1);
        else
            XCopyArea(xDisplay, xImagePixmap, XtWindow(widget),
                xWinGc, sx, sy, w, h, dx, dy);
    } else
        XPutImage(xDisplay, XtWindow(widget), xWinGc, xImage,
            sx, sy, dx, dy, w, h);
}

void
ResizeProc()
{
    Dimension w_width, w_height;
    int xo, yo, ww, wh;
    XEvent fake_event;
    Arg args[2];

    if ((xOffset == 0) && (yOffset == 0))
        return;

    XtSetArg(args[0], XtNwidth, &w_width);
    XtSetArg(args[1], XtNheight, &w_height);
    XtGetValues(shellWidget, args, 2);
    ww = w_width;
    wh = w_height;
    XtGetValues(listWidget, args, 2);
    wh -= w_height;

    xo = xOffset; yo = yOffset;

    if ((xOffset + ww) >= tfImageWidth)
        xOffset = MAX((int) tfImageWidth - ww, 0);
    if ((yOffset + wh) >= tfImageHeight)
        yOffset = MAX((int) tfImageHeight - wh, 0);

    /*
     * Send an ExposeEvent if the origin changed.
     * We have to do this because of the use and semantics of bit gravity.
     */
    if ((xo != xOffset) || (yo != yOffset)) {
        fake_event.type = Expose;
        fake_event.xexpose.x = fake_event.xexpose.y = 0;
        fake_event.xexpose.width = tfImageWidth;
        fake_event.xexpose.height = tfImageHeight;
        EventProc(imageWidget, NULL, &fake_event);
    }
}

int
XTiffErrorHandler(display, error_event)
    Display *display;
    XErrorEvent *error_event;
{
    char message[80];

    /*
     * Some X servers limit the size of pixmaps.
     */
    if ((error_event->error_code == BadAlloc)
            && (error_event->request_code == X_CreatePixmap))
        fprintf(stderr, "xtiff: requested pixmap too big for display\n");
    else {
        XGetErrorText(display, error_event->error_code, message, 80);
        fprintf(stderr, "xtiff: error code %s\n", message);
    }

    exit(0);
}

void
Usage()
{
    fprintf(stderr, "Usage xtiff: [options] tiff-file\n");
    fprintf(stderr, "\tstandard Xt options\n");
    fprintf(stderr, "\t[-help]\n");
    fprintf(stderr, "\t[-gamma gamma]\n");
    fprintf(stderr, "\t[-usePixmap (True | False)]\n");
    fprintf(stderr, "\t[-viewportWidth pixels]\n");
    fprintf(stderr, "\t[-viewportHeight pixels]\n");
    fprintf(stderr, "\t[-translate pixels]\n");
    fprintf(stderr, "\t[-verbose (True | False)]\n");
    exit(0);
}

/* vim: set ts=8 sts=8 sw=8 noet: */

/*
 * Local Variables:
 * mode: c
 * c-basic-offset: 8
 * fill-column: 78
 * End:
 */

/*
 * xtif_dir.c
 *
 * Extended TIFF Directory Tag Support.
 *
 *  You may use this file as a template to add your own
 *  extended tags to the library. Only the parts of the code
 *  marked with "XXX" require modification. Three example tags
 *  are shown; you should replace them with your own.
 *
 *  Author: Niles D. Ritter
 */
 
#include "xtiffiop.h"
#include <stdio.h>

/*  Tiff info structure.
 *
 *     Entry format:
 *        { TAGNUMBER, ReadCount, WriteCount, DataType, FIELDNUM, 
 *          OkToChange, PassDirCountOnSet, AsciiName }
 *
 *     For ReadCount, WriteCount, -1 = unknown; used for mult-valued
 *         tags and ASCII.
 */

static const TIFFFieldInfo xtiffFieldInfo[] = {
  
  /* XXX Replace these example tags with your own extended tags */
    { TIFFTAG_EXAMPLE_MULTI,	-1,-1, TIFF_DOUBLE,	FIELD_EXAMPLE_MULTI,
      TRUE,	TRUE,	"MyMultivaluedTag" },
    { TIFFTAG_EXAMPLE_SINGLE,	 1, 1, TIFF_LONG,	FIELD_EXAMPLE_SINGLE,
      TRUE,	FALSE,	"MySingleLongTag" },
    { TIFFTAG_EXAMPLE_ASCII,	-1,-1, TIFF_ASCII,	FIELD_EXAMPLE_ASCII,
      TRUE,	FALSE,	"MyAsciiTag" },
};
#define	N(a)	(sizeof (a) / sizeof (a[0]))


static void
_XTIFFPrintDirectory(TIFF* tif, FILE* fd, long flags)
{
	xtiff *xt = XTIFFDIR(tif);
	XTIFFDirectory *xd = &xt->xtif_dir;
	int i,num;

	/* call the inherited method */
	if (PARENT(xt,printdir))
		(PARENT(xt,printdir))(tif,fd,flags);

	/* XXX Add field printing here. Replace the three example
         *    tags implemented below with your own.
         */

	fprintf(fd,"--My Example Tags--\n");

	/* Our first example tag may have a lot of values, so we
         * will only print them out if the TIFFPRINT_MYMULTIDOUBLES
         * flag is passed into the print method.
         */
	if (TIFFFieldSet(tif,FIELD_EXAMPLE_MULTI))
	{
		fprintf(fd, "  My Multi-Valued Doubles:");
		if (flags & TIFFPRINT_MYMULTIDOUBLES) 
		{
			double *value = xd->xd_example_multi;
	
			num = xd->xd_num_multi;
			fprintf(fd,"(");
			for (i=0;i<num;i++) fprintf(fd, " %lg", *value++);
			fprintf(fd,")\n");
		} else
			fprintf(fd, "(present)\n");
	}

	if (TIFFFieldSet(tif,FIELD_EXAMPLE_SINGLE))
	{
		fprintf(fd, "  My Single Long Tag:  %lu\n", xd->xd_example_single);
	}

	if (TIFFFieldSet(tif,FIELD_EXAMPLE_ASCII))
	{
		_TIFFprintAsciiTag(fd,"My ASCII Tag",
			 xd->xd_example_ascii);
	}
}

static int
_XTIFFVSetField(TIFF* tif, ttag_t tag, va_list ap)
{
	xtiff *xt = XTIFFDIR(tif);
	XTIFFDirectory* xd = &xt->xtif_dir;
	int status = 1;
	uint32 v32=0;
	int i=0, v=0;
	va_list ap1 = ap;

	/* va_start is called by the calling routine */
	
	switch (tag) {
		/* 
                 * XXX put your extended tags here; replace the implemented
                 *     example tags with your own. 
                 */
	case TIFFTAG_EXAMPLE_MULTI:
		/* multi-valued tags need to store the count as well */
		xd->xd_num_multi = (uint16) va_arg(ap, int);
		_TIFFsetDoubleArray(&xd->xd_example_multi, va_arg(ap, double*),
			(long) xd->xd_num_multi);
		break;
	case TIFFTAG_EXAMPLE_SINGLE:
		xd->xd_example_single = va_arg(ap, uint32);
		break;
	case TIFFTAG_EXAMPLE_ASCII:
		_TIFFsetString(&xd->xd_example_ascii, va_arg(ap, char*));
		break;
	default:
		/* call the inherited method */
		return (PARENT(xt,vsetfield))(tif,tag,ap);
		break;
	}
	if (status) {
		/* we have to override the print method here,
 		 * after the compression tags have gotten to it.
		 * This makes sense because the only time we would
		 * need the extended print method is if an extended
		 * tag is set by the reader.
		 */
		if (!(xt->xtif_flags & XTIFFP_PRINT))
		{
	        	PARENT(xt,printdir) =  TIFFMEMBER(tif,printdir);
      		  	TIFFMEMBER(tif,printdir) = _XTIFFPrintDirectory;
			xt->xtif_flags |= XTIFFP_PRINT;
		}
		TIFFSetFieldBit(tif, _TIFFFieldWithTag(tif, tag)->field_bit);
		tif->tif_flags |= TIFF_DIRTYDIRECT;
	}
	va_end(ap);
	return (status);
badvalue:
	TIFFErrorExt(tif->tif_clientdata, tif->tif_name, "%d: Bad value for \"%s\"", v,
	    _TIFFFieldWithTag(tif, tag)->field_name);
	va_end(ap);
	return (0);
badvalue32:
	TIFFErrorExt(tif->tif_clientdata, tif->tif_name, "%ld: Bad value for \"%s\"", v32,
	    _TIFFFieldWithTag(tif, tag)->field_name);
	va_end(ap);
	return (0);
}


static int
_XTIFFVGetField(TIFF* tif, ttag_t tag, va_list ap)
{
	xtiff *xt = XTIFFDIR(tif);
	XTIFFDirectory* xd = &xt->xtif_dir;

	switch (tag) {
		/* 
                 * XXX put your extended tags here; replace the implemented
                 *     example tags with your own.
                 */
	case TIFFTAG_EXAMPLE_MULTI:
		*va_arg(ap, uint16*) = xd->xd_num_multi;
		*va_arg(ap, double**) = xd->xd_example_multi;
		break;
	case TIFFTAG_EXAMPLE_ASCII:
		*va_arg(ap, char**) = xd->xd_example_ascii;
		break;
	case TIFFTAG_EXAMPLE_SINGLE:
		*va_arg(ap, uint32*) = xd->xd_example_single;
		break;
	default:
		/* return inherited method */
		return (PARENT(xt,vgetfield))(tif,tag,ap);
		break;
	}
	return (1);
}

#define	CleanupField(member) {		\
    if (xd->member) {			\
	_TIFFfree(xd->member);		\
	xd->member = 0;			\
    }					\
}
/*
 * Release storage associated with a directory.
 */
static void
_XTIFFFreeDirectory(xtiff* xt)
{
	XTIFFDirectory* xd = &xt->xtif_dir;

	/* 
	 *  XXX - Purge all Your allocated memory except
	 *  for the xtiff directory itself. This includes
	 *  all fields that require a _TIFFsetXXX call in
	 *  _XTIFFVSetField().
	 */
	
	CleanupField(xd_example_multi);
	CleanupField(xd_example_ascii);
	
}
#undef CleanupField

static void _XTIFFLocalDefaultDirectory(TIFF *tif)
{
	xtiff *xt = XTIFFDIR(tif);
	XTIFFDirectory* xd = &xt->xtif_dir;

	/* Install the extended Tag field info */
	_TIFFMergeFieldInfo(tif, xtiffFieldInfo, N(xtiffFieldInfo));

	/*
	 *  free up any dynamically allocated arrays
	 *  before the new directory is read in.
	 */
	 
	_XTIFFFreeDirectory(xt);	
	_TIFFmemset(xt,0,sizeof(xtiff));

	/* Override the tag access methods */

	PARENT(xt,vsetfield) =  TIFFMEMBER(tif,vsetfield);
	TIFFMEMBER(tif,vsetfield) = _XTIFFVSetField;
	PARENT(xt,vgetfield) =  TIFFMEMBER(tif,vgetfield);
	TIFFMEMBER(tif,vgetfield) = _XTIFFVGetField;

	/* 
	 * XXX Set up any default values here.
	 */
	
	xd->xd_example_single = 234;
}



/**********************************************************************
 *    Nothing below this line should need to be changed.
 **********************************************************************/

static TIFFExtendProc _ParentExtender;

/*
 *  This is the callback procedure, and is
 *  called by the DefaultDirectory method
 *  every time a new TIFF directory is opened.
 */

static void
_XTIFFDefaultDirectory(TIFF *tif)
{
	xtiff *xt;
	
	/* Allocate Directory Structure if first time, and install it */
	if (!(tif->tif_flags & XTIFF_INITIALIZED))
	{
		xt = _TIFFmalloc(sizeof(xtiff));
		if (!xt)
		{
			/* handle memory allocation failure here ! */
			return;
		}
		_TIFFmemset(xt,0,sizeof(xtiff));
		/*
		 * Install into TIFF structure.
		 */
		TIFFMEMBER(tif,clientdir) = (tidata_t)xt;
		tif->tif_flags |= XTIFF_INITIALIZED; /* dont do this again! */
	}
	
	/* set up our own defaults */
	_XTIFFLocalDefaultDirectory(tif);

	/* Since an XTIFF client module may have overridden
	 * the default directory method, we call it now to
	 * allow it to set up the rest of its own methods.
         */

	if (_ParentExtender) 
		(*_ParentExtender)(tif);

}

/*
 *  XTIFF Initializer -- sets up the callback
 *   procedure for the TIFF module.
 */

static
void _XTIFFInitialize(void)
{
	static first_time=1;
	
	if (! first_time) return; /* Been there. Done that. */
	first_time = 0;
	
	/* Grab the inherited method and install */
	_ParentExtender = TIFFSetTagExtender(_XTIFFDefaultDirectory);
}


/*
 * Public File I/O Routines.
 */
TIFF*
XTIFFOpen(const char* name, const char* mode)
{
	/* Set up the callback */
	_XTIFFInitialize();	
	
	/* Open the file; the callback will set everything up
	 */
	return TIFFOpen(name, mode);
}

TIFF*
XTIFFFdOpen(int fd, const char* name, const char* mode)
{
	/* Set up the callback */
	_XTIFFInitialize();	

	/* Open the file; the callback will set everything up
	 */
	return TIFFFdOpen(fd, name, mode);
}


void
XTIFFClose(TIFF *tif)
{
	xtiff *xt = XTIFFDIR(tif);
	
	/* call inherited function first */
	TIFFClose(tif);
	
	/* Free up extended allocated memory */
	_XTIFFFreeDirectory(xt);
	_TIFFfree(xt);
}
/*
 * Local Variables:
 * mode: c
 * c-basic-offset: 8
 * fill-column: 78
 * End:
 */

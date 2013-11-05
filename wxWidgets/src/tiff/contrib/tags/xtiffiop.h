/*
 * Private Extended TIFF library interface.
 *
 *  uses private LIBTIFF interface.
 *
 *  The portions of this module marked "XXX" should be
 *  modified to support your tags instead.
 *
 *  written by: Niles D. Ritter
 *
 */

#ifndef __xtiffiop_h
#define __xtiffiop_h

#include "tiffiop.h"
#include "xtiffio.h"

/**********************************************************************
 *               User Configuration
 **********************************************************************/

/* XXX - Define number of your extended tags here */
#define NUM_XFIELD 3
#define XFIELD_BASE (FIELD_LAST-NUM_XFIELD)

/*  XXX - Define your Tag Fields here  */
#define	FIELD_EXAMPLE_MULTI     (XFIELD_BASE+0)
#define	FIELD_EXAMPLE_SINGLE    (XFIELD_BASE+1)
#define	FIELD_EXAMPLE_ASCII      (XFIELD_BASE+2)


/* XXX - Define Private directory tag structure here */
struct XTIFFDirectory {
	uint16	 xd_num_multi; /* dir-count for the multi tag */
	double*  xd_example_multi;
	uint32   xd_example_single; 
	char*    xd_example_ascii;
};
typedef struct XTIFFDirectory XTIFFDirectory;

/**********************************************************************
 *    Nothing below this line should need to be changed by the user.
 **********************************************************************/

struct xtiff {
	TIFF 		*xtif_tif;	/* parent TIFF pointer */
	uint32		xtif_flags;
#define       XTIFFP_PRINT   0x00000001
	XTIFFDirectory	xtif_dir;	/* internal rep of current directory */
	TIFFVSetMethod	xtif_vsetfield;	/* inherited tag set routine */
	TIFFVGetMethod	xtif_vgetfield;	/* inherited tag get routine */
	TIFFPrintMethod	xtif_printdir;  /* inherited dir print method */
};
typedef struct xtiff xtiff;


#define PARENT(xt,pmember) ((xt)->xtif_ ## pmember) 
#define TIFFMEMBER(tf,pmember) ((tf)->tif_ ## pmember) 
#define XTIFFDIR(tif) ((xtiff *)TIFFMEMBER(tif,clientdir))
	
/* Extended TIFF flags */
#define XTIFF_INITIALIZED 0x80000000
	
#endif /* __xtiffiop_h */
/*
 * Local Variables:
 * mode: c
 * c-basic-offset: 8
 * fill-column: 78
 * End:
 */

//
// BIN2CPP - Some hack-up Job of some mess I found online.
//
// Original was uncredited public domain.  This is uncredited public domain.
// Who am I?  You'll have to guess.  Walrus, Eggman, or the taxman?  Maybe.
//
// (Officially: Provided to the PCSX2 Dev Team by a combined effort on the part of
// the PCSX2 Dev Team and the glorious expanse of the information superhighway).
//
// ------------------------------------------------------------------------
//
// Changes from uncredited online version:
//  * Lots of code cleanups.
//  * Upgraded from K&R syntax (!) to CPP syntax.
//  * added wxWidgets class-based interface for instantiating images in neat fashion.
//    The class and interface used to read images from the host app can be found in
//    wxEmbeddedImage.cpp.
//
// Actually I changed every line of code pretty much, except one that I felt really
// embodied the personality and spirit of this utility.  It's the line that makes it
// print the following message upon command line misuse:
//
//    Bad arguments !!! You must give me all the parameters !!!!
//
// ... I love it.  I really do.
//
// Warning: This program is full of stack overflow holes and other mess.  Maybe we'll
// rewrite it in C# someday and fix all that stuff, but for now it serves its purpose
// and accomplishes its menial tasks with sufficent effectiveness.
//

#include <cstdio>
#include <cstring>
#include <ctype.h>

#include <sys/stat.h>

#if defined (__linux__) && !defined(__LINUX__)  // some distributions are lower case
#define __LINUX__
#endif

#ifdef __LINUX__
#define _stat stat
#define _fcloseall fcloseall
#include <string.h>
#endif

typedef unsigned char u8;
typedef char s8;

using namespace std;

static const unsigned int BUF_LEN = 1;
static const unsigned int LINE = 16;

/* Tell u the file size in bytes */

long getfilesize( const char* filename )
{
	struct _stat result;
	
	_stat( filename, &result );
	return result.st_size;
}

enum
{
	ARG_SRCFILE = 1,
	ARG_DESTFILE,
	ARG_CLASSNAME,
};

int main(int argc, char* argv[])
{
	FILE *source,*dest;
	u8 buffer[BUF_LEN];
	s8 Dummy[260];
	s8 srcfile[260];
	s8 classname[260];
	int c;

	if ( (argc <= ARG_SRCFILE) )
	{

		if ( ( argc == 2 ) && ( strcmp(argv[1],"/?")==0 ) )
		{
			puts(
				" - <<< BIN2CPP V1.1 For Win32 >>> by the PCSX2 Team - \n\n"
				"USAGE: Bin2CPP  <SOURCE image> [TARGET file] [CLASS]\n"
				"  <SOURCE> = without extension!\n"
				"  [TARGET] = without extension '.h' it will be added by program.\n"
				"             (defaults to <SOURCE> if unspecified)\n"
				"  [CLASS]  = name of the C++ class in the destination file name.\n"
				"             (defaults to res_<TARGET> if unspecified)\n"
			);
			return 0L;
		}
		else
		{
			puts( "Bad arguments !!! You must give me all the parameters !!!!\n"
				  "Type 'BIN2CPP /?' to read the help.\n"
			);
			return 0L;
		}

	}

	// ----------------------------------------------------------------------------
	//     Determine Source Name, and Open Source for Reading
	// ----------------------------------------------------------------------------

	strcpy(srcfile,argv[ARG_SRCFILE]);

	int srcfn_len = strlen( srcfile );
	if( srcfile[srcfn_len-4] != '.' )
	{
		printf( "ERROR : Malformed source filename.  I'm a crap utility and I demand 3-letter extensions only!\n" );
		return 18;
	}

	if( (source=fopen( srcfile, "rb" )) == NULL )
	{
		printf( "ERROR : I can't find source file   %s\n", srcfile );
		return 20;
	}
	
	const int filesize( getfilesize( srcfile ) );

	char wxImgTypeUpper[24];
	char wxImgTypeLower[24];
	strcpy( wxImgTypeUpper, &srcfile[srcfn_len-3] );
	//strcpy( wxImgTypeLower, argv[ARG_IMGEXT] );
#ifdef __LINUX__
	for (int i = 0; i < 24; i++)
		wxImgTypeUpper[i] = toupper(wxImgTypeUpper[i]);
#else
	strupr( wxImgTypeUpper );
#endif
	//strupr( wxImgTypeLower );

	if( strcmp( wxImgTypeUpper, "JPG" ) == 0 )
		strcpy( wxImgTypeUpper, "JPEG" );		// because wxWidgets defines it as JPEG >_<

	argv[ARG_SRCFILE][srcfn_len-4] = 0;

	// ----------------------------------------------------------------------------
	//     Determine Target Name, and Open Target File for Writing
	// ----------------------------------------------------------------------------
	
	strcpy( Dummy, argv[(argc <= ARG_DESTFILE) ? ARG_SRCFILE : ARG_DESTFILE] );

	strcat( Dummy,".h" );

	if( (dest=fopen( Dummy, "wb+" )) == NULL )
	{
		printf( "ERROR : I can't open destination file   %s\n", Dummy );
		(void)_fcloseall();
		return 0L;
	}
	
	// ----------------------------------------------------------------------------
	
	printf( "Bin2CPP Output > %s\n", Dummy );
	
	if( argc <= ARG_CLASSNAME )
	{
		strcpy( classname, "res_" );
		strcat( classname, argv[(argc <= ARG_DESTFILE) ? ARG_SRCFILE : ARG_DESTFILE] );
	}
	else
		strcpy( classname, argv[ARG_CLASSNAME] );

	/* It writes the header information */

	fprintf( dest,
		"#pragma once\n\n"
		"#include \"Pcsx2Types.h\"\n"
		"#include <wx/gdicmn.h>\n\n"
		"class %s\n{\n"
		"public:\n"
		"\tstatic const uint Length = %d;\n"
		"\tstatic const u8 Data[Length];\n"
		"\tstatic wxBitmapType GetFormat() { return wxBITMAP_TYPE_%s; }\n};\n\n"
		"const u8 %s::Data[Length] =\n{\n",
		classname, filesize, wxImgTypeUpper, classname
	);

	if( ferror( dest ) )
	{
		printf( "ERROR writing on target file:  %s\n", Dummy );
		(void)_fcloseall();
		return 20L;
	}

	/* It writes the binary data information! */
	do
	{
		fprintf(dest,"\t");
		for ( c=0; c <= LINE; ++c )
		{
			if( fread( buffer, 1, 1, source ) == 0 ) break;
			
			if( c != 0 )
				fprintf( dest, "," );
			fprintf( dest,"0x%02x", *buffer );
		}
		if( !feof( source ) )
			fprintf( dest, "," );
		fprintf(dest,"\n");
	}
	while( ! feof( source ) );

	fprintf(dest,"};\n");

	_fcloseall();

	return 0;
}


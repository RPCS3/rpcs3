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


typedef unsigned char u8;
typedef char s8;

int main(int argc, char* argv[])
{
	FILE *source,*dest;
	u8 buffer[BUF_LEN];
	s8 Dummy[260];
	s8 classname[260];
	int c;

	if ( (argc < 3) )
	{

		if ( ( argc == 2 ) && ( strcmp(argv[1],"/?")==0 ) )
		{
			puts(
				" - <<< BIN2CPP V1.0 For Win32 >>> by the Pcsx2 Team - \n\n"
				"USAGE: Bin2CPP  <BINARY file name> <TARGET file name> [CLASS]\n"
				"  <TARGET> = without extension '.h' it will be added by program.\n"
				"  <CLASS>  = name of the C++ class in the destination file name.\n"
				"             (defaults to <TARGET> if unspecified)\n"
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

	if( (source=fopen( argv[1], "rb" )) == NULL )
	{
		printf("ERROR : I can't find source file   %s\n",argv[1]);
		return 20;
	}
	
	const int filesize( getfilesize( argv[1] ) );

	strcpy(Dummy,argv[2]);
	strcat(Dummy,".h");               /* add suffix .h to target name */

	if( (dest=fopen( Dummy, "wb+" )) == NULL )
	{
		printf("ERROR : I can't open destination file   %s\n",Dummy);
		(void)_fcloseall();
		return 0L;
	}
	
	// reuse the target parameter for the classname only if argc==3.
	strcpy( classname, argv[(argc==3) ? 2 : 3] );

	char* wxImageType = "PNG";		// todo - base this on input extension

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
		classname, filesize, wxImageType, classname
	);

	if( ferror( dest ) )
	{
		printf( "ERROR writing on target file:  %s\n",argv[2] );
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


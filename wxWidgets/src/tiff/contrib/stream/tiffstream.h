// tiff stream interface class definition

#ifndef _TIFF_STREAM_H_
#define _TIFF_STREAM_H_

#include <iostream.h>

#include "tiffio.h"

class TiffStream {

public:
    // ctor/dtor
    TiffStream();
	~TiffStream();

public:
    enum SeekDir {
	    beg,
		cur,
		end,
    };

public:
    // factory methods
    TIFF* makeFileStream(iostream* str);
	TIFF* makeFileStream(istream* str);
	TIFF* makeFileStream(ostream* str);

public:
    // tiff client methods
	static tsize_t read(thandle_t fd, tdata_t buf, tsize_t size);
	static tsize_t write(thandle_t fd, tdata_t buf, tsize_t size);
	static toff_t seek(thandle_t fd, toff_t offset, int origin);
	static toff_t size(thandle_t fd);
	static int close(thandle_t fd);
	static int map(thandle_t fd, tdata_t* phase, toff_t* psize);
	static void unmap(thandle_t fd, tdata_t base, tsize_t size);

public:
    // query method
	TIFF* getTiffHandle() const { return m_tif; }
	unsigned int getStreamLength() { return m_streamLength; }

private:
	// internal methods
    unsigned int getSize(thandle_t fd);
	unsigned int tell(thandle_t fd);
	bool seekInt(thandle_t fd, unsigned int offset, int origin);
	bool isOpen(thandle_t fd);

private:
	thandle_t m_this;
	TIFF* m_tif;
	static const char* m_name;
	istream* m_inStream;
	ostream* m_outStream;
	iostream* m_ioStream;
	int m_streamLength;
	
};

#endif // _TIFF_STREAM_H_
/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 8
 * fill-column: 78
 * End:
 */

////////////////////////////////////////////////////////////////////////////////
///
/// Classes for easy reading & writing of WAV sound files. 
///
/// For big-endian CPU, define _BIG_ENDIAN_ during compile-time to correctly
/// parse the WAV files with such processors.
/// 
/// Admittingly, more complete WAV reader routines may exist in public domain,
/// but the reason for 'yet another' one is that those generic WAV reader 
/// libraries are exhaustingly large and cumbersome! Wanted to have something
/// simpler here, i.e. something that's not already larger than rest of the
/// SoundTouch/SoundStretch program...
///
/// Author        : Copyright (c) Olli Parviainen
/// Author e-mail : oparviai 'at' iki.fi
/// SoundTouch WWW: http://www.surina.net/soundtouch
///
////////////////////////////////////////////////////////////////////////////////
//
// Last changed  : $Date: 2006/02/05 16:44:06 $
// File revision : $Revision: 1.15 $
//
// $Id: WavFile.cpp,v 1.15 2006/02/05 16:44:06 Olli Exp $
//
////////////////////////////////////////////////////////////////////////////////
//
// License :
//
//  SoundTouch audio processing library
//  Copyright (c) Olli Parviainen
//
//  This library is free software; you can redistribute it and/or
//  modify it under the terms of the GNU Lesser General Public
//  License as published by the Free Software Foundation; either
//  version 2.1 of the License, or (at your option) any later version.
//
//  This library is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//  Lesser General Public License for more details.
//
//  You should have received a copy of the GNU Lesser General Public
//  License along with this library; if not, write to the Free Software
//  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
////////////////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <stdexcept>
#include <string>
#include <assert.h>
#include <limits.h>

#include <cstdlib>
#include <cstring>

#include "WavFile.h"

using namespace std;

const static char riffStr[] = "RIFF";
const static char waveStr[] = "WAVE";
const static char fmtStr[]  = "fmt ";
const static char dataStr[] = "data";


//////////////////////////////////////////////////////////////////////////////
//
// Helper functions for swapping byte order to correctly read/write WAV files 
// with big-endian CPU's: Define compile-time definition _BIG_ENDIAN_ to
// turn-on the conversion if it appears necessary. 
//
// For example, Intel x86 is little-endian and doesn't require conversion,
// while PowerPC of Mac's and many other RISC cpu's are big-endian.

#ifdef BYTE_ORDER
    // In gcc compiler detect the byte order automatically
    #if BYTE_ORDER == BIG_ENDIAN
        // big-endian platform.
        #define _BIG_ENDIAN_
    #endif
#endif
    
#ifdef _BIG_ENDIAN_
    // big-endian CPU, swap bytes in 16 & 32 bit words

    // helper-function to swap byte-order of 32bit integer
    static inline void _swap32(unsigned int &dwData)
    {
        dwData = ((dwData >> 24) & 0x000000FF) | 
                 ((dwData >> 8)  & 0x0000FF00) | 
                 ((dwData << 8)  & 0x00FF0000) | 
                 ((dwData << 24) & 0xFF000000);
    }   

    // helper-function to swap byte-order of 16bit integer
    static inline void _swap16(unsigned short &wData)
    {
        wData = ((wData >> 8) & 0x00FF) | 
                ((wData << 8) & 0xFF00);
    }

    // helper-function to swap byte-order of buffer of 16bit integers
    static inline void _swap16Buffer(unsigned short *pData, unsigned int dwNumWords)
    {
        unsigned long i;

        for (i = 0; i < dwNumWords; i ++)
        {
            _swap16(pData[i]);
        }
    }

#else   // BIG_ENDIAN
    // little-endian CPU, WAV file is ok as such

    // dummy helper-function
    static inline void _swap32(unsigned int &dwData)
    {
        // do nothing
    }   

    // dummy helper-function
    static inline void _swap16(unsigned short &wData)
    {
        // do nothing
    }

    // dummy helper-function
    static inline void _swap16Buffer(unsigned short *pData, unsigned int dwNumBytes)
    {
        // do nothing
    }

#endif  // BIG_ENDIAN


//////////////////////////////////////////////////////////////////////////////
//
// Class WavInFile
//

WavInFile::WavInFile(const char *fileName)
{
    int hdrsOk;

    // Try to open the file for reading
    fptr = fopen(fileName, "rb");
    if (fptr == NULL) 
    {
        // didn't succeed
        string msg = "Error : Unable to open file \"";
        msg += fileName;
        msg += "\" for reading.";
        throw runtime_error(msg);
    }

    // Read the file headers
    hdrsOk = readWavHeaders();
    if (hdrsOk != 0) 
    {
        // Something didn't match in the wav file headers 
        string msg = "File \"";
        msg += fileName;
        msg += "\" is corrupt or not a WAV file";
        throw runtime_error(msg);
    }

    if (header.format.fixed != 1)
    {
        string msg = "File \"";
        msg += fileName;
        msg += "\" uses unsupported encoding.";
        throw runtime_error(msg);
    }

    dataRead = 0;
}



WavInFile::~WavInFile()
{
    close();
}



void WavInFile::rewind()
{
    int hdrsOk;

    fseek(fptr, 0, SEEK_SET);
    hdrsOk = readWavHeaders();
    assert(hdrsOk == 0);
    dataRead = 0;
}


int WavInFile::checkCharTags()
{
    // header.format.fmt should equal to 'fmt '
    if (memcmp(fmtStr, header.format.fmt, 4) != 0) return -1;
    // header.data.data_field should equal to 'data'
    if (memcmp(dataStr, header.data.data_field, 4) != 0) return -1;

    return 0;
}


int WavInFile::read(char *buffer, int maxElems)
{
    int numBytes;
    uint afterDataRead;

    // ensure it's 8 bit format
    if (header.format.bits_per_sample != 8)
    {
        throw runtime_error("Error: WavInFile::read(char*, int) works only with 8bit samples.");
    }
    assert(sizeof(char) == 1);

    numBytes = maxElems;
    afterDataRead = dataRead + numBytes;
    if (afterDataRead > header.data.data_len) 
    {
        // Don't read more samples than are marked available in header
        numBytes = header.data.data_len - dataRead;
        assert(numBytes >= 0);
    }

    numBytes = fread(buffer, 1, numBytes, fptr);
    dataRead += numBytes;

    return numBytes;
}


int WavInFile::read(short *buffer, int maxElems)
{
    unsigned int afterDataRead;
    int numBytes;
    int numElems;

    if (header.format.bits_per_sample == 8)
    {
        // 8 bit format
        char *temp = new char[maxElems];
        int i;

        numElems = read(temp, maxElems);
        // convert from 8 to 16 bit
        for (i = 0; i < numElems; i ++)
        {
            buffer[i] = temp[i] << 8;
        }
        delete[] temp;
    }
    else
    {
        // 16 bit format
        assert(header.format.bits_per_sample == 16);
        assert(sizeof(short) == 2);

        numBytes = maxElems * 2;
        afterDataRead = dataRead + numBytes;
        if (afterDataRead > header.data.data_len) 
        {
            // Don't read more samples than are marked available in header
            numBytes = header.data.data_len - dataRead;
            assert(numBytes >= 0);
        }

        numBytes = fread(buffer, 1, numBytes, fptr);
        dataRead += numBytes;
        numElems = numBytes / 2;

        // 16bit samples, swap byte order if necessary
        _swap16Buffer((unsigned short *)buffer, numElems);
    }

    return numElems;
}



int WavInFile::read(float *buffer, int maxElems)
{
    short *temp = new short[maxElems];
    int num;
    int i;
    double fscale;

    num = read(temp, maxElems);

    fscale = 1.0 / 32768.0;
    // convert to floats, scale to range [-1..+1[
    for (i = 0; i < num; i ++)
    {
        buffer[i] = (float)(fscale * (double)temp[i]);
    }

    delete[] temp;

    return num;
}


int WavInFile::eof() const
{
    // return true if all data has been read or file eof has reached
    return (dataRead == header.data.data_len || feof(fptr));
}


void WavInFile::close()
{
    fclose(fptr);
    fptr = NULL;
}



// test if character code is between a white space ' ' and little 'z'
static int isAlpha(char c)
{
    return (c >= ' ' && c <= 'z') ? 1 : 0;
}


// test if all characters are between a white space ' ' and little 'z'
static int isAlphaStr(char *str)
{
    int c;

    c = str[0];
    while (c) 
    {
        if (isAlpha(c) == 0) return 0;
        str ++;
        c = str[0];
    }

    return 1;
}


int WavInFile::readRIFFBlock()
{
    fread(&(header.riff), sizeof(WavRiff), 1, fptr);

    // swap 32bit data byte order if necessary
    _swap32((unsigned int &)header.riff.package_len);

    // header.riff.riff_char should equal to 'RIFF');
    if (memcmp(riffStr, header.riff.riff_char, 4) != 0) return -1;
    // header.riff.wave should equal to 'WAVE'
    if (memcmp(waveStr, header.riff.wave, 4) != 0) return -1;

    return 0;
}




int WavInFile::readHeaderBlock()
{
    char label[5];
    string sLabel;

    // lead label string
    fread(label, 1, 4, fptr);
    label[4] = 0;

    if (isAlphaStr(label) == 0) return -1;    // not a valid label

    // Decode blocks according to their label
    if (strcmp(label, fmtStr) == 0)
    {
        int nLen, nDump;

        // 'fmt ' block 
        memcpy(header.format.fmt, fmtStr, 4);

        // read length of the format field
        fread(&nLen, sizeof(int), 1, fptr);
        // swap byte order if necessary
        _swap32((unsigned int &)nLen); // int format_len;
        header.format.format_len = nLen;

        // calculate how much length differs from expected
        nDump = nLen - (sizeof(header.format) - 8);

        // if format_len is larger than expected, read only as much data as we've space for
        if (nDump > 0)
        {
            nLen = sizeof(header.format) - 8;
        }

        // read data
        fread(&(header.format.fixed), nLen, 1, fptr);

        // swap byte order if necessary
        _swap16((unsigned short &)header.format.fixed);            // short int fixed;
        _swap16((unsigned short &)header.format.channel_number);   // short int channel_number;
        _swap32((unsigned int   &)header.format.sample_rate);      // int sample_rate;
        _swap32((unsigned int   &)header.format.byte_rate);        // int byte_rate;
        _swap16((unsigned short &)header.format.byte_per_sample);  // short int byte_per_sample;
        _swap16((unsigned short &)header.format.bits_per_sample);  // short int bits_per_sample;

        // if format_len is larger than expected, skip the extra data
        if (nDump > 0)
        {
            fseek(fptr, nDump, SEEK_CUR);
        }

        return 0;
    }
    else if (strcmp(label, dataStr) == 0)
    {
        // 'data' block
        memcpy(header.data.data_field, dataStr, 4);
        fread(&(header.data.data_len), sizeof(uint), 1, fptr);

        // swap byte order if necessary
        _swap32((unsigned int &)header.data.data_len);

        return 1;
    }
    else
    {
        uint len, i;
        uint temp;
        // unknown block

        // read length
        fread(&len, sizeof(len), 1, fptr);
        // scan through the block
        for (i = 0; i < len; i ++)
        {
            fread(&temp, 1, 1, fptr);
            if (feof(fptr)) return -1;   // unexpected eof
        }
    }
    return 0;
}


int WavInFile::readWavHeaders()
{
    int res;

    memset(&header, 0, sizeof(header));

    res = readRIFFBlock();
    if (res) return 1;
    // read header blocks until data block is found
    do
    {
        // read header blocks
        res = readHeaderBlock();
        if (res < 0) return 1;  // error in file structure
    } while (res == 0);
    // check that all required tags are legal
    return checkCharTags();
}


uint WavInFile::getNumChannels() const
{
    return header.format.channel_number;
}


uint WavInFile::getNumBits() const
{
    return header.format.bits_per_sample;
}


uint WavInFile::getBytesPerSample() const
{
    return getNumChannels() * getNumBits() / 8;
}


uint WavInFile::getSampleRate() const
{
    return header.format.sample_rate;
}



uint WavInFile::getDataSizeInBytes() const
{
    return header.data.data_len;
}


uint WavInFile::getNumSamples() const
{
    return header.data.data_len / header.format.byte_per_sample;
}


uint WavInFile::getLengthMS() const
{
   uint numSamples;
   uint sampleRate;

   numSamples = getNumSamples();
   sampleRate = getSampleRate();

   assert(numSamples < UINT_MAX / 1000);
   return (1000 * numSamples / sampleRate);
}


//////////////////////////////////////////////////////////////////////////////
//
// Class WavOutFile
//

WavOutFile::WavOutFile(const char *fileName, int sampleRate, int bits, int channels)
{
    bytesWritten = 0;
    fptr = fopen(fileName, "wb");
    if (fptr == NULL) 
    {
        string msg = "Error : Unable to open file \"";
        msg += fileName;
        msg += "\" for writing.";
        //pmsg = msg.c_str;
        throw runtime_error(msg);
    }

    fillInHeader(sampleRate, bits, channels);
    writeHeader();
}



WavOutFile::~WavOutFile()
{
    close();
}



void WavOutFile::fillInHeader(uint sampleRate, uint bits, uint channels)
{
    // fill in the 'riff' part..

    // copy string 'RIFF' to riff_char
    memcpy(&(header.riff.riff_char), riffStr, 4);
    // package_len unknown so far
    header.riff.package_len = 0;
    // copy string 'WAVE' to wave
    memcpy(&(header.riff.wave), waveStr, 4);


    // fill in the 'format' part..

    // copy string 'fmt ' to fmt
    memcpy(&(header.format.fmt), fmtStr, 4);

    header.format.format_len = 0x10;
    header.format.fixed = 1;
    header.format.channel_number = (short)channels;
    header.format.sample_rate = sampleRate;
    header.format.bits_per_sample = (short)bits;
    header.format.byte_per_sample = (short)(bits * channels / 8);
    header.format.byte_rate = header.format.byte_per_sample * sampleRate;
    header.format.sample_rate = sampleRate;

    // fill in the 'data' part..

    // copy string 'data' to data_field
    memcpy(&(header.data.data_field), dataStr, 4);
    // data_len unknown so far
    header.data.data_len = 0;
}


void WavOutFile::finishHeader()
{
    // supplement the file length into the header structure
    header.riff.package_len = bytesWritten + 36;
    header.data.data_len = bytesWritten;

    writeHeader();
}



void WavOutFile::writeHeader()
{
    WavHeader hdrTemp;

    // swap byte order if necessary
    hdrTemp = header;
    _swap32((unsigned int   &)hdrTemp.riff.package_len);
    _swap32((unsigned int   &)hdrTemp.format.format_len);
    _swap16((unsigned short &)hdrTemp.format.fixed);
    _swap16((unsigned short &)hdrTemp.format.channel_number);
    _swap32((unsigned int   &)hdrTemp.format.sample_rate);
    _swap32((unsigned int   &)hdrTemp.format.byte_rate);
    _swap16((unsigned short &)hdrTemp.format.byte_per_sample);
    _swap16((unsigned short &)hdrTemp.format.bits_per_sample);
    _swap32((unsigned int   &)hdrTemp.data.data_len);

    // write the supplemented header in the beginning of the file
    fseek(fptr, 0, SEEK_SET);
    fwrite(&hdrTemp, sizeof(hdrTemp), 1, fptr);
    // jump back to the end of the file
    fseek(fptr, 0, SEEK_END);
}



void WavOutFile::close()
{
    finishHeader();
    fclose(fptr);
    fptr = NULL;
}


void WavOutFile::write(const char *buffer, int numElems)
{
    int res;

    if (header.format.bits_per_sample != 8)
    {
        throw runtime_error("Error: WavOutFile::write(const char*, int) accepts only 8bit samples.");
    }
    assert(sizeof(char) == 1);

    res = fwrite(buffer, 1, numElems, fptr);
    if (res != numElems) 
    {
        throw runtime_error("Error while writing to a wav file.");
    }

    bytesWritten += numElems;
}


void WavOutFile::write(const short *buffer, int numElems)
{
    int res;

    // 16 bit samples
    if (numElems < 1) return;   // nothing to do

    if (header.format.bits_per_sample == 8)
    {
        int i;
        char *temp = new char[numElems];
        // convert from 16bit format to 8bit format
        for (i = 0; i < numElems; i ++)
        {
            temp[i] = buffer[i] >> 8;
        }
        // write in 8bit format
        write(temp, numElems);
        delete[] temp;
    }
    else
    {
        // 16bit format
        unsigned short *pTemp = new unsigned short[numElems];

        assert(header.format.bits_per_sample == 16);

        // allocate temp buffer to swap byte order if necessary
        memcpy(pTemp, buffer, numElems * 2);
        _swap16Buffer(pTemp, numElems);

        res = fwrite(pTemp, 2, numElems, fptr);

        delete[] pTemp;

        if (res != numElems) 
        {
            throw runtime_error("Error while writing to a wav file.");
        }
        bytesWritten += 2 * numElems;
    }
}


void WavOutFile::write(const float *buffer, int numElems)
{
    int i;
    short *temp = new short[numElems];
    int iTemp;

    // convert to 16 bit integer
    for (i = 0; i < numElems; i ++)
    {
        // convert to integer
        iTemp = (int)(32768.0f * buffer[i]);

        // saturate
        if (iTemp < -32768) iTemp = -32768;
        if (iTemp > 32767)  iTemp = 32767;
        temp[i] = (short)iTemp;
    }

    write(temp, numElems);

    delete[] temp;
}

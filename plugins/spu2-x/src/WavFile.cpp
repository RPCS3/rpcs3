/* SPU2-X, A plugin for Emulating the Sound Processing Unit of the Playstation 2
 * Developed and maintained by the Pcsx2 Development Team.
 *
 * The file is based on WavFile.h from SoundTouch library. 
 * Original portions are (c) 2009 by Olli Parviainen (oparviai 'at' iki.fi)
 *
 * SPU2-X is free software: you can redistribute it and/or modify it under the terms
 * of the GNU Lesser General Public License as published by the Free Software Found-
 * ation, either version 3 of the License, or (at your option) any later version.
 *
 * SPU2-X is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with SPU2-X.  If not, see <http://www.gnu.org/licenses/>.
 */

// Note the file is mostly a copy paste of the WavFile.h from SoundTouch library. It was 
// shrunken to support only output 16 bits wav files

#include <stdio.h>
#include <stdexcept>
#include <string>
#include <cstring>
#include <assert.h>
#include <limits.h>

#include "WavFile.h"

using namespace std;

static const char riffStr[] = "RIFF";
static const char waveStr[] = "WAVE";
static const char fmtStr[]  = "fmt ";
static const char dataStr[] = "data";

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
    finishHeader();
    if (fptr) fclose(fptr);
    fptr = NULL;
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
    header.format.sample_rate = (int)sampleRate;
    header.format.bits_per_sample = (short)bits;
    header.format.byte_per_sample = (short)(bits * channels / 8);
    header.format.byte_rate = header.format.byte_per_sample * (int)sampleRate;
    header.format.sample_rate = (int)sampleRate;

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
    int res;

    // write the supplemented header in the beginning of the file
    fseek(fptr, 0, SEEK_SET);
    res = fwrite(&header, sizeof(header), 1, fptr);
    if (res != 1)
    {
        throw runtime_error("Error while writing to a wav file.");
    }

    // jump back to the end of the file
    fseek(fptr, 0, SEEK_END);
}


void WavOutFile::write(const short *buffer, int numElems)
{
    int res;

    // 16bit format & 16 bit samples

    assert(header.format.bits_per_sample == 16);
    if (numElems < 1) return;   // nothing to do

    res = fwrite(buffer, 2, numElems, fptr);

    if (res != numElems) 
    {
        throw runtime_error("Error while writing to a wav file.");
    }
    bytesWritten += 2 * numElems;
}

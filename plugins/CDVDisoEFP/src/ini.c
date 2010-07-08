/*  ini.c
 *  Copyright (C) 2002-2005  PCSX2 Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 *
 *  PCSX2 members can be contacted through their website at www.pcsx2.net.
 */


#include <stddef.h> // NULL
#include <stdio.h> // sprintf()

#include "logfile.h"
#include "actualfile.h"
#include "ini.h"


const char INIext[] = ".ini";
const char INInewext[] = ".new";


// Returns: position where new extensions should be added.
int INIRemoveExt(char *argname, char *tempname)
{
	int i;
	int j;
	int k;

#ifdef VERBOSE_FUNCTION_INI
	PrintLog("CDVDiso ini: RemoveExt(%s)", argname);
#endif /* VERBOSE_FUNCTION_INI */

	i = 0;
	while ((i <= INIMAXLEN) && (*(argname + i) != 0))
	{
		*(tempname + i) = *(argname + i);
		i++;
	} // ENDWHILE- Copying the argument name into a temporary area;
	*(tempname + i) = 0; // And 0-terminate
	k = i;
	k--;

	j = 0;
	while ((j <= INIMAXLEN) && (INIext[j] != 0)) j++;
	j--;

	while ((j >= 0) && (*(tempname + k) == INIext[j]))
	{
		k--;
		j--;
	} // ENDWHILE- Comparing the ending characters to the INI ext.
	if (j < 0)
	{
		k++;
		i = k;
		*(tempname + i) = 0; // 0-terminate, cutting off ".ini"
	} // ENDIF- Do we have a match? Then remove the end chars.

	return(i);
} // END INIRemoveExt()


void INIAddInExt(char *tempname, int temppos)
{
	int i;

#ifdef VERBOSE_FUNCTION_INI
	PrintLog("CDVDiso ini: AddInExt(%s, %i)", tempname, temppos);
#endif /* VERBOSE_FUNCTION_INI */

	i = 0;
	while ((i + temppos < INIMAXLEN) && (INIext[i] != 0))
	{
		*(tempname + temppos + i) = INIext[i];
		i++;
	} // ENDWHILE- Attaching extenstion to filename
	*(tempname + temppos + i) = 0; // And 0-terminate
} // END INIAddInExt()


void INIAddOutExt(char *tempname, int temppos)
{
	int i;

#ifdef VERBOSE_FUNCTION_INI
	PrintLog("CDVDiso ini: AddOutExt(%s, %i)", tempname, temppos);
#endif /* VERBOSE_FUNCTION_INI */

	i = 0;
	while ((i + temppos < INIMAXLEN) && (INInewext[i] != 0))
	{
		*(tempname + temppos + i) = INInewext[i];
		i++;
	} // ENDWHILE- Attaching extenstion to filename
	*(tempname + temppos + i) = 0; // And 0-terminate
} // END INIAddInExt()


// Returns number of bytes read to get line (0 means end-of-file)
int INIReadLine(ACTUALHANDLE infile, char *buffer)
{
	int charcount;
	int i;
	char tempin[2];
	int retflag;
	int retval;

#ifdef VERBOSE_FUNCTION_INI
	PrintLog("CDVDiso ini: ReadLine()");
#endif /* VERBOSE_FUNCTION_INI */

	charcount = 0;
	i = 0;
	tempin[1] = 0;
	retflag = 0;

	while ((i < INIMAXLEN) && (retflag < 2))
	{
		retval = ActualFileRead(infile, 1, tempin);
		charcount++;
		if (retval != 1)
		{
			retflag = 2;
			charcount--;

		}
		else if (tempin[0] == '\n')
		{
			retflag = 2;

		}
		else if (tempin[0] >= ' ')
		{
			*(buffer + i) = tempin[0];
			i++;
		} // ENDLONGIF- How do we react to the next character?
	} // ENDWHILE- Loading up on characters until an End-of-Line appears
	*(buffer + i) = 0; // And 0-terminate

#ifdef VERBOSE_FUNCTION_INI
	PrintLog("CDVDiso ini:   Line: %s", buffer);
#endif /* VERBOSE_FUNCTION_INI */

	return(charcount);
} // END INIReadLine()
// Note: Do we need to back-skip a char if something other \n follows \r?


// Returns: number of bytes to get to start of section (or -1)
int INIFindSection(ACTUALHANDLE infile, char *section)
{
	int charcount;
	int i;
	int retflag;
	int retval;
	char scanbuffer[INIMAXLEN+1];

#ifdef VERBOSE_FUNCTION_INI
	PrintLog("CDVDiso ini: FindSection(%s)", section);
#endif /* VERBOSE_FUNCTION_INI */

	charcount = 0;
	retflag = 0;

	while (retflag == 0)
	{
		retval = INIReadLine(infile, scanbuffer);
		if (retval == 0)  return(-1); // EOF? Stop here.

		if (scanbuffer[0] == '[')
		{
			i = 0;
			while ((i < INIMAXLEN) &&
			        (*(section + i) != 0) &&
			        (*(section + i) == scanbuffer[i + 1]))  i++;
			if ((i < INIMAXLEN - 2) && (*(section + i) == 0))
			{
				if ((scanbuffer[i + 1] == ']') && (scanbuffer[i + 2] == 0))
				{
					retflag = 1;
				} // ENDIF- End marks look good? Return successful.
			} // ENDIF- Do we have a section match?
		} // ENDIF- Does this look like a section header?

		if (retflag == 0)  charcount += retval;
	} // ENDWHILE- Scanning lines for the correct [Section] header.

	return(charcount);
} // END INIFindSection()


// Returns: number of bytes to get to start of keyword (or -1)
int INIFindKeyword(ACTUALHANDLE infile, char *keyword, char *buffer)
{
	int charcount;
	int i;
	int j;
	int retflag;
	int retval;
	char scanbuffer[INIMAXLEN+1];

#ifdef VERBOSE_FUNCTION_INI
	PrintLog("CDVDiso ini: FindKeyword(%s)", keyword);
#endif /* VERBOSE_FUNCTION_INI */

	charcount = 0;
	retflag = 0;

	while (retflag == 0)
	{
		retval = INIReadLine(infile, scanbuffer);
		if (retval == 0)  return(-1); // EOF? Stop here.
		if (scanbuffer[0] == '[')  return(-1); // New section? Stop here.

		i = 0;
		while ((i < INIMAXLEN) &&
		        (*(keyword + i) != 0) &&
		        (*(keyword + i) == scanbuffer[i]))  i++;
		if ((i < INIMAXLEN - 2) && (*(keyword + i) == 0))
		{
			if (scanbuffer[i] == '=')
			{
				retflag = 1;
				if (buffer != NULL)
				{
					i++;
					j = 0;
					while ((i < INIMAXLEN) && (scanbuffer[i] != 0))
					{
						*(buffer + j) = scanbuffer[i];
						i++;
						j++;
					} // ENDWHILE- Copying the value out to the outbound buffer.
					*(buffer + j) = 0; // And 0-terminate.
				} // ENDIF- Return the value as well?
			} // ENDIF- End marks look good? Return successful.
		} // ENDIF- Do we have a section match?

		if (retflag == 0)  charcount += retval;
	} // ENDWHILE- Scanning lines for the correct [Section] header.

#ifdef VERBOSE_FUNCTION_INI
	PrintLog("CDVDiso ini:   Value: %s", buffer);
#endif /* VERBOSE_FUNCTION_INI */

	return(charcount);
} // END INIFindKeyWord()


// Returns: number of bytes left to write... (from charcount back)
int INICopy(ACTUALHANDLE infile, ACTUALHANDLE outfile, int charcount)
{
	char buffer[4096];
	int i;
	int chunk;
	int retval;

#ifdef VERBOSE_FUNCTION_INI
	PrintLog("CDVDiso ini: Copy(%i)", charcount);
#endif /* VERBOSE_FUNCTION_INI */

	i = charcount;
	chunk = 4096;
	if (i < chunk)  chunk = i;
	while (chunk > 0)
	{
		retval = ActualFileRead(infile, chunk, buffer);
		if (retval <= 0)  return(i); // Trouble? Stop here.
		if (retval < chunk)  chunk = retval; // Short block? Note it.

		retval = ActualFileWrite(outfile, chunk, buffer);
		if (retval <= 0)  return(i); // Trouble? Stop here.
		i -= retval;
		if (retval < chunk)  return(i); // Short block written? Stop here.

		chunk = 4096;
		if (i < chunk)  chunk = i;
	} // ENDWHILE- Copying a section of file across, one chunk at a time.

	return(0);
} // END INICopyToPos()


int INISaveString(char *file, char *section, char *keyword, char *value)
{
	char inname[INIMAXLEN+1];
	char outname[INIMAXLEN+1];
	int filepos;
	ACTUALHANDLE infile;
	ACTUALHANDLE outfile;
	int i;
	int retval;
	char templine[INIMAXLEN+1];

	if (file == NULL)  return(-1);
	if (section == NULL)  return(-1);
	if (keyword == NULL)  return(-1);
	if (value == NULL)  return(-1);

#ifdef VERBOSE_FUNCTION_INI
	PrintLog("CDVDiso ini: SaveString(%s, %s, %s, %s)",
	         file, section, keyword, value);
#endif /* VERBOSE_FUNCTION_INI */

	filepos = INIRemoveExt(file, inname);
	for (i = 0; i <= filepos; i++)  outname[i] = inname[i];
	INIAddInExt(inname, filepos);
	INIAddOutExt(outname, filepos);

	filepos = 0;
	infile = ActualFileOpenForRead(inname);
	if (infile == ACTUALHANDLENULL)
	{
#ifdef VERBOSE_FUNCTION_INI
		PrintLog("CDVDiso ini:   creating new file");
#endif /* VERBOSE_FUNCTION_INI */
		outfile = ActualFileOpenForWrite(inname);
		if (outfile == ACTUALHANDLENULL)  return(-1); // Just a bad name? Abort.

		sprintf(templine, "[%s]\r\n", section);
		i = 0;
		while ((i < INIMAXLEN) && (templine[i] != 0)) i++;
		retval = ActualFileWrite(outfile, i, templine);
		if (retval < i)
		{
			ActualFileClose(outfile);
			outfile = ACTUALHANDLENULL;
			ActualFileDelete(inname);
			return(-1);
		} // ENDIF- Trouble writing it out? Abort.

		sprintf(templine, "%s=%s\r\n", keyword, value);
		i = 0;
		while ((i < INIMAXLEN) && (templine[i] != 0)) i++;
		retval = ActualFileWrite(outfile, i, templine);
		ActualFileClose(outfile);
		outfile = ACTUALHANDLENULL;
		if (retval < i)
		{
			ActualFileDelete(inname);
			return(-1);
		} // ENDIF- Trouble writing it out? Abort.
		return(0);
	} // ENDIF- No input file? Create a brand new .ini file then.

	retval = INIFindSection(infile, section);
	if (retval < 0)
	{
#ifdef VERBOSE_FUNCTION_INI
		PrintLog("CDVDiso ini:   creating new section");
#endif /* VERBOSE_FUNCTION_INI */
		outfile = ActualFileOpenForWrite(outname);
		if (outfile == ACTUALHANDLENULL)
		{
			ActualFileClose(infile);
			infile = ACTUALHANDLENULL;
			return(-1);
		} // ENDIF- Couldn't open a temp file? Abort

		ActualFileSeek(infile, 0); // Move ini to beginning of file...
		INICopy(infile, outfile, 0x0FFFFFFF); // Copy the whole file out...

		sprintf(templine, "\r\n[%s]\r\n", section);
		i = 0;
		while ((i < INIMAXLEN) && (templine[i] != 0)) i++;
		retval = ActualFileWrite(outfile, i, templine);
		if (retval < i)
		{
			ActualFileClose(infile);
			infile = ACTUALHANDLENULL;
			ActualFileClose(outfile);
			outfile = ACTUALHANDLENULL;
			ActualFileDelete(outname);
			return(-1);
		} // ENDIF- Trouble writing it out? Abort.

		sprintf(templine, "%s=%s\r\n", keyword, value);
		i = 0;
		while ((i < INIMAXLEN) && (templine[i] != 0)) i++;
		retval = ActualFileWrite(outfile, i, templine);
		ActualFileClose(infile);
		infile = ACTUALHANDLENULL;
		ActualFileClose(outfile);
		outfile = ACTUALHANDLENULL;
		if (retval < i)
		{
			ActualFileDelete(outname);
			return(-1);
		} // ENDIF- Trouble writing it out? Abort.

		ActualFileDelete(inname);
		ActualFileRename(outname, inname);
		return(0);
	} // ENDIF- Couldn't find the section? Make a new one!

	filepos = retval;
	ActualFileSeek(infile, filepos);
	filepos += INIReadLine(infile, templine); // Get section line's byte count

	retval = INIFindKeyword(infile, keyword, NULL);
	if (retval < 0)
	{
#ifdef VERBOSE_FUNCTION_INI
		PrintLog("CDVDiso ini:   creating new keyword");
#endif /* VERBOSE_FUNCTION_INI */
		ActualFileSeek(infile, filepos);
		retval = INIReadLine(infile, templine);
		i = 0;
		while ((i < INIMAXLEN) && (templine[i] != 0) && (templine[i] != '='))  i++;
		while ((retval > 0) && (templine[i] == '='))
		{
			filepos += retval;
			retval = INIReadLine(infile, templine);
			i = 0;
			while ((i < INIMAXLEN) && (templine[i] != 0) && (templine[i] != '='))  i++;
		} // ENDWHILE- skimming to the bottom of the section

		outfile = ActualFileOpenForWrite(outname);
		if (outfile == ACTUALHANDLENULL)
		{
			ActualFileClose(infile);
			infile = ACTUALHANDLENULL;
			return(-1);
		} // ENDIF- Couldn't open a temp file? Abort

		ActualFileSeek(infile, 0);
		retval = INICopy(infile, outfile, filepos);
		if (retval > 0)
		{
			ActualFileClose(infile);
			infile = ACTUALHANDLENULL;
			ActualFileClose(outfile);
			outfile = ACTUALHANDLENULL;
			ActualFileDelete(outname);
			return(-1);
		} // ENDIF- Trouble writing everything up to keyword? Abort.

		sprintf(templine, "%s=%s\r\n", keyword, value);
		i = 0;
		while ((i < INIMAXLEN) && (templine[i] != 0)) i++;
		retval = ActualFileWrite(outfile, i, templine);
		if (retval < i)
		{
			ActualFileClose(infile);
			infile = ACTUALHANDLENULL;
			ActualFileClose(outfile);
			outfile = ACTUALHANDLENULL;
			ActualFileDelete(outname);
			return(-1);
		} // ENDIF- Trouble writing it out? Abort.

	}
	else
	{
#ifdef VERBOSE_FUNCTION_INI
		PrintLog("CDVDiso ini:   replacing keyword");
#endif /* VERBOSE_FUNCTION_INI */
		filepos += retval; // Position just before old version of keyword

		outfile = ActualFileOpenForWrite(outname);
		if (outfile == ACTUALHANDLENULL)
		{
			ActualFileClose(infile);
			infile = ACTUALHANDLENULL;
			return(-1);
		} // ENDIF- Couldn't open a temp file? Abort

		ActualFileSeek(infile, 0);
		retval = INICopy(infile, outfile, filepos);
		if (retval > 0)
		{
			ActualFileClose(infile);
			infile = ACTUALHANDLENULL;
			ActualFileClose(outfile);
			outfile = ACTUALHANDLENULL;
			ActualFileDelete(outname);
			return(-1);
		} // ENDIF- Trouble writing everything up to keyword? Abort.

		INIReadLine(infile, templine); // Read past old keyword/value...

		// Replace with new value
		sprintf(templine, "%s=%s\r\n", keyword, value);
		i = 0;
		while ((i < INIMAXLEN) && (templine[i] != 0)) i++;
		retval = ActualFileWrite(outfile, i, templine);
		if (retval < i)
		{
			ActualFileClose(infile);
			infile = ACTUALHANDLENULL;
			ActualFileClose(outfile);
			outfile = ACTUALHANDLENULL;
			ActualFileDelete(outname);
			return(-1);
		} // ENDIF- Trouble writing it out? Abort.
	} // ENDIF- Need to add a new keyword?

	INICopy(infile, outfile, 0xFFFFFFF); // Write out rest of file
	ActualFileClose(infile);
	infile = ACTUALHANDLENULL;
	ActualFileClose(outfile);
	outfile = ACTUALHANDLENULL;
	ActualFileDelete(inname);
	ActualFileRename(outname, inname);
	return(0);
} // END INISaveString()


int INILoadString(char *file, char *section, char *keyword, char *buffer)
{
	char inname[INIMAXLEN+1];
	int filepos;
	ACTUALHANDLE infile;
	int retval;

	if (file == NULL)  return(-1);
	if (section == NULL)  return(-1);
	if (keyword == NULL)  return(-1);
	if (buffer == NULL)  return(-1);

#ifdef VERBOSE_FUNCTION_INI
	PrintLog("CDVDiso ini: LoadString(%s, %s, %s)",
	         file, section, keyword);
#endif /* VERBOSE_FUNCTION_INI */

	filepos = INIRemoveExt(file, inname);
	INIAddInExt(inname, filepos);

	filepos = 0;
	infile = ActualFileOpenForRead(inname);
	if (infile == ACTUALHANDLENULL)  return(-1);

	retval = INIFindSection(infile, section);
	if (retval < 0)
	{
		ActualFileClose(infile);
		infile = ACTUALHANDLENULL;
		return(-1);
	} // ENDIF- Didn't find it? Abort.

	retval = INIFindKeyword(infile, keyword, buffer);
	if (retval < 0)
	{
		ActualFileClose(infile);
		infile = ACTUALHANDLENULL;
		return(-1);
	} // ENDIF- Didn't find it? Abort.

	ActualFileClose(infile);
	infile = ACTUALHANDLENULL;
	return(0);
} // END INILoadString()


int INIRemove(char *file, char *section, char *keyword)
{
	char inname[INIMAXLEN+1];
	char outname[INIMAXLEN+1];
	int filepos;
	ACTUALHANDLE infile;
	ACTUALHANDLE outfile;
	char templine[INIMAXLEN+1];
	int i;
	int retval;

	if (file == NULL)  return(-1);
	if (section == NULL)  return(-1);

#ifdef VERBOSE_FUNCTION_INI
	PrintLog("CDVDiso ini: Remove(%s, %s, %s)",
	         file, section, keyword);
#endif /* VERBOSE_FUNCTION_INI */

	filepos = INIRemoveExt(file, inname);
	for (i = 0; i <= filepos; i++)  outname[i] = inname[i];
	INIAddInExt(inname, filepos);
	INIAddOutExt(outname, filepos);

	infile = ActualFileOpenForRead(inname);
	if (infile == ACTUALHANDLENULL)  return(-1);

	retval = INIFindSection(infile, section);
	if (retval == -1)
	{
		ActualFileClose(infile);
		infile = ACTUALHANDLENULL;
		return(-1);
	} // ENDIF- Couldn't even find the section? Abort

	filepos = retval;
	if (keyword == NULL)
	{
#ifdef VERBOSE_FUNCTION_INI
		PrintLog("CDVDiso ini:   removing section");
#endif /* VERBOSE_FUNCTION_INI */
		outfile = ActualFileOpenForWrite(outname);
		if (outfile == ACTUALHANDLENULL)
		{
			ActualFileClose(infile);
			infile = ACTUALHANDLENULL;
			return(-1);
		} // ENDIF- Couldn't open a temp file? Abort

		ActualFileSeek(infile, 0);
		retval = INICopy(infile, outfile, filepos);
		if (retval > 0)
		{
			ActualFileClose(infile);
			infile = ACTUALHANDLENULL;
			ActualFileClose(outfile);
			outfile = ACTUALHANDLENULL;
			ActualFileDelete(outname);
			return(-1);
		} // ENDIF- Trouble writing everything up to the section? Abort.

		templine[0] = 0;
		retval = 1;
		while ((retval > 0) && (templine[0] != '['))
		{
			retval = INIReadLine(infile, templine);
		} // ENDWHILE- Read to the start of the next section... or EOF.

		if (templine[0] == '[')
		{
			i = 0;
			while ((i < INIMAXLEN) && (templine[i] != 0)) i++;
			retval = ActualFileWrite(outfile, i, templine);
			if (retval < i)
			{
				ActualFileClose(infile);
				infile = ACTUALHANDLENULL;
				ActualFileClose(outfile);
				outfile = ACTUALHANDLENULL;
				ActualFileDelete(outname);
				return(-1);
			} // ENDIF- Trouble writing it out? Abort.
		} // ENDIF- Are there other sections after this one? Save them then.

	}
	else
	{
		filepos = retval;
		ActualFileSeek(infile, filepos);
		filepos += INIReadLine(infile, templine); // Get section line's byte count

		retval = INIFindKeyword(infile, keyword, NULL);
		if (retval == -1)
		{
			ActualFileClose(infile);
			infile = ACTUALHANDLENULL;
			return(-1);
		} // ENDIF- Couldn't find the keyword? Abort
		filepos += retval;

#ifdef VERBOSE_FUNCTION_INI
		PrintLog("CDVDiso ini:   removing keyword");
#endif /* VERBOSE_FUNCTION_INI */
		outfile = ActualFileOpenForWrite(outname);
		if (outfile == ACTUALHANDLENULL)
		{
			ActualFileClose(infile);
			infile = ACTUALHANDLENULL;
			return(-1);
		} // ENDIF- Couldn't open a temp file? Abort

		ActualFileSeek(infile, 0);
		retval = INICopy(infile, outfile, filepos);
		if (retval > 0)
		{
			ActualFileClose(infile);
			infile = ACTUALHANDLENULL;
			ActualFileClose(outfile);
			outfile = ACTUALHANDLENULL;
			ActualFileDelete(outname);
			return(-1);
		} // ENDIF- Trouble writing everything up to keyword? Abort.

		INIReadLine(infile, templine); // Read (and discard) the keyword line
	} // ENDIF- Wipe out the whole section? Or just a keyword?

	INICopy(infile, outfile, 0xFFFFFFF); // Write out rest of file
	ActualFileClose(infile);
	infile = ACTUALHANDLENULL;
	ActualFileClose(outfile);
	outfile = ACTUALHANDLENULL;
	ActualFileDelete(inname);
	ActualFileRename(outname, inname);
	return(0);
} // END INIRemove()


int INISaveUInt(char *file, char *section, char *keyword, unsigned int value)
{
	char numvalue[INIMAXLEN+1];

	sprintf(numvalue, "%u", value);
	return(INISaveString(file, section, keyword, numvalue));
} // END INISaveUInt()


int INILoadUInt(char *file, char *section, char *keyword, unsigned int *buffer)
{
	char numvalue[INIMAXLEN+1];
	int retval;
	unsigned int value;
	// unsigned int sign; // Not needed in unsigned numbers
	int pos;

	if (buffer == NULL)  return(-1);
	*(buffer) = 0;

	retval = INILoadString(file, section, keyword, numvalue);
	if (retval < 0)  return(retval);

	value = 0;
	// sign = 1; // Start positive
	pos = 0;

	// Note: skip leading spaces? (Shouldn't have to, I hope)

	// if(numvalue[pos] == '-') {
	//   pos++;
	//   sign = -1;
	// } // ENDIF- Negative sign check

	while ((pos < INIMAXLEN) && (numvalue[pos] != 0))
	{
		if (value > (0xFFFFFFFF / 10))  return(-1); // Overflow?

		if ((numvalue[pos] >= '0') && (numvalue[pos] <= '9'))
		{
			value *= 10;
			value += numvalue[pos] - '0';
			pos++;
		}
		else
		{
			numvalue[pos] = 0;
		} // ENDIF- Add a digit in? Or stop searching for digits?
	} // ENDWHILE- Adding digits of info to our ever-increasing value

	// value *= sign
	*(buffer) = value;
	return(0);
} // END INILoadUInt()

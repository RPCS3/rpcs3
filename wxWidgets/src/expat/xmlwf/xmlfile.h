/* Copyright (c) 1998, 1999 Thai Open Source Software Center Ltd
   See the file COPYING for copying permission.
*/

#define XML_MAP_FILE 01
#define XML_EXTERNAL_ENTITIES 02

extern int XML_ProcessFile(XML_Parser parser,
                           const XML_Char *filename,
                           unsigned flags);

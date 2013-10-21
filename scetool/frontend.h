/*
* Copyright (c) 2011-2013 by naehrwert
* This file is released under the GPLv2.
*/


#ifndef _FRONTEND_H_
#define _FRONTEND_H_

void frontend_print_infos(scetool::s8 *file);
void frontend_decrypt(scetool::s8 *file_in, scetool::s8 *file_out);
void frontend_encrypt(scetool::s8 *file_in, scetool::s8 *file_out);

#endif

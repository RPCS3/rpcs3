/*********************************************************************
 * Copyright (C) 2003 Tord Lindstrom (pukko@home.se)
 * This file is subject to the terms and conditions of the PS2Link License.
 * See the file LICENSE in the main directory of this distribution for more
 * details.
 */

#define PS2E_FIO_OPEN_CMD     0xcc2e0101
#define PS2E_FIO_CLOSE_CMD    0xcc2e0102
#define PS2E_FIO_READ_CMD     0xcc2e0103
#define PS2E_FIO_WRITE_CMD    0xcc2e0104
#define PS2E_FIO_LSEEK_CMD    0xcc2e0105
#define PS2E_FIO_OPENDIR_CMD  0xcc2e0106
#define PS2E_FIO_CLOSEDIR_CMD 0xcc2e0107
#define PS2E_FIO_READDIR_CMD  0xcc2e0108
#define PS2E_FIO_REMOVE_CMD   0xcc2e0109
#define PS2E_FIO_MKDIR_CMD    0xcc2e010a
#define PS2E_FIO_RMDIR_CMD    0xcc2e010b

#define PS2E_FIO_PRINTF_CMD   0xcc2e0201

#define PS2E_FIO_MAX_PATH   256

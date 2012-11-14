/*
 *  hdbc.h
 *
 *  $Id: hdbc.h 1446 1999-01-22 10:52:42Z RR $
 *
 *  Data source connect object management functions
 *
 *  The iODBC driver manager.
 *  
 *  Copyright (C) 1995 by Ke Jin <kejin@empress.com> 
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public
 *  License along with this library; if not, write to the Free
 *  Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
#ifndef	_HDBC_H
#define	_HDBC_H

typedef struct DBC
  {
    int type;			/* must be 1st field */
    struct DBC FAR *
     next;

    HENV genv;			/* back point to global env object */

    HDBC dhdbc;			/* driver's private dbc */
    HENV henv;			/* back point to instant env object */
    HSTMT hstmt;		/* list of statement object handle(s) */
    HERR herr;

    int state;

    /* options */
    UDWORD access_mode;
    UDWORD autocommit;

    UDWORD login_timeout;
    UDWORD odbc_cursors;
    UDWORD packet_size;
    UDWORD quiet_mode;
    UDWORD txn_isolation;
    SWORD cb_commit;
    SWORD cb_rollback;

    char FAR *
     current_qualifier;

    int trace;			/* trace flag */
    char FAR *
     tfile;
    void FAR *
     tstm;			/* trace stream */
  }
DBC_t;

/* 
 * Note:
 *  - ODBC applications can see address of driver manager's 
 *    connection object, i.e connection handle -- a void pointer, 
 *    but not detail of it. ODBC applications can neither see 
 *    detail driver's connection object nor its address.
 *
 *  - ODBC driver manager knows its own connection objects and
 *    exposes their address to an ODBC application. Driver manager
 *    also knows address of driver's connection objects and keeps
 *    it via dhdbc field in driver manager's connection object.
 * 
 *  - ODBC driver exposes address of its own connection object to
 *    driver manager without detail.
 *
 *  - Applications can get driver's connection object handle by
 *    SQLGetInfo() with fInfoType equals to SQL_DRIVER_HDBC.
 */

enum
  {
    en_dbc_allocated,
    en_dbc_needdata,
    en_dbc_connected,
    en_dbc_hstmt
  };
#endif

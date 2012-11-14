/*
 *  henv.h
 *
 *  $Id: henv.h 2613 1999-06-01 15:32:12Z VZ $
 *
 *  Environment object management functions
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
#ifndef	_HENV_H
#define	_HENV_H

#include	"config.h"
#include	"dlproc.h"

#include	"isql.h"
#include	"isqlext.h"

#ifndef SYSERR
#define   SYSERR -1
#endif

enum
  {

#if (ODBCVER >= 0x0300)
    en_AllocHandle = SQL_API_SQLALLOCHANDLE,
    en_FreeHandle = SQL_API_SQLFREEHANDLE,
#endif

    en_AllocEnv = SQL_API_SQLALLOCENV,
    en_AllocConnect = SQL_API_SQLALLOCCONNECT,
    en_Connect = SQL_API_SQLCONNECT,
    en_DriverConnect = SQL_API_SQLDRIVERCONNECT,
    en_BrowseConnect = SQL_API_SQLBROWSECONNECT,

    en_DataSources = SQL_API_SQLDATASOURCES,
    en_Drivers = SQL_API_SQLDRIVERS,
    en_GetInfo = SQL_API_SQLGETINFO,
    en_GetFunctions = SQL_API_SQLGETFUNCTIONS,
    en_GetTypeInfo = SQL_API_SQLGETTYPEINFO,

    en_SetConnectOption = SQL_API_SQLSETCONNECTOPTION,
    en_GetConnectOption = SQL_API_SQLGETCONNECTOPTION,
    en_SetStmtOption = SQL_API_SQLSETSTMTOPTION,
    en_GetStmtOption = SQL_API_SQLGETSTMTOPTION,

    en_AllocStmt = SQL_API_SQLALLOCSTMT,
    en_Prepare = SQL_API_SQLPREPARE,
    en_BindParameter = SQL_API_SQLBINDPARAMETER,
    en_ParamOptions = SQL_API_SQLPARAMOPTIONS,
    en_GetCursorName = SQL_API_SQLGETCURSORNAME,
    en_SetCursorName = SQL_API_SQLSETCURSORNAME,
    en_SetScrollOptions = SQL_API_SQLSETSCROLLOPTIONS,
    en_SetParam = SQL_API_SQLSETPARAM,

    en_Execute = SQL_API_SQLEXECUTE,
    en_ExecDirect = SQL_API_SQLEXECDIRECT,
    en_NativeSql = SQL_API_SQLNATIVESQL,
    en_DescribeParam = SQL_API_SQLDESCRIBEPARAM,
    en_NumParams = SQL_API_SQLNUMPARAMS,
    en_ParamData = SQL_API_SQLPARAMDATA,
    en_PutData = SQL_API_SQLPUTDATA,

    en_RowCount = SQL_API_SQLROWCOUNT,
    en_NumResultCols = SQL_API_SQLNUMRESULTCOLS,
    en_DescribeCol = SQL_API_SQLDESCRIBECOL,
    en_ColAttributes = SQL_API_SQLCOLATTRIBUTES,
    en_BindCol = SQL_API_SQLBINDCOL,
    en_Fetch = SQL_API_SQLFETCH,
    en_ExtendedFetch = SQL_API_SQLEXTENDEDFETCH,
    en_GetData = SQL_API_SQLGETDATA,
    en_SetPos = SQL_API_SQLSETPOS,
    en_MoreResults = SQL_API_SQLMORERESULTS,
    en_Error = SQL_API_SQLERROR,

    en_ColumnPrivileges = SQL_API_SQLCOLUMNPRIVILEGES,
    en_Columns = SQL_API_SQLCOLUMNS,
    en_ForeignKeys = SQL_API_SQLFOREIGNKEYS,
    en_PrimaryKeys = SQL_API_SQLPRIMARYKEYS,
    en_ProcedureColumns = SQL_API_SQLPROCEDURECOLUMNS,
    en_Procedures = SQL_API_SQLPROCEDURES,
    en_SpecialColumns = SQL_API_SQLSPECIALCOLUMNS,
    en_Statistics = SQL_API_SQLSTATISTICS,
    en_TablePrivileges = SQL_API_SQLTABLEPRIVILEGES,
    en_Tables = SQL_API_SQLTABLES,

    en_FreeStmt = SQL_API_SQLFREESTMT,
    en_Cancel = SQL_API_SQLCANCEL,
    en_Transact = SQL_API_SQLTRANSACT,

    en_Disconnect = SQL_API_SQLDISCONNECT,
    en_FreeConnect = SQL_API_SQLFREECONNECT,
    en_FreeEnv = SQL_API_SQLFREEENV,

    en_NullProc = SYSERR
  };

typedef struct
  {
    int type;			/* must be 1st field */

    HENV henv;			/* driver's env list */
    HDBC hdbc;			/* driver's dbc list */
    HERR herr;			/* err list          */
    int state;
  }
GENV_t;

typedef struct
  {
    HENV next;			/* next attached env handle */
    int refcount;		/* Driver's bookkeeping reference count */
    HPROC dllproc_tab[SQL_EXT_API_LAST + 1];	/* driver api calls  */

    HENV dhenv;			/* driver env handle    */
    HDLL hdll;			/* drvier share library handle */
  }
ENV_t;

/* Note:

 *  - ODBC applications only know about global environment handle, 
 *    a void pointer points to a GENV_t object. There is only one
 *    this object per process(however, to make the library reentrant,
 *    we still keep this object on heap). Applications only know 
 *    address of this object and needn't care about its detail.
 *
 *  - ODBC driver manager knows about instance environment handles,
 *    void pointers point to ENV_t objects. There are maybe more
 *    than one this kind of objects per process. However, multiple
 *    connections to a same data source(i.e. call same share library)
 *    will share one instance environment object.
 *
 *  - ODBC drvier manager knows about their own environemnt handle,
 *    a void pointer point to a driver defined object. Every driver
 *    keeps one of its own environment object and driver manager
 *    keeps address of it by the 'dhenv' field in the instance
 *    environment object without care about its detail.
 *
 *  - Applications can get driver's environment object handle by
 *    SQLGetInfo() with fInfoType equals to SQL_DRIVER_HENV
 */
#endif

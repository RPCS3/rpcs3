/*
 *
 *  isqlext.h
 *
 *  $Id: isqlext.h 30070 2004-10-22 19:11:07Z KH $
 *
 *  iODBC defines (ext)
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
#ifndef _ISQLEXT_H
#define _ISQLEXT_H

#include "wx/isql.h"

/*
 *  Generic constants
 */
#define SQL_MAX_OPTION_STRING_LENGTH 256

/*
 *  Additional return codes
 */
#define SQL_STILL_EXECUTING         2
#define SQL_NEED_DATA               99

/*
 *  SQL extended datatypes
 */
#define SQL_DATE                        9
#define SQL_TIME                        10
#define SQL_TIMESTAMP                   11
#define SQL_LONGVARCHAR                 (-1)
#define SQL_BINARY                      (-2)
#define SQL_VARBINARY                   (-3)
#define SQL_LONGVARBINARY               (-4)
#define SQL_BIGINT                      (-5)
#define SQL_TINYINT                     (-6)
#define SQL_BIT                         (-7)

#define SQL_INTERVAL_YEAR               (-80)
#define SQL_INTERVAL_MONTH              (-81)
#define SQL_INTERVAL_YEAR_TO_MONTH      (-82)
#define SQL_INTERVAL_DAY                (-83)
#define SQL_INTERVAL_HOUR               (-84)
#define SQL_INTERVAL_MINUTE             (-85)
#define SQL_INTERVAL_SECOND             (-86)
#define SQL_INTERVAL_DAY_TO_HOUR        (-87)
#define SQL_INTERVAL_DAY_TO_MINUTE      (-88)
#define SQL_INTERVAL_DAY_TO_SECOND      (-89)
#define SQL_INTERVAL_HOUR_TO_MINUTE     (-90)
#define SQL_INTERVAL_HOUR_TO_SECOND     (-91)
#define SQL_INTERVAL_MINUTE_TO_SECOND   (-92)
#define SQL_UNICODE                     (-95)

#define SQL_TYPE_DRIVER_START   SQL_INTERVAL_YEAR
#define SQL_TYPE_DRIVER_END     SQL_UNICODE

#define SQL_SIGNED_OFFSET       (-20)
#define SQL_UNSIGNED_OFFSET     (-22)

/*
 *  C datatype to SQL datatype mapping
 */
#define SQL_C_DATE              SQL_DATE
#define SQL_C_TIME              SQL_TIME
#define SQL_C_TIMESTAMP         SQL_TIMESTAMP
#define SQL_C_BINARY            SQL_BINARY
#define SQL_C_BIT               SQL_BIT
#define SQL_C_TINYINT           SQL_TINYINT
#define SQL_C_SLONG             SQL_C_LONG+SQL_SIGNED_OFFSET
#define SQL_C_SSHORT            SQL_C_SHORT+SQL_SIGNED_OFFSET
#define SQL_C_STINYINT          SQL_TINYINT+SQL_SIGNED_OFFSET
#define SQL_C_ULONG             SQL_C_LONG+SQL_UNSIGNED_OFFSET
#define SQL_C_USHORT            SQL_C_SHORT+SQL_UNSIGNED_OFFSET
#define SQL_C_UTINYINT          SQL_TINYINT+SQL_UNSIGNED_OFFSET
#define SQL_C_BOOKMARK          SQL_C_ULONG

/*
 *  Extended data types override sql.h defined
 */
#undef SQL_TYPE_MIN
#define SQL_TYPE_MIN SQL_BIT
#define SQL_ALL_TYPES 0

/*
 *  SQL portable types for C - DATE, TIME, TIMESTAMP
 */
typedef struct _DATE_STRUCT
  {
    SWORD year;
    UWORD month;
    UWORD day;
  }
DATE_STRUCT;

typedef struct _TIME_STRUCT
  {
    UWORD hour;
    UWORD minute;
    UWORD second;
  }
TIME_STRUCT;

typedef struct _TIMESTAMP_STRUCT
  {
    SWORD year;
    UWORD month;
    UWORD day;
    UWORD hour;
    UWORD minute;
    UWORD second;
    UDWORD fraction;
  }
TIMESTAMP_STRUCT;

typedef unsigned long int BOOKMARK;

/*
 * ----------------------------------------------------------------------
 *  Level 1 Functions
 * ----------------------------------------------------------------------
 */

/*
 *  SQLDriverConnect
 */
#define SQL_DRIVER_NOPROMPT             0
#define SQL_DRIVER_COMPLETE             1
#define SQL_DRIVER_PROMPT               2
#define SQL_DRIVER_COMPLETE_REQUIRED    3

/*
 *  SQLGetData
 */
#define SQL_NO_TOTAL                    (-4)

/*
 *  SQLBindParameter
 */
#define SQL_DEFAULT_PARAM               (-5)
#define SQL_IGNORE                      (-6)
#define SQL_LEN_DATA_AT_EXEC_OFFSET     (-100)
#define SQL_LEN_DATA_AT_EXEC(length)    (-length+SQL_LEN_DATA_AT_EXEC_OFFSET)

/*
 *  SQLGetFunctions
 */
#define SQL_API_SQLALLOCCONNECT         1 /* Core Functions */
#define SQL_API_SQLALLOCENV             2
#define SQL_API_SQLALLOCSTMT            3
#define SQL_API_SQLBINDCOL              4
#define SQL_API_SQLCANCEL               5
#define SQL_API_SQLCOLATTRIBUTES        6
#define SQL_API_SQLCONNECT              7
#define SQL_API_SQLDESCRIBECOL          8
#define SQL_API_SQLDISCONNECT           9
#define SQL_API_SQLERROR                10
#define SQL_API_SQLEXECDIRECT           11
#define SQL_API_SQLEXECUTE              12
#define SQL_API_SQLFETCH                13
#define SQL_API_SQLFREECONNECT          14
#define SQL_API_SQLFREEENV              15
#define SQL_API_SQLFREESTMT             16
#define SQL_API_SQLGETCURSORNAME        17
#define SQL_API_SQLNUMRESULTCOLS        18
#define SQL_API_SQLPREPARE              19
#define SQL_API_SQLROWCOUNT             20
#define SQL_API_SQLSETCURSORNAME        21
#define SQL_API_SQLSETPARAM             22
#define SQL_API_SQLTRANSACT             23

#define SQL_NUM_FUNCTIONS               23

#define SQL_EXT_API_START               40

#define SQL_API_SQLCOLUMNS              40 /* Level 1 Functions */
#define SQL_API_SQLDRIVERCONNECT        41
#define SQL_API_SQLGETCONNECTOPTION     42
#define SQL_API_SQLGETDATA              43
#define SQL_API_SQLGETFUNCTIONS         44
#define SQL_API_SQLGETINFO              45
#define SQL_API_SQLGETSTMTOPTION        46
#define SQL_API_SQLGETTYPEINFO          47
#define SQL_API_SQLPARAMDATA            48
#define SQL_API_SQLPUTDATA              49
#define SQL_API_SQLSETCONNECTOPTION     50
#define SQL_API_SQLSETSTMTOPTION        51
#define SQL_API_SQLSPECIALCOLUMNS       52
#define SQL_API_SQLSTATISTICS           53
#define SQL_API_SQLTABLES               54

#define SQL_API_SQLBROWSECONNECT        55 /* Level 2 Functions */
#define SQL_API_SQLCOLUMNPRIVILEGES     56
#define SQL_API_SQLDATASOURCES          57
#define SQL_API_SQLDESCRIBEPARAM        58
#define SQL_API_SQLEXTENDEDFETCH        59
#define SQL_API_SQLFOREIGNKEYS          60
#define SQL_API_SQLMORERESULTS          61
#define SQL_API_SQLNATIVESQL            62
#define SQL_API_SQLNUMPARAMS            63
#define SQL_API_SQLPARAMOPTIONS         64
#define SQL_API_SQLPRIMARYKEYS          65
#define SQL_API_SQLPROCEDURECOLUMNS     66
#define SQL_API_SQLPROCEDURES           67
#define SQL_API_SQLSETPOS               68
#define SQL_API_SQLSETSCROLLOPTIONS     69
#define SQL_API_SQLTABLEPRIVILEGES      70

#define SQL_API_SQLDRIVERS              71
#define SQL_API_SQLBINDPARAMETER        72
#define SQL_EXT_API_LAST                SQL_API_SQLBINDPARAMETER

#define SQL_API_ALL_FUNCTIONS           0

#define SQL_NUM_EXTENSIONS (SQL_EXT_API_LAST-SQL_EXT_API_START+1)

/*
 *  SQLGetInfo
 */
#define SQL_INFO_FIRST                  0
#define SQL_ACTIVE_CONNECTIONS          0
#define SQL_ACTIVE_STATEMENTS           1
#define SQL_DATA_SOURCE_NAME            2
#define SQL_DRIVER_HDBC                 3
#define SQL_DRIVER_HENV                 4
#define SQL_DRIVER_HSTMT                5
#define SQL_DRIVER_NAME                 6
#define SQL_DRIVER_VER                  7
#define SQL_FETCH_DIRECTION             8
#define SQL_ODBC_API_CONFORMANCE        9
#define SQL_ODBC_VER                    10
#define SQL_ROW_UPDATES                 11
#define SQL_ODBC_SAG_CLI_CONFORMANCE    12
#define SQL_SERVER_NAME                 13
#define SQL_SEARCH_PATTERN_ESCAPE       14
#define SQL_ODBC_SQL_CONFORMANCE        15
#define SQL_DBMS_NAME                   17
#define SQL_DBMS_VER                    18
#define SQL_ACCESSIBLE_TABLES           19
#define SQL_ACCESSIBLE_PROCEDURES       20
#define SQL_PROCEDURES                  21
#define SQL_CONCAT_NULL_BEHAVIOR        22
#define SQL_CURSOR_COMMIT_BEHAVIOR      23
#define SQL_CURSOR_ROLLBACK_BEHAVIOR    24
#define SQL_DATA_SOURCE_READ_ONLY       25
#define SQL_DEFAULT_TXN_ISOLATION       26
#define SQL_EXPRESSIONS_IN_ORDERBY      27
#define SQL_IDENTIFIER_CASE             28
#define SQL_IDENTIFIER_QUOTE_CHAR       29
#define SQL_MAX_COLUMN_NAME_LEN         30
#define SQL_MAX_CURSOR_NAME_LEN         31
#define SQL_MAX_OWNER_NAME_LEN          32
#define SQL_MAX_PROCEDURE_NAME_LEN      33
#define SQL_MAX_QUALIFIER_NAME_LEN      34
#define SQL_MAX_TABLE_NAME_LEN          35
#define SQL_MULT_RESULT_SETS            36
#define SQL_MULTIPLE_ACTIVE_TXN         37
#define SQL_OUTER_JOINS                 38
#define SQL_OWNER_TERM                  39
#define SQL_PROCEDURE_TERM              40
#define SQL_QUALIFIER_NAME_SEPARATOR    41
#define SQL_QUALIFIER_TERM              42
#define SQL_SCROLL_CONCURRENCY          43
#define SQL_SCROLL_OPTIONS              44
#define SQL_TABLE_TERM                  45
#define SQL_TXN_CAPABLE                 46
#define SQL_USER_NAME                   47
#define SQL_CONVERT_FUNCTIONS           48
#define SQL_NUMERIC_FUNCTIONS           49
#define SQL_STRING_FUNCTIONS            50
#define SQL_SYSTEM_FUNCTIONS            51
#define SQL_TIMEDATE_FUNCTIONS          52
#define SQL_CONVERT_BIGINT              53
#define SQL_CONVERT_BINARY              54
#define SQL_CONVERT_BIT                 55
#define SQL_CONVERT_CHAR                56
#define SQL_CONVERT_DATE                57
#define SQL_CONVERT_DECIMAL             58
#define SQL_CONVERT_DOUBLE              59
#define SQL_CONVERT_FLOAT               60
#define SQL_CONVERT_INTEGER             61
#define SQL_CONVERT_LONGVARCHAR         62
#define SQL_CONVERT_NUMERIC             63
#define SQL_CONVERT_REAL                64
#define SQL_CONVERT_SMALLINT            65
#define SQL_CONVERT_TIME                66
#define SQL_CONVERT_TIMESTAMP           67
#define SQL_CONVERT_TINYINT             68
#define SQL_CONVERT_VARBINARY           69
#define SQL_CONVERT_VARCHAR             70
#define SQL_CONVERT_LONGVARBINARY       71
#define SQL_TXN_ISOLATION_OPTION        72
#define SQL_ODBC_SQL_OPT_IEF            73

/*
 *  ODBC SDK 1.0 Additions
 */
#define SQL_CORRELATION_NAME            74
#define SQL_NON_NULLABLE_COLUMNS        75

/*
 *  ODBC SDK 2.0 Additions
 */
#define SQL_DRIVER_HLIB                 76
#define SQL_DRIVER_ODBC_VER             77
#define SQL_LOCK_TYPES                  78
#define SQL_POS_OPERATIONS              79
#define SQL_POSITIONED_STATEMENTS       80
#define SQL_GETDATA_EXTENSIONS          81
#define SQL_BOOKMARK_PERSISTENCE        82
#define SQL_STATIC_SENSITIVITY          83
#define SQL_FILE_USAGE                  84
#define SQL_NULL_COLLATION              85
#define SQL_ALTER_TABLE                 86
#define SQL_COLUMN_ALIAS                87
#define SQL_GROUP_BY                    88
#define SQL_KEYWORDS                    89
#define SQL_ORDER_BY_COLUMNS_IN_SELECT  90
#define SQL_OWNER_USAGE                 91
#define SQL_QUALIFIER_USAGE             92
#define SQL_QUOTED_IDENTIFIER_CASE      93
#define SQL_SPECIAL_CHARACTERS          94
#define SQL_SUBQUERIES                  95
#define SQL_UNION                       96
#define SQL_MAX_COLUMNS_IN_GROUP_BY     97
#define SQL_MAX_COLUMNS_IN_INDEX        98
#define SQL_MAX_COLUMNS_IN_ORDER_BY     99
#define SQL_MAX_COLUMNS_IN_SELECT       100
#define SQL_MAX_COLUMNS_IN_TABLE        101
#define SQL_MAX_INDEX_SIZE              102
#define SQL_MAX_ROW_SIZE_INCLUDES_LONG  103
#define SQL_MAX_ROW_SIZE                104
#define SQL_MAX_STATEMENT_LEN           105
#define SQL_MAX_TABLES_IN_SELECT        106
#define SQL_MAX_USER_NAME_LEN           107
#define SQL_MAX_CHAR_LITERAL_LEN        108
#define SQL_TIMEDATE_ADD_INTERVALS      109
#define SQL_TIMEDATE_DIFF_INTERVALS     110
#define SQL_NEED_LONG_DATA_LEN          111
#define SQL_MAX_BINARY_LITERAL_LEN      112
#define SQL_LIKE_ESCAPE_CLAUSE          113
#define SQL_QUALIFIER_LOCATION          114

/*
 *  ODBC SDK 2.01 Additions
 */
#define SQL_OJ_CAPABILITIES             65003 /* Temp value until ODBC 3.0 */

#define SQL_INFO_LAST                   SQL_QUALIFIER_LOCATION
#define SQL_INFO_DRIVER_START           1000


/*
 *  SQL_CONVERT_* bitmask values
 */
#define SQL_CVT_CHAR            0x00000001L
#define SQL_CVT_NUMERIC         0x00000002L
#define SQL_CVT_DECIMAL         0x00000004L
#define SQL_CVT_INTEGER         0x00000008L
#define SQL_CVT_SMALLINT        0x00000010L
#define SQL_CVT_FLOAT           0x00000020L
#define SQL_CVT_REAL            0x00000040L
#define SQL_CVT_DOUBLE          0x00000080L
#define SQL_CVT_VARCHAR         0x00000100L
#define SQL_CVT_LONGVARCHAR     0x00000200L
#define SQL_CVT_BINARY          0x00000400L
#define SQL_CVT_VARBINARY       0x00000800L
#define SQL_CVT_BIT             0x00001000L
#define SQL_CVT_TINYINT         0x00002000L
#define SQL_CVT_BIGINT          0x00004000L
#define SQL_CVT_DATE            0x00008000L
#define SQL_CVT_TIME            0x00010000L
#define SQL_CVT_TIMESTAMP       0x00020000L
#define SQL_CVT_LONGVARBINARY   0x00040000L

/*
 *  SQL_CONVERT_FUNCTIONS
 */
#define SQL_FN_CVT_CONVERT      0x00000001L

/*
 *  SQL_STRING_FUNCTIONS
 */
#define SQL_FN_STR_CONCAT       0x00000001L
#define SQL_FN_STR_INSERT       0x00000002L
#define SQL_FN_STR_LEFT         0x00000004L
#define SQL_FN_STR_LTRIM        0x00000008L
#define SQL_FN_STR_LENGTH       0x00000010L
#define SQL_FN_STR_LOCATE       0x00000020L
#define SQL_FN_STR_LCASE        0x00000040L
#define SQL_FN_STR_REPEAT       0x00000080L
#define SQL_FN_STR_REPLACE      0x00000100L
#define SQL_FN_STR_RIGHT        0x00000200L
#define SQL_FN_STR_RTRIM        0x00000400L
#define SQL_FN_STR_SUBSTRING    0x00000800L
#define SQL_FN_STR_UCASE        0x00001000L
#define SQL_FN_STR_ASCII        0x00002000L
#define SQL_FN_STR_CHAR         0x00004000L
#define SQL_FN_STR_DIFFERENCE   0x00008000L
#define SQL_FN_STR_LOCATE_2     0x00010000L
#define SQL_FN_STR_SOUNDEX      0x00020000L
#define SQL_FN_STR_SPACE        0x00040000L

/*
 *  SQL_NUMERIC_FUNCTIONS
 */
#define SQL_FN_NUM_ABS          0x00000001L
#define SQL_FN_NUM_ACOS         0x00000002L
#define SQL_FN_NUM_ASIN         0x00000004L
#define SQL_FN_NUM_ATAN         0x00000008L
#define SQL_FN_NUM_ATAN2        0x00000010L
#define SQL_FN_NUM_CEILING      0x00000020L
#define SQL_FN_NUM_COS          0x00000040L
#define SQL_FN_NUM_COT          0x00000080L
#define SQL_FN_NUM_EXP          0x00000100L
#define SQL_FN_NUM_FLOOR        0x00000200L
#define SQL_FN_NUM_LOG          0x00000400L
#define SQL_FN_NUM_MOD          0x00000800L
#define SQL_FN_NUM_SIGN         0x00001000L
#define SQL_FN_NUM_SIN          0x00002000L
#define SQL_FN_NUM_SQRT         0x00004000L
#define SQL_FN_NUM_TAN          0x00008000L
#define SQL_FN_NUM_PI           0x00010000L
#define SQL_FN_NUM_RAND         0x00020000L
#define SQL_FN_NUM_DEGREES      0x00040000L
#define SQL_FN_NUM_LOG10        0x00080000L
#define SQL_FN_NUM_POWER        0x00100000L
#define SQL_FN_NUM_RADIANS      0x00200000L
#define SQL_FN_NUM_ROUND        0x00400000L
#define SQL_FN_NUM_TRUNCATE     0x00800000L

/*
 *  SQL_TIMEDATE_FUNCTIONS
 */
#define SQL_FN_TD_NOW           0x00000001L
#define SQL_FN_TD_CURDATE       0x00000002L
#define SQL_FN_TD_DAYOFMONTH    0x00000004L
#define SQL_FN_TD_DAYOFWEEK     0x00000008L
#define SQL_FN_TD_DAYOFYEAR     0x00000010L
#define SQL_FN_TD_MONTH         0x00000020L
#define SQL_FN_TD_QUARTER       0x00000040L
#define SQL_FN_TD_WEEK          0x00000080L
#define SQL_FN_TD_YEAR          0x00000100L
#define SQL_FN_TD_CURTIME       0x00000200L
#define SQL_FN_TD_HOUR          0x00000400L
#define SQL_FN_TD_MINUTE        0x00000800L
#define SQL_FN_TD_SECOND        0x00001000L
#define SQL_FN_TD_TIMESTAMPADD  0x00002000L
#define SQL_FN_TD_TIMESTAMPDIFF 0x00004000L
#define SQL_FN_TD_DAYNAME       0x00008000L
#define SQL_FN_TD_MONTHNAME     0x00010000L

/*
 *  SQL_SYSTEM_FUNCTIONS
 */
#define SQL_FN_SYS_USERNAME     0x00000001L
#define SQL_FN_SYS_DBNAME       0x00000002L
#define SQL_FN_SYS_IFNULL       0x00000004L

/*
 *  SQL_TIMEDATE_ADD_INTERVALS
 *  SQL_TIMEDATE_DIFF_INTERVALS
 */
#define SQL_FN_TSI_FRAC_SECOND  0x00000001L
#define SQL_FN_TSI_SECOND       0x00000002L
#define SQL_FN_TSI_MINUTE       0x00000004L
#define SQL_FN_TSI_HOUR         0x00000008L
#define SQL_FN_TSI_DAY          0x00000010L
#define SQL_FN_TSI_WEEK         0x00000020L
#define SQL_FN_TSI_MONTH        0x00000040L
#define SQL_FN_TSI_QUARTER      0x00000080L
#define SQL_FN_TSI_YEAR         0x00000100L

/*
 *  SQL_ODBC_API_CONFORMANCE
 */
#define SQL_OAC_NONE            0x0000
#define SQL_OAC_LEVEL1          0x0001
#define SQL_OAC_LEVEL2          0x0002

/*
 * SQL_ODBC_SAG_CLI_CONFORMANCE
 */
#define SQL_OSCC_NOT_COMPLIANT  0x0000
#define SQL_OSCC_COMPLIANT      0x0001

/*
 *  SQL_ODBC_SQL_CONFORMANCE
 */
#define SQL_OSC_MINIMUM         0x0000
#define SQL_OSC_CORE            0x0001
#define SQL_OSC_EXTENDED        0x0002

/*
 *  SQL_CONCAT_NULL_BEHAVIOR
 */
#define SQL_CB_NULL             0x0000
#define SQL_CB_NON_NULL         0x0001

/*
 *  SQL_CURSOR_COMMIT_BEHAVIOR
 *  SQL_CURSOR_ROLLBACK_BEHAVIOR
 */
#define SQL_CB_DELETE           0x0000
#define SQL_CB_CLOSE            0x0001
#define SQL_CB_PRESERVE         0x0002

/*
 *  SQL_IDENTIFIER_CASE
 */
#define SQL_IC_UPPER            0x0001
#define SQL_IC_LOWER            0x0002
#define SQL_IC_SENSITIVE        0x0003
#define SQL_IC_MIXED            0x0004

/*
 *  SQL_TXN_CAPABLE
 */
#define SQL_TC_NONE             0x0000
#define SQL_TC_DML              0x0001
#define SQL_TC_ALL              0x0002
#define SQL_TC_DDL_COMMIT       0x0003
#define SQL_TC_DDL_IGNORE       0x0004

/*
 *  SQL_SCROLL_OPTIONS
 */
#define SQL_SO_FORWARD_ONLY     0x00000001L
#define SQL_SO_KEYSET_DRIVEN    0x00000002L
#define SQL_SO_DYNAMIC          0x00000004L
#define SQL_SO_MIXED            0x00000008L
#define SQL_SO_STATIC           0x00000010L

/*
 * SQL_SCROLL_CONCURRENCY
 */
#define SQL_SCCO_READ_ONLY      0x00000001L
#define SQL_SCCO_LOCK           0x00000002L
#define SQL_SCCO_OPT_ROWVER     0x00000004L
#define SQL_SCCO_OPT_VALUES     0x00000008L

/*
 *  SQL_FETCH_DIRECTION
 */
#define SQL_FD_FETCH_NEXT       0x00000001L
#define SQL_FD_FETCH_FIRST      0x00000002L
#define SQL_FD_FETCH_LAST       0x00000004L
#define SQL_FD_FETCH_PRIOR      0x00000008L
#define SQL_FD_FETCH_ABSOLUTE   0x00000010L
#define SQL_FD_FETCH_RELATIVE   0x00000020L
#define SQL_FD_FETCH_RESUME     0x00000040L
#define SQL_FD_FETCH_BOOKMARK   0x00000080L

/*
 *  SQL_TXN_ISOLATION_OPTION
 */
#define SQL_TXN_READ_UNCOMMITTED    0x00000001L
#define SQL_TXN_READ_COMMITTED      0x00000002L
#define SQL_TXN_REPEATABLE_READ     0x00000004L
#define SQL_TXN_SERIALIZABLE        0x00000008L
#define SQL_TXN_VERSIONING          0x00000010L

/*
 *  SQL_CORRELATION_NAME
 */
#define SQL_CN_NONE             0x0000
#define SQL_CN_DIFFERENT        0x0001
#define SQL_CN_ANY              0x0002

/*
 * SQL_NON_NULLABLE_COLUMNS
 */
#define SQL_NNC_NULL            0x0000
#define SQL_NNC_NON_NULL        0x0001

/*
 *  SQL_NULL_COLLATION
 */
#define SQL_NC_HIGH             0x0000
#define SQL_NC_LOW              0x0001
#define SQL_NC_START            0x0002
#define SQL_NC_END              0x0004

/*
 * SQL_FILE_USAGE
 */
#define SQL_FILE_NOT_SUPPORTED  0x0000
#define SQL_FILE_TABLE          0x0001
#define SQL_FILE_QUALIFIER      0x0002

/*
 *  SQL_GETDATA_EXTENSIONS
 */
#define SQL_GD_ANY_COLUMN       0x00000001L
#define SQL_GD_ANY_ORDER        0x00000002L
#define SQL_GD_BLOCK            0x00000004L
#define SQL_GD_BOUND            0x00000008L

/*
 * SQL_ALTER_TABLE
 */
#define SQL_AT_ADD_COLUMN       0x00000001L
#define SQL_AT_DROP_COLUMN      0x00000002L

/*
 *  SQL_POSITIONED_STATEMENTS
 */
#define SQL_PS_POSITIONED_DELETE    0x00000001L
#define SQL_PS_POSITIONED_UPDATE    0x00000002L
#define SQL_PS_SELECT_FOR_UPDATE    0x00000004L

/*
 *  SQL_GROUP_BY
 */
#define SQL_GB_NOT_SUPPORTED            0x0000
#define SQL_GB_GROUP_BY_EQUALS_SELECT   0x0001
#define SQL_GB_GROUP_BY_CONTAINS_SELECT 0x0002
#define SQL_GB_NO_RELATION              0x0003

/*
 *  SQL_OWNER_USAGE
 */
#define SQL_OU_DML_STATEMENTS       0x00000001L
#define SQL_OU_PROCEDURE_INVOCATION 0x00000002L
#define SQL_OU_TABLE_DEFINITION     0x00000004L
#define SQL_OU_INDEX_DEFINITION     0x00000008L
#define SQL_OU_PRIVILEGE_DEFINITION 0x00000010L

/*
 * SQL_QUALIFIER_USAGE
 */
#define SQL_QU_DML_STATEMENTS       0x00000001L
#define SQL_QU_PROCEDURE_INVOCATION 0x00000002L
#define SQL_QU_TABLE_DEFINITION     0x00000004L
#define SQL_QU_INDEX_DEFINITION     0x00000008L
#define SQL_QU_PRIVILEGE_DEFINITION 0x00000010L

/*
 *  SQL_SUBQUERIES
 */
#define SQL_SQ_COMPARISON               0x00000001L
#define SQL_SQ_EXISTS                   0x00000002L
#define SQL_SQ_IN                       0x00000004L
#define SQL_SQ_QUANTIFIED               0x00000008L
#define SQL_SQ_CORRELATED_SUBQUERIES    0x00000010L

/*
 *  SQL_UNION
 */
#define SQL_U_UNION         0x00000001L
#define SQL_U_UNION_ALL     0x00000002L

/*
 *  SQL_BOOKMARK_PERSISTENCE
 */
#define SQL_BP_CLOSE        0x00000001L
#define SQL_BP_DELETE       0x00000002L
#define SQL_BP_DROP         0x00000004L
#define SQL_BP_TRANSACTION  0x00000008L
#define SQL_BP_UPDATE       0x00000010L
#define SQL_BP_OTHER_HSTMT  0x00000020L
#define SQL_BP_SCROLL       0x00000040L

/*
 * SQL_STATIC_SENSITIVITY
 */
#define SQL_SS_ADDITIONS    0x00000001L
#define SQL_SS_DELETIONS    0x00000002L
#define SQL_SS_UPDATES      0x00000004L

/*
 *  SQL_LOCK_TYPES
 */
#define SQL_LCK_NO_CHANGE   0x00000001L
#define SQL_LCK_EXCLUSIVE   0x00000002L
#define SQL_LCK_UNLOCK      0x00000004L

/*
 *  SQL_POS_OPERATIONS
 */
#define SQL_POS_POSITION    0x00000001L
#define SQL_POS_REFRESH     0x00000002L
#define SQL_POS_UPDATE      0x00000004L
#define SQL_POS_DELETE      0x00000008L
#define SQL_POS_ADD         0x00000010L

/*
 *  SQL_QUALIFIER_LOCATION
 */
#define SQL_QL_START        0x0001L
#define SQL_QL_END          0x0002L

/*
 *  SQL_OJ_CAPABILITIES
 */
#define SQL_OJ_LEFT                 0x00000001L
#define SQL_OJ_RIGHT                0x00000002L
#define SQL_OJ_FULL                 0x00000004L
#define SQL_OJ_NESTED               0x00000008L
#define SQL_OJ_NOT_ORDERED          0x00000010L
#define SQL_OJ_INNER                0x00000020L
#define SQL_OJ_ALL_COMPARISON_OPS   0x00000040L

/*
 *  SQLGetStmtOption/SQLSetStmtOption
 */
#define SQL_QUERY_TIMEOUT   0
#define SQL_MAX_ROWS        1
#define SQL_NOSCAN          2
#define SQL_MAX_LENGTH      3
#define SQL_ASYNC_ENABLE    4
#define SQL_BIND_TYPE       5
#define SQL_CURSOR_TYPE     6
#define SQL_CONCURRENCY     7
#define SQL_KEYSET_SIZE     8
#define SQL_ROWSET_SIZE     9
#define SQL_SIMULATE_CURSOR 10
#define SQL_RETRIEVE_DATA   11
#define SQL_USE_BOOKMARKS   12
#define SQL_GET_BOOKMARK    13
#define SQL_ROW_NUMBER      14

#define SQL_STMT_OPT_MIN    SQL_QUERY_TIMEOUT
#define SQL_STMT_OPT_MAX    SQL_ROW_NUMBER


/*
 * SQL_QUERY_TIMEOUT
 */
#define SQL_QUERY_TIMEOUT_DEFAULT 0UL

/*
 *  SQL_MAX_ROWS
 */
#define SQL_MAX_ROWS_DEFAULT 0UL

/*
 *  SQL_NOSCAN
 */
#define SQL_NOSCAN_OFF      0UL /* 1.0 FALSE */
#define SQL_NOSCAN_ON       1UL /* 1.0 TRUE */
#define SQL_NOSCAN_DEFAULT  SQL_NOSCAN_OFF

/*
 *  SQL_MAX_LENGTH
 */
#define SQL_MAX_LENGTH_DEFAULT      0UL

/*
 *  SQL_ASYNC_ENABLE
 */
#define SQL_ASYNC_ENABLE_OFF        0UL
#define SQL_ASYNC_ENABLE_ON         1UL
#define SQL_ASYNC_ENABLE_DEFAULT    SQL_ASYNC_ENABLE_OFF

/*
 *  SQL_BIND_TYPE
 */
#define SQL_BIND_BY_COLUMN          0UL
#define SQL_BIND_TYPE_DEFAULT       SQL_BIND_BY_COLUMN

/*
 *  SQL_CONCURRENCY
 */
#define SQL_CONCUR_READ_ONLY        1
#define SQL_CONCUR_LOCK             2
#define SQL_CONCUR_ROWVER           3
#define SQL_CONCUR_VALUES           4
#define SQL_CONCUR_DEFAULT          SQL_CONCUR_READ_ONLY

/*
 *  SQL_CURSOR_TYPE
 */
#define SQL_CURSOR_FORWARD_ONLY     0UL
#define SQL_CURSOR_KEYSET_DRIVEN    1UL
#define SQL_CURSOR_DYNAMIC          2UL
#define SQL_CURSOR_STATIC           3UL
#define SQL_CURSOR_TYPE_DEFAULT     SQL_CURSOR_FORWARD_ONLY

/*
 *  SQL_ROWSET_SIZE
 */
#define SQL_ROWSET_SIZE_DEFAULT     1UL

/*
 *  SQL_KEYSET_SIZE
 */
#define SQL_KEYSET_SIZE_DEFAULT     0UL

/*
 *  SQL_SIMULATE_CURSOR
 */
#define SQL_SC_NON_UNIQUE           0UL
#define SQL_SC_TRY_UNIQUE           1UL
#define SQL_SC_UNIQUE               2UL

/*
 *  SQL_RETRIEVE_DATA
 */
#define SQL_RD_OFF                  0UL
#define SQL_RD_ON                   1UL
#define SQL_RD_DEFAULT              SQL_RD_ON

/*
 *  SQL_USE_BOOKMARKS
 */
#define SQL_UB_OFF                  0UL
#define SQL_UB_ON                   1UL
#define SQL_UB_DEFAULT              SQL_UB_OFF

/*
 *  SQLSetConnectOption/SQLGetConnectOption
 */
#define SQL_ACCESS_MODE             101
#define SQL_AUTOCOMMIT              102
#define SQL_LOGIN_TIMEOUT           103
#define SQL_OPT_TRACE               104
#define SQL_OPT_TRACEFILE           105
#define SQL_TRANSLATE_DLL           106
#define SQL_TRANSLATE_OPTION        107
#define SQL_TXN_ISOLATION           108
#define SQL_CURRENT_QUALIFIER       109
#define SQL_ODBC_CURSORS            110
#define SQL_QUIET_MODE              111
#define SQL_PACKET_SIZE             112

#define SQL_CONN_OPT_MIN            SQL_ACCESS_MODE
#define SQL_CONN_OPT_MAX            SQL_PACKET_SIZE
#define SQL_CONNECT_OPT_DRVR_START  1000


/*
 *  SQL_ACCESS_MODE
 */
#define SQL_MODE_READ_WRITE         0UL
#define SQL_MODE_READ_ONLY          1UL
#define SQL_MODE_DEFAULT            SQL_MODE_READ_WRITE

/*
 *  SQL_AUTOCOMMIT
 */
#define SQL_AUTOCOMMIT_OFF          0UL
#define SQL_AUTOCOMMIT_ON           1UL
#define SQL_AUTOCOMMIT_DEFAULT      SQL_AUTOCOMMIT_ON

/*
 *  SQL_LOGIN_TIMEOUT
 */
#define SQL_LOGIN_TIMEOUT_DEFAULT   15UL

/*
 *  SQL_OPT_TRACE
 */
#define SQL_OPT_TRACE_OFF           0UL
#define SQL_OPT_TRACE_ON            1UL
#define SQL_OPT_TRACE_DEFAULT       SQL_OPT_TRACE_OFF
#define SQL_OPT_TRACE_FILE_DEFAULT  "odbc.log"

/*
 *  SQL_ODBC_CURSORS
 */
#define SQL_CUR_USE_IF_NEEDED       0UL
#define SQL_CUR_USE_ODBC            1UL
#define SQL_CUR_USE_DRIVER          2UL
#define SQL_CUR_DEFAULT             SQL_CUR_USE_DRIVER

/*
 *  SQLSpecialColumns - Column types and scopes
 */
#define SQL_BEST_ROWID              1
#define SQL_ROWVER                  2

#define SQL_SCOPE_CURROW            0
#define SQL_SCOPE_TRANSACTION       1
#define SQL_SCOPE_SESSION           2

/*
 *  SQLSetPos
 */
#define SQL_ENTIRE_ROWSET           0

/*
 *  SQLSetPos
 */
#define SQL_POSITION                0
#define SQL_REFRESH                 1
#define SQL_UPDATE                  2
#define SQL_DELETE                  3
#define SQL_ADD                     4

/*
 *  SQLSetPos
 */
#define SQL_LOCK_NO_CHANGE          0
#define SQL_LOCK_EXCLUSIVE          1
#define SQL_LOCK_UNLOCK             2

/*
 *  SQLSetPos
 */
#define SQL_POSITION_TO(hstmt,irow) \
    SQLSetPos(hstmt,irow,SQL_POSITION,SQL_LOCK_NO_CHANGE)
#define SQL_LOCK_RECORD(hstmt,irow,fLock) \
    SQLSetPos(hstmt,irow,SQL_POSITION,fLock)
#define SQL_REFRESH_RECORD(hstmt,irow,fLock) \
    SQLSetPos(hstmt,irow,SQL_REFRESH,fLock)
#define SQL_UPDATE_RECORD(hstmt,irow) \
    SQLSetPos(hstmt,irow,SQL_UPDATE,SQL_LOCK_NO_CHANGE)
#define SQL_DELETE_RECORD(hstmt,irow) \
    SQLSetPos(hstmt,irow,SQL_DELETE,SQL_LOCK_NO_CHANGE)
#define SQL_ADD_RECORD(hstmt,irow) \
    SQLSetPos(hstmt,irow,SQL_ADD,SQL_LOCK_NO_CHANGE)

/*
 *  All the ODBC keywords
 */
#define SQL_ODBC_KEYWORDS \
"ABSOLUTE,ACTION,ADA,ADD,ALL,ALLOCATE,ALTER,AND,ANY,ARE,AS,"\
"ASC,ASSERTION,AT,AUTHORIZATION,AVG,"\
"BEGIN,BETWEEN,BIT,BIT_LENGTH,BOTH,BY,CASCADE,CASCADED,CASE,CAST,CATALOG,"\
"CHAR,CHAR_LENGTH,CHARACTER,CHARACTER_LENGTH,CHECK,CLOSE,COALESCE,"\
"COBOL,COLLATE,COLLATION,COLUMN,COMMIT,CONNECT,CONNECTION,CONSTRAINT,"\
"CONSTRAINTS,CONTINUE,CONVERT,CORRESPONDING,COUNT,CREATE,CROSS,CURRENT,"\
"CURRENT_DATE,CURRENT_TIME,CURRENT_TIMESTAMP,CURRENT_USER,CURSOR,"\
"DATE,DAY,DEALLOCATE,DEC,DECIMAL,DECLARE,DEFAULT,DEFERRABLE,"\
"DEFERRED,DELETE,DESC,DESCRIBE,DESCRIPTOR,DIAGNOSTICS,DISCONNECT,"\
"DISTINCT,DOMAIN,DOUBLE,DROP,"\
"ELSE,END,END-EXEC,ESCAPE,EXCEPT,EXCEPTION,EXEC,EXECUTE,"\
"EXISTS,EXTERNAL,EXTRACT,"\
"FALSE,FETCH,FIRST,FLOAT,FOR,FOREIGN,FORTRAN,FOUND,FROM,FULL,"\
"GET,GLOBAL,GO,GOTO,GRANT,GROUP,HAVING,HOUR,"\
"IDENTITY,IMMEDIATE,IN,INCLUDE,INDEX,INDICATOR,INITIALLY,INNER,"\
"INPUT,INSENSITIVE,INSERT,INTEGER,INTERSECT,INTERVAL,INTO,IS,ISOLATION,"\
"JOIN,KEY,LANGUAGE,LAST,LEADING,LEFT,LEVEL,LIKE,LOCAL,LOWER,"\
"MATCH,MAX,MIN,MINUTE,MODULE,MONTH,MUMPS,"\
"NAMES,NATIONAL,NATURAL,NCHAR,NEXT,NO,NONE,NOT,NULL,NULLIF,NUMERIC,"\
"OCTET_LENGTH,OF,ON,ONLY,OPEN,OPTION,OR,ORDER,OUTER,OUTPUT,OVERLAPS,"\
"PAD,PARTIAL,PASCAL,PLI,POSITION,PRECISION,PREPARE,PRESERVE,"\
"PRIMARY,PRIOR,PRIVILEGES,PROCEDURE,PUBLIC,"\
"REFERENCES,RELATIVE,RESTRICT,REVOKE,RIGHT,ROLLBACK,ROWS,"\
"SCHEMA,SCROLL,SECOND,SECTION,SELECT,SEQUENCE,SESSION,SESSION_USER,SET,SIZE,"\
"SMALLINT,SOME,SPACE,SQL,SQLCA,SQLCODE,SQLERROR,SQLSTATE,SQLWARNING,"\
"SUBSTRING,SUM,SYSTEM_USER,"\
"TABLE,TEMPORARY,THEN,TIME,TIMESTAMP,TIMEZONE_HOUR,TIMEZONE_MINUTE,"\
"TO,TRAILING,TRANSACTION,TRANSLATE,TRANSLATION,TRIM,TRUE,"\
"UNION,UNIQUE,UNKNOWN,UPDATE,UPPER,USAGE,USER,USING,"\
"VALUE,,VARCHAR,VARYING,VIEW,WHEN,WHENEVER,WHERE,WITH,WORK,YEAR"

/*
 * ----------------------------------------------------------------------
 *  Level 2 Functions
 * ----------------------------------------------------------------------
 */

/*
 *  SQLExtendedFetch - fFetchType
 */
#define SQL_FETCH_NEXT          1
#define SQL_FETCH_FIRST         2
#define SQL_FETCH_LAST          3
#define SQL_FETCH_PRIOR         4
#define SQL_FETCH_ABSOLUTE      5
#define SQL_FETCH_RELATIVE      6
#define SQL_FETCH_BOOKMARK      8

/*
 *  SQLExtendedFetch - rgfRowStatus
 */
#define SQL_ROW_SUCCESS         0
#define SQL_ROW_DELETED         1
#define SQL_ROW_UPDATED         2
#define SQL_ROW_NOROW           3
#define SQL_ROW_ADDED           4
#define SQL_ROW_ERROR           5

/*
 *  SQLForeignKeys - UPDATE_RULE/DELETE_RULE
 */
#define SQL_CASCADE             0
#define SQL_RESTRICT            1
#define SQL_SET_NULL            2

/*
 *  SQLBindParameter - fParamType
 *  SQLProcedureColumns - COLUMN_TYPE
 */
#define SQL_PARAM_TYPE_UNKNOWN  0
#define SQL_PARAM_INPUT         1
#define SQL_PARAM_INPUT_OUTPUT  2
#define SQL_RESULT_COL          3
#define SQL_PARAM_OUTPUT        4
#define SQL_RETURN_VALUE        5

/*
 *  SQLSetParam to SQLBindParameter conversion
 */
#define SQL_PARAM_TYPE_DEFAULT  SQL_PARAM_INPUT_OUTPUT
#define SQL_SETPARAM_VALUE_MAX  (-1L)

/*
 *  SQLStatistics - fUnique
 */
#define SQL_INDEX_UNIQUE        0
#define SQL_INDEX_ALL           1

/*
 *  SQLStatistics - fAccuracy
 */
#define SQL_QUICK               0
#define SQL_ENSURE              1

/*
 *  SQLStatistics - TYPE
 */
#define SQL_TABLE_STAT          0
#define SQL_INDEX_CLUSTERED     1
#define SQL_INDEX_HASHED        2
#define SQL_INDEX_OTHER         3

/*
 *  SQLProcedures - PROCEDURE_TYPE
 */
#define SQL_PT_UNKNOWN          0
#define SQL_PT_PROCEDURE        1
#define SQL_PT_FUNCTION         2

/*
 *  SQLSpecialColumns - PSEUDO_COLUMN
 */
#define SQL_PC_UNKNOWN          0
#define SQL_PC_NOT_PSEUDO       1
#define SQL_PC_PSEUDO           2

/*
 *  Deprecated defines from prior versions of ODBC
 */
#define SQL_DATABASE_NAME           16
#define SQL_FD_FETCH_PREV           SQL_FD_FETCH_PRIOR
#define SQL_FETCH_PREV              SQL_FETCH_PRIOR
#define SQL_CONCUR_TIMESTAMP        SQL_CONCUR_ROWVER
#define SQL_SCCO_OPT_TIMESTAMP      SQL_SCCO_OPT_ROWVER
#define SQL_CC_DELETE               SQL_CB_DELETE
#define SQL_CR_DELETE               SQL_CB_DELETE
#define SQL_CC_CLOSE                SQL_CB_CLOSE
#define SQL_CR_CLOSE                SQL_CB_CLOSE
#define SQL_CC_PRESERVE             SQL_CB_PRESERVE
#define SQL_CR_PRESERVE             SQL_CB_PRESERVE
#define SQL_FETCH_RESUME            7
#define SQL_SCROLL_FORWARD_ONLY     0L
#define SQL_SCROLL_KEYSET_DRIVEN    (-1L)
#define SQL_SCROLL_DYNAMIC          (-2L)
#define SQL_SCROLL_STATIC           (-3L)
#define SQL_PC_NON_PSEUDO           SQL_PC_NOT_PSEUDO

#ifdef __cplusplus
extern "C" {
#endif
/*
 *  Level 1 function prototypes
 */
RETCODE SQL_API SQLColumns (HSTMT hstmt, UCHAR FAR * szTableQualifier,
    SWORD cbTableQualifier, UCHAR FAR * szTableOwner, SWORD cbTableOwner,
    UCHAR FAR * szTableName, SWORD cbTableName, UCHAR FAR * szColumnName,
    SWORD cbColumnName);
/* glt - Changed HWND to SQLHWND to match MSW header typing */
RETCODE SQL_API SQLDriverConnect (HDBC hdbc, SQLHWND hwnd,
    UCHAR FAR * szConnStrIn, SWORD cbConnStrIn, UCHAR FAR * szConnStrOut,
    SWORD cbConnStrOutMax, SWORD FAR * pcbConnStrOut, UWORD fDriverCompletion);
RETCODE SQL_API SQLGetConnectOption (HDBC hdbc, UWORD fOption, PTR pvParam);
RETCODE SQL_API SQLGetData (HSTMT hstmt, UWORD icol, SWORD fCType,
    PTR rgbValue, SDWORD cbValueMax, SDWORD FAR * pcbValue);
RETCODE SQL_API SQLGetFunctions (HDBC hdbc, UWORD fFunction,
    UWORD FAR * pfExists);
RETCODE SQL_API SQLGetInfo (HDBC hdbc, UWORD fInfoType, PTR rgbInfoValue,
    SWORD cbInfoValueMax, SWORD FAR * pcbInfoValue);
RETCODE SQL_API SQLGetStmtOption (HSTMT hstmt, UWORD fOption, PTR pvParam);
RETCODE SQL_API SQLGetTypeInfo (HSTMT hstmt, SWORD fSqlType);
RETCODE SQL_API SQLParamData (HSTMT hstmt, PTR FAR * prgbValue);
RETCODE SQL_API SQLPutData (HSTMT hstmt, PTR rgbValue, SDWORD cbValue);
RETCODE SQL_API SQLSetConnectOption (HDBC hdbc, UWORD fOption, UDWORD vParam);
RETCODE SQL_API SQLSetStmtOption (HSTMT hstmt, UWORD fOption, UDWORD vParam);
RETCODE SQL_API SQLSpecialColumns (HSTMT hstmt, UWORD fColType,
    UCHAR FAR * szTableQualifier, SWORD cbTableQualifier,
    UCHAR FAR * szTableOwner, SWORD cbTableOwner, UCHAR FAR * szTableName,
    SWORD cbTableName, UWORD fScope, UWORD fNullable);
RETCODE SQL_API SQLStatistics (HSTMT hstmt, UCHAR FAR * szTableQualifier,
    SWORD cbTableQualifier, UCHAR FAR * szTableOwner, SWORD cbTableOwner,
    UCHAR FAR * szTableName, SWORD cbTableName, UWORD fUnique, UWORD fAccuracy);
RETCODE SQL_API SQLTables (HSTMT hstmt, UCHAR FAR * szTableQualifier,
    SWORD cbTableQualifier, UCHAR FAR * szTableOwner, SWORD cbTableOwner,
    UCHAR FAR * szTableName, SWORD cbTableName, UCHAR FAR * szTableType,
    SWORD cbTableType);

/*
 *  Level 2 function prototypes
 */
RETCODE SQL_API SQLBrowseConnect (HDBC hdbc,
    UCHAR FAR * szConnStrIn, SWORD cbConnStrIn, UCHAR FAR * szConnStrOut,
    SWORD cbConnStrOutMax, SWORD FAR * pcbConnStrOut);
RETCODE SQL_API SQLColumnPrivileges (HSTMT hstmt,
    UCHAR FAR * szTableQualifier, SWORD cbTableQualifier,
    UCHAR FAR * szTableOwner, SWORD cbTableOwner, UCHAR FAR * szTableName,
    SWORD cbTableName, UCHAR FAR * szColumnName, SWORD cbColumnName);
RETCODE SQL_API SQLDataSources (HENV henv, UWORD fDirection,
    UCHAR FAR * szDSN, SWORD cbDSNMax, SWORD FAR * pcbDSN,
    UCHAR FAR * szDescription, SWORD cbDescriptionMax,
    SWORD FAR * pcbDescription);
RETCODE SQL_API SQLDescribeParam (HSTMT hstmt, UWORD ipar,
    SWORD FAR * pfSqlType, UDWORD FAR * pcbColDef, SWORD FAR * pibScale,
    SWORD FAR * pfNullable);
RETCODE SQL_API SQLExtendedFetch (HSTMT hstmt, UWORD fFetchType, SDWORD irow,
    UDWORD FAR * pcrow, UWORD FAR * rgfRowStatus);
RETCODE SQL_API SQLForeignKeys (HSTMT hstmt, UCHAR FAR * szPkTableQualifier,
    SWORD cbPkTableQualifier, UCHAR FAR * szPkTableOwner, SWORD cbPkTableOwner,
    UCHAR FAR * szPkTableName, SWORD cbPkTableName,
    UCHAR FAR * szFkTableQualifier, SWORD cbFkTableQualifier,
    UCHAR FAR * szFkTableOwner, SWORD cbFkTableOwner, UCHAR FAR * szFkTableName,
    SWORD cbFkTableName);
RETCODE SQL_API SQLMoreResults (HSTMT hstmt);
RETCODE SQL_API SQLNativeSql (HDBC hdbc, UCHAR FAR * szSqlStrIn,
    SDWORD cbSqlStrIn, UCHAR FAR * szSqlStr, SDWORD cbSqlStrMax,
    SDWORD FAR * pcbSqlStr);
RETCODE SQL_API SQLNumParams (HSTMT hstmt, SWORD FAR * pcpar);
RETCODE SQL_API SQLParamOptions (HSTMT hstmt, UDWORD crow, UDWORD FAR * pirow);
RETCODE SQL_API SQLPrimaryKeys (HSTMT hstmt, UCHAR FAR * szTableQualifier,
    SWORD cbTableQualifier, UCHAR FAR * szTableOwner, SWORD cbTableOwner,
    UCHAR FAR * szTableName, SWORD cbTableName);
RETCODE SQL_API SQLProcedureColumns (HSTMT hstmt, UCHAR FAR * szProcQualifier,
    SWORD cbProcQualifier, UCHAR FAR * szProcOwner, SWORD cbProcOwner,
    UCHAR FAR * szProcName, SWORD cbProcName, UCHAR FAR * szColumnName,
    SWORD cbColumnName);
RETCODE SQL_API SQLProcedures (HSTMT hstmt, UCHAR FAR * szProcQualifier,
    SWORD cbProcQualifier, UCHAR FAR * szProcOwner, SWORD cbProcOwner,
    UCHAR FAR * szProcName, SWORD cbProcName);
RETCODE SQL_API SQLSetPos (HSTMT hstmt, UWORD irow, UWORD fOption, UWORD fLock);
RETCODE SQL_API SQLTablePrivileges (HSTMT hstmt, UCHAR FAR * szTableQualifier,
    SWORD cbTableQualifier, UCHAR FAR * szTableOwner, SWORD cbTableOwner,
    UCHAR FAR * szTableName, SWORD cbTableName);

/*
 *  SDK 2.0 Additional function prototypes
 */
RETCODE SQL_API SQLDrivers (HENV henv, UWORD fDirection,
    UCHAR FAR * szDriverDesc, SWORD cbDriverDescMax, SWORD FAR * pcbDriverDesc,
    UCHAR FAR * szDriverAttributes, SWORD cbDrvrAttrMax,
    SWORD FAR * pcbDrvrAttr);
RETCODE SQL_API SQLBindParameter (HSTMT hstmt, UWORD ipar, SWORD fParamType,
    SWORD fCType, SWORD fSqlType, UDWORD cbColDef, SWORD ibScale, PTR rgbValue,
    SDWORD cbValueMax, SDWORD FAR * pcbValue);

/*
 *  Deprecated - use SQLSetStmtOptions
 */
RETCODE SQL_API SQLSetScrollOptions (HSTMT hstmt, UWORD fConcurrency,
    SDWORD crowKeyset, UWORD crowRowset);

#ifdef __cplusplus
}
#endif

#endif

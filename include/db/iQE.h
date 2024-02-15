/**
 * @file iQE.h
 * @author Rediet Worku aka Aethiops II ben Zahab (aethiopis2rises@gmail.com)
 * 
 * @brief iQE is an acronym for integerted Queries Engine, is an ODBC based library designed to interact with many
 *  RDBMS on the market today with none or little change to the implementation regardless of the RDBMS used.
 * @version 1.2
 * @date 2023-12-08
 * 
 * @copyright Copyright (c) 2023
 * 
 */
#if defined(_MSC_VER)           // compiling on Visual Studio?
#pragma once
#endif


#ifndef INTAPS_iQE_H
#define INTAPS_iQE_H




//=====================================================================================|
//          DEFINES
//=====================================================================================|
#define DB_ERR_BUFFER_SIZE        1024       /* maximum limit for ODBC buffer size */
#define DB_BULK_FETCH_SIZE        50         /* amout to fetch in chunks */




//=====================================================================================|
//          INCLUDES
//=====================================================================================|
#include "basics.h"
#include <sql.h>            /* odbc stuff */
#include <sqlext.h>




//=====================================================================================|
//          EXTERNS
//=====================================================================================|
// these global's are the basic bare bones required to access a RDBMS thru ODBC libs.
// NOTES: the application can define it's own variables whenever it desire's to choose 
//  one of its own
extern SQLHENV g_henv;       // an odbc en't handle; describes info about the odbc env't, 
    // must be the first to allocated in odbc
    
extern SQLHDBC g_hdbc;       // handle to database connectivity; used to connect to database
extern SQLHSTMT g_hstmt;     // handle to obdc statment; used to run quries and stuff


/**
 * these buffers are used and are set during function call erros; their consitency is 
 *  not guranteed up until a function call is made and return's only with an error.
 */
extern SQLCHAR gsz_err_buffer[MAXPATH][DB_ERR_BUFFER_SIZE];    // dump's error's as human readable string
extern SQLCHAR gsz_odbc_state[MAXPATH][6];                     // a five character code indicating the error
extern SQLINTEGER gn_odbc_native[MAXPATH];                     // the native error code
extern int gn_err_count;                                        // store's the count of error's returned in our buffer








//=====================================================================================|
//          MACROS
//=====================================================================================|
/**
 * this macro expands to initalze the ODBC handle passed as the argument. This is more 
 *  of a generic wrapper.
 * 
 * ARGUMENTS:
 *  handle_type :   a 32-bit descriptor that identifies the type of handle for odbc
 *  parent_handle:  another 32-bit decriptor that identifies the parent for this new handle
 *  handle:         a pointer to store the newly allocated handle
 */
#define DB_ALLOC_HANDLE(handle_type, parent_handle, handle)   SQLAllocHandle(handle_type, parent_handle, &handle)



/**
 * this macro expands to allocate handle for odbc environment; the macro takes the 
 *  global g_henv as a handle parameter.
 */
#define DB_ALLOC_ENVT_HANDLE    DB_ALLOC_HANDLE(SQL_HANDLE_ENV, NULL, g_henv)




/**
 * this is another specific macro that allocates handle to database connectivity, 
 *  the macro uses the global g_hdbc as a parameter and g_henv as handle to parent.
 */
#define DB_ALLOC_DBC_HANDLE     DB_ALLOC_HANDLE(SQL_HANDLE_DBC, g_henv, g_hdbc)




/**
 * macro specifically expands to allocate handle for statmet, it uses the global's 
 *  g_hdbc and g_hstmt as statment handle.
 */
#define DB_ALLOC_STMT_HANDLE    DB_ALLOC_HANDLE(SQL_HANDLE_STMT, g_hdbc, g_hstmt)




/**
 * this macro expands to call an odbc function that is used to free up all the used 
 *  handles used..
 * 
 * ARGUMENTS:
 *  handle_type:    the type of handle to free
 *  handle:         the pointer to release
 */
#define DB_FREE_HANDLE(handle_type, handle)   SQLFreeHandle(handle_type, handle)




/**
 * this macro expands to free the global environment handle g_henv.
 */
#define DB_FREE_ENVT_HANDLE     DB_FREE_HANDLE(SQL_HANDLE_ENV, g_henv)



/**
 * expands to free the global contectivty handle.
 * WARNING: macro does not close an open connection; users should first disconnect.
 */
#define DB_FREE_DBC_HANDLE      DB_FREE_HANDLE(SQL_HANDLE_DBC, g_hdbc)


/**
 * expands to free odbc statment handle; the global g_hstmt is at play here
 */
#define DB_FREE_STMT_HANDLE      DB_FREE_HANDLE(SQL_HANDLE_STMT, g_hstmt)



/**
 * this macro is used to set the connection attributes; i.e. change the mode from auto-commit 
 *  transaction mode to manual-commit and vice-versa
 *  
 * ARGUMENTS:
 *  handle  : the database connection handle
 *  b_value : a TRUE or FALSE; when TRUE auto-commit is enabled (default for ODBC)
 */
#define DB_SET_CONN_ATTR(handle, b_value)   SQLSetConnectAttr(handle, SQL_ATTR_AUTOCOMMIT, (SQLPOINTER)b_value, 0)



/**
 * this macro expand to SQLPrepare which is used to prepare a query that will most likely be run muliple
 *  times possible with changing paramters; this makes our query operation much much faster than before.
 * 
 * ARGUMENTS:
 *  handle  : the statment handle
 *  sz_qwery: the query statment to prepare (possibly with parameters stripped off).
 */
#define DB_PREPARE_QUERY(handle, sz_qwery)  SQLPrepare(handle, (SQLCHAR *)sz_qwery, SQL_NTS)



/**
 * this macro expands to run SQLFetch function of ODBC. Use this macro to fetch the results
 *  of an sql query; once having done that, one can use DB_GET_DATA or something similar to
 *  store the actual resutls in a buffer (if the result set is not already bounded).
 * 
 * ARGUMENTS:
 *  handle  : ODBC statment handle
 */
#define DB_FETCH(handle)  SQLFetch(handle)



/**
 * this macro is used to fetch multiple rows at once; this is quite effiencent when compared to fetching
 *  long lists one by one; on the contrary it becomes ideal tool for fetching large data in a single call,
 *  in combination with prepared query, this may result in signifcant speed boosts.
 * 
 * ARGUMENTS:
 *  handle      : the sql statment handle
 *  row_count   : get's the total number of rows actually read as opposed to desired length
 *  row_buffer  : buffer to store the results of the fetch (must be large enough to accomidate for the bulk)
 */
#define DB_BULK_FETCH(handle, row_count, row_buffer)    SQLExtendedFetch(handle, SQL_FETCH_NEXT, 0, \
    &row_count, row_buffer)



/**
 * this macro expands to acquire the results of a query operation given it's type and
 * column number (start's from 1 in ODBC not 0) one column at a time; the caller must repatedly
 * expand this macro to acquire all results of a column followed by the row (nested loops). This
 * makes for a less effiencent method to acquire data; a more faster implementation would return
 * one row at a time at least or even faster a bulk of rows.
 * 
 * ARGUMENTS:
 *  handle  : an ODBC statment handle
 *  n_col   : the column to fetch data from
 *  buffer  : a buffer to store the result
 *  n_buf_type : type of buffer (one of those defined constants for ODBC)
 */
#define DB_GET_DATA(handle, n_col, buffer, n_buf_type, n_len) \
    SQLGetData(handle, n_col, n_buf_type, (SQLPOINTER)&buffer, sizeof(buffer), &n_len)

    


/**
 * this macro expands to SQLBindCol ODBC lib; this is a slightly faster way of returning
 *  result set's as opposed to the SQLGetData function; the macro gains it's speed advantage
 *  because the query run is known prior to program compliation (there is a more faster way
 *  for kooked quries however). The macro uses the global: g_hstmt.
 * 
 * ARGUMENTS:
 *  n_col_num   : the column number to bind
 *  n_type      : the type for the column (more faster if type is native rather than casted)
 *  p_value     : pointer to the buffer that get's the result of the query
 *  n_len       : pointer to other sql indicator
 */
#define DB_BIND_COL(n_col_num, n_type, p_value, n_len)    \
    SQLBindCol(g_hstmt, n_col_num, n_type, (SQLPOINTER)p_value, sizeof(p_value), &n_len)
    


/**
 * this macro expands to SQLBindParameter which used during prepared execution; which is fast for
 *  queries running more than once (multiple times); say during the payment app.
 * 
 * ARGUMENTS:
 *  handle  : the odbc statment handle
 *  col_num : the column number for this object
 *  table_type : the datatype in the table 
 *  param_type : the type in the code i.e. the variable type
 *  col_length : column length ?
 *  decimal_length : number of decimal points (applicable to floats and doubles)
 *  buf     : the storage buffer; i.e. the input parameter (or output incase of OUT param)
 *  buf_len : the length of buffer (0 for non-string buffers)
 *  indicator : ??? (consult an ODBC manual)
 */
#define DB_BIND_IN_PAR(handle, col_num, table_type, param_type, col_length, decimal_length, buf, buf_len, indicator) \
    SQLBindParameter(handle, col_num, SQL_PARAM_INPUT, table_type, param_type, col_length, decimal_length, \
    (SQLPOINTER)buf, buf_len, &indicator)
    
  
    

/**
 * like it's counter part above this is used for output paramters; i.e. things that require select statment
 *
 * ARGUMENTS:
 *     see above DB_BIND_IN_PAR
 */
#define DB_BIND_OUT_PAR(handle, col_num, table_type, param_type, col_length, decimal_length, buf, buf_len, indicator) \
    SQLBindParameter(handle, col_num, SQL_PARAM_OUTPUT, table_type, param_type, col_length, decimal_length, \
    (SQLPOINTER)buf, buf_len, &indicator)
    
    

/**
 * this macro is called after query statments have been executed and is used to
 *  close any open statment cursors. unless closed new query operations won't work.
 * 
 * ARGUMENTS:
 *  hstat   : handle to the open statment
 */
#define DB_CLOSE_CURSOR(hstmt)    SQLCloseCursor(hstmt)




/**
 * this macro expands to SQLSetStmtAttr ODBC function, which is used to set some attributes
 *  regranding to the statment handle; I use this for fetching multiple rows at once with a
 *  single call for high speed performance. 
 * NOTE: to get the results back use macro DB_BULK_FETCH as it uses SQLExtendedFetch to match
 *  with macro
 * 
 * ARGUMENTS:
 *  handle  : the statment handle (refer to ODBC manuals on ODBC handles be it statment or envt)
 *  size    : the size of the rows to fetch at once (from 2 ... size)
 */
#define DB_SET_STMT_ATTR(handle, size)  SQLSetStmtAttr(handle, SQL_ROWSET_SIZE, (SQLPOINTER)size, 0)




/**
 * this macro expands to call SQLGetDiagRec which is used to dump any error messages that occured during
 *  function calls to a global buffer sz_err_buffer as human readble text. This function can dump up to
 *  260 errors in one buffer; normally up to the size of MAX_PATH value.
 * 
 * ARGUMENTS:
 *  handle_type:    the type of handle that caused the offense
 *  handle:         the offending handle
 * 
 * ERRORS:
 *  set's the global rc when function calls fail.
 */
#define DB_EXTRACT_ERROR(handle_type, handle) { \
    SQLSMALLINT len;    /* actual length of string */ \
    u16 i;              /* row counter */ \
    SQLRETURN rc;       /* odbc function call results */ \
    for (i = 0; i < MAXPATH; i++) { \
        rc = SQLGetDiagRec(handle_type, handle, i + 1, gsz_odbc_state[i], &gn_odbc_native[i], \
                           gsz_err_buffer[i], DB_ERR_BUFFER_SIZE, &len); \
        if (rc != SQL_SUCCESS) break; \
    } /* end for */ \
    gn_err_count = i; \
} // end ODBC_EXTRACT_ERROR









//=====================================================================================|
//          PROTOTYPES
//=====================================================================================|
namespace iQE 
{
    int Init_ODBC(HENV &henv = g_henv, HDBC &hdbc = g_hdbc);
    int Shutdown_ODBC(HENV &henv = g_henv, HDBC &hdbc = g_hdbc, HSTMT &hstmt = g_hstmt);


    //int Connect_DB(SQLCHAR *sz_dsn, HDBC hdbc = g_hdbc);
    int Driver_Connect_DB(SQLCHAR *sz_connection, HDBC &hdbc = g_hdbc, HSTMT &hstmt = g_hstmt);
    int Disconnect_DB(HDBC hdbc = g_hdbc);


    int Run_Query_Direct(SQLCHAR *sz_qwery, HSTMT hstmt = g_hstmt);
    int Get_Column_Count(HSTMT hstmt = g_hstmt);


    /**
     * Utlilty catalog functions
     */
    int List_Drivers(SQLCHAR *sz_driver, const int n_drive_len, SQLCHAR *sz_attr, const int n_attr_len,
                    SQLSMALLINT n_direction, HENV henvt = g_henv);
    int List_Tables(SQLCHAR *sz_catalog, SQLCHAR *sz_schema, SQLCHAR *sz_table, HSTMT hstmt = g_hstmt);


    /**
     * Error handlers
     */
    void Dump_DB_Error(void);
} //end iQE namespace






#endif
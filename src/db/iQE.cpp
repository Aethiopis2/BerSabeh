/**
 * @file iQE.cpp
 * @author Rediet Worku aka Aethiops II ben Zahab (aethiopis2rises@gmail.com)
 * 
 * @brief implementation details for the prototypes at iQE.h
 * @version 1.2
 * @date 2023-12-08
 * 
 * @copyright Copyright (c) 2023
 * 
 */
#if defined(_MSC_VER)
#pragma warning(disable:4996)           /* disable the legacy error crap on Microsoft's compiler */
#endif




//=====================================================================================|
//          INCLUDES
//=====================================================================================|
#include "iQE.h"






//=====================================================================================|
//          GLOBALS
//=====================================================================================|
// the basic and important ODBC based globals; 
//  NOTE: The calling app, can define it's own values and disregrad these variables 
//  all together, so long as the calling app, complies to the standard of ODBC
SQLHENV g_henv   = NULL;         /* environment handle */
SQLHDBC g_hdbc   = NULL;         /* database connectivity handle */
SQLHSTMT g_hstmt = NULL;         /* statment handle */



// these buffers are used for dumping of errors that may have occured druing ODBC function calls.
SQLCHAR gsz_err_buffer[MAXPATH][DB_ERR_BUFFER_SIZE];   /* dumps errors as strings, up to 260 of them */
SQLCHAR gsz_odbc_state[MAXPATH][6];                    /* dumps the 5-character state codes; the 6th is for null */
SQLINTEGER gn_odbc_native[MAXPATH];                    /* dumps the native error code as SQLINTEGER */
int gn_err_count;                                      /* count of error's from ODBC function calls. */





//=====================================================================================|
//          FUNCTIONS
//=====================================================================================|
/**
 * @brief This function is the first function that should be called when accessing 
 *  RDBMS's from ODBC libs. The function initalizes the global handles and makes them 
 *  ready for access. This function uses the global ODBC handles; 
 * 
 * @param henv handle to environment to allocated
 * @param hdbc handle to databse connectivity
 * @return int 0 on success a -1 on fail with global gz_err_buffer containing description 
 */
int iQE::Init_ODBC(HENV &henv, HDBC &hdbc) 
{
    // test the return code
    if (!SQL_SUCCEEDED(DB_ALLOC_HANDLE(SQL_HANDLE_ENV, NULL, henv)))
    {
        DB_EXTRACT_ERROR(SQL_HANDLE_ENV, henv);
        return -1;
    } // end if
    
    // set to ODBC version 3
    if (!SQL_SUCCEEDED(SQLSetEnvAttr(henv, SQL_ATTR_ODBC_VERSION, (void *)SQL_OV_ODBC3, 0)))
    {
        DB_EXTRACT_ERROR(SQL_HANDLE_ENV, henv);
        return -1;
    } // end if fail to set ODBC ver 3
    
    
    // at the very last allocate handle for database connectivity
    if (!SQL_SUCCEEDED(DB_ALLOC_HANDLE(SQL_HANDLE_DBC, henv, hdbc)))
    {
        DB_EXTRACT_ERROR(SQL_HANDLE_DBC, hdbc);
        return -1;
    } // end if dbc alloc fail
    
    // success
    return 0;
} // end Init_ODBC



//=====================================================================================|
/**
 * @brief This function is used to perform a little house-cleaning, it is called when 
 *  the application is done with ODBC stuff. The function makes sure that all resources 
 *  have been properly release and avoids memory leaks; this is kinda like Java's GC.
 * 
 * @param henv handle to envt
 * @param hdbc handle to database connectivity
 * @param hstmt handle to statment
 * @return int 0 on success -1 on fail
 */
int iQE::Shutdown_ODBC(HENV &henv, HDBC &hdbc, HSTMT &hstmt)
{
    // here we free our handle's in reverse order of allocation, i.e. we start from the 
    //  last item that could possibly be allocated ...
    
    // kill off the statment handle
    if (hstmt)
    {
        if (!SQL_SUCCEEDED(DB_FREE_HANDLE(SQL_HANDLE_STMT, hstmt)))
        {
            DB_EXTRACT_ERROR(SQL_HANDLE_STMT, hstmt);
            return -1;
        } // end if error
        
        hstmt = nullptr;       // double tap
    } // end if hstmt
    
    // now for the database connectivity
    if (hdbc)
    {
        // disconnect first if connected
        Disconnect_DB(hdbc);
        if (!SQL_SUCCEEDED(DB_FREE_HANDLE(SQL_HANDLE_DBC, hdbc)))
        {
            DB_EXTRACT_ERROR(SQL_HANDLE_DBC, hdbc);
            return -1;
        } // end if error
        
        hdbc = nullptr;
    } // end if hdbc
    
    // at the very last the env't handle
    if (henv)
    {
        if (!SQL_SUCCEEDED(DB_FREE_HANDLE(SQL_HANDLE_ENV, henv)))
        {
            DB_EXTRACT_ERROR(SQL_HANDLE_ENV, henv);
            return -1;
        } // end if error
        
        henv = nullptr;
    } // end if henv
    
    // success
    return 0;
} // end Shutdown_ODBC



//=====================================================================================|
/**
 * @brief This function is used to connect with RDBMS using it's native driver thu a 
 *  connection string. For an application to connect, the connection_string at minimum 
 *  must contain the following info; driver name, server_name (instance name) and user 
 *  credientials if connecting thru LAN or trusted connection locally. Optionally the string 
 *  can contain the catalog name (database name) to connect with.
 * 
 * @param sz_connection the null terminated connection string
 * @param hdbc handle to obdc db; deafults to global
 * @param hstmt a reference to a statment handle to allocate
 * @return int 0 on success -1 on fail with deatils pulled thru Dump_Err function
 */
int iQE::Driver_Connect_DB(SQLCHAR *sz_connection, HDBC &hdbc, HSTMT &hstmt)
{
    SQLRETURN ret;                              // get's return values from functions
    SQLCHAR sz_conn_out[DB_ERR_BUFFER_SIZE];   // buffer to store the full connection string returned from driver
        
    SQLSMALLINT n_out_conn_len;                 // length of connection string returned above (set by driver)
    
    
    ret = SQLDriverConnect(
        hdbc,                               /* handle to database connectivity */
        NULL,                               /* Window handle; Windows stuff, just ignore */
        sz_connection,                      /* connection string passed from application */
        SQL_NTS,                            /* means the above argument is null terminated string */
        sz_conn_out,                        /* buffer to get the full connection string from driver */
        sizeof(sz_conn_out) >> 1,           /* length of buffer above (don't know why we have to divide) */
        &n_out_conn_len,                    /* length of the above string actually returned from driver */
        SQL_DRIVER_COMPLETE);               /* let the driver finish off the remaing strings */
    
    // test error
    if (!SQL_SUCCEEDED(ret))
    {
        DB_EXTRACT_ERROR(SQL_HANDLE_DBC, hdbc);
        return -1;
    } // end if error connecting
    
    if (!SQL_SUCCEEDED(DB_ALLOC_HANDLE(SQL_HANDLE_STMT, hdbc, hstmt)))
    {
        DB_EXTRACT_ERROR(SQL_HANDLE_STMT, hstmt);
        return -1;
    } // end if error with statment allocation

    // success
    return 0;
} // end Driver_Connect_DB




//=====================================================================================|
/**
 * @brief This function terminates a connection and cleans up memory.
 * 
 * @param hdbc the db statment handle to disconnect from
 * @return int 0 on success -1 on fail
 */
int iQE::Disconnect_DB(HDBC hdbc)
{
    // do a trivial rejection
    if (!hdbc)
        return 0;       /* no error no nothing; already disconnected */
    
    if ( !SQL_SUCCEEDED(SQLDisconnect(hdbc)) )
    {
        DB_EXTRACT_ERROR(SQL_HANDLE_DBC, hdbc);
        return -1;
    } // end if error
    
    // success
    return 0;
} // end Disconnect_DB



//=====================================================================================|
/**
 * @brief This function is used to run queries direct from user's entry; i.e. it makes 
 *  no preparations and bindgs and all, it's up to the caller to set these values 
 *  after the query call.
 * 
 * @param szw_qwery the query statment to run
 * @param hstmt a query statment handle
 * @return int 0 on success alas -1 on fail with globals having details on the error
 */
int iQE::Run_Query_Direct(SQLCHAR *sz_qwery, HSTMT hstmt)
{
    if (!SQL_SUCCEEDED( SQLExecDirect(hstmt, sz_qwery, SQL_NTS) ))
    {
        DB_EXTRACT_ERROR(SQL_HANDLE_STMT, hstmt);
        return(-1);
    } // end if
    
    // success
    return(0);
} // end Run_Query_Direct



//=====================================================================================|
/**
 * @brief this function returns the column count of a given dataset usually as a result 
 *  of a query operation that occured at runtime; i.e. due to a "SELECT *" kinda operation.
 * 
 * @param hstmt the statment handle that a qwery operation was performed on
 * @return int the number of columns on success alas -1
 */
int iQE::Get_Column_Count(HSTMT hstmt)
{
    SQLSMALLINT n_cols;     // get's our actual columns
    
    if ( !SQL_SUCCEEDED(SQLNumResultCols(hstmt, &n_cols)) )
    {
        DB_EXTRACT_ERROR(SQL_HANDLE_STMT, hstmt);
        return(-1);
    } // end if no col 
    
    // return our count
    return (int)n_cols;
} // end Get_Column_Count




//=====================================================================================|
//  UTLITIES
//=====================================================================================|
/**
 * @brief This function list's all ODBC drivers installed on the system. This function 
 *  can help the user decide which drivers to choose or if drivers are installed. 
 *  This function is meant to be called repeatdly possibly in a loop to list all drivers 
 *  installed on the system.
 * 
 * @param sz_driver buffer to receive the driver identifer as null terminated string
 * @param n_driver_len length of buffer in param 1
 * @param sz_attr buffer to store attributes related to the dirver.
 * @param n_attr_len length of buffer above
 * @param n_direction direction of fetch; we usually want to walk forward begining from the top
 * @param henvt an optional environmnet handle initialzed outside of this library
 * @return int a 0 when successfuly completed; a 1 to indicate successful compeltion but 
 *  data remains indicating that the function called again. a -ve to indicate error with 
 *  global error buffers with details on the error.
 */
int iQE::List_Drivers(SQLCHAR *sz_driver, const int n_driver_len, SQLCHAR *sz_attr, 
                 const int n_attr_len, SQLSMALLINT n_direction, HENV henvt)
{
    SQLRETURN ret;
    SQLSMALLINT n_driver_ret;       /* length of the driver string returned from function; ignored */
    SQLSMALLINT n_att_ret;          /* length of the attr buffer returned */
        
    ret = SQLDrivers(
        henvt,                          /* env't handle; */
        n_direction,                    /* fetch direction; usually forward */
        sz_driver,                      /* initalized to driver string after this call */
        (SQLSMALLINT)n_driver_len,      /* length of buffer sent from application */
        &n_driver_ret,                  /* actual length of the string */
        sz_attr,                        /* driver attributes */
        (SQLSMALLINT)n_attr_len,        /* length of buffer sent */
        &n_att_ret);                    /* length of the actual string attr */
    
    
    if (SQL_SUCCEEDED(ret))
        return 1;                   /* tell the calling application we can fetch more items. */
    else if (ret != SQL_NO_DATA)    /* something went wrong; find out what ... */
    {
        DB_EXTRACT_ERROR(SQL_HANDLE_ENV, henvt);
        return -1;
    } // end else if
    
    // success
    return 0;
} // end List_Drivers



//=====================================================================================|
/**
 * @brief this function list's all the table's present in a connected database; 
 * 
 * Notes: after running this function the caller must invoke the ODBC_GET_DATA macro to 
 * reterive results.
 *
 * @param sz_catalog this is the database name to search (set to null for everything)
 * @param sz_schema the schema for db (% for all schemas)
 * @param sz_table the table name (null for all tables)
 * @param hstmt handle to the statment 
 * @return int a 0 when successfuly completed; a -ve to indicate error with global error buffers having
 *   details on the error.
 */
int iQE::List_Tables(SQLCHAR *sz_catalog, SQLCHAR *sz_schema, SQLCHAR *sz_table, HSTMT hstmt)
{
    /* run the pre-kooked query for fetching tables using ODBC catalog functions */
    SQLRETURN ret = SQLTables(
        hstmt,                      /* handle to statment */ 
        sz_catalog,                 /* the db name or % for all */ 
        SQL_NTS,                    /* length of db name string */
        sz_schema,                  /* schema name */
        SQL_NTS,                    /* length for schema string, null terminated string */
        sz_table,                   /* table name */
        SQL_NTS,                    /* length of table name */
        (SQLCHAR *)"Table",         /* table type or view */
        SQL_NTS);                   /* length of table string */
    
    // test the return
    if (!SQL_SUCCEEDED(ret))
    {
        DB_EXTRACT_ERROR(SQL_HANDLE_STMT, hstmt);
        return -1;
    } // end if bad newz
    
    return 0;
} // end List_Tables




//=====================================================================================|
//  ERROR HANDLERS
//=====================================================================================|
/**
 * @brief this method dumps the messages of odbc function call to the standard error by 
 *  converting to a standard unicode encoding compatible with the operating system
 */
void iQE::Dump_DB_Error(void)
{
    int i;      // looping variable
    
    for (i = 0; i < gn_err_count; i++)
    {
        fprintf(stderr, "\n\033[31m*** ERROR: \033[37m%d --- %s --- %s \033[31m***\033[37m\n", 
                (int)gn_odbc_native[i], gsz_odbc_state[i], gsz_err_buffer[i]);
    } // end for 
} // end Dump_DB_Error
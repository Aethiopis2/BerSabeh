/**
 * @file Messages.h
 * @author Dr. Rediet Worku aka Aethiops ben Zahab (PanaceaSolutionsEth@gmail.com)
 * 
 * @brief Implementation of Messages class; simply maniuplates SMS stored messages
 *  from a database.
 * @version 0.1
 * @date 2024-01-17
 * 
 * @copyright Copyright (c) 2024
 * 
 */





//===============================================================================|
//          INCLUDES
//===============================================================================|
#include "messages.h"







//===============================================================================|
//          CLASS DEFINITION
//===============================================================================|
/**
 * @brief Construct a new Messages:: Messages object create an empty object to 
 *  initalaize at much later time
 * 
 */
Messages::Messages() 
    :db_state{DB_DISCONNECTED}, henv{nullptr}, hdbc{nullptr}, hstmt{nullptr} {}



//===============================================================================|
/**
 * @brief Construct a new Messages:: Messages object constructor that start's by
 *  connecting with db.
 * 
 * @param con_str ODBC formatted connection string
 * 
 * @throw runtime_error when connection with db fails.
 */
Messages::Messages(const std::string &con_str)
{
    if (Connect_DB(con_str) < 0)
        throw std::runtime_error("DB connection failed.");
} // end constructor




//===============================================================================|
/**
 * @brief Destroy the Messages:: Messages object
 * 
 */
Messages::~Messages()
{
    Disconnect_DB();
} // end Destructor



//===============================================================================|
/**
 * @brief Connectes to a database using ODBC driver by using ODBC formatted 
 *  connection string to database
 * 
 * @param con_str the ODBC connection string to db
 * @return int 0 on success alas -1.
 */
int Messages::Connect_DB(const std::string &con_str)
{
    if (!(db_state & DB_CONNECTED))
    {
        if (iQE::Init_ODBC(henv, hdbc) < 0)
            return -1;

        if (iQE::Driver_Connect_DB((SQLCHAR*)con_str.c_str(), hdbc, hstmt) < 0)
            return -1;   

        db_state = (DB_CONNECTED | DB_CUR_CLOSED);
    } // end if already connected

    return 0;
} // end Connect_DB



//===============================================================================|
/**
 * @brief Disconnects db and terminates all active sessions.
 * 
 * @return int 0 on success, -1 on error
 */
int Messages::Disconnect_DB()
{
    if (db_state & DB_CONNECTED)
    {
        if (iQE::Shutdown_ODBC(henv, hdbc, hstmt) < 0)
            return -1;

        db_state = DB_DISCONNECTED; 
    } // end db_state

    return 0;
} // end Disconnect_DB



//===============================================================================|
void Messages::Get_Unsent_Messages(SmsOut_Ptr pmsg)
{
    //SQLSMALLINT cols;
    SQLLEN len;

    std::string qwery = "SELECT * FROM Subscriber.dbo.SmsOut WHERE status <= 2";
    iQE::Run_Query_Direct((SQLCHAR*)qwery.c_str(), hstmt);

    iZero(&sms_out, sizeof(sms_out));
    SQLBindCol(hstmt, 1, SQL_C_SLONG, (SQLPOINTER)&sms_out.id, 0, &len);
    SQLBindCol(hstmt, 2, SQL_C_CHAR, (SQLPOINTER)&sms_out.phoneno, sizeof(sms_out.phoneno), &len);
    SQLBindCol(hstmt, 3, SQL_C_CHAR, (SQLPOINTER)&sms_out.message, sizeof(sms_out.message), &len);
    SQLBindCol(hstmt, 4, SQL_C_SBIGINT, (SQLPOINTER)&sms_out.logTicks, 0, &len);
    SQLBindCol(hstmt, 5, SQL_C_SLONG, (SQLPOINTER)&sms_out.status, 0, &len);
    SQLBindCol(hstmt, 6, SQL_C_SLONG, (SQLPOINTER)&sms_out.statusTicks, 0, &len);
    SQLBindCol(hstmt, 7, SQL_C_CHAR, (SQLPOINTER)&sms_out.statusMessage, sizeof(sms_out.statusMessage), &len);
    SQLBindCol(hstmt, 8, SQL_C_SLONG, (SQLPOINTER)&sms_out.sequenceNo, 0, &len);
    SQLBindCol(hstmt, 9, SQL_C_CHAR, (SQLPOINTER)&sms_out.messageID, sizeof(sms_out.messageID), &len);
    SQLBindCol(hstmt, 10, SQL_C_SLONG, (SQLPOINTER)&sms_out.aid, 0, &len);

    if (!SQL_SUCCEEDED(SQLFetch(hstmt)))
    {
        if (len == SQL_NO_DATA)
        {
            SQLCloseCursor(hstmt);
            db_state = DB_CUR_CLOSED;
        }
    } // end if not succeed

    iCpy(pmsg, &sms_out, sizeof(sms_out));
} // end Get_Unsent_Messages
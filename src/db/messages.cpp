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
    :henv{nullptr}, hdbc{nullptr}, hstmt{nullptr} 
{
    period_id = audit_id = last_msg_id = 0;
} // end Constructor



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
    if (iQE::Init_ODBC(henv, hdbc) < 0)
        return -1;

    if (iQE::Driver_Connect_DB((SQLCHAR*)con_str.c_str(), hdbc, hstmt) < 0)
        return -1;   

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
    if (iQE::Shutdown_ODBC(henv, hdbc, hstmt) < 0)
        return -1;

    return 0;
} // end Disconnect_DB



//===============================================================================|
/**
 * @brief Fetches the pre-kooked messages that are stored in WSIS databases and
 *  have not been delivered to user state.
 * 
 * @return std::vector<SmsOut> a list of unsent messages stored in db
 */
std::vector<SmsOut> Messages::Load_Messages()
{
    SQLLEN len;
    SmsOut sms_out;
    std::vector<SmsOut> messages;

    std::string sql{"SELECT * FROM Subscriber.dbo.SmsOut WHERE status <= 2"};

    if (iQE::Run_Query_Direct((SQLCHAR *)sql.c_str(), hstmt) < 0)
    {
        iQE::Dump_DB_Error();
        return messages;
    } // end Get_Out_Sms

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

    while ( SQL_SUCCEEDED(SQLFetch(hstmt)))
        messages.push_back(sms_out);

    SQLCloseCursor(hstmt);
    return messages;
} // end Get_Unsent_Messages



//===============================================================================|
/**
 * @brief Load's the current WSIS period ID from database. It reads the current 
 *  reading period from Subscriber database SystemParameters table and returns the 
 *  result after it set's it's own attribute period_id to that value.
 * 
 * @return int period_id on success alas -2 on fail
 */
int Messages::Load_Current_Period()
{
    SQLLEN len;
    std::string sql{"SELECT CAST(CAST(ParValue AS nvarchar(max)) AS int) \
        FROM Subscriber.dbo.SystemParameter WHERE ParName = 'currentPeriod'"};
    
    if (iQE::Run_Query_Direct((SQLCHAR*)sql.c_str(), hstmt) < 0)
    {
        iQE::Dump_DB_Error();
        return -2;
    } // end if

    SQLBindCol(hstmt, 1, SQL_C_SLONG, (SQLPOINTER)&period_id, 0, &len);
    if ( !SQL_SUCCEEDED(SQLFetch(hstmt)))
    {
        iQE::Dump_DB_Error();
        return -2;
    } // end if

    SQLCloseCursor(hstmt);
    return period_id;
} // end Load_Current_Period



//===============================================================================|
/**
 * @brief Load's the reading WSIS period ID from database. It reads the 
 *  reading period from Subscriber database SystemParameters table and returns the 
 *  result after it set's it's own attribute period_id to that value.
 * 
 * @return int period_id on success alas -2 on fail
 */
int Messages::Load_Reading_Period()
{
    SQLLEN len;
    std::string sql{"SELECT CAST(CAST(ParValue AS nvarchar(max)) AS int) \
        FROM Subscriber.dbo.SystemParameter WHERE ParName = 'readingPeriod'"};
    
    if (iQE::Run_Query_Direct((SQLCHAR*)sql.c_str(), hstmt) < 0)
    {
        iQE::Dump_DB_Error();
        return -2;
    } // end if

    SQLBindCol(hstmt, 1, SQL_C_SLONG, (SQLPOINTER)&reading_period, 0, &len);
    if ( !SQL_SUCCEEDED(SQLFetch(hstmt)))
    {
        iQE::Dump_DB_Error();
        return -2;
    } // end if

    SQLCloseCursor(hstmt);
    return reading_period;
} // end Load_Current_Period



//===============================================================================|
/**
 * @brief Fetches the current period name from the database using it's period_id
 *  memeber data, this implies that period_id must be initalized into a valid 
 *  value prior to call. 
 * 
 * @return std::string period name for the current period on fail an empty string
 */
std::string Messages::Load_Current_Period_Name()
{
    SQLLEN len;
    char buf[200]{0};

    std::string sql{"SELECT name FROM Subscriber.dbo.BillPeriod WHERE id = " + 
        std::to_string(period_id)};

    if (iQE::Run_Query_Direct((SQLCHAR*)sql.c_str(), hstmt) < 0)
    {
        iQE::Dump_DB_Error();
        return "";
    } // end if

    SQLBindCol(hstmt, 1, SQL_C_CHAR, (SQLPOINTER)buf, 200, &len);
    if ( !SQL_SUCCEEDED(SQLFetch(hstmt)))
    {
        iQE::Dump_DB_Error();
        return "";
    } // end if

    SQLCloseCursor(hstmt);
    period_name = buf;
    return period_name;
} // end Load_Current_Period_Name



//===============================================================================|
/**
 * @brief Fetches the current period name from the database using it's period_id
 *  memeber data, this implies that period_id must be initalized into a valid 
 *  value prior to call. 
 * 
 * @return std::string period name for the current period on fail an empty string
 */
std::string Messages::Load_Reading_Period_ToDate()
{
    SQLLEN len;
    char buf[200]{0};

    std::string sql{"SELECT toDate FROM Subscriber.dbo.BillPeriod WHERE id = " + 
        std::to_string(reading_period)};

    if (iQE::Run_Query_Direct((SQLCHAR*)sql.c_str(), hstmt) < 0)
    {
        iQE::Dump_DB_Error();
        return "";
    } // end if

    SQLBindCol(hstmt, 1, SQL_C_CHAR, (SQLPOINTER)buf, 200, &len);
    if ( !SQL_SUCCEEDED(SQLFetch(hstmt)))
    {
        iQE::Dump_DB_Error();
        return "";
    } // end if

    SQLCloseCursor(hstmt);
    return buf;
} // end Load_Reading_Period_ToDate



//===============================================================================|
/**
 * @brief Load's the AuditID from the manually maintained database id's stored in
 *  WSISApp database. The function also update's the ID to the next integer
 * 
 * @return int the audit id used for auditing data
 */
int Messages::Load_AID()
{
    SQLLEN len;
    std::string sql{"SELECT LastValue FROM WSISApp.dbo.AutoIncrementFields WHERE Name = 'AuditID'"};

    if (iQE::Run_Query_Direct((SQLCHAR*)sql.c_str(), hstmt) < 0)
    {
        iQE::Dump_DB_Error();
        return -2;
    } // end if

    SQLBindCol(hstmt, 1, SQL_C_SLONG, (SQLPOINTER)&audit_id, 0, &len);
    if ( !SQL_SUCCEEDED(SQLFetch(hstmt)))
    {
        iQE::Dump_DB_Error();
        return -2;
    } // end if

    SQLCloseCursor(hstmt);
    audit_id++;
    sql = "UPDATE WSISApp.dbo.AutoIncrementFields SET LastValue = " + std::to_string(audit_id) +
        " WHERE Name = 'AuditID'";
    
    if (iQE::Run_Query_Direct((SQLCHAR*)sql.c_str(), hstmt) < 0)
    {
        iQE::Dump_DB_Error();
        return -2;
    } // end if

    return audit_id;
} // end Load_AID



//===============================================================================|
/**
 * @brief Load's the last message ID from the manually kept AutoIncrementFields
 *  table of WSISApp database. 
 * 
 * @return int the next messageID to use in the database
 */
int Messages::Load_Last_Message_ID()
{
    SQLLEN len;

    std::string sql{"SELECT LastValue FROM WSISApp.dbo.AutoIncrementFields WHERE Name = 'MessageID'"};

    if (iQE::Run_Query_Direct((SQLCHAR*)sql.c_str(), hstmt) < 0)
    {
        iQE::Dump_DB_Error();
        return -2;
    } // end if

    SQLBindCol(hstmt, 1, SQL_C_SLONG, (SQLPOINTER)&last_msg_id, 0, &len);
    if ( !SQL_SUCCEEDED(SQLFetch(hstmt)))
    {
        iQE::Dump_DB_Error();
        return -2;
    } // end if

    SQLCloseCursor(hstmt);
    return ++last_msg_id;
} // end Load_Last_Message_ID



//===============================================================================|
/**
 * @brief Updates the content's of Database for MessageID for permanent effects.
 * 
 * @return int 0 on success -2 on fail
 */
int Messages::Update_Last_Message_ID() const
{
    std::string sql{"UPDATE WSISApp.dbo.AutoIncrementFields SET LastValue = " +
        std::to_string(last_msg_id) + " WHERE Name = 'MessageID'"};

    if (iQE::Run_Query_Direct((SQLCHAR*)sql.c_str(), hstmt) < 0)
    {
        iQE::Dump_DB_Error();
        return -2;
    } // end if

    return 0;
} // end Update_Last_Message_ID


//===============================================================================|
/**
 * @brief Return's the cooked SMS bill format from Subscriber.dbo.SystemParamter.
 *  This is just template of text that is to be sent over SMS after replacing
 *  the tokken strings.
 * 
 * @return std::string the bill format string
 */
std::string Messages::Load_SMS_Bill_Format()
{
    char buf[MAXLINE]{0};
    SQLLEN len;
    std::string sql{"SELECT CAST(ParValue AS nvarchar(max)) \
        FROM Subscriber.dbo.SystemParameter WHERE ParName = 'sms_bill_format'"};

    if (iQE::Run_Query_Direct((SQLCHAR*)sql.c_str(), hstmt) < 0)
    {
        iQE::Dump_DB_Error();
        return "";
    } // end if

    SQLBindCol(hstmt, 1, SQL_C_CHAR, (SQLPOINTER)buf, MAXLINE, &len);
    if ( !SQL_SUCCEEDED(SQLFetch(hstmt)))
    {
        iQE::Dump_DB_Error();
        return "";
    } // end if

    SQLCloseCursor(hstmt);
    bill_format = buf;
    return bill_format;
} // end Get_SMS_Bill_Format



//===============================================================================|
/**
 * @brief Return's the cooked SMS unread format from Subscriber.dbo.SystemParamter.
 *  This is just template of text that is to be sent over SMS after replacing
 *  the tokken strings.
 * 
 * @return std::string the bill format string
 */
std::string Messages::Load_Unread_Format()
{
    char buf[MAXLINE]{0};
    SQLLEN len;
    std::string sql{"SELECT CAST(ParValue AS nvarchar(max)) \
        FROM Subscriber.dbo.SystemParameter WHERE ParName = 'sms_unread_format'"};

    if (iQE::Run_Query_Direct((SQLCHAR*)sql.c_str(), hstmt) < 0)
    {
        iQE::Dump_DB_Error();
        return "";
    } // end if

    SQLBindCol(hstmt, 1, SQL_C_CHAR, (SQLPOINTER)buf, MAXLINE, &len);
    if ( !SQL_SUCCEEDED(SQLFetch(hstmt)))
    {
        iQE::Dump_DB_Error();
        SQLCloseCursor(hstmt);
        return "";
    } // end if

    SQLCloseCursor(hstmt);
    unread_format = buf;
    return unread_format;
} // end Get_Unread_Format



//===============================================================================|
/**
 * @brief Prepare's a cooked message for bills to be paid for customer under the
 *  current period
 * 
 * @param subscriber_id when this is used then the message is single user
 * 
 * @return std::vector<SmsOut> a vector of out messages to write to db
 */
std::vector<SmsOut> Messages::Preview_Bill_SMS(const int subscriber_id)
{
    SQLLEN len;
    std::vector<SmsOut> out_sms;

    char buf[MAXLINE*4]{0};
    if (subscriber_id == -1)
    {
        snprintf(buf, MAXLINE*4, 
            "SELECT b.connectionID, b.phoneNo, b.name, b.customerCode, a.reading, \
                a.consumption, a.cur, b.overd \
            FROM ( \
            SELECT b.connectionID, a.phoneNo, a.name, a.customerCode, d.reading, \
                d.consumption, SUM(c.price) cur \
            FROM Subscriber.dbo.Subscriber a \
            JOIN Subscriber.dbo.CustomerBill b \
            ON a.id = b.customerID \
            JOIN Subscriber.dbo.CustomerBillItem c \
            ON b.id = c.customerBillID \
            JOIN Subscriber.dbo.BWFMeterReading d \
            ON b.connectionID = d.subscriptionID \
            WHERE b.paymentDocumentID = -1 AND b.paymentDiffered = 0 AND a.phoneNo != '' \
                AND d.periodID = %d \
            GROUP BY b.connectionID, a.phoneNo, a.name, a.customerCode, d.reading, \
                d.consumption ) AS a \
            JOIN ( \
            SELECT b.connectionID, a.phoneNo, a.name, a.customerCode, \
                SUM(c.price - c.settledFromDepositAmount) overd \
            FROM Subscriber.dbo.Subscriber a \
            JOIN Subscriber.dbo.CustomerBill b \
            ON a.id = b.customerID \
            JOIN Subscriber.dbo.CustomerBillItem c \
            ON b.id = c.customerBillID \
            WHERE b.paymentDocumentID = -1 AND b.paymentDiffered = 0 AND a.phoneNo != '' \
                AND b.periodID = %d \
            GROUP BY b.connectionID, a.phoneNo, a.name, a.customerCode ) AS b \
            ON a.connectionID = b.connectionID", 
            period_id, period_id);
    } // end if general
    else
    {
        snprintf(buf, MAXLINE*4, "SELECT b.connectionID, b.phoneNo, b.name, b.customerCode, a.reading, \
                a.consumption, a.cur, b.overd \
            FROM ( \
            SELECT b.connectionID, a.phoneNo, a.name, a.customerCode, d.reading, \
                d.consumption, SUM(c.price) cur \
            FROM Subscriber.dbo.Subscriber a \
            JOIN Subscriber.dbo.CustomerBill b \
            ON a.id = b.customerID \
            JOIN Subscriber.dbo.CustomerBillItem c \
            ON b.id = c.customerBillID \
            JOIN Subscriber.dbo.BWFMeterReading d \
            ON b.connectionID = d.subscriptionID \
            WHERE b.paymentDocumentID = -1 AND b.paymentDiffered = 0 AND a.phoneNo != '' \
                AND d.periodID = %d AND b.connectionID = %d \
            GROUP BY b.connectionID, a.phoneNo, a.name, a.customerCode, d.reading, \
                d.consumption ) AS a \
            JOIN ( \
            SELECT b.connectionID, a.phoneNo, a.name, a.customerCode, \
                SUM(c.price - c.settledFromDepositAmount) overd \
            FROM Subscriber.dbo.Subscriber a \
            JOIN Subscriber.dbo.CustomerBill b \
            ON a.id = b.customerID \
            JOIN Subscriber.dbo.CustomerBillItem c \
            ON b.id = c.customerBillID \
            WHERE b.paymentDocumentID = -1 AND b.paymentDiffered = 0 AND a.phoneNo != '' \
                AND b.periodID = %d AND b.connectionID = %d \
            GROUP BY b.connectionID, a.phoneNo, a.name, a.customerCode ) AS b \
            ON a.connectionID = b.connectionID",
            period_id, subscriber_id, period_id, subscriber_id);
    } // end else single

    if (iQE::Run_Query_Direct((SQLCHAR*)buf, hstmt) < 0)
    {
        iQE::Dump_DB_Error();
        return out_sms;
    } // end if

    int connectionID;
    char phone[50];
    char name[200];
    double cur{0}, overd{0};
    int reading{0}, consumption{0};
    char customer_code[200];


    SQLBindCol(hstmt, 1, SQL_C_SLONG, (SQLPOINTER)&connectionID, 0, &len);
    SQLBindCol(hstmt, 2, SQL_C_CHAR, (SQLPOINTER)phone, 40, &len);
    SQLBindCol(hstmt, 3, SQL_C_CHAR, (SQLPOINTER)name, 200, &len);
    SQLBindCol(hstmt, 4, SQL_C_CHAR, (SQLPOINTER)customer_code, 200, &len);
    SQLBindCol(hstmt, 5, SQL_C_SLONG, (SQLPOINTER)&reading, 0, &len);
    SQLBindCol(hstmt, 6, SQL_C_SLONG, (SQLPOINTER)&consumption, 0, &len);
    SQLBindCol(hstmt, 7, SQL_C_DOUBLE, (SQLPOINTER)&cur, 0, &len);
    SQLBindCol(hstmt, 8, SQL_C_DOUBLE, (SQLPOINTER)&overd, 0, &len);;


    while ( SQL_SUCCEEDED(SQLFetch(hstmt)))
    {
        std::string msg{bill_format};

        msg = Replace_String(msg, "$name", name);
        msg = Replace_String(msg, "$period", period_name);
        msg = Replace_String(msg, "$contractNo", std::to_string(connectionID));
        msg = Replace_String(msg, "$cont", std::to_string(connectionID));
        msg = Replace_String(msg, "$bill", Format_Numerics(cur + overd));
        msg = Replace_String(msg, "$currentReading", std::to_string(reading));
        msg = Replace_String(msg, "$consumption", std::to_string(consumption));

        SmsOut out;
        out.id = last_msg_id;
        iCpy(out.phoneno, phone, strlen(phone) + 1);
        iCpy(out.message, msg.c_str(), msg.length()+1);
        out.logTicks = time(NULL);
        out.status = 0;
        out.statusTicks = time(NULL);
        iCpy(out.statusMessage, "Sending", strlen("Sending"));
        out.sequenceNo = last_msg_id++;
        out.aid = audit_id;

        out_sms.push_back(out);
    } // end while

    SQLCloseCursor(hstmt);
    return out_sms;
} // end Preview_Bill_SMS



//===============================================================================|
std::vector<SmsOut> Messages::Preview_Reading_SMS(const int subscriber_id)
{
    char buf[MAXLINE]{0};
    snprintf(buf, MAXLINE, "SELECT s.name, ss.contactNo, s.phoneNo, s.customerCode \
        FROM Subscriber.dbo.Subscription ss INNER JOIN Subscriber.dbo.Subscriber s \
        ON ss.subscriberID = s.id AND ss.ticksTo = -1 WHERE ss.subscriptionStatus = 2 \
        AND phoneNo IS NOT NULL AND LEN(phoneNo) > 9 AND ss.id NOT IN ( \
        SELECT SubscriptionID FROM Subscriber.dbo.BWFMeterReading WHERE periodID = %d \
        AND bwfStatus = 1);", reading_period);

    if (subscriber_id != -1)
        snprintf(buf + strlen(buf), MAXLINE - strlen(buf), "AND s.id = %d", 
        subscriber_id);

    int connectionID;
    char phone[50];
    char name[200];
    char customer_code[200];

    SQLLEN len;
    SQLBindCol(hstmt, 1, SQL_C_CHAR, (SQLPOINTER)name, 200, &len);
    SQLBindCol(hstmt, 2, SQL_C_SLONG, (SQLPOINTER)&connectionID, 0, &len);
    SQLBindCol(hstmt, 3, SQL_C_CHAR, (SQLPOINTER)phone, 200, &len);
    SQLBindCol(hstmt, 4, SQL_C_CHAR, (SQLPOINTER)customer_code, 200, &len);

    std::vector<SmsOut> vout;
    while ( SQL_SUCCEEDED(SQLFetch(hstmt)))
    {
        std::string msg{unread_format};

        msg = Replace_String(msg, "$name", name);
        msg = Replace_String(msg, "$date", Load_Reading_Period_ToDate());

        SmsOut out;
        out.id = last_msg_id;
        iCpy(out.phoneno, phone, strlen(phone) + 1);
        iCpy(out.message, msg.c_str(), msg.length()+1);
        out.logTicks = time(NULL);
        out.status = 0;
        out.statusTicks = time(NULL);
        iCpy(out.statusMessage, "Sending", strlen("Sending"));
        out.sequenceNo = last_msg_id++;
        out.aid = audit_id;

        vout.push_back(out);
    } // end while

    SQLCloseCursor(hstmt);
    return vout;
} // end Preview_Reading_SMS



//===============================================================================|
std::vector<SmsOut> Messages::Preview_General_SMS(const int subscriber_id)
{
    char buf[MAXLINE]{0};
    snprintf(buf, MAXLINE, "SELECT phoneNo, name, customerCode FROM \
        Subscriber.dbo.Subscriber WHERE phoneNo != ''");

    if (subscriber_id != -1)
        snprintf(buf + strlen(buf), MAXLINE - strlen(buf), " AND id = %d", 
        subscriber_id);

    char phone[50];
    char name[200];
    char customer_code[200];

    SQLLEN len;
    SQLBindCol(hstmt, 1, SQL_C_CHAR, (SQLPOINTER)phone, 200, &len);
    SQLBindCol(hstmt, 2, SQL_C_CHAR, (SQLPOINTER)name, 200, &len);
    SQLBindCol(hstmt, 3, SQL_C_CHAR, (SQLPOINTER)customer_code, 200, &len);

    std::vector<SmsOut> vout;
    while ( SQL_SUCCEEDED(SQLFetch(hstmt)))
    {
        std::string msg{msg_format};

        msg = Replace_String(msg, "$name", name);

        SmsOut out;
        out.id = last_msg_id;
        iCpy(out.phoneno, phone, strlen(phone) + 1);
        iCpy(out.message, msg.c_str(), msg.length()+1);
        out.logTicks = time(NULL);
        out.status = 0;
        out.statusTicks = time(NULL);
        iCpy(out.statusMessage, "Sending", strlen("Sending"));
        out.sequenceNo = last_msg_id++;
        out.aid = audit_id;

        vout.push_back(out);
    } // end while

    SQLCloseCursor(hstmt);
    return vout;
} // end Preview_General_SMS



//===============================================================================|
void Messages::Write_SMSOut(std::vector<SmsOut> &msgs)
{
    char buf[MAXLINE*4]{0};

    for (size_t i{0}; i < msgs.size(); i++)
    {
        snprintf(buf, MAXLINE*4, "INSERT INTO Subscriber.dbo.SmsOut \
            (phoneNo, message, logTicks, status, statusTicks, statusMessage, seqNo, messageID, __AID) \
            VALUES ('%s', '%s', %ld, %d, %ld, '%s', %d, '%s', %d)",
            msgs[i].phoneno, msgs[i].message, msgs[i].logTicks, msgs[i].status, 
            msgs[i].statusTicks, msgs[i].statusMessage, msgs[i].sequenceNo, 
            msgs[i].messageID, msgs[i].aid);
        
        if (iQE::Run_Query_Direct((SQLCHAR*)buf, hstmt) < 0)
        {
            iQE::Dump_DB_Error();
        } // end if
    } // end for
} // end Write_SMSOut



//===============================================================================|
void Messages::Update_SMSOut(SmsOut_Ptr msg)
{
    char buf[512]{0};
    snprintf(buf, 512, 
        "UPDATE Subscriber.dbo.SmsOut SET status = %d WHERE messageID = '%s';",
        msg->status, msg->messageID);

    if (iQE::Run_Query_Direct((SQLCHAR*)buf) < 0)
    {
        iQE::Dump_DB_Error();
        return;
    } // end if
} // end Update_Out_SMS_DB
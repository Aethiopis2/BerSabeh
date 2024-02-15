/**
 * @file Messages.h
 * @author Dr. Rediet Worku aka Aethiops ben Zahab (PanaceaSolutionsEth@gmail.com)
 * 
 * @brief Definition of a Message class that models database stored messages and is used to manage
 *  messages and stuff
 * @version 0.1
 * @date 2024-01-17
 * 
 * @copyright Copyright (c) 2024
 * 
 */
#ifndef MESSAGES_H
#define MESSAGES_H



//===============================================================================|
//          INCLUDES
//===============================================================================|
// database states
#define DB_DISCONNECTED         0
#define DB_CONNECTED            1
#define DB_CUR_OPEN             2
#define DB_CUR_CLOSED           1




//===============================================================================|
//          INCLUDES
//===============================================================================|
#include "iQE.h"






//===============================================================================|
//          CLASS
//===============================================================================|
typedef struct SMSOUT_TABLE_TYPE
{
    u32 id;
    char phoneno[50];
    char message[3000];     // nvarchar(max) is it?
    u64 logTicks;
    u32 status;
    u64 statusTicks;
    char statusMessage[300];
    u32 sequenceNo;
    char messageID[50];
    u32 aid;
} SmsOut, *SmsOut_Ptr;



typedef struct SMSIN_TABLE_TYPE
{
    u32 id;
    char phoneno[50];
    char message[1000];
    u64 recvdTicks;
    u8 error;
} SmsIn, *SmsIn_Ptr;




class Messages
{
public:

    Messages();
    Messages(const std::string &con_str);
    ~Messages();

    int Connect_DB(const std::string &con_str);
    int Disconnect_DB();
    void Get_Unsent_Messages(SmsOut_Ptr pmsg);
    void Set_Message();


private:

    u8 db_state;        // one of the following state; dis/connected, cursor_open/closed

    SmsOut sms_out;
    SmsIn sms_in;


    // ODBC database stuff
    HENV henv;
    HDBC hdbc;
    HSTMT hstmt;
};




#endif
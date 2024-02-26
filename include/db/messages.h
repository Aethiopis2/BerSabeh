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
#include "utils.h"






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

    std::vector<SmsOut> Load_Messages();
    int Load_Current_Period();
    int Load_Reading_Period();
    std::string Load_Current_Period_Name();
    std::string Load_Reading_Period_ToDate();
    int Load_AID();
    int Load_Last_Message_ID();
    int Update_Last_Message_ID() const;
    std::string Load_SMS_Bill_Format();
    std::string Load_Unread_Format();
    std::vector<SmsOut> Preview_Bill_SMS(const int subscriber_id = -1);
    std::vector<SmsOut> Preview_Reading_SMS(const int subscriber_id = -1);
    std::vector<SmsOut> Preview_General_SMS(const int subscriber_id = -1);

    void Write_SMSOut(std::vector<SmsOut> &msgs);
    void Update_SMSOut(SmsOut_Ptr msg);
    void Write_SMSIn(SmsIn_Ptr msg);

    
private:

    
    // ol'skool WSIS stuff
    u32 period_id;
    u32 reading_period;
    u32 audit_id;
    u32 last_msg_id;

    std::string period_name;
    std::string bill_format;
    std::string unread_format;
    std::string msg_format;


    // ODBC database stuff
    HENV henv;
    HDBC hdbc;
    HSTMT hstmt;
};




#endif
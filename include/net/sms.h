/**
 * @file sms.h
 * @author Rediet Worku aka Aethiopis II ben Zahab (aethiopis2rises@gmail.com)
 * 
 * @brief Implements SMS functionalites based on SMPPv3 protocol specs.
 * @version 0.1
 * @date 2024-01-10
 * 
 * @copyright Copyright (c) 2024
 * 
 */
#ifndef INTAPS_SMS_H
#define INTAPS_SMS_H



//===============================================================================|
//              INCLUDES
//===============================================================================|
#include "tcp-client.h"
#include "smpp-konstants.h"





//===============================================================================|
//              MACROS
//===============================================================================|





//===============================================================================|
//              GLOBALS
//===============================================================================|




//===============================================================================|
//              TYPES
//===============================================================================|
/**
 * @brief A little make life easy structure that captures the mandatory feilds
 *  for smpp pdu.
 * 
 */
#pragma pack(1)
typedef struct SMPP_PDU_CMD_HDR
{
    u32 command_length;         // the length of the command
    u32 command_id;             // the specific command (see defines above)
    u32 command_status;         // indicates success or fail state of command
    u32 sequence_num{0};        // the sequence num; defaulted to 0
} Command_Hdr, *Command_Hdr_Ptr;




/**
 * @brief This little struct reprsents all the mandatory paramters in 
 *  SMPP v3.4 as a little option table that can be controlled by users through
 *  UI's and command interface.
 * 
 */
typedef struct SMPP_MANDATORY_PARAMETERS
{
    u8 interface_ver{SMPP_VER};                     // SMPP version (0x34 supported by this driver)
    u8 service_type{ST_NULL};                       // the service type (see defines above)
    u8 src_ton{TON_NATIONAL};                       // the type of number (see define above)
    u8 src_npi{NPI_NATIONAL};                       // the numbering plan indicator
    u8 dest_ton{TON_NATIONAL};                      // the type of number (see define above)
    u8 dest_npi{NPI_NATIONAL};                      // the numbering plan indicator
    u8 esm_class{ESM_DEFAULT};                      // indicates message type and mode (see define above)
    u8 protocol_id{0};                              // set by SMSC, not generally used
    u8 priority_flag{1};                            // 4 levels. 0 - 3 lowest to highest. >= 4 reservered
    std::string schedule_delivery_time{""};         // format YYMMDDhhmmsstnnp (see SMPP specs)
    std::string validity_period{""};                // expiary date (see SMPP specs on date format)
    u8 registered_delivery{REG_DELV_REQ_RECEIPT};   // require SMSC recipts or SME reciepts?
    u8 replace_present{1};                          // 0 don't replace, 1 replace, >=2 reserved
    u8 data_coding{DATA_CODE_DEFAULT};              // defines the encoding scheme of sms
    u8 sm_id{0};                                    // indicates the id for canned messages to send (0 for custom)
} Smpp_Options, *Smpp_Options_Ptr;




/**
 * @brief This is a structure that is used to keep track of all the active items
 *  the application needs. By keeping track of sent messages that have not yet been
 *  confrimed the application has the chance of re-submitting messages or do other
 *  stuff like change/update or even remove the messages before final arrival.
 * This litlle struct is used for single messages as distinct from bulk.
 * 
 */
typedef struct SMS_INFO_STRUCT
{
    u8 msg_state;           // state of our little message
    std::string id;         // sms id sent from ESME
    std::string msg;        // the sent message
    std::string dst;        // the destination numerics
    Smpp_Options opts;      // extra options associtated with this message
} Single_Sms_Info, *Single_Sms_Info_Ptr;




/**
 * @brief This is the same structure as SMS_INFO_STRUCT. However this has been
 *  slightly modified to handle bulk messages only. The differences in the two
 *  structures is only in "dst" feilds, which holds the destination addresses
 *  for the messages.
 * This struct implements "dst" as a queued item.
 * 
 */
typedef struct SMS_INFO_STRUCT_BULK
{
    u8 msg_state;           // state of message
    std::string id;         // sms id sent from ESME
    std::string msg;        // the sent message
    std::queue<std::string> dst;        // the destination numerics
    Smpp_Options opts;      // extra options assc
} Bulk_Sms_Info, *Bulk_Sms_Info_Ptr;








/**
 * @brief Main Sms class used to handle all comms using SMPPv3.4 Protocol. The class is threaded
 *  so as to work in async and implement realtime functionalities, i.e. Sms messages trigger
 *  notifications to the user whenever they are recieved.
 * 
 */
class Sms
{
public: 
    Sms();
    Sms(const std::string hostname, const std::string port, const std::string sys_id, 
        const std::string pwd, const std::string sms_no = "", 
        const u32 mode = bind_transceiver, const bool hbt = false, 
        const bool debug = false);
    ~Sms();

    friend void Heartbeat(Sms *psms);
    
    int Startup(const std::string hostname, const std::string port, 
        const std::string sys_id, const std::string pwd, const std::string sms_no = "",
        const u32 mode = bind_transceiver,
        const bool hbeat = false, const bool dbug = false);
    int Shutdown();
    int Disconnect();

    
    int Send_Bulk_Message(const std::string msg, std::list<std::string> &dest_nums,
        const Smpp_Options_Ptr poptions = nullptr);
    int Send_Message(const std::string msg, const std::string dest_num, 
        const Smpp_Options_Ptr poptions = nullptr);
    int Process_Incoming();


    // stright up smpp's
    int Bind(const u32 command_id);
    int Bind_Rsp();
    int Outbind();
    int Unbind();
    int Unbind_Resp(const u32 resp = ESME_ROK);
    int Generic_Nack();
    int Submit(const std::string &msg, const std::string &dest_num, 
        const Smpp_Options_Ptr poptions, const u8 can_id = 0);
    int Submit_Multi(const std::string &msg, std::queue<std::string> dest_nums,
        const Smpp_Options_Ptr poptions, const u8 can_id = 0);
    int Query(const std::string &msg_id, const Smpp_Options_Ptr poptions, 
        const std::string src_addr = "");
    int Query_Rsp(const char *buffer, const size_t len);
    int Cancel(const std::string msg_id, const Smpp_Options_Ptr popts, const std::string src_addr="");
    int Replace(const std::string msg_id, const Smpp_Options_Ptr popts, 
        const std::string msg, const std::string src_addr="");
    int Enquire();
    int Enquire_Rsp(const u32 resp = ESME_ROK);
    
    //int Submit_Rsp(const u32 resp = ESME_ROK);
    int Deliver_Rsp(const u32 resp = ESME_ROK);


    // accessors
    int Get_Connection() const;
    void Set_HB_Interval(const u32 interval);
    u32 Get_HB_Interval() const;

    int Get_State() const;
    std::string Get_SystemID() const;
    std::string Get_Err() const;


    void Toggle_Heartbeat();
    void Toggle_Debug();

private:

    u8 sms_state;               // state of our little sms
    u32 seq_num;                // the current message sequence #
    u32 heartbeat_interval;     // determines the interval for heartbeat signal

    char snd_buffer[SMS_BUFFER_SIZE];     // sending buffer
    char rcv_buffer[SMS_BUFFER_SIZE];     // recieving buffer

    std::string err_desc;       // a little error description buffer
    std::string smsc_id;        // idenitifer for smsc, sent as a result of Bind

    std::string sms_id;         // the sms number (no more than 21 chars long including null)
    std::string system_id;      // the user system id
    std::string pwd;            // the password for authentication
    
    TcpClient tcp;              // an object of Tcp for handling the tcp stuff
    Smpp_Options options;       // options for our little smpp client
    Command_Hdr cmd_hdr;        // used for sending
    Command_Hdr cmd_rsp;        // used during reception
    std::map<u32, Single_Sms_Info> queued_msg;          // messages in queue used for quering stuff
    std::map<u32, Bulk_Sms_Info> queued_blk_msg;        // same as above, but for bulks
    

    bool bdebug;                // used for dumping hex views
    bool bheartbeat;            // toggles heart beat on/off

    std::thread *phbeat;        // handle to heartbeat thread

}; // end class



#endif
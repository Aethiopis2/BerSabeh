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






//===============================================================================|
//              MACROS
//===============================================================================|
// SMS interface states
#define SMS_DISCONNECTED    0x00           // interface is in disconnected state
#define SMS_CONNECTED       0x01           // connected to serivce center through TCP
#define SMS_BOUND_TRX       0x02           // TCP connected and bound to transiver interface; ready to rock.





// SMPP command codes
#define generic_nack            0x80000000
#define bind_receiver           0x00000001
#define bind_receiver_resp      0x80000001
#define bind_transmitter        0x00000002
#define bind_transmitter_resp   0x80000002
#define query_sm                0x00000003
#define query_sm_resp           0x80000003
#define submit_sm               0x00000004
#define submit_sm_resp          0x80000004
#define deliver_sm              0x00000005
#define deliver_sm_resp         0x80000005
#define unbind                  0x00000006
#define unbind_resp             0x80000006
#define replace_sm              0x00000007
#define replace_sm_resp         0x80000007
#define cancel_sm               0x00000008
#define cancel_sm_resp          0x80000008
#define bind_transceiver        0x00000009
#define bind_transceiver_resp   0x80000009
#define outbind                 0x0000000B
#define enquire_link            0x00000015
#define enquire_link_resp       0x80000015


// how bout some few status codes
#define ESME_ROK                0x00000000      // it's all kool
#define ESME_RINVMSGLEN         0x00000001      // mesage length is invalid
#define ESME_RINVCMDLEN         0x00000002      // command length is invalid
#define ESME_RINVCMDID          0x00000003      // command id is invalid
#define ESME_RINVBENDSTS        0x00000004      // invalid bind staus for given command
#define ESME_RSYSERR            0x00000008      // system error



// TON Type of numbers definitions; applies to addr_ton, soruce_addr_ton, dest_addr_ton, 
//  esme_addr_ton
#define TON_UNKOWN              0
#define TON_INTERNATIONAL       1
#define TON_NATIONAL            2
#define TON_NETWORKSPECIFIC     3
#define TON_SUBSCRIBERNUM       4
#define TON_ALPHANUM            5
#define TON_ABBREVIATED         6


// NPI; Numeric Plan Indicators; applies to addr_npi, source_addr_npi,
//  dest_addr_npi, esme_addr_npi
#define NPI_UNKOWN              0
#define NPI_ISDN                1       // E163/E164
#define NPI_DATA                2       // X.121
#define NPI_TELEX               4       // F.69
#define NPI_LANDMOBILE          6       // E.212
#define NPI_NATIONAL            8
#define NPI_PRIVATE             9
#define NPI_ERMES               10
#define NPI_INTERNET            14      // IP
#define NPI_WAP_CLIENTID        18      // to be defined by WAP Forum



// command status
#define ESME_ROK                0x00000000      // No Error
#define ESME_RINVMSGLEN         0x00000001      // Message Length is invalid
#define ESME_RINVCMDLEN         0x00000002      // Command Length is invalid
#define ESME_RINVCMDID          0x00000003      // Invalid Command ID
#define ESME_RINVBNDSTS         0x00000004      // Incorrect BIND Status for given com-
#define ESME_RALYBND            0x00000005      // ESME Already in Bound State
#define ESME_RINVPRTFLG         0x00000006      // Invalid Priority Flag
#define ESME_RINVREGDLVFLG      0x00000007      // Invalid Registered Delivery Flag
#define ESME_RSYSERR            0x00000008      // System Error
#define ESME_RINVSRCADR         0x0000000A      // Invalid Source Address
#define ESME_RINVDSTADR         0x0000000B      // Invalid Dest Addr
#define ESME_RINVMSGID          0x0000000C      // Message ID is invalid
#define ESME_RBINDFAIL          0x0000000D      // Bind Failed
#define ESME_RINVPASWD          0x0000000E      // Invalid Password
#define ESME_RINVSYSID          0x0000000F      // Invalid System ID
#define ESME_RCANCELFAIL        0x00000011      // Cancel SM Failed
#define ESME_RREPLACEFAIL       0x00000013      // Replace SM Failed



// service types
#define ST_NULL                 0               // default
#define ST_CMT                  "CMT"           // Cellular Message Type
#define ST_CPT                  "CPT"           // Cellular Paging
#define ST_VMN                  "VMN"           // Voice Mail Notification
#define ST_VMA                  "VMA"           // Voice Mail Alerting
#define ST_WAP                  "WAP"           // Wireless Application Protocol
#define ST_USSD                 "USSD"          // Unstructured Supplementary Services Data



// esm classes (these are bits set to mean something on the PDU); these bits
//  can be combined using bitwise OR (See protocol specs for more)
#define ESM_DEFAULT             0b00000000      // default store and forward mode
#define ESM_DATAGRAM            0b00000001      // datagram mode
#define ESM_FORWARD             0b00000010      // transaction mode (may not be supported)
#define ESM_DELVIER             0b00001000      // Short Message contains ESME Delivery Acknowledgement
#define ESM_USR_ACK             0b00010000      // user acknowledgment
#define ESM_UDHI                0b01000000      // UDHI Indicator (only relevant for MT short messages)
#define ESM_REP_PATH            0b10000000      // Set Reply Path (only relevant for GSM network)



// registered deliveries (see protocol specs for more)
#define REG_DELV_DEFAULT         0b00000000     // No SMSC Delivery Receipt requested (default)
#define REG_DELV_REQ_RECEIPT     0b00000001     // SMSC Delivery Receipt requested where final delivery
                                                // outcome is delivery success or failure
#define REG_DELV_DELV_FRECEIPT   0b00000010     // SMSC Delivery Receipt requested where the final
                                                // delivery outcome is delivery failure
#define REG_DELV_RSRVD           0b00000011     // reserved

// SME originated Acknowledgement (bits 3 and 2)
#define REG_DELV_SME_ACK         0b00000100     // SME Delivery Acknowledgement requested
#define REG_DELV_SME_USR_ACK     0b00001000     // SME Manual/User Acknowledgment requested
#define REG_DELV_SME_BOTH        0b00001100     // Both Delivery and Manual/User Acknowledgment requested

// Intermediate Notification (bit 5)
#define REG_DELV_DELV_INT        0b00010000     // Intermediate notification requested



// data codings
#define DATA_CODE_DEFAULT        0b00000000      // default data coding (character encoding)


// Optional Paramters (in the native network order)
#define MESSAGE_PAYLOAD         HTONS(0x0424)




// misc
#define SMS_VER                 0x34            // smpp version





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
    





class Sms : public TcpClient
{
public: 
    Sms();
    Sms(std::string hostname, std::string port, bool debug=false);
    ~Sms();

    int Bind_Trx(const std::string &sys_id, const std::string &pwd);
    int Unbind();
    int Unbind_Resp();
    int Enquire();
    int Enquire_Rsp();
    int Submit(std::string msg, std::string dest_num);
    int Submit_Rsp();


    int Process_Incoming(char *buffer, const u32 len);
    int Get_State() const;
    std::string Get_SystemID() const;
    int Get_ConnectionID() const;
    bool Get_Debug() const;

private:

    bool bdebug;                // used for dumping hex views
    u8 sms_state;               // state of our little sms
    u32 seq_num;                // the current message sequence #
    std::string err_desc;       // a little error description buffer
    std::string smsc_name;      // idenitifer for smsc, sent as a result of Bind

    Command_Hdr cmd_hdr;        // sms pdu header instance

    // pdu body (mandatory) parameters
    u8 interface_ver;           // SMPP version (0x34 supported by this driver)
    u8 service_type;            // the service type (see defines above)
    u8 ton;                     // the type of number (see define above)
    u8 npi;                     // the numbering plan indicator
    u8 esm_class;               // indicates message type and mode (see define above)
    u8 protocol_id;             // set by SMSC, not generally used
    u8 priority_flag;           // 4 levels. 0 - 3 lowest to highest. >= 4 reservered
    std::string schedule_delivery_time;     // format YYMMDDhhmmsstnnp (see SMPP specs)
    std::string validity_period;            // expiary date (see SMPP specs on date format)
    u8 registered_delivery;                 // require SMSC recipts or SME reciepts?
    u8 replace_present;          // 0 don't replace, 1 replace, >=2 reserved
    u8 data_coding;              // defines the encoding scheme of sms
    u8 sm_id;                    // indicates the id for canned messages to send (0 for custom)
}; // end namespace



#endif
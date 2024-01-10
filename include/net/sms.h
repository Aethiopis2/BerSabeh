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




//===============================================================================|
//              GLOBALS
//===============================================================================|




//===============================================================================|
//              TYPES
//===============================================================================|
namespace Sms
{
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





//===============================================================================|
//              PROTOTYPES
//===============================================================================|
    int Bind_Trx(std::string sys_id, std::string pwd);
    int Enquire();
    int Enquire_Rsp();
    int Submit(std::string msg, std::string dest_num, const int id);
    int Submit_Rsp();

    //void Run();
    void Process_Incoming(char *buffer, const u32 len);
} // end namespace



#endif
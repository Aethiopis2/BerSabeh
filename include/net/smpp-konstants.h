/**
 * @file smpp-konstants.h
 * @author Rediet Worku aka Aethiopis II ben Zahab (aethiopis2rises@gmail.com)
 * 
 * @brief Contains the definitions for constants that are used in SMPP protocol.
 * @version 0.1
 * @date 2024-02-07
 * 
 * @copyright Copyright (c) 2024
 * 
 */
#ifndef SMPP_KONSTANTS_H
#define SMPP_KONSTANTS_H



//===============================================================================|
//              DEFINES
//===============================================================================|
// SMS interface states
#define SMS_DISCONNECTED    0x00           // interface is in disconnected state
#define SMS_CONNECTED       0x01           // connected to serivce center through TCP
#define SMS_BOUNDED         0x02           // TCP connected and bound to transiver interface; ready to rock.





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
#define submit_multi            0x00000021
#define submit_multi_resp       0x80000021



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
#define MESSAGE_PAYLOAD         htons(0x0424)
#define USER_MESSAGE_REFERENCE  htons(0x0204)



// destination flags
#define DL_SME_ADDRESS          0
#define DL_DLIST                1



// SMPP Message states
#define SMPP_ENROUTE            1           // message is in enroute
#define SMPP_DELIVERED          2           // message is in a delivered to destination
#define SMPP_EXPIRED            3           // message validity has expired
#define SMPP_DELETED            4           // message has been deleted
#define SMPP_UNDLIVERABLE       5           // message is undeliverable
#define SMPP_ACCEPTED           6           // message is accepted on behest of subscriber
#define SMPP_UNKOWN             7           // message is invalid
#define SMPP_REJECTED           8           // message is in rejected state





// Application specific message state
#define MSG_STATE_SENT          0           // the message is in a sent state but not confirmed
#define MSG_STATE_SUBMIT        1           // the SMCS has confirmed the message state but not user
#define MSG_STATE_DELIVERED     2           // the message is delivered to subsciber




// misc
#define SMPP_VER                0x34            // smpp version
#define HEARTBEAT_INTERVAL      10              // default heartbeat, every 10 mins or so
#define SMS_BUFFER_SIZE         96000           // buffer size for incoming connection 





#endif
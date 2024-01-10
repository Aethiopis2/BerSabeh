/**
 * @file sms.cpp
 * @author Rediet Worku aka Aethiopis II ben Zahab (aethiopis2rises@gmail.com)
 * 
 * @brief Implementation details for sms.h
 * @version 0.1
 * @date 2024-01-10
 * 
 * @copyright Copyright (c) 2024
 * 
 */



//===============================================================================|
//        INCLUDES
//===============================================================================|
#include "sms.h"
#include "utils.h"




//===============================================================================|
//        MACROS
//===============================================================================|
#define SET_PDU_HEADER(buf, command_len, command_id, status, snum) { command_len=HTONL(command_len);\
    iCpy(buf, &command_len, sizeof(u32)); \
    buf+=sizeof(u32); u32 cid=HTONL(command_id);\
    iCpy(buf, &cid, sizeof(u32)); \
    buf+=sizeof(32); u32 s=HTONL(status);\
    iCpy(buf, &s, sizeof(u32)); \
    buf+=sizeof(u32); u32 sn=HTONL(snum);\
    iCpy(buf, &sn, sizeof(u32));\
    buf+=sizeof(u32);\
}




//===============================================================================|
//        GLOBALS
//===============================================================================|
int sms_state{SMS_DISCONNECTED};    // tracks the state of interface
Sms::Command_Hdr cmd_hdr;           // a command header info SMS PDU format
std::string err_desc;               // a little error description buffer






//===============================================================================|
//        FUNCTIONS
//===============================================================================|
/**
 * @brief Connects to server/SMSC using TCP/IP enabled network. Set's the state 
 *  to SMS_CONNECTED
 * 
 * @param hostname host ip, name or SMSC
 * @param port port for SMSC
 * 
 * @return int 0 on success alas -1 
 */
int Connect_Tcp(std::string hostname, std::string port)
{
    if (TcpClient::Connect(hostname, port) < 0)
        return -1;

    //psms_thread = new std::thread(Run);
    sms_state = SMS_CONNECTED;
    return 0;
} // end if Connect_Tcp



//===============================================================================|
int Bind_Trx(const std::string hostname, const std::string port, 
  std::string sys_id, std::string pwd)
{
    char buffer[98]{0};
    char *alias{buffer};
    char interface_version = 0x34;

    if (Connect_Tcp(hostname, port) < 0)
      return -1;

    u32 sysid_len = (sys_id.length() > 15 ? 15 : (u32)sys_id.length());
    u32 pwd_len = (pwd.length() > 8 ? 8 : (u32)pwd.length());


    if (sms_state & SMS_CONNECTED)
    {
        u32 len = sysid_len + pwd_len + 22;
        SET_PDU_HEADER(alias, len, bind_transceiver, 0, ++seq_num);

        // system id
        iCpy(alias, sys_id.c_str(), sysid_len);
        alias += (sysid_len + 1);

        // password
        iCpy(alias, pwd.c_str(), pwd_len);
        alias += (pwd_len + 2);

        // interface version, skipping over system type
        iCpy(alias++, &interface_version, 1);
        alias += 3;

        // extended
        //iCpy(alias++, &empty, 1);   // system type
        //iCpy(alias++, &empty, 1);   // interface version
        //iCpy(alias++, &empty, 1);   // addr_ton
        //iCpy(alias++, &empty, 1);   // addr_npi
        //iCpy(alias++, &empty, 1);   // addr_range

        // get it on the go
        return TcpClient::Send(buffer, alias - buffer);
    } // end if connected

    err_desc = "Underlying network not connected.";
    return -2;
} // end Bind_Tx



//===============================================================================|
/**
 * @brief Send's an enquire link to SMSC
 * 
 * @return int -ve on fail alas 0 on success
 */
int Sms::Enquire()
{
  char buffer[16]{0};
  char *alias{buffer};
  u32 len{16};    // header len

  if (sms_state & SMS_BOUND_TRX)
  {
    SET_PDU_HEADER(alias, len, enquire_link, 0, ++seq_num);
    return net.Send(buffer, alias - buffer);
  } 

  err_desc = "Interface not bound";
  return -2;
} // end Enquire


//===============================================================================|
/**
 * @brief Sends an enquire_link response to SMSC
 * 
 * @return int a -ve on fail, 0 on success
 */
int Sms::SmsEnquire_Rsp()
{
  char buffer[16]{0};
  char *alias{buffer};
  u32 len{16};    // header len

  if (sms_state & SMS_BOUND_TRX)
  {
    SET_PDU_HEADER(alias, len, enquire_link_resp, 0, ++seq_num);
    return net.Send(buffer, alias - buffer);
  } 

  err_desc = "Interface not bound";
  return -2;
} // end Enquire_Rsp


//===============================================================================|
int Sms::Submit(std::string msg, std::string dest_num, const int id)
{
  char buffer[512]{0};
  char *alias{buffer};

  if (sms_state & SMS_BOUND_TRX)
  {
    u8 msg_len = (msg.length() > 254 ? 254 : msg.length());
    u16 dest_len = (dest_num.length() > 20 ? 20 : dest_num.length());
    u32 pack_len = 33 + dest_len + msg_len;


    SET_PDU_HEADER(alias, pack_len, submit_sm, 0, ++seq_num);
    alias += 6;

    iCpy(alias, dest_num.c_str(), dest_len);
    alias += (dest_len + 1);
    *(++alias) = 0x02;      // esm class
    *(++alias) = 0x0;       // prtocol id
    *(++alias) = 0x01;      // priority flag
    *(++alias) = 0x0;       // schedule delivery time
    *(++alias) = 0x0;       // default validty period
    *(++alias) = 0x01;      // require delivery reports
    *(++alias) = 0x0;       // replace existing
    *(++alias) = 0x0;       // data coding
    *(++alias) = 0x0;       // sm default message id
    
    iCpy(alias++, &msg_len, 1);
    iCpy(alias, msg.c_str(), msg_len);
    alias += (msg_len + 1);

    Dump_Hex(buffer, alias- buffer);
    return net.Send(buffer, alias - buffer);
  } // end if bounded

  err_desc = "SMS interface not bound";
  return -2;
} // end Submit



//===============================================================================|
void Sms::Process_Incoming(char *buffer, const u32 len)
{
    if (len == 0)
    return;

  Dump_Hex(buffer, len);

  // now let's act on the response and do stuff up (like updating dbs and all)
  switch (NTOHL(*((u32*)(buffer + 4))))
  {
    case bind_transceiver_resp:
    {
      std::cout << "Transceiver bounded" << std::endl;
      sms_state = SMS_BOUND_TRX;
    } break;

    case submit_sm_resp:
    {
      std::cout << "Submit response" << std::endl;
    } break;

    case deliver_sm:
    {
      std::cout << "A delivery report" << std::endl;
    } break;

    case deliver_sm_resp:
    {
      std::cout << "A delivery report response" << std::endl;
    } break;

    case enquire_link:
    {
      std::cout << "Link enquiry" << std::endl;
      if (Enquire_Rsp() < 0);
        //sms_running = 0;
    } break;


    default:
      std::cout << "Is unimplemented or something ..." << std::endl;
  } // end switch
} // end Process_Incomming
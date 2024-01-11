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






//===============================================================================|
//        GLOBALS
//===============================================================================|
int sms_state{SMS_DISCONNECTED};    // tracks the state of interface
Sms::Command_Hdr cmd_hdr;
int seq_num{0};                     // tracks the sequence # of packets
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
/**
 * @brief Sends a bind_transceiver message to SMSC, which enables the interface
 *  to both send and receive sms messages. If successful it sets, the state of
 *  the sms interface to bound.
 * 
 * @param hostname ip/hostname to connect to; i.e. SMSC
 * @param port the port # as string
 * @param sys_id a system identefier (acquired from SMSC)
 * @param pwd the pwd given by SMSC to authenticate user
 * @return int socket desc on success with interface bound; alas -1 on sys error, 
 *  -2 on app error
 */
int Sms::Bind_Trx(const std::string hostname, const std::string port, 
  std::string sys_id, std::string pwd)
{
    char buffer[98]{0};
    char *alias{buffer+sizeof(cmd_hdr)};
    char interface_version = SMS_VER;
    char ton = TON_NATIONAL;    // type of number, we are using national code
    char npi = NPI_NATIONAL;    // don't go further from national thing
    int fds{-1};

    if ( (fds = Connect_Tcp(hostname, port)) < 0)
      return -1;


    if (sms_state & SMS_CONNECTED)
    {
      cmd_hdr.command_id = HTONL(bind_transceiver);
      cmd_hdr.sequence_num = HTONL(++seq_num);
      cmd_hdr.command_status = 0;   // unused

      // copy the payload info, begining with system id, followed by password
      //  interface and anyother mandate.
      u32 sysid_len = (sys_id.length() > 15 ? 15 : (u32)sys_id.length());
      u32 pwd_len = (pwd.length() > 8 ? 8 : (u32)pwd.length());

      iCpy(alias, sys_id.c_str(), sysid_len);
      alias += (sysid_len + 1);

      // password, +2 because we are skipping over system type, as null
      iCpy(alias, pwd.c_str(), pwd_len);
      alias += (pwd_len + 2);

      // interface version, type of number and numbering plan indicator
      iCpy(alias++, &interface_version, 1);
      iCpy(alias++, &ton, 1);
      iCpy(alias++, &npi, 1);
      ++alias;    // for address range

      cmd_hdr.command_length = NTOHL(alias - buffer);
      iCpy(buffer, &cmd_hdr, sizeof(cmd_hdr));

      Dump_Hex(buffer, alias - buffer);
      if (TcpClient::Send(buffer, alias - buffer) < 0)
        return -1;

      sms_state = SMS_BOUND_TRX;
      return fds;
    } // end if connected

    err_desc = "Underlying network not connected.";
    return -2;
} // end Bind_Tx



//===============================================================================|
/**
 * @brief Send's an enquire link to SMSC; this is nice for heartbeat implementaton
 * 
 * @return int -ve on fail alas 0 on success
 */
int Sms::Enquire()
{
  if (sms_state & SMS_BOUND_TRX)
  {
    cmd_hdr.command_id = HTONL(enquire_link);
    cmd_hdr.sequence_num = HTONL(++seq_num);
    cmd_hdr.command_length = HTONL(sizeof(cmd_hdr));

    if (TcpClient::Send((const char*)&cmd_hdr, sizeof(cmd_hdr)) < 0)
      return -1;

    return 0;
  } // end if bount 

  err_desc = "Interface not bound";
  return -2;
} // end Enquire


//===============================================================================|
/**
 * @brief Sends an enquire_link response to SMSC; since the actual enquire link
 *  can be initiated from both sides at any moment
 * 
 * @return int a -ve on fail, 0 on success
 */
int Sms::Enquire_Rsp()
{
  if (sms_state & SMS_BOUND_TRX)
  {
    cmd_hdr.command_length = sizeof(cmd_hdr);
    cmd_hdr.command_id = HTONL(enquire_link_resp);
    cmd_hdr.sequence_num = HTONL(++seq_num);

    if (TcpClient::Send((const char*)&cmd_hdr, sizeof(cmd_hdr)) < 0)
      return -1;

    return 0;
  } // end Enquire_Rsp

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
    //u32 pack_len = 33 + dest_len + msg_len;


    //SET_PDU_HEADER(alias, pack_len, submit_sm, 0, ++seq_num);
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
    //return net.Send(buffer, alias - buffer);
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
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
#define SET_PDU_HEADR(cmd, len, id, stat, seq) { cmd.command_length = HTONL(len);\
  cmd.command_id = HTONL(id); cmd.command_status=HTONL(stat); \
  cmd.sequence_num = HTONL(seq); }





//===============================================================================|
//        GLOBALS
//===============================================================================|







//===============================================================================|
//        CLASS IMP
//===============================================================================|
/**
 * @brief Construct a new Sms:: Sms object
 *  does not do anything useful
 */
Sms::Sms()
  :bdebug{false}, sms_state{SMS_DISCONNECTED}, seq_num{0} 
{
  service_type = ST_NULL;
  esm_class = ESM_DEFAULT;
  ton = TON_NATIONAL;
  npi = NPI_NATIONAL;
  protocol_id = 0;
  priority_flag = 1;      // normal priority in most cases
  schedule_delivery_time = "";
  validity_period  = "";
  registered_delivery = REG_DELV_REQ_RECEIPT;
  replace_present = 1;    // replace previous message if present
  data_coding = DATA_CODE_DEFAULT;
  sm_id = 0;              // short message id set by SMSC during *_rsp messages
  interface_ver = SMS_VER;
} // end Constructor



//===============================================================================|
/**
 * @brief Construct a new Sms:: Sms object Connects to server/SMSC using TCP/IP 
 *  enabled network. Set's the state to SMS_CONNECTED. Throws a runtime error.
 * 
 * @param hostname host ip, name or SMSC
 * @param port port for SMSC
 */
Sms::Sms(std::string hostname, std::string port, bool debug)
  :bdebug{debug}, sms_state{SMS_DISCONNECTED}, seq_num{0}
{
  if (TcpClient::Connect(hostname, port) < 0)
    throw std::runtime_error("tcp connection fail.");

  sms_state = SMS_CONNECTED;

  service_type = ST_NULL;
  esm_class = ESM_DEFAULT;
  ton = TON_NATIONAL;
  npi = NPI_NATIONAL;
  protocol_id = 0;
  priority_flag = 1;      // normal priority in most cases
  schedule_delivery_time = "";
  validity_period  = "";
  registered_delivery = REG_DELV_REQ_RECEIPT;
  replace_present = 1;    // replace previous message if present
  data_coding = DATA_CODE_DEFAULT;
  sm_id = 0;              // short message id set by SMSC during *_rsp messages
  interface_ver = SMS_VER;
} // end constructor



//===============================================================================|
/**
 * @brief Destroy the Sms:: Sms object
 * 
 */
Sms::~Sms()
{
  Unbind();
  TcpClient::Disconnect();
} // end Destructor



//===============================================================================|
/**
 * @brief Sends a bind_transceiver message to SMSC, which enables the interface
 *  to both send and receive sms messages. If successful it sets, the state of
 *  the sms interface to bound.
 * 
 * @param sys_id a system identefier (acquired from SMSC)
 * @param pwd the pwd given by SMSC to authenticate user
 * @return int 0 on success with interface bound; alas -2 on app error
 */
int Sms::Bind_Trx(const std::string &sys_id, const std::string &pwd)
{
  char buffer[98]{0};
  char *alias{buffer+sizeof(cmd_hdr)};
  
  // do a trivial rejection
  if (sys_id.length() > 15 || pwd.length() > 8)
  {
    err_desc = "System id and/or password does not meet SMPPv3.4 specs.";
    err_desc += " System id must not be 15 and password 8 characters long.";
    return -2;
  } // end if no good length

  if (sms_state & SMS_CONNECTED)
  {
    iCpy(alias, sys_id.c_str(), sys_id.length());
    alias += (sys_id.length() + 1);

    // password, +2 because we are skipping over system type, as null
    iCpy(alias, pwd.c_str(), pwd.length());
    alias += (pwd.length() + 2);

    // interface version, type of number and numbering plan indicator
    iCpy(alias++, &interface_ver, 1);
    iCpy(alias++, &ton, 1);
    iCpy(alias++, &npi, 1);
    ++alias;    // for address range

    SET_PDU_HEADR(cmd_hdr, alias - buffer, bind_transceiver, 0, ++seq_num);
    iCpy(buffer, &cmd_hdr, sizeof(cmd_hdr));

    if (bdebug)
      Dump_Hex(buffer, alias - buffer);

    if (Send(buffer, alias - buffer) < 0)
      return -1;

    sms_state = SMS_BOUND_TRX;
    return 0;
  } // end if connected

  err_desc = "Underlying network not connected.";
  return -2;
} // end Bind_Tx




//===============================================================================|
/**
 * @brief Sends Unbind signal to terminate the bound state
 * 
 * @return int 0 on success alas -ve on fail
 */
int Sms::Unbind()
{
  if (sms_state & SMS_BOUND_TRX)
  {
    SET_PDU_HEADR(cmd_hdr, sizeof(cmd_hdr), unbind, 0, ++seq_num);
    if (bdebug)
      Dump_Hex((const char*)&cmd_hdr, sizeof(cmd_hdr));

    if (Send((const char*)&cmd_hdr, sizeof(cmd_hdr)) < 0)
      return -1;

    return 0;
  } // end if bount 

  err_desc = "Interface not bound";
  return -2;
} // end UnBind



//===============================================================================|
/**
 * @brief Sends an Unbind_Response with an OK to terminate the session. 
 * 
 * @return int 0 on success -ve on fail
 */
int Sms::Unbind_Resp()
{
  if (sms_state & SMS_BOUND_TRX)
  {
    SET_PDU_HEADR(cmd_hdr, sizeof(cmd_hdr), unbind_resp, 0, cmd_hdr.sequence_num);
    if (bdebug)
      Dump_Hex((const char*)&cmd_hdr, sizeof(cmd_hdr));

    if (Send((const char*)&cmd_hdr, sizeof(cmd_hdr)) < 0)
      return -1;

    return 0;
  } // end if bount 

  err_desc = "Interface not bound";
  return -2;
} // end UnBind_Resp



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
    SET_PDU_HEADR(cmd_hdr, sizeof(cmd_hdr), enquire_link, 0, ++seq_num);
    if (bdebug)
      Dump_Hex((const char*)&cmd_hdr, sizeof(cmd_hdr));

    if (Send((const char*)&cmd_hdr, sizeof(cmd_hdr)) < 0)
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
    SET_PDU_HEADR(cmd_hdr, sizeof(cmd_hdr), enquire_link_resp, ESME_ROK, cmd_hdr.sequence_num);
    if (bdebug)
      Dump_Hex((const char*)&cmd_hdr, sizeof(cmd_hdr));

    if (Send((const char*)&cmd_hdr, sizeof(cmd_hdr)) < 0)
      return -1;

    return 0;
  } // end Enquire_Rsp

  err_desc = "Interface not bound";
  return -2;
} // end Enquire_Rsp


//===============================================================================|
/**
 * @brief Submits short messages to SMSC. If the message length exceeds 254 it
 *  will use payload_message optional feild. However even here, it must not exceed
 *  65,534 characters long.
 * 
 * @param msg the message to send
 * @param dest_num phone num of the receipent
 * @return int 0 on success alas -ve on fail
 */
int Sms::Submit(std::string msg, std::string dest_num)
{
  char buffer[512 + 65'536]{0};
  char *alias{buffer + sizeof(cmd_hdr)};

  if (dest_num.length() > 20)
  {
    err_desc = "destination number can not exceed 20 characters.";
    return -2;
  } // end if dest num

  if (sms_state & SMS_BOUND_TRX)
  {
    // skip over source_addr_ton, source_addr_npi, source_addr
    *alias++ = service_type;   // the service type
    alias += 3;

    *alias++ = ton;   // dest ton
    *alias++ = npi;   // dest npi

    // copy the dest addr aka phone no
    iCpy(alias, dest_num.c_str(), dest_num.length());
    alias += (dest_num.length() + 1);

    *(alias++) = esm_class;   // esm class
    *(alias++) = 0x0;         // prtocol id
    *(alias++) = priority_flag;      // priority flag

    if (schedule_delivery_time.length() == 0)
      *(alias++) = 0x0;       // schedule delivery time
    else 
    {
      if (schedule_delivery_time.length() > 16)
      {
        err_desc = "Invaild time format in delivery time.";
        return -2;
      } // end if bad newz

      iCpy(alias, schedule_delivery_time.c_str(), schedule_delivery_time.length());
      alias += (schedule_delivery_time.length() + 1);
    } // end else scheduling

    if (validity_period.length() == 0)
      *(alias++) = 0x0;       // default validty period
    else 
    {
      if (validity_period.length() > 16)
      {
        err_desc = "Invaild time format in validity period.";
        return -2;
      } // end if bad newz

      iCpy(alias, validity_period.c_str(), validity_period.length());
      alias += (validity_period.length() + 1);
    } // end else set validity period
    
    *(alias++) = registered_delivery;      // require delivery reports
    *(alias++) = replace_present;          // replace existing
    *(alias++) = data_coding;              // data coding
    *(alias++) = 0x0;                      // sm default message id (for canned messages)
    
    u8 msg_len = msg.length();
    if (msg_len < 255)
    {
      iCpy(alias++, &msg_len, 1);
      iCpy(alias, msg.c_str(), msg_len);
      alias += (msg_len + 1);
    } // end if message less than 255 chars long
    else 
    {
      // message exceeds 254 characters. Send as payload optional data
      *(alias++) = 0x0;   // sm_length
      *(alias++) = 0x0;   // sm_message

      // trim the message if it exceeds unreasonably
      u16 param = MESSAGE_PAYLOAD;
      msg_len = (msg_len > 65354 ? 65534 : msg_len);
      iCpy(alias, &param, sizeof(u16));   // the parameter
      alias += 2;
      iCpy(alias, &msg_len, sizeof(u16)); // the length
      alias += 2;
      iCpy(alias, msg.c_str(), msg_len);
      alias += (msg_len + 1);
    } // end else long message

    SET_PDU_HEADR(cmd_hdr, alias - buffer, submit_sm, 0, ++seq_num);
    iCpy(buffer, &cmd_hdr, sizeof(cmd_hdr));

    if (bdebug)
      Dump_Hex(buffer, alias - buffer);
    
    return Send(buffer, alias - buffer);
  } // end if bounded

  err_desc = "SMS interface not bound";
  return -2;
} // end Submit



//===============================================================================|
int Sms::Process_Incoming(char *buffer, const u32 len)
{
  if (len == 0)
    return 0;

  if (bdebug)
    Dump_Hex(buffer, len);

  // now let's act on the response and do stuff up (like updating dbs and all)
  Command_Hdr_Ptr phdr = (Command_Hdr_Ptr)buffer;
  switch (NTOHL(phdr->command_id))
  {
    case bind_transceiver_resp:
    {
      if (NTOHL(phdr->command_status) != ESME_ROK)
      {
        err_desc = "Binding TRX failed";
        return -2;
      } // end if no cool

      // get the smsc name
      char *alias = buffer + sizeof(cmd_hdr);
      while (*alias != '\0')
        ++alias;
      
      smsc_name.resize(alias - (buffer + sizeof(cmd_hdr))+1);
      smsc_name.copy(buffer + sizeof(cmd_hdr), alias - (buffer + sizeof(cmd_hdr)));
      std::cout << smsc_name << std::endl;
      sms_state = SMS_BOUND_TRX;
    } return 0;

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
      cmd_hdr.sequence_num = phdr->sequence_num;
      if (Enquire_Rsp() < 0)
        return -1;
    } return 0;


    default:
      std::cout << "Is unimplemented or something ..." << std::endl;
  } // end switch

  return 0;
  } // end Process_Incomming



//===============================================================================|
/**
 * @brief Returns the current state of the sms
 * 
 * @return int one of the enum constants to define each state
 */
int Sms::Get_State() const
{
  return sms_state;
} // end Get_State



//===============================================================================|
/**
 * @brief Get's the SMSC system identifier
 * 
 * @return std::string the system identifer not more than 16 chars long
 */
std::string Sms::Get_SystemID() const
{
  return smsc_name;
} // end std::GetSystemID



//===============================================================================|
/**
 * @brief Retruns the current connectionID or descriptor
 * 
 * @return int the descriptor to the socket is returned
 */
int Sms::Get_ConnectionID() const
{
  return fds;
} // end Get_ConnectionID



//===============================================================================|
/**
 * @brief Retruns the boolean value to indicate if on debugging mode
 * 
 * @return bool debudding cap of sms
 */
bool Sms::Get_Debug() const
{
  return bdebug;
} // end Get_Debug
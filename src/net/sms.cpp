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
/**
 * @brief This macro writes info into command header structures, since filling 
 *  this structure is repeatative.
 * 
 */
#define SET_PDU_HEADR(cmd, len, id, stat, seq) { \
    cmd.command_length = htonl(len);\
    cmd.command_id = htonl(id); \
    cmd.command_status = htonl(stat); \
    cmd.sequence_num = htonl(seq); \
}




/**
 * @brief This macro reverses the endian order back into host byte order; this is
 *  done usually after reception from network.
 * 
 */
#define HOST_ENDIAN(cmd)  { \
    cmd.command_length = ntohl(cmd.command_length); \
    cmd.command_id = ntohl(cmd.command_id); \
    cmd.command_status = ntohl(cmd.command_status); \
    cmd.sequence_num = ntohl(cmd.sequence_num); \
}




#define CPY_OPTIONS(opt, popt) { \
    opt.interface_ver = popt->interface_ver; \
    opt.service_type = popt->service_type; \
    opt.src_ton = popt->src_ton; \
    opt.src_npi = popt->src_npi; \
    opt.dest_ton = popt->dest_ton; \
    opt.dest_npi = popt->dest_npi; \
    opt.esm_class = popt->esm_class; \
    opt.protocol_id = popt->protocol_id; \
    opt.priority_flag = popt->priority_flag; \
    opt.schedule_delivery_time = popt->schedule_delivery_time; \
    opt.validity_period = popt->validity_period; \
    opt.registered_delivery = popt->registered_delivery; \
    opt.replace_present = popt->replace_present; \
    opt.data_coding = popt->data_coding; \
    opt.sm_id = popt->sm_id; \
}



//===============================================================================|
//        GLOBALS
//===============================================================================|
void Heartbeat(Sms *psms);






//===============================================================================|
//        CLASS IMP
//===============================================================================|
/**
 * @brief Construct a new Sms:: Sms object Initalizes the object to an empty and
 *  disconnected state.
 * 
 */
Sms::Sms()
    :phbeat{nullptr}
{
    heartbeat_interval = HEARTBEAT_INTERVAL;
    sms_state = SMS_DISCONNECTED;
    seq_num = 0;

    bheartbeat = false;
    bdebug = false;

    smsc_id = "";
    system_id = "";
    pwd = "";

    iZero(snd_buffer, SMS_BUFFER_SIZE);
    iZero(rcv_buffer, SMS_BUFFER_SIZE);
    iZero(err_desc, MAXLINE);
} // end Constructor




//===============================================================================|
/**
 * @brief Construct a new Sms:: Sms object Connects to server/SMSC using TCP/IP 
 *  enabled network. It also initalizes the members into default state.
 * 
 * @param hostname host ip, name or SMSC
 * @param port port for SMSC
 * @param sys_id the system id provided for SMS subscriber from SMCS
 * @param pwd the password provided alongside the system id
 * @param sms_no the sms_id or phone number 
 * @param mode the binding mode by default is transceiver
 * @param hbt toggles heartbeat signal; i.e. enquire_link command
 * @param debug toggles debuger mode; deafult is false
 * 
 * @throws std::runtime_error
 */
Sms::Sms(const std::string hostname, const std::string port, const std::string sys_id, 
    const std::string pwd, const std::string sms_no, const u32 mode, bool hbt, bool debug)
    :phbeat{nullptr}
{
    heartbeat_interval = HEARTBEAT_INTERVAL;
    sms_state = SMS_DISCONNECTED;
    seq_num = 0;

    bheartbeat = hbt;
    bdebug = debug;

    smsc_id = "";
    
    iZero(snd_buffer, SMS_BUFFER_SIZE);
    iZero(rcv_buffer, SMS_BUFFER_SIZE);
    iZero(err_desc, MAXLINE);

    if (Startup(hostname, port, sys_id, pwd, sms_no, mode, hbt, debug) < 0)
        throw std::runtime_error(strerror(errno));
} // end constructor



//===============================================================================|
/**
 * @brief Destroy the Sms:: Sms object Terminates the SMS Session and destories
 *  the object.
 * 
 */
Sms::~Sms()
{
    Disconnect();
    Shutdown();
} // end Destructor



//===============================================================================|
/**
 * @brief Start's the Sms and runs function Run() as a thread process that run's
 *  through out the lifetime of this object. It also start's the heartbeat thead
 *  that is used to keep connections alive if it's last parameter is true.
 * 
 * @param hostname ip/hostname to connect to
 * @param port the port aka service name for server
 * @param sys_id the system id provided to EMSE by SMCS
 * @param pwd the password provided along with system id
 * @param sms_no the phone/cell sms id of EMSE
 * @param mode the binding mode recvr vs transmitter vs transvr
 * @param hbeat toggles the heartbeat signal
 * @param dbug toggles debug mode, which dumps hex views to console
 * 
 * @return int a 0 on success, alas -ve.
 */
int Sms::Startup(const std::string hostname, const std::string port, 
    const std::string sys_id, const std::string pwd, const std::string sms_no, 
    const u32 mode, const bool hbeat, const bool dbug)
{
    int ret;            // get's return values   
    bdebug = dbug;

    if ( (ret = tcp.Connect(hostname, port)) < 0)
        return ret;

    u32 on{1};
    if (ioctl(tcp.Get_Socket(), FIONBIO, (char *)&on) < 0)
        return -1;

    sms_state = SMS_CONNECTED;
    this->system_id = sys_id;
    this->pwd = pwd;
    this->sms_id = (sms_no.length() > 20 ? "" : sms_no);

    if ( (ret = Bind(mode)) < 0)
        return ret;
    
    bheartbeat = hbeat;
    return 0;
} // end Startup



//===============================================================================|
/**
 * @brief Disconnects the sms session, first it sends UNBIND_TRX signal to SMCS
 *  it then wait's for response and Disconnects the TCP session.
 * 
 * @return int 0 on success alas a -1
 */
int Sms::Shutdown()
{
    if (phbeat)
    {
        phbeat->join();
        phbeat = nullptr;
    } // end if heartbeat on

    if (tcp.Disconnect() < 0)
        return -1;

    sms_state = SMS_DISCONNECTED;
    return 0;
} // end Shutdown



//===============================================================================|
/**
 * @brief Sends unbind signal to disconnect from service at application level.
 * 
 * @return int 0 on success -1 on fail.
 */
int Sms::Disconnect()
{
    if (!(sms_state & SMS_BOUNDED))
    {
        snprintf(err_desc, MAXLINE, "Not authorized. Please Bind interface first.");
        return -2;
    } // end if bounded not

    if (Unbind() < 0)
        return -1;
        
    return 0;
} // end Disconnect



//===============================================================================|
/**
 * @brief Sends a single SMS message to recepient at dest_num.
 * 
 * @param msg The message to send no limit the on the length of message.
 * @param dest_num The destination number
 * @param poptions SMPP options controlling the specific message
 * 
 * @return int a 0 on success alas -ve on fail
 */
int Sms::Send_Message(const std::string msg, const std::string dest_num, 
    const Smpp_Options_Ptr poptions)
{
    int ret;
    size_t sent{0};

    Smpp_Options_Ptr popt = poptions == nullptr ? &options : poptions;
    if (sms_state & SMS_BOUNDED)
    {
        size_t len = msg.length();
        while (len > 0)
        {
            size_t snd_len = (len > 65'534 ? 65'534 : len);
            ret = Submit(msg.substr(sent, snd_len), dest_num, popt);
            if (ret < 0)
                return ret;

            sent += snd_len;
            len -= snd_len;
        } // end while
        
        return 0;
    } // end if

    snprintf(err_desc, MAXLINE, "Not authorized. Please Bind interface first.");
    return -2;
} // end Send_Message



//===============================================================================|
/**
 * @brief Sends a single message to multiple parites, upto 254 as defined by the
 *  protocol. The function also trims messages that exceed 65,535 characters and
 *  sends them in batches one after the other.
 * 
 * @param msg the message to send at once
 * @param dest_nums list of destination numbers
 * @param poptions various smpp based options for the specific message
 * 
 * @return int 0 on success alas -ve on fail.
 */
int Sms::Send_Bulk_Message(const std::string msg, std::list<std::string> &dest_nums,
    const Smpp_Options_Ptr poptions)
{
    int ret;
    size_t sent{0};
    std::queue<std::string> dst;         // destination of 254 queues

    for (std::string s : dest_nums)
    {
        if (s.length() > 20)
        {
            snprintf(err_desc, MAXLINE, "Invalid length. Number %s is too long.", s.c_str());
            return -2;
        } // end if dest num

        dst.emplace(s);
    } // end for

    Smpp_Options_Ptr popt = poptions == nullptr ? &options : poptions;
    if ( sms_state & SMS_BOUNDED)
    {
        size_t len = msg.length();
        while (len > 0)
        {
            size_t snd_len = (len > 65'534 ? 65'534 : len);
            ret = Submit_Multi(msg.substr(sent, snd_len), dst, popt);
            if (ret < 0)
                return ret;

            sent += snd_len;
            len -= snd_len;
        } // end while
        
        return 0;
    } // end if bounded

    snprintf(err_desc, MAXLINE, "Not authorized. Please Bind interface first.");
    return -2;
} // end Send_Bulk_Message





//===============================================================================|
//      ACCESSORS
//===============================================================================|
/**
 * @brief Returns the connection indentifier for this session, i.e. the socket
 *  descriptor.
 * 
 * @return int a socket descriptor if connected.
 */
int Sms::Get_Connection() const
{
    return tcp.Get_Socket();
} // end Get_Connection



//===============================================================================|
/**
 * @brief Set's the Heartbeat interval. The heartbeat is an enquire request sent
 *  from the system every set interval from the thread function.
 * 
 * @param interval the interval of heartbeat in milliseconds
 */
void Sms::Set_HB_Interval(const u32 interval)
{
    heartbeat_interval = interval;
} // end Set_State



//===============================================================================|
/**
 * @brief Return's the current set heartbeat interval
 * 
 * @return u32 the heartbeat interval in miliseconds
 */
u32 Sms::Get_HB_Interval() const
{
    return heartbeat_interval;
} // end Get_HB_Interval



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
    return smsc_id;
} // end std::GetSystemID



//===============================================================================|
/**
 * @brief Retruns the error description for application specific errors
 * 
 * @return application specific error description
 */
std::string Sms::Get_Err() const
{
    std::cout << err_desc << std::endl;
    return err_desc;
} // end Get_ConnectionID



//===============================================================================|
/**
 * @brief Toggles Heartbeat on/off.
 * 
 */
void Sms::Toggle_Heartbeat() 
{
    bheartbeat = !bheartbeat;
    if (bheartbeat)
    {
        phbeat = new std::thread(Heartbeat, this);
    } // end if turn on
    else
    {
        phbeat->join();
        phbeat = nullptr;
    } // end else turing off
} // end Get_Debug



//===============================================================================|
/**
 * @brief Turns debugging mode on/off
 * 
 */
void Sms::Toggle_Debug()
{
    bdebug = !bdebug;
} // end Toggle_Debug





//===============================================================================|
//      UTILS
//===============================================================================|
/**
 * @brief Sends a bind_* message to SMCS for authentication or authorization. The
 *  message is sent during startup or in response to outbind message.
 * 
 * @param command_id one of SMPP v3.4 binding modes. Defaults to bind_transceiver
 * 
 * @return int 0 on success with interface bound; alas -2 on app error
 */
int Sms::Bind(const u32 command_id)
{
    if ( !(sms_state & SMS_CONNECTED))
    {
        snprintf(err_desc, MAXLINE, "Interface is not connected to a network.");
        return -2;
    } // end if not connected
    
    char *alias{snd_buffer+sizeof(cmd_hdr)};

    if (system_id.length() > 15 || pwd.length() > 8)
    {
        snprintf(err_desc, MAXLINE, "System id and/or password is too long.");
        return -2;
    } // end if no good length

    iCpy(alias, system_id.c_str(), system_id.length());
    alias += system_id.length();
    *alias++ = 0x0;

    // password, followed by system types
    iCpy(alias, pwd.c_str(), pwd.length());
    alias += pwd.length();
    *alias++ = 0x0;

    // service type, interface version, type of number and numbering plan indicator
    *alias++ = options.service_type;
    *alias++ = options.interface_ver;
    *alias++ = options.src_ton;
    *alias++ = options.src_npi;
    *alias++ = 0x0;    // for address range

    SET_PDU_HEADR(cmd_hdr, alias - snd_buffer, command_id, 0, ++seq_num);
    iCpy(snd_buffer, &cmd_hdr, sizeof(cmd_hdr));

    if ( tcp.Send(snd_buffer, alias - snd_buffer) < 0)
        return -1;

    if (bdebug)
      Dump_Hex(snd_buffer, alias - snd_buffer);

    return 0;
} // end Bind



//===============================================================================|
/**
 * @brief This message is sent from SMCS to an ESME to generate a bind_receiver
 *  signal. This is probably sent when ESME is connected as transmitter only.
 * 
 * @return int 0 on success, alas -1 on fail.
 */
int Sms::Outbind()
{
    int ret;

    if ( (ret = Bind(bind_receiver) )< 0)
        return ret;

    return 0;
} // end Outbind



//===============================================================================|
/**
 * @brief Sends Unbind signal to terminate the bound state
 * 
 * @return int 0 on success alas -ve on fail
 */
int Sms::Unbind()
{
    if ( !(sms_state & SMS_BOUNDED))
    {
        snprintf(err_desc, MAXLINE, "Not authorized. Please Bind interface first.");
        return -2;
    } // end if
    
    SET_PDU_HEADR(cmd_hdr, sizeof(cmd_hdr), unbind, 0, ++seq_num);
    if ( tcp.Send((const char*)&cmd_hdr, sizeof(cmd_hdr)) < 0)
        return -1;
    
    if (bdebug)
      Dump_Hex((const char*)&cmd_hdr, sizeof(cmd_hdr));

    return 0;
} // end UnBind



//===============================================================================|
/**
 * @brief Sends Unbind response to unbind signals
 * 
 * @param resp the response status code by default is ESME_ROK
 * 
 * @return int 0 on success alas -ve on fail
 */
int Sms::Unbind_Resp(const u32 resp)
{
    if ( !(sms_state & SMS_CONNECTED))
    {
        snprintf(err_desc, MAXLINE, "Not authorized. Please Bind interface first.");
        return -2;
    } // end if not connected
    
    SET_PDU_HEADR(cmd_hdr, sizeof(cmd_hdr), unbind_resp, resp, cmd_rsp.sequence_num);
    if ( tcp.Send((const char*)&cmd_hdr, sizeof(cmd_hdr)) < 0)
        return -1;

    if (bdebug)
      Dump_Hex((const char*)&cmd_hdr, sizeof(cmd_hdr));

    return 0;
} // end Unbind_Resp



//===============================================================================|
/**
 * @brief Sends a generic negative acknowledgement whenever the sizeof the sent
 *  command structure is not the same size as SMPP v3.4 command structure or when
 *  an unknown command_id is sent by the peer, i.e. SMCS
 * 
 * @return int 0 on success alas -1 on fail
 */
int Sms::Generic_Nack()
{
    SET_PDU_HEADR(cmd_hdr, sizeof(cmd_hdr), generic_nack, ESME_ROK, ++seq_num);
    if ( tcp.Send((const char*)&cmd_hdr, sizeof(cmd_hdr)) < 0)
        return -1;

    if (bdebug)
        Dump_Hex((const char*)&cmd_hdr, sizeof(cmd_hdr));
    
    return 0;
} // end Generic_Nack



//===============================================================================|
/**
 * @brief Submits short messages to SMSC. If the message length exceeds 254 it
 *  will use payload_message optional feild. However even here, it must not exceed
 *  65,534 characters long.
 * 
 * @param msg the message to send
 * @param dest_num phone num of the receipent
 * @param poptions SMPP options controlling the specific message
 * @param can_id 0 default to mean not canned (1-255 SMSC specific canned messages)
 * 
 * @return int 0 on success alas -ve on fail
 */
int Sms::Submit(const std::string &msg, const std::string &dest_num,
    const Smpp_Options_Ptr poptions, const u8 can_id)
{
    if ( !(sms_state & SMS_BOUNDED))
    {
        snprintf(err_desc, MAXLINE, "Not authorized. Please Bind interface first.");
        return -2;
    } // end if not bounded

    if (dest_num.length() > 20)
    {
        snprintf(err_desc, MAXLINE, "Buffer overflow. Destination number exceeds space.");
        return -2;
    } // end if dest num

    char *alias{snd_buffer + sizeof(cmd_hdr)};

    // skip over source_addr_ton, source_addr_npi, source_addr
    *alias++ = poptions->service_type;    // the service type
    *alias++ = poptions->src_ton;         // the source type of number
    *alias++ = poptions->src_npi;         // the source numbering plan indicator

    if (sms_id.length() == 0)
        *alias++ = 0x0;
    else
    {
        iCpy(alias, sms_id.c_str(), sms_id.length());
        alias += sms_id.length();
        *alias++ = 0x0;
    } // end else

    *alias++ = poptions->dest_ton;   // dest ton
    *alias++ = poptions->dest_npi;   // dest npi

    // copy the dest addr aka phone no
    iCpy(alias, dest_num.c_str(), dest_num.length());
    alias += dest_num.length();
    *alias++ = 0x0;

    *alias++ = poptions->esm_class;           // esm class
    *alias++ = 0x0;                         // prtocol id
    *alias++ = poptions->priority_flag;       // priority flag

    if (poptions->schedule_delivery_time.length() == 0)
        *alias++ = 0x0;               // schedule delivery time
    else 
    {
        if (poptions->schedule_delivery_time.length() > 16)
        {
            snprintf(err_desc, MAXLINE, "Invalid time format in delivery time");
            return -2;
        } // end if bad newz

        iCpy(alias, poptions->schedule_delivery_time.c_str(), 
            poptions->schedule_delivery_time.length());
        alias += poptions->schedule_delivery_time.length();
        *alias++ = 0x0;
    } // end else scheduling

    if (poptions->validity_period.length() == 0)
      *alias++ = 0x0;               // default validty period
    else 
    {
        if (poptions->validity_period.length() > 16)
        {
            snprintf(err_desc, MAXLINE, "Invalid time format in validty period.");
            return -2;
        } // end if bad newz

        iCpy(alias, poptions->validity_period.c_str(), 
            poptions->validity_period.length());
        alias += poptions->validity_period.length();
        *alias++ = 0x0;
    } // end else set validity period
    
    *alias++ = poptions->registered_delivery; // require delivery reports
    *alias++ = poptions->replace_present;     // replace existing
    *alias++ = poptions->data_coding;         // data coding
    *alias++ = can_id;                      // sm default message id (for canned messages)
    
    // skip these if we are processing canned messages
    if (can_id == 0)
    {
        u32 len = msg.length() + 1;
        if (len < 255)
        {
            iCpy(alias++, (u8*)&len, sizeof(u8));
            iCpy(alias, msg.c_str(), len - 1);
            alias += len;
            *alias = 0x0;
        } // end if message less than 255 chars long
        else 
        {
            // message exceeds 254 characters. Send as payload optional data
            *alias = 0x0;
            *alias = 0x0;

            // send large messages as TLV data
            u16 param = MESSAGE_PAYLOAD;
            iCpy(alias, &param, sizeof(u16));   // the parameter
            alias += sizeof(16);
            param = htons(len);
            iCpy(alias, &param, sizeof(u16));   // the length
            alias += sizeof(u16);
            iCpy(alias, msg.c_str(), len - 1);  // the value
            alias += (len);
            *alias++ = 0x0;
        } // end else long message
    } // end if not canned
    else
    {
        // we are processing canned messages from SMSC;
        //  skip over the message len and message
        *alias = 0x0;
        *alias = 0x0;
    } // end else canned

    // let's add a reference number to this message sent.
    u16 param = USER_MESSAGE_REFERENCE;
    iCpy(alias, &param, sizeof(u16));
    alias += sizeof(u16);
    param = htons(sizeof(u16));
    iCpy(alias, &param, sizeof(u16));
    alias += sizeof(u16);
    param = htons(++seq_num);
    iCpy(alias, &param, sizeof(u16));
    alias += sizeof(u16);

    SET_PDU_HEADR(cmd_hdr, alias - snd_buffer, submit_sm, 0, seq_num);
    iCpy(snd_buffer, &cmd_hdr, sizeof(cmd_hdr));

    if ( tcp.Send(snd_buffer, alias - snd_buffer) < 0)
        return -1;

    Single_Sms_Info info{MSG_STATE_SENT, "", msg, dest_num};
    CPY_OPTIONS(info.opts, poptions);
    queued_msg.emplace(ntohl(cmd_hdr.sequence_num), info);

    if (bdebug)
      Dump_Hex(snd_buffer, alias - snd_buffer);
    
    return 0;
} // end Submit



// //===============================================================================|
// /**
//  * @brief Sends an SMS submit_resp in response to submit_sm messages that may
//  *  have originated from an SMCS center. This should never be encountered in normal
//  *  operation.
//  * 
//  * @param resp the response code; see ESME_* constants in sms.h
//  * 
//  * @return int 0 on success -1 on fail
//  */
// int Sms::Submit_Rsp(const u32 resp)
// {
//   char buf[98]{0};
//   std::string msgid = std::to_string(++sub_id);
  
//   iCpy(buf + sizeof(cmd_hdr), msgid.c_str(), msgid.length());
//   SET_PDU_HEADR(cmd_hdr, sizeof(cmd_hdr) + msgid.length() + 1, submit_sm_resp, 
//     ESME_ROK, cmd_hdr.sequence_num);
  
//   iCpy(buf, (char *)&cmd_hdr, sizeof(cmd_hdr));
//   return ptcp->Send(buf, sizeof(cmd_hdr) + msgid.length() + 1);
// } // end Submit_Rsp



//===============================================================================|
/**
 * @brief Same as submit_sm command, but this sends single message to multiple
 *  clients at once, upto 254 as defined by the protocol.
 * 
 * @param msg the message to send
 * @param dest_nums queue of destination numbers
 * @param poptions SMPP options controlling the specific message
 * @param can_id canned id if not 0
 * 
 * @return int 0 on success, -ve on fail.
 */
int Sms::Submit_Multi(const std::string &msg, std::queue<std::string> dest_nums, 
    const Smpp_Options_Ptr poptions, const u8 can_id)
{
    char *alias{snd_buffer + sizeof(cmd_hdr)};
    size_t len = dest_nums.size();

    while ( len > 0)
    {
        if (sms_state & SMS_BOUNDED)
        {
            // skip over source_addr_ton, source_addr_npi, source_addr
            *alias++ = poptions->service_type;   // the service type
            alias += 3;

            // set the number of dests for this round
            u8 cpy_len = (len > 254 ? 254 : 254 - len);
            *alias++ = cpy_len;

            // SME Struct'ish'
            for (u8 index = 0; index < cpy_len; index++)
            {
                std::string dest_num = dest_nums.front();
                dest_nums.pop();

                *alias++ = DL_SME_ADDRESS;    // always use sme address
                *alias++ = poptions->dest_ton;   // dest ton
                *alias++ = poptions->dest_npi;   // dest npi

                // copy the dest addr aka phone no
                iCpy(alias, dest_num.c_str(), dest_num.length());
                alias += (dest_num.length() + 1);
            } // end for

            *(alias++) = poptions->esm_class;           // esm class
            *(alias++) = 0x0;                 // prtocol id
            *(alias++) = poptions->priority_flag;       // priority flag

            if (poptions->schedule_delivery_time.length() == 0)
                *(alias++) = 0x0;               // schedule delivery time
            else 
            {
                if (poptions->schedule_delivery_time.length() > 16)
                {
                    snprintf(err_desc, MAXLINE, "Invalid time format in delivery time.");
                    return -2;
                } // end if bad newz

                iCpy(alias, poptions->schedule_delivery_time.c_str(), 
                    poptions->schedule_delivery_time.length());
                alias += (poptions->schedule_delivery_time.length() + 1);
            } // end else scheduling

            if (poptions->validity_period.length() == 0)
                *(alias++) = 0x0;               // default validty period
            else 
            {
                if (poptions->validity_period.length() > 16)
                {
                    snprintf(err_desc, MAXLINE, "Invalid time format in validity period");
                    return -2;
                } // end if bad newz

                iCpy(alias, poptions->validity_period.c_str(), 
                    poptions->validity_period.length());
                alias += (poptions->validity_period.length() + 1);
            } // end else set validity period

            *(alias++) = poptions->registered_delivery; // require delivery reports
            *(alias++) = poptions->replace_present;     // replace existing
            *(alias++) = poptions->data_coding;         // data coding
            *(alias++) = can_id;              // sm default message id (for canned messages)

            // skip these if we are processing canned messages
            if (can_id == 0)
            {
                u16 msg_len = (u16)msg.length() + 1;
                if (msg_len < 255)
                {
                    iCpy(alias++, (u8*)&msg_len, sizeof(u8));
                    iCpy(alias, msg.c_str(), msg_len - 1);
                    alias += (msg_len + 1);
                } // end if message less than 255 chars long
                else 
                {
                    // message exceeds 254 characters. Send as payload optional data
                    alias += 2;

                    // trim the message if it exceeds unreasonably
                    u16 param = MESSAGE_PAYLOAD;
                    iCpy(alias, &param, sizeof(u16));   // the parameter
                    alias += sizeof(u16);
                    param = htons(msg_len);
                    iCpy(alias, &param, sizeof(u16)); // the length
                    alias += sizeof(u16);
                    iCpy(alias, msg.c_str(), msg_len - 1);  // the value
                    alias += (msg_len + 1);
                } // end else long message
            } // end if not canned
            else
            {
              // we are processing canned messages from SMSC;
              //  skip over the message len and message
              alias += 2;
            } // end else canned

            // let's add a reference number to this message sent.
            u16 param = USER_MESSAGE_REFERENCE;
            iCpy(alias, &param, sizeof(u16));
            alias += sizeof(u16);
            param = sizeof(u16);
            iCpy(alias, &param, sizeof(u16));
            alias += sizeof(u16);
            param = htons(++seq_num);
            iCpy(alias, &param, sizeof(u16));
            alias += sizeof(u16);

            SET_PDU_HEADR(cmd_hdr, alias - snd_buffer, submit_multi, 0, seq_num);
            iCpy(snd_buffer, &cmd_hdr, sizeof(cmd_hdr));

            if ( tcp.Send(snd_buffer, alias - snd_buffer) < 0)
                return -1;

            Bulk_Sms_Info info{MSG_STATE_SENT, "", msg, dest_nums};
            CPY_OPTIONS(info.opts, poptions);
            queued_blk_msg.emplace(ntohl(cmd_hdr.sequence_num), info);

            if (bdebug)
              Dump_Hex(snd_buffer, alias - snd_buffer);

            return 0;
        } // end if bounded
        else 
        {
            snprintf(err_desc, MAXLINE, "Not authorized. Please Bind interface first.");
            return -2;
        } // end else

        len -= 254;
    } // end while

    return 0;
} // end Submit_Multi



//===============================================================================|
/**
 * @brief Sends SMS Query message, that queries the status of sent messages from
 *  SMCS given the message id.
 * 
 * @param msg_id the identification acquired from smcs
 * @param poptions pointer to smpp options pretaining the queried message
 * @param src_addr the address for the source of sms
 * 
 * @return int 0 on success, -ve on fail
 */
int Sms::Query(const std::string &msg_id, const Smpp_Options_Ptr poptions, 
    const std::string src_addr)
{
    if ( !(sms_state & SMS_BOUNDED))
        return -2;

    char *alias{snd_buffer + sizeof(cmd_hdr)};

    // skip over source_addr_ton, source_addr_npi, source_addr
    if (msg_id.length() > 64)
    {
        snprintf(err_desc, MAXLINE, "Invalid msg id to query %s", msg_id.c_str());
        return -2;
    } // end if

    iCpy(alias, msg_id.c_str(), msg_id.length());
    alias += msg_id.length();
    *alias++ = 0x0;
    
    *alias = poptions->src_ton;
    *alias = poptions->src_npi;

    if (src_addr.length() == 0)
        *alias++ = 0x0;
    else
    {
        iCpy(alias, src_addr.c_str(), src_addr.length());
        alias += src_addr.length();
        *alias++ = 0x0;
    } // end else

    SET_PDU_HEADR(cmd_hdr, alias - snd_buffer, query_sm, 0, ++seq_num);
    iCpy(snd_buffer, &cmd_hdr, sizeof(cmd_hdr));
    if ( tcp.Send(snd_buffer, alias - snd_buffer) < 0)
        return -1;

    if (bdebug)
        Dump_Hex(snd_buffer, alias - snd_buffer);
    
    return 0;
} // end Query



//===============================================================================|
/**
 * @brief Handles a query response singal and takes on realtime actions based on
 *  the message feild shown
 * 
 * @param buffer the buffer containing the extra info
 * @param len length of buffer above
 * 
 * @return int 0 on success alias -ve. 
 */
int Sms::Handle_Query(char *err, const size_t buf_len)
{
    if (cmd_rsp.command_status == ESME_ROK)
    {
        // skipping over msg_id and final_date
        const char *alias = rcv_buffer + sizeof(cmd_hdr) + strlen(rcv_buffer);
        std::string final_date = alias;
        alias += final_date.length();

        // check the message states
        switch (*((const u8 *)alias))
        {
            case SMPP_DELIVERED:
            {
                auto it = queued_msg.find(cmd_rsp.sequence_num);
                if (it != queued_msg.end())
                {
                    queued_msg.erase(cmd_rsp.sequence_num);

                    // signal db to update

                    return 0;
                } // end if

                auto it2 = queued_blk_msg.find(cmd_rsp.sequence_num);
                if (it2 != queued_blk_msg.end())
                {
                    queued_blk_msg.erase(cmd_hdr.sequence_num);

                    // signal

                    return 0;
                } // end if
            } break;

            case SMPP_EXPIRED:
            case SMPP_DELETED:
            case SMPP_UNDLIVERABLE:
            case SMPP_UNKOWN:
            case SMPP_REJECTED:
            {
                auto it = queued_msg.find(cmd_rsp.sequence_num);
                if (it != queued_msg.end())
                    queued_msg.erase(cmd_rsp.sequence_num);

                if (it == queued_msg.end())
                {
                    auto it2 = queued_blk_msg.find(cmd_rsp.sequence_num);
                    if (it2 != queued_blk_msg.end())
                        queued_blk_msg.erase(cmd_rsp.sequence_num);

                    snprintf(err, buf_len, 
                        "Message is either deleted, expired, undliverable, \
                        invalid, or rejected. Error code = %d", *((const u8 *)alias));
                } // end if not from single
                return -2;
            } break;

        } // end switch
    } // end if
    else
    {
        snprintf(err, buf_len, "Handle_Query returned error: 0x%08X.\n", 
            cmd_rsp.command_status);

        return -2;
    } // end else

    return 0;
} // end Query_Rsp



//===============================================================================|
/**
 * @brief This function is used to cancel pending SMS messages from the SMCS.
 * 
 * @param msg_id the id for message sent from SMCS
 * @param popts the options associated with the message
 * @param src_addr the tel# or source address
 * 
 * @return int 0 on success alas -ve
 */
int Sms::Cancel(const std::string msg_id, const Smpp_Options_Ptr popts, 
    const std::string src_addr)
{
    if (!(sms_state & SMS_BOUNDED))
    {
        snprintf(err_desc, MAXLINE, "Not authorized. Please Bind interface first.");
        return -2;
    } // end if not bounded

    char *alias{snd_buffer + sizeof(cmd_hdr)};

    *alias++ = popts->service_type;
    iCpy(alias, msg_id.c_str(), msg_id.length());
    alias += msg_id.length();
    *alias++ = 0x0;

    *alias++ = popts->src_ton;
    *alias++ = popts->src_npi;
    iCpy(alias, src_addr.c_str(), src_addr.length());
    alias += src_addr.length();
    *alias++ = 0x0;

    *alias++ = popts->dest_ton;
    *alias++ = popts->dest_npi;
    *alias = 0x0;       // null dests since we've message id

    SET_PDU_HEADR(cmd_hdr, (alias - snd_buffer), cancel_sm, 
        ESME_ROK, ++seq_num);
    iCpy(snd_buffer, &cmd_hdr, sizeof(cmd_hdr));

    if ( tcp.Send(snd_buffer, alias - snd_buffer) < 0)
        return -1;

    if (bdebug)
        Dump_Hex(snd_buffer, alias - snd_buffer);

    return 0;
} // end Cancel



//===============================================================================|
/**
 * @brief This message is used to replace pending messages from SMCS by using the
 *  message id sent from SMCS
 * 
 * @param msg_id the message id sent from SMCS
 * @param popts the options for the particular message
 * @param msg a new message to replace the old
 * @param src_addr the soruce tel #
 * 
 * @return int 0 on success alas -1 on fai;/ 
 */
int Sms::Replace(const std::string msg_id, const Smpp_Options_Ptr popts, 
    const std::string msg, const std::string src_addr)
{
    if (!(sms_state & SMS_BOUNDED))
    {
        snprintf(err_desc, MAXLINE, "Not authorized. Please Bind interface first.");
        return -2;
    } // end if not bounded

    char *alias{snd_buffer + sizeof(cmd_hdr)};

    iCpy(alias, msg_id.c_str(), msg_id.length());
    alias += msg_id.length();
    *alias++ = 0x0;

    *alias++ = popts->src_ton;
    *alias++ = popts->src_npi;
    iCpy(alias, src_addr.c_str(), src_addr.length());
    alias += src_addr.length();
    *alias++ = 0x0;

    iCpy(alias, popts->schedule_delivery_time.c_str(), popts->schedule_delivery_time.length());
    alias += popts->schedule_delivery_time.length();
    *alias++ = 0x0;

    iCpy(alias, popts->validity_period.c_str(), popts->validity_period.length());
    alias += popts->validity_period.length();
    *alias++ = 0x0;

    *alias++ = popts->registered_delivery;
    *alias++ = 0;
    *alias++ = msg.length();
    iCpy(alias, msg.c_str(), msg.length());
    *alias = 0x0;

    SET_PDU_HEADR(cmd_hdr, (alias - snd_buffer), replace_sm, 
        ESME_ROK, ++seq_num);
    iCpy(snd_buffer, &cmd_hdr, sizeof(cmd_hdr));

    if ( tcp.Send(snd_buffer, alias - snd_buffer) < 0)
        return -1;

    if (bdebug)
        Dump_Hex(snd_buffer, alias - snd_buffer);
    
    return 0;
} // end Replace



//===============================================================================|
/**
 * @brief Send's an enquire link to SMSC; this is nice for heartbeat implementaton
 * 
 * @return int -ve on fail alas 0 on success
 */
int Sms::Enquire()
{
    if ( !(sms_state & SMS_BOUNDED))
    {
        snprintf(err_desc, MAXLINE, "Not authorized. Please Bind interface first.");
        return -2;
    } // end if not connected

    SET_PDU_HEADR(cmd_hdr, sizeof(cmd_hdr), enquire_link, 0, ++seq_num);
    if ( tcp.Send((const char*)&cmd_hdr, sizeof(cmd_hdr)) < 0)
        return -1;

    if (bdebug)
      Dump_Hex((const char*)&cmd_hdr, sizeof(cmd_hdr));

    return 0;
} // end Enquire



//===============================================================================|
/**
 * @brief Sends an enquire_link response to SMSC; since the actual enquire link
 *  can be initiated from both sides at any moment
 * 
 * @param resp the response code for the Enquire; see ESME_* constants in sms.h
 * 
 * @return int a -ve on fail, 0 on success
 */
int Sms::Enquire_Rsp(const u32 resp)
{
    if ( !(sms_state & SMS_BOUNDED))
        return -2;

    SET_PDU_HEADR(cmd_hdr, sizeof(cmd_hdr), enquire_link_resp, resp, 
        cmd_hdr.sequence_num);

    if ( tcp.Send((const char*)&cmd_hdr, sizeof(cmd_hdr)) < 0)
        return -1;

    if (bdebug)
      Dump_Hex((const char*)&cmd_hdr, sizeof(cmd_hdr));

    return 0;
} // end Enquire_Rsp



//===============================================================================|
/**
 * @brief A response sent to deliver_sm message requests; which is normally sent
 *  by SMCS as a report to deliveray confirmation for submitted messages
 * 
 * @param resp a response code for the request
 * 
 * @return int 0 on success alas -1
 */
int Sms::Deliver_Rsp(const u32 resp)
{
    if ( !(sms_state & SMS_BOUNDED))
        return -2;
    
    iZero(snd_buffer, sizeof(cmd_hdr) + 1);
    SET_PDU_HEADR(cmd_hdr, sizeof(cmd_hdr) + 1, deliver_sm_resp, 
        resp, cmd_hdr.sequence_num);

    iCpy(snd_buffer, (char *)&cmd_hdr, sizeof(cmd_hdr));
    if ( tcp.Send(snd_buffer, sizeof(cmd_hdr) + 1) < 0)
        return -1;

    return 0;
} // end Deliver_Rsp



//===============================================================================|
/**
 * @brief Processes incomming messages using a switch table. This is fired on async
 *  and in realtime as messages arrive.
 *  The function persumes that cmd_rsp now store's the responses as sent from
 *  the connected SMCS and they are arranged in the host-byte-order.
 * 
 * @param buffer buffer containing any extra info depending on the message type
 * @param len length of the extra info above
 * @return int 
 */
int Sms::Process_Incoming(char *err, const size_t buf_len)
{
    // let's get ready to handle this sms message, we'll do it in two parts, first
    //  we read the command header struct, followed by any extra data.
    iZero((char *)&cmd_rsp, sizeof(cmd_rsp));
    int n = tcp.Recv(rcv_buffer, SMS_BUFFER_SIZE);
    if (n < 0)
        return n;

    iCpy(&cmd_rsp, rcv_buffer, sizeof(cmd_rsp));
    HOST_ENDIAN(cmd_rsp);
    if (bdebug)
    {
        Dump_Hex(rcv_buffer, n);
    } // end if


    int ret;        // store's return values

    // now let's act on the response and do stuff up (like updating dbs and all)
    switch (cmd_rsp.command_id)
    {
        case bind_transmitter_resp:
        case bind_receiver_resp:
        case bind_transceiver_resp:
        {
            if ( (ret = Handle_Bind(err, buf_len) < 0))
                return ret;

            Print("Interface bound to SMSC: " + smsc_id);
        } break;

        case unbind:
        {
            Unbind_Resp();
            Shutdown();

            Print("Unbound and disconnected.");
        } break;

        case unbind_resp:
        {
            if (Handle_Unbind(err, buf_len) < 0)
                return -1;

            Print("Unbind response.");
        } break;

        case submit_sm_resp:
        {
            if (Handle_Submit(err, buf_len) < 0)
                return -1;

            Print("Submit Response. Message ID = " + 
                queued_msg[cmd_rsp.sequence_num].id);
        } break;

        case submit_multi_resp:
        {
            std::cout << "Submit multi response" << std::endl;
            if (cmd_rsp.command_status != ESME_ROK)
            {
                snprintf(err_desc, MAXLINE, "Submit mulit failed with error code: 0x%08X",
                    cmd_rsp.command_status);
                return -2;
            } // end if not cool

            auto it = queued_blk_msg.find(cmd_rsp.sequence_num);
            if (it != queued_blk_msg.end())
            {
                it->second.id = rcv_buffer;
                it->second.msg_state = MSG_STATE_SUBMIT;
            } // end if message found
        } break;

        case deliver_sm:
        {
            std::string phone_no;
            if ( (ret = Handle_Deliver(err, buf_len, phone_no)) < 0)
                return ret;

            Print("Delivery confirmation from number: " + phone_no);
        } break;

        case query_sm_resp:
        {
            if (Handle_Query(err, buf_len) < 0)
                return -1;

            Print("Query response.");
        } break;


        case cancel_sm_resp:
        case replace_sm_resp:
        {
            std::string rsp = cmd_rsp.command_id == cancel_sm_resp ? "cancel_sm_resp" : "replace_sm_resp";

            if (cmd_rsp.command_status != ESME_ROK)
            {
                snprintf(err, buf_len, "%s returend error code: 0x%X.", 
                    rsp.c_str(), cmd_rsp.command_status);
                return -2;
            } // end if

            Print(rsp + " response.");
        } break;

        case enquire_link:
        {
            cmd_hdr.sequence_num = cmd_rsp.sequence_num;
            if (Enquire_Rsp() < 0)
                return -1;

            Print("Enquire link.");
        } break;

        case enquire_link_resp:
        {
            // do nothing ...
            Print("Enquire link response.");
        } break;

        case outbind:
        {
            if (Outbind() < 0)
                return -1;

        } break;

        default:
            Generic_Nack();
            Print("Negative Ack.");
    } // end switch

    return 0;
} // end Process_Incomming



//===============================================================================|
/**
 * @brief Handles the bind_resp signal sent from SMCS.
 * 
 * @param err used to get error codes as a result of this call
 * @param buf_len the length of buffer for storage
 * 
 * @return int 0 on success, -ve on fail. 
 */
int Sms::Handle_Bind(char *err, const size_t len)
{
    if (cmd_rsp.command_status != ESME_ROK)
    {
        if (cmd_rsp.command_status == ESME_RINVCMDID)
        {
            snprintf(err, len, "SMCS does not support bind_trx.");
            int ret;
            if ( (ret = Bind(bind_transmitter)) < 0)
                return ret;
        } // end if bind trx fail
        else 
        {
            snprintf(err, len, "Bind failed with error code = 0x%X", 
                cmd_rsp.command_status);
            return -2;
        } // end else bind fail for other reason
    } // end if status not ok

    sms_state |= SMS_BOUNDED;
    smsc_id = rcv_buffer + sizeof(cmd_rsp);

    // igonre TLV if any

    return 0;
} // end Handle_Bind



//===============================================================================|
/**
 * @brief Handles unbind responses, termiates the session for the object
 * 
 * @param err used to get error codes as a result of this call
 * @param buf_len the length of buffer for storage
 * 
 * @return int 0 on success alas -ve on fail
 */
int Sms::Handle_Unbind(char *err, const size_t buf_len)
{
    if ( !(sms_state & SMS_CONNECTED))
        return 0;

    sms_state = SMS_CONNECTED;
    return Shutdown();
} // end Unbind_Resp



//===============================================================================|
/**
 * @brief Handles submit_sm_resp sent from SMCS. It simply saves the message_id
 *  from the SMCS into the application queue for later tracking and changes the
 *  message state to MSG_STATE_SUBMIT.
 * 
 * @param err used to get error codes as a result of this call
 * @param buf_len the length of buffer for storage
 * 
 * @return int 0 on success -ve on fail, when -2 the parameter contains a null
 *  terminated string containing the error description.
 */
int Sms::Handle_Submit(char *err, const size_t buf_len)
{
    if (cmd_rsp.command_status == ESME_ROK)
    {
        auto imsg = queued_msg.find(cmd_rsp.sequence_num);
        if (imsg != queued_msg.end())
        {
            imsg->second.id = rcv_buffer + sizeof(cmd_rsp);
            imsg->second.msg_state = MSG_STATE_SUBMIT;
        } // end if on queue

        return 0;
    } // end if all is OK

    snprintf(err, buf_len, "Submit failed with code: 0x%08X", 
        cmd_rsp.command_status);

    return -2;
} // end Handle_Submit



//===============================================================================|
int Sms::Handle_Deliver(char *err, const size_t buf_len, std::string &phone_no)
{
    u32 payload_length = cmd_rsp.command_length - sizeof(cmd_rsp);


    DeliverQueue dq;

    // now get the source phone no and msg
    char *alias = rcv_buffer + sizeof(cmd_rsp);
    alias += strlen(rcv_buffer) + 3;     // skip over the service type, src_npi and
        //  src_ton

    phone_no = alias;  // should be pointing at phone #
    alias += strlen(alias) + 3; // skip src phone, dest_npi & ton

    alias += strlen(alias) + 10;    // skip over esm_class, protocol_id, priorty_flag,
        // schedule_delivery_time, validity_period, registered_delivery, replace_if_present_flag,
        //  data_coding, sm_default_message_id

    u8 len = *((u8 *)alias);         // length of message
    // if (len == 0)
    // {
    //     snprintf(err, buf_len, "Driver does not support long message reception.");
    //     return -2;
    // } // end if not cool

    std::string msg = ++alias;
    
    // find a message id if there is one...
    alias += len;       // to the start of TLV

    u16 tlv_code = *((u16*)alias);
    while (tlv_code != RECIEPTED_MESSAGE_ID && (alias - rcv_buffer) < payload_length)
    {
        alias += 2;
        u16 tl = *((u16*)alias);
        alias += htons(tl) + 2;
        tlv_code = *((u16*)alias);
    } // end while looking for RECIPTED_MESSAGE_ID

    if (tlv_code == RECIEPTED_MESSAGE_ID)
    {
        alias += 4; // skips T and L and points at V
        std::string msg_id = alias;

        //Update_SMS_DB(msg_id, 4);
        
        // now remove item from queue
        auto it = std::find_if(queued_msg.begin(), queued_msg.end(), [msg_id](const auto &m){
            return m.second.id == msg_id;
        });

        if (it != queued_msg.end())
            queued_msg.erase(it->first);

        if (Deliver_Rsp() < 0)
            return -1;

    } // end if delivery confirmation
    else
    {
        
    } // end else rcvd message

    return 0;
} // end Handle_Deliver



//===============================================================================|
/**
 * @brief Sends application specific keep-alive signal every set interval; i.e.
 *  a heartbeat.
 * 
 */
void Heartbeat(Sms *psms)
{
    if (psms->sms_state & SMS_BOUNDED)
    {
        while (psms->bheartbeat)
        {
            if (psms->Enquire() < 0)
            {
                psms->bheartbeat = false;
                break;
            } // end if

            sleep(psms->heartbeat_interval);
        } // end while
    } // end if bounded

    psms->bheartbeat = false;
} // end Heartbeat
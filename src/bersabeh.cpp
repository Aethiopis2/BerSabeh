/**
 * @file bersabeh.cpp
 * @author Dr. Rediet Worku aka Aethiops ben Zahab (PanaceaSolutionsEth@gmail.com)
 * 
 * @brief Bersabe SMS program with a web based control interface
 * @version 0.1
 * @date 2024-01-09
 * 
 * @copyright Copyright (c) 2024
 * 
 */


//===============================================================================|
//              INCLUDES
//===============================================================================|
#include "sms.h"
#include "messages.h"
#include "utils.h"
#include "errors.h"
using namespace std;





//===============================================================================|
//          TYPES
//===============================================================================|
/**
 * @brief A little structure that organizes different object togther for the app.
 *  This is so because we don't want different SMS providers or better known as
 *  SMCS's get into each other's hair. We want to keep each SMCS with it's own
 *  belonging so that each can do its own task and not worry about the other.
 * 
 */
typedef struct APP_CONTAINER
{
    u32 id{0};
    Sms sms;

    std::string host;
    std::string port;
    std::string system_id;
    std::string pwd;
} AppContainer, *AppContainer_Ptr;




typedef struct SESSION_TAG
{
    int fd;
    std::string ip;
    std::string port;
    int header_full;
    int body_full;
    int header_len;
    int body_len;
    int total;

    char buf[MAXLINE];
} Session, *Session_Ptr;






//===============================================================================|
//              GLOBALS
//===============================================================================|
int daemon_proc = 0;
SYS_CONFIG sys_config;

std::vector<AppContainer> app_container;     // list of SMS objects
std::map<int, Session> session;              // session object mapped to its socket
std::vector<SmsOut> db_messages;             // queue of db messages

Messages db;
bool sender_running{false};
std::mutex _mutex;
double msg_per_second{20.0 / (double)1'000};





//===============================================================================|
//              PROTOYPES
//===============================================================================|
void Print_Title();
void inline Print(const std::string text);
void Init_Config(std::string &filename);
void Init_SMS();
void Signal_Handler(int signum);


int Get_Http_Request(Session_Ptr psession);
int Parse_Header(char *buf);
std::string Parse_Json_String(const char *buf, const std::string &key);
int Parse_Json_Int(const char *buf, const std::string &key, int &value);


void Http_Ok(const int sockfd);
void Http_Error(const int sockfd, const int err_code);

void Sender_Thread(Sms *psms);
void Clean_Up();




//===============================================================================|
//              FUNCTIONS
//===============================================================================|
int main(int argc, char *argv[])
{ 
    std::string filename{"config.dat"};
    int listen_fd{-1};
    int port{7778};
    bool brun{true};
    struct sockaddr_in serv_addr;

    std::vector<pollfd> vpoll;
    pollfd tmppoll;
    

    Print_Title();
    Init_Config(filename);

    // connect to database, where-ever that may be.
    Print("Connecting to database.");
    if (db.Connect_DB(sys_config.config["db_connection"]) < 0)
    {
        iQE::Dump_DB_Error();
        return -1;
    } // end if

    Print("Loading outgoing SMS from database.");
    db.Load_AID();
    db.Load_Current_Period();
    db.Load_Current_Period_Name();
    db.Load_SMS_Bill_Format();
    Print(db.Load_Unread_Format());
    db_messages = db.Load_Messages();
    //Print(std::to_string(db.Load_Reading_Period()));
    //db_messages = db.Preview_Bill_SMS();
    //db.Write_SMSOut(db_messages);

    Print("Starting server.");
    if ( (listen_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        Dump_Err_Exit("failed to open listening socket");

    u32 on{1};
    if (setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) < 0)
        Dump_Err("failed to set socket options to resuse address");

    iZero(&serv_addr, sizeof(serv_addr));
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);

    if ( bind(listen_fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
        Dump_Err_Exit("failed to bind address to listening socket");

    
    if ( listen(listen_fd, 32) < 0)
        Dump_Err_Exit("failed to listen");

    signal(SIGINT, Signal_Handler);
    Print("Now initializing SMS.");
    Init_SMS();

    iZero(&tmppoll, sizeof(tmppoll));
    tmppoll.events = POLLIN;
    tmppoll.fd = listen_fd;
    vpoll.push_back(tmppoll);

    for ( size_t i{0}; i < app_container.size(); i++)
    {
        pollfd t2;
        iZero(&t2, sizeof(t2));
        t2.events = POLLIN;
        t2.fd = app_container[i].sms.Get_Connection();
        vpoll.push_back(t2);
    } // end for all


    
    Print("Now listening on [*:" + std::to_string(port) + "]");
    //int x{0};
    while (brun)
    {
        //app_container[0].sms.Enquire();
        int ret;
        if ( (ret = POLL(vpoll.data(), vpoll.size())) <= 0)
        {
            if (ret < 0)
                Dump_Err_Exit("poll error");

            brun = false; 
            break;
        } // end if not cool

        std::vector<pollfd> tmp{vpoll};
        for (size_t i = 0; i < tmp.size(); i++)
        {
            if (tmp[i].revents == 0 || !(tmp[i].revents & POLLIN))
                continue;

            if (tmp[i].fd == listen_fd)
            {
                // this is the listening socket, accept an incoming conn
                sockaddr_in addr;
                socklen_t addr_len = sizeof(sockaddr_in);
                char ip[INET_ADDRSTRLEN];

                int clifd = accept(listen_fd, (sockaddr*)&addr, &addr_len);
                if (clifd < 0)
                    Dump_Err("Accept error");


                if (!inet_ntop(AF_INET, &addr.sin_addr, ip, INET_ADDRSTRLEN))
                    Dump_Err("Converting address to human notation.");

                Session s{clifd, ip, std::to_string(ntohs(addr.sin_port)), 0, 0, 0, 0, 0};
                session.emplace(clifd, s);

                pollfd t;
                iZero(&t, sizeof(t));
                t.events = POLLIN;
                t.fd = clifd;
                vpoll.push_back(t);

                Print("New connection from " + s.ip + ":" + s.port);
            } // end if listening socket
            else 
            {
                bool issms{false};

                // test which of the sms descriptors are ready
                for (size_t item = 0; item < app_container.size(); item++)
                {
                    int n;
                    if (tmp[i].fd == app_container[item].sms.Get_Connection())
                    {
                        char b[MAXLINE];
                        if ( ( n = app_container[item].sms.Process_Incoming(b)) < 0)
                        {
                            if (n < 0)
                            {
                                if (n == -2)
                                    Print(b);
                                else
                                   Dump_Err("Disconnected");
                                // int fd = tmp[i].fd;
                                // auto it = std::find_if(vpoll.begin(), vpoll.end(), [&fd](auto &v){ return v.fd == fd; });
                                // if (it != vpoll.end())
                                //     vpoll.erase(it);
                                //Dump_Err_Exit("recv error");
                            } // end if
                        } // end if error
    
                        issms = true;
                        // if (x++ < 1)
                        //     for (size_t k{0}; k < db_messages.size(); k++)
                        //     {
                        //         app_container[0].sms.Send_Message(db_messages[k].message, db_messages[k].phoneno);
                        //         if (k+1 % 10 == 0)
                        //             break;
                        //     } // end if
                        break;
                    } // end if among the sms
                } // end for

                if (!issms)
                {
                    // the ready descriptor by odd one out is a web request over http
                    if (!Get_Http_Request(&session[tmp[i].fd]))
                    {
                        char *buf = session[tmp[i].fd].buf;
                        int sockfd = tmp[i].fd;

                        Print(buf);   
                        //Print("Send Type = " + std::to_string(Parse_SendType(session[tmp[i].fd].buf)));
                        //Print("SubscriberID = " + std::to_string(Parse_SubscriberID(session[tmp[i].fd].buf)));
                        //Print("Message Format = " + Parse_Message_Format(session[tmp[i].fd].buf));
                        //Print("Phone No = " + Parse_PhoneNo(session[tmp[i].fd].buf));
                        
                        if (Parse_Header(buf) < 0)
                            Http_Error(sockfd, 504);
                        else
                            Http_Ok(sockfd);

                        auto it = std::find_if(vpoll.begin(), vpoll.end(), 
                            [&sockfd](auto &v){ return v.fd == sockfd; });

                        if (it != vpoll.end())
                            vpoll.erase(it);

                        CLOSE(sockfd);
                    } // end if 
                } // end if not sms
            } // end else either sms or http
        } // end for

    } // end while forever

    
    return 0;
} // end main



//===============================================================================|
/**
 * @brief Prints title; i.e. company name and website info among other things.
 * 
 */
void Print_Title()
{
    cout << "\n\t  \033[36mINTAPS Software Engineering\033[37m\n"
         << "\t\t\033[34mhttp://www.intaps.com\033[037m\n" << endl;
} // end Print_Ttile



//===============================================================================|
/**
 * @brief Prints the text formatted in a standard manner, so as to give the app
 *  a standard look and feel.
 * 
 * @param text the text/message to print on consle
 */
void inline Print(const std::string text)
{
    std::cout << Console_Out(APP_NAME) << text << endl;
} // end Print



//===============================================================================|
/**
 * @brief Initalizes the global sys_config that is initalized from the filename
 *  provided. The file is a simple text file containing key value pairs separated
 *  using a space.
 * 
 * @param filename the name of file to open and read or full path when not relative
 */
void Init_Config(std::string &filename)
{
    Print("Reading configuration.");
    if (Init_Configuration(filename) < 0)
        Fatal("configuration file \"%s\" not found.", filename.c_str()); 
} // end Init_Config



//===============================================================================|
/**
 * @brief This function connects to all SMCS providers listed in the config file
 *  by parsing first the semi-colons thus establishing count of sms objects and
 *  next by parsing the username@password@host:port format supplied in each 
 *  parameter.
 * 
 */
void Init_SMS()
{
    int ret;
    size_t err{0};
    std::vector<std::string> host_addresses = Split_String(
            sys_config.config["sms_address"], ';');

    for (size_t i{0}; i < host_addresses.size(); i++)
    {
        AppContainer app;

        std::vector<std::string> id_pw_host_port = 
            Split_String(host_addresses[i], '@');
        
        if (id_pw_host_port.empty() || id_pw_host_port.size() < 3)
        {
            Fatal("invalid value \"%s\" for key \"sms_address\" in configuration file", 
                host_addresses[i].c_str());
        } // end if fatal error in config
        
        std::vector<std::string> host_port = Split_String(id_pw_host_port[2], ':');
        if (host_port.empty() || host_port.size() < 2)
        {
            Fatal("invalid value \"%s\" for key \"sms_address\" in configuration file", 
                host_addresses[i].c_str());
        } // end if fatal error in config

        // save these for future ref
        app.id = i + 1;
        app.host = host_port[0];
        app.port = host_port[1];
        app.system_id = id_pw_host_port[0];
        app.pwd = id_pw_host_port[1];
        app_container.push_back(app);

        if ( (ret = app_container[i].sms.Startup(app.host, app.port, app.system_id, app.pwd)) < 0)
        {
            if (ret == -2)
                Fatal(app_container[i].sms.Get_Err().c_str());
            else
            {
                Dump_Err("failed to connect with SMCS #%d at %s:%s", app.id, 
                    app.host.c_str(), app.port.c_str());
                ++err;
            } // end else
        } // end if
    } // end for

    if (err == host_addresses.size())
        Fatal("cannot connect with any SMCS!");
} // end Init_SMS



//===============================================================================|
/**
 * @brief Called whenever POSIX signals interrupt our application; mostly when
 *  terminating the app. In which cases the function does clean up and frees
 *  resources being used.
 * 
 * @param signum the signal identifier to handle
 */
void Signal_Handler(int signum)
{
    std::cout << "\nInterrupted.\nShutting down." << std::endl;
    app_container[0].sms.Shutdown();
    iQE::Shutdown_ODBC();
    exit(1);
} // end Signal_Handler



//===============================================================================|
int Get_Http_Request(Session_Ptr psession)
{
    char *ptr;
    int n = recv(psession->fd, psession->buf + psession->total, MAXLINE - psession->total, 0);
    if (n < 0)
        return -1;

    psession->total += n;
    if (!psession->header_full)
    {
        // processing header bytes
        // locate the special header dilimeters within the buffer
        ptr = strstr(psession->buf, "\r\n\r\n");
        if (!ptr)
            return 1;   // not done yet

        psession->header_full = 1;
        psession->header_len = (ptr + 4) - psession->buf;
    } // end if

    if (psession->header_full && !psession->body_full)
    {
        // locate the content length from the header buffer
        ptr = strstr(psession->buf, "Content-Length:");
        if (!ptr)
            return -2;      // invalid request

        ptr += strlen("Content-Length:") + 1;
        char *ptr2 = strstr(ptr, "\r\n");
        int digits = ptr2 - ptr;
        char l[12];
        strncpy(l, ptr, digits);
        psession->body_len = atoi(l);
        if (psession->body_len >= (psession->total))
            return 1;

        psession->body_full = 1;
    } // end if body validation

    if (psession->header_full && psession->body_full)
        psession->header_full = psession->body_full = psession->total =  0;
    
    return 0;
} // end Handle_HTTP



//===============================================================================|
int Parse_Header(char *buf)
{
    char *ptr = strstr(buf, "POST");
    if (!ptr)
        return -1;      // protocol not accepted

    ptr += strlen("POST");
    char *ptr2 = strstr(ptr, "/sendSMS");
    if (!ptr2)
        return -2;      // a 404 page not found

    return 0;
} // end Parse_Header



//===============================================================================|
/**
 * @brief This function parses the value part of a string from a Json encoded
 *  stream or buffer given it's key. The value to be parsed must adhere to strict
 *  Json regulation, alas the function will fail to parse.
 * 
 * @param buf buffer containing Json data to parse
 * @param key the key to parse the value for
 * 
 * @return std::string a prased Json string for C++ to use. Alas a null string.
 */
std::string Parse_Json_String(const char *buf, const std::string &key)
{
    const char *ptr = strstr(buf, key.c_str());
    if (!ptr)
        return "";      // key not found.

    ptr += key.length();

    // locate ':' speartor in Json format
    const char *ptr2 = strchr(ptr, ':');
    if (!ptr2)
        return "";      // Json error
             
    ptr = strchr(ptr2, '\"');
    if (!ptr)
        return "";

    ++ptr;              // the 1st char in string data

    // from the first char, locate the last, quote(")
    ptr2 = strchr(ptr, '\"');
    if (!ptr2)
        return "";

    --ptr2;             // the last char in string

    std::string ret{ptr, (size_t)(ptr2 - ptr)};
    return ret;
} // end Parse_Json_String



//===============================================================================|
/**
 * @brief Parses the integer value from Json formatted stream or buffer. The 
 *  function assumes the caller already knows the parsed value is an integer.
 * 
 * @param buf the buffer to parse
 * @param key the ky who's value are we looking for
 * @param value the parsed integer value is retured here
 * 
 * @return int 0 on success alas -1.
 */
int Parse_Json_Int(const char *buf, const std::string &key, int &value)
{
    const char *ptr = strstr(buf, key.c_str());
    if (!ptr)
        return -1;

    ptr += key.length();
    const char *ptr2 = strchr(ptr, ':');
    if (!ptr2)
        return -1;

    // skip over any white space and stuff ...
    ++ptr2;
    while (*ptr2 == ' ' || *ptr2 == '\t' || *ptr2 == '\n')
        ++ptr2;
    
    // now we at the first digit, now the ticky part, locate the last portion
    // it's separted by comma (,) or closed with left brace (}).
    ptr = strchr(ptr2, ',');
    if (!ptr)
    {
        ptr = strchr(ptr2, '}');
        if (!ptr)
            return -1;
    } // end if not found


    // let's apply a hack, copy into a string then convert string
    //  since our pointer won't null terminate.
    std::string s{ptr2, (size_t)(ptr - ptr2)};
    value = atoi(s.c_str());

    return 0;
} // end Parse_Json_Int



//===============================================================================|
/**
 * @brief Send's a very simple kooked http ok response along side some rudimentary
 *  HTML incase this is on the web.
 * 
 * @param sockfd the connection descriptior
 */
void Http_Ok(const int sockfd)
{
    char b[MAXLINE]{0};
    snprintf(b, MAXLINE, "HTTP/1.1 200 Success\r\nConnection: Close\r\n\
    Content-Type:text/html\r\n\r\n\
    <html><head><title>Bersabeh OK</title></head><body>Bersabeh Ok response\
    </body></html>\r\n");

    if (send(sockfd, b, strlen(b), 0) < 0)
        Dump_Err("Sending OK response.");
} // end Http_Ok



//===============================================================================|
/**
 * @brief Send's a canned error response given the code as param.
 * 
 * @param sockfd the socket descriptor
 * @param err_code a code describing the http error
 */
void Http_Error(const int sockfd, const int err_code)
{
    char b[MAXLINE]{0};
    snprintf(b, MAXLINE, "HTTP/1.1 %d Error Occurred\r\n\r\n", err_code);

    if (send(sockfd, b, strlen(b), 0) < 0)
        Dump_Err("Sending Error response.");
} // end Http_Error



//===============================================================================|
void Sender_Thread(Sms *psms)
{
    std::chrono::duration<double>  time_span;
    while (!db_messages.empty())
    {
        std::chrono::high_resolution_clock::time_point start_t = 
            std::chrono::high_resolution_clock::now();

        SmsOut out = db_messages.back();

        if (psms->Send_Message(out.message, out.phoneno) < 0)
            Dump_Err("Sending fail.");

        db.Update_SMSOut(&out);

        _mutex.lock();
        db_messages.pop_back();
        _mutex.unlock();

        do
        {
            std::chrono::high_resolution_clock::time_point end_t = 
                std::chrono::high_resolution_clock::now();

            time_span = std::chrono::duration_cast<std::chrono::duration<double>>(end_t - start_t);
        } while (time_span.count() <= msg_per_second );
        
    } // end while sending
} // end Sender_Thread



//===============================================================================|
/**
 * @brief Does house cleaning before the app terminates or is interrupted.
 * 
 */
void Clean_Up()
{
    
} // end Clean_Up
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
    Messages db;

    std::string host;
    std::string port;
    std::string system_id;
    std::string pwd;
} AppContainer, *AppContainer_Ptr;




//===============================================================================|
//              GLOBALS
//===============================================================================|
int daemon_proc = 0;
SYS_CONFIG sys_config;

std::vector<AppContainer> app_container;     // list of SMS objects






//===============================================================================|
//              PROTOYPES
//===============================================================================|
void Print_Title();
void inline Print(const std::string text);
void Init_Config(std::string &filename);
void Init_SMS();
void Signal_Handler(int signum);
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

    for ( AppContainer app : app_container)
    {
        pollfd t2;
        iZero(&t2, sizeof(t2));
        t2.events = POLLIN;
        t2.fd = app.sms.Get_Connection();
        vpoll.push_back(t2);
    } // end for all

    Print("Now listening on [*:" + std::to_string(port) + "]");
    while (brun)
    {
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

            // test which of the sms descriptors are ready
            for (size_t item = 0; item < app_container.size(); item++)
            {
                int n;
                if (tmp[i].fd == app_container[item].sms.Get_Connection())
                {
                    if ( ( n = app_container[item].sms.Process_Incoming()) < 0)
                    {
                        if (n == -3)
                        {
                            Print("SMCS: " + app_container[item].sms.Get_SystemID() + " disconnected.");
                            int fd = tmp[i].fd;
                            auto it = std::find_if(vpoll.begin(), vpoll.end(), [&fd](auto &v){ return v.fd == fd; });
                            if (it != vpoll.end())
                                vpoll.erase(it);
                        } // end if
                        else
                            Dump_Err_Exit("recv error");
                    } // end if error

                    break;
                } // end if among the sms
            } // end for
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

        if ( (ret = app_container[0].sms.Startup(app.host, app.port, app.system_id, app.pwd)) < 0)
        {
            if (ret == -2)
                Fatal(app_container[0].sms.Get_Err().c_str());
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
    exit(signum);
} // end Signal_Handler



//===============================================================================|
/**
 * @brief Does house cleaning before the app terminates or is interrupted.
 * 
 */
void Clean_Up()
{
    
} // end Clean_Up
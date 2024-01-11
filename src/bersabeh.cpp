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
#include "utils.h"
#include "errors.h"
using namespace std;





//===============================================================================|
//              GLOBALS
//===============================================================================|
int daemon_proc = 0;
SYS_CONFIG sys_config;




//===============================================================================|
//              PROTOYPES
//===============================================================================|
void Init_Config(std::string &filename);
int Init_SMS(int &fds);




//===============================================================================|
//              FUNCTIONS
//===============================================================================|
int main(int argc, char *argv[])
{
    int fds_listen;                 // descriptor for listening 
    int fds_sms;                    // socket for sms
    struct sockaddr_in serv;        // address info for listening server
    std::string filename{"config.dat"};


    Init_Config(filename);
    if (Init_SMS(fds_sms) < 0)
        Dump_Err_Exit("%sFailed to connect with SMSC", Display_Time().c_str());

    if ( (fds_listen = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        Dump_Err_Exit("%ssocket connection failed.\n", Display_Time().c_str());
        return 1;
    } // end if


    iZero(&serv, sizeof(serv));
    serv.sin_family = AF_INET;
    serv.sin_port = HTONS(SERV_PORT);
    serv.sin_addr.s_addr = INADDR_ANY;

    if ( bind(fds_listen, (SA *)&serv, sizeof(serv)) < 0)
    {
        Dump_Err_Exit("%sfailed to bind socket.\n", Display_Time().c_str());
        return 1;
    } // end if no bind


    if ( listen(fds_listen, LISTENQ) < 0)
    {
        Dump_Err_Exit("%sListen failed.\n", Display_Time().c_str());
        return 1;
    } // end if no listening

    cout << Display_Time() << "Now running." << endl;

    while (1)
    {
        fd_set rset;
        FD_ZERO(&rset);

        FD_SET(fds_listen, &rset);
        //FD_SET(fds_sms, &rset);
        if ( select(fds_listen + 1, &rset, nullptr, nullptr, nullptr) > 0)
        {
            
        } // end if selecting good
    } // end while

    cout << Display_Time() << "Shutting down." << endl;
    CLOSE(fds_listen);
    return 0;
} // end main



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
    // print title
    cout << "\n\t  \033[36mINTAPS Software Engineering\033[37m\n"
         << "\t\t\033[34mhttp://www.intaps.com\033[037m\n" << endl;

    cout << Display_Time() << "Reading configuration." << endl;

    if (Init_Configuration(filename) < 0)
        Fatal("%s\"%s\" not found.", Display_Time().c_str(), filename.c_str()); 
} // end Init_Config



//===============================================================================|
int Init_SMS(int &fds)
{
    vector<std::string> host = Split_String(sys_config.config["sms_host_address"], ':');

    cout << Display_Time() << "Interface binding with SMSC." << endl;
    if ( (fds = Sms::Bind_Trx(host[0], host[1], sys_config.config["sms_system_id"], 
        sys_config.config["sms_password"])) < 0)
        Dump_Err_Exit("%sFailed to bind tranceiver.", Display_Time().c_str());

    char buffer[MAXLINE];
    int bytes = recv(fds, buffer, MAXLINE, 0);
    if (bytes < 0)
        return -1;

    Sms::Process_Incoming(buffer, bytes);
    return 0;
}
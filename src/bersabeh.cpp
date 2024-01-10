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
int Init_SMS();




//===============================================================================|
//              FUNCTIONS
//===============================================================================|
int main(int argc, char *argv[])
{
    int fds_listen;                 // descriptor for listening 
    struct sockaddr_in serv;        // address info for listening server

    if ( (fds_listen = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        Dump_Err_Exit("%ssocket connection failed.\n", Display_Time());
        return 1;
    } // end if


    iZero(&serv, sizeof(serv));
    serv.sin_family = AF_INET;
    serv.sin_port = HTONS(SERV_PORT);
    serv.sin_addr.s_addr = INADDR_ANY;

    if ( bind(fds_listen, (SA *)&serv, sizeof(serv)) < 0)
    {
        Dump_Err_Exit("%sfailed to bind socket.\n", Display_Time());
        return 1;
    } // end if no bind


    if ( listen(fds_listen, LISTENQ) < 0)
    {
        Dump_Err_Exit("%sListen failed.\n", Display_Time());
        return 1;
    } // end if no listening

    cout << Display_Time() << "Now running." << endl;

    while (1)
    {
        fd_set rset;
        FD_ZERO(&rset);

        FD_SET(fds_listen, &rset);
        if ( select(fds_listen + 1, &rset, nullptr, nullptr, nullptr) > 0)
        {

        } // end if selecting good
    } // end while

    cout << Display_Time() << "Shutting down." << endl;
    CLOSE(fds_listen);
    return 0;
} // end main
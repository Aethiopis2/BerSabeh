//==========================================================================================================|
// playground.cpp:
//  a little test driver
//
// Date Created:
//  20th of July 2023, Thursday.
//
// Last Updated:
//  20th of July 2023, Thursday.
//
// Program Authors:
//  Rediet Worku aka Aethiopis II ben Zahab
//==========================================================================================================|


//==========================================================================================================|
// INCLUDES
//==========================================================================================================|
#include <iostream>
using namespace std;


#include "sms.h"
#include "utils.h"
#include "errors.h"




//==========================================================================================================|
// GLOBALS
//==========================================================================================================|
int daemon_proc{0};
SYS_CONFIG sys_config;      // curses of the black pearl



//==========================================================================================================|
// FUNCTIONS
//==========================================================================================================|
int main()
{
    string sms_ip{"192.168.1.13"};
    string port{"2775"};
    string sys_id{"RedTst"};
    string pwd{"12345"};

    cout << "Connecting to Short Message Service Center (SMCS) ..." << endl;
    if (Connect_Tcp(sms_ip, port) < 0)
        Dump_Err_Exit("connect error");

    cout << "Connected to SMSC." << endl;
    
    if (Bind_Trx(sys_id, pwd) < 0)
        Dump_App_Err("Err");

    int j = 0;
    while (++j < 100'000'000);

    cout << "Sending text" << endl;
    if (Submit("This is a hello message", "0911486301", 1) < 0)
        Dump_App_Err("Err");

    while (true);       // wait forever for now...
    return 0;
} // end main



//==========================================================================================================|
//          THE END
//==========================================================================================================|
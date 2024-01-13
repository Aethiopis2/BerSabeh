/**
 * @file TcpBase.h
 * @author Rediet Worku aka Aethiopis II ben Zahab (Aethiopis2rises@gmail.com)
 * 
 * @brief Definition for base functions of any tcp based client or server.
 * @version 0.1
 * @date 2024-01-12
 * 
 * @copyright Copyright (c) 2024
 * 
 */
#ifndef TCP_BASE_H
#define TCP_BASE_H



//===============================================================================|
//          INCLUDES
//===============================================================================|
#include "basics.h"





//===============================================================================|
//          MACROS
//===============================================================================|
// this macro get's defined for the platform at hand
#if defined(__unix__) || defined(__linux__)
#define CLOSE(s)    close(s)
#else 
#if defined(WIN32) || defined(__WIN64)
#define CLOSE(s)    closesocket(s);
#endif
#endif




// Connection/Socket states
#define APP_SOCK_WAIT        0       // is waiting or has completed recv
#define APP_SOCK_RECV        1       // is getting some




//===============================================================================|
//          TYPES
//===============================================================================|
   




//===============================================================================|
//          CLASS
//===============================================================================|
class TcpBase
{
public:

    TcpBase();
    int Send(const char *buffer, const size_t len);
    int Recv(char *buffer, const size_t len);
    int Get_Socket() const;
    int Get_State() const;
    
protected:

    int fds;        // the socket descriptor
    int state;      // the state of the socket; when applicable
};


#endif
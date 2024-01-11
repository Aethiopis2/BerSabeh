/**
 * @file tcp-client.h
 * @author Rediet Worku aka Aethiopis II ben Zahab (Aethiopis2rises@gmail.com)
 * 
 * @brief Prototypes/wrapper functions for basic tcp based system calls; tailed
 *  for tcp enabled clients running on IPv4/v6.
 * @version 0.1
 * @date 2022-07-18
 * 
 * @copyright Copyright (c) 2024
 * 
 */
#ifndef TCP_CLIENT_H
#define TCP_CLIENT_H



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
namespace TcpClient
{  
    typedef struct CONNECTION_INFO
    {
        int fds{-1};                // a descriptor to socket
        u8 state{APP_SOCK_WAIT};    // the state of socket; receving or waiting.
    } Connection, *Connection_Ptr;



//===============================================================================|
//          PROTOTYPES
//===============================================================================|
 
    int Connect(const std::string hostname, const std::string port);
    int Disconnect();

    int Get_Socket();
    int Send(const char *buffer, const size_t len);
    int Recv(char *buffer, const size_t len);

    //addrinfo *Get_Addr() const;
    //sockaddr_storage Get_Addr_Storage() const;
}


#endif
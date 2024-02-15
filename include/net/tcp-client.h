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
#include "tcp-base.h"





//===============================================================================|
//          MACROS
//===============================================================================|





//===============================================================================|
//          TYPES
//===============================================================================|




//===============================================================================|
//          CLASS
//===============================================================================|
class TcpClient : public TcpBase
{
public:

    TcpClient();
    TcpClient(const std::string &hostname, const std::string &port);
    ~TcpClient();

    int Connect(const std::string &hostname, const std::string &port);
    int Disconnect();


    //addrinfo *Get_Addr() const;
    //sockaddr_storage Get_Addr_Storage() const;
private:

    struct addrinfo *paddr;         // a protocol independant network address stuct

    /* Utlity */
    int Fill_Addr(const std::string &hostname, const std::string &port);
};


#endif
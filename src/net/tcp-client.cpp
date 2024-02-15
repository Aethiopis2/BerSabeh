/**
 * @file tcp-client.cpp
 * @author Rediet Worku aka Aethiopis II ben Zahab (Aethiopis2rises@gmail.com)
 * 
 * @brief implementation for tcp-client.h functions
 * @version 0.1
 * @date 2022-07-18
 * 
 * @copyright Copyright (c) 2024
 * 
 */


//===============================================================================|
//          INCLUDES
//===============================================================================|
#include "tcp-client.h"





//===============================================================================|
//          GLOBALS
//===============================================================================|








//===============================================================================|
//          CLASS IMP
//===============================================================================|
/**
 * @brief Construct a new Tcp Client:: Tcp Client object Default do nothing cons
 * 
 */
TcpClient::TcpClient()
    : paddr{nullptr} {}



//===============================================================================|
/**
 * @brief Construct a new Tcp Client:: Tcp Client object Initalizes the object
 *  in a connected state.
 * 
 * @param hostname the host name or ip address to connect to
 * @param port the corresponding port number
 */
TcpClient::TcpClient(const std::string &hostname, const std::string &port)
    : paddr{nullptr}
{
    if (Connect(hostname, port) < 0)
        throw std::runtime_error("Connect failed.");
} // end constructor II



//===============================================================================|
/**
 * @brief Destroy the Tcp Client:: Tcp Client object Closes the socket if already
 *  active.
 */
TcpClient::~TcpClient()
{
    Disconnect();
} // end Destructor



//===============================================================================|
/**
 * @brief Connects to server/host at the provided address using the best protocol 
 *  the kernel can determine.
 * 
 * @param hostname ip address or hostname to connect to
 * @param port aka the port number or service name
 * 
 * @return int a 0 on success, -1 on fail.
 */
int TcpClient::Connect(const std::string &hostname, const std::string &port)
{
    // initalize the base addr info first
    if (Fill_Addr(hostname, port) == -1)
        return -1;  

    struct addrinfo *p_alias = paddr;
    do {

        fds = socket(p_alias->ai_family, p_alias->ai_socktype, p_alias->ai_protocol);
        if (fds < 0)
            continue; 

        if (connect(fds, p_alias->ai_addr, p_alias->ai_addrlen) == 0)
            return 0;       // success

    } while ( (p_alias = p_alias->ai_next) != NULL);
    
    // at the end of the day if socket is null
    return -1;
} // end Connect



//===============================================================================|
/**
 * @brief Releases resources and explicitly closes an open socket.
 * 
 * @return int 0 on success alas -1 for error
 */
int TcpClient::Disconnect()
{
    if (fds > 0)
    {
        if (paddr)
        {
            free(paddr);
            paddr = nullptr;    // Andre style but with c++11 taste
        } // end if addr

        fds = -1;
        return CLOSE(fds);
    } // end closing socket

    return 0;
} // end Disconnect



//===============================================================================|
/**
 * @brief fills in the protocol independant structure addrinfo; from there the 
 *  kernel determines what protocol to use.
 * 
 * @param hostname an ip address or hostname
 * @param port the port number as string
 * 
 * @return int 0 for success -1 for error
 */
int TcpClient::Fill_Addr(const std::string &hostname, const std::string &port)
{
    struct addrinfo hints; 

    // is this Windows?
#if defined(WINDOWS)
    WSADATA wsa;        // required by the startup function
    if (WSAStartup(MAKEWORD(2,2), &wsa))
        return -1;      // WSAGetLastError() for details

#endif

    iZero(&hints, sizeof(hints));
    hints.ai_flags = 0;
    hints.ai_addr = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    
    // get an address info
    if (getaddrinfo(hostname.c_str(), port.c_str(), &hints, &paddr) != 0)
        return -1;

    return 0;       // success
} // end Fill_Addr
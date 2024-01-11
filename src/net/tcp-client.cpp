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
TcpClient::Connection conn;     //  a simple abstraction of a socket
struct addrinfo *paddr;         // a protocol independant network address stuct








//===============================================================================|
//          FUNCTIONS
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
int Fill_Addr(const std::string &hostname, const std::string &port)
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



//===============================================================================|
/**
 * @brief Connects to server/host at the provided address using the best protocol 
 *  the kernel can determine.
 * 
 * @param hostname ip address or hostname to connect to
 * @param port aka the port number or service name
 * 
 * @return int a socket descriptor on success, -1 on fail.
 */
int TcpClient::Connect(const std::string hostname, const std::string port)
{
    // initalize the base addr info first
    if (Fill_Addr(hostname, port) == -1)
        return -1;  

    struct addrinfo *p_alias = paddr;
    do {

        conn.fds = socket(p_alias->ai_family, p_alias->ai_socktype, p_alias->ai_protocol);
        if (conn.fds < 0)
            continue; 

        if (connect(conn.fds, p_alias->ai_addr, p_alias->ai_addrlen) == 0)
            return conn.fds;       // success

        TcpClient::Disconnect();
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
    if (conn.fds > 0)
    {
        if (paddr)
        {
            free(paddr);
            paddr = nullptr;    // Andre style but with c++11 taste
        } // end if addr

        return CLOSE(conn.fds);
    } // end closing socket

    return 0;
} // end Disconnect




//===============================================================================|
/**
 * @brief returns the socket descriptor
 * 
 * @return int a copy of socket descriptor
 */
int TcpClient::Get_Socket()
{
    return conn.fds;
} // end Get_Socket



//===============================================================================|
/**
 * @brief Send's the buffer to peer as tcp streaming data
 * 
 * @param buffer data to send 
 * @param len length of data in bytes
 * 
 * @return int total bytes sent on success or -1 on error 
 */
int TcpClient::Send(const char *buffer, const size_t len)
{
    int total{0};
    int n;

    while ( (n = send(conn.fds, buffer + total, len - total, 0)) > 0 && ((size_t)total < len))
        total += n;

    return total;
} // end Send



//===============================================================================|
/**
 * @brief retuns a buffer of data from the peer over tcp enabled network
 * 
 * @param buffer space to get data from peer
 * @param len length of sent space in bytes
 * 
 * @return int number of bytes received on success -1 on error
 */
int TcpClient::Recv(char *buffer, const size_t len)
{
    return recv(conn.fds, buffer, len, 0);
} // end Recv
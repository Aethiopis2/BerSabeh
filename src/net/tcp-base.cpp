/**
 * @file TcpBase.h
 * @author Rediet Worku aka Aethiopis II ben Zahab (Aethiopis2rises@gmail.com)
 * 
 * @brief Implementation of base class tcp-base memebers
 * @version 0.1
 * @date 2024-01-12
 * 
 * @copyright Copyright (c) 2024
 * 
 */


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
TcpBase::TcpBase()
    :fds{-1}, state{0} {}



//===============================================================================|
/**
 * @brief Send's the buffer to peer as tcp streaming data
 * 
 * @param buffer data to send 
 * @param len length of data in bytes
 * 
 * @return int total bytes sent on success or -1 on error 
 */
int TcpBase::Send(const char *buffer, const size_t len)
{
    int total{0};
    int n;

    while ( (n = send(fds, buffer + total, len - total, 0)) > 0 && ((size_t)total < len))
        total += n;

    return total;
} // end Send



//===============================================================================|
/**
 * @brief retuns a buffer of data from the peer over tcp enabled network. The 
 *  function reads until the number of bytes sent is read or EWOULBLOCK error is
 *  sent by the socket.
 * 
 * @param buffer space to get data from peer
 * @param len length of sent space in bytes
 * 
 * @return int number of bytes received on success -1 on error
 */
int TcpBase::Recv(char *buffer, const size_t len)
{
    int n;              // the bytes recieved at one stroke

    if ( (n = recv(fds, buffer, len, MSG_WAITALL)) <= 0)
    {
        if (errno != EWOULDBLOCK)
            return -1;
    } // end if

    return n;
} // end Recv



//===============================================================================|
/**
 * @brief returns the socket descriptor
 * 
 * @return int a copy of socket descriptor
 */
int TcpBase::Get_Socket() const
{
    return fds;
} // end Get_Socket



//===============================================================================|
/**
 * @brief returns the socket descriptor
 * 
 * @return int a copy of socket descriptor
 */
int TcpBase::Get_State() const
{
    return state;
} // end Get_Socket
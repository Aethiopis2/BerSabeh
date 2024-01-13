/**
 * @file basics.h
 * @author Rediet Worku aka Aethiops II ben Zahab (aethiopis2rises@gmail.com)
 * 
 * @brief contains some basic C/C++ and OS related includes and macros
 * @version 1.2
 * @date 2023-12-01
 * 
 * @copyright Copyright (c) 2023
 * 
 */
#ifndef REDBASICS_H
#define REDBASICS_H



#if defined(WIN32)
#define WINDOWS
#elif defined(_WIN64)
#define WINDOWS
#endif




//=====================================================================================|
//          INCLUDES
//=====================================================================================|
#include <iostream>             // C++ headers
#include <iomanip>
#include <string>
#include <fstream>
#include <sstream>
#include <vector>
#include <stdexcept>
#include <list>
#include <map>
#include <unordered_map>
#include <algorithm>
#include <thread>
#include <ctime>


#include <sys/types.h>          // C headers
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdarg.h>
#include <inttypes.h>
#include <signal.h>


#if defined(WINDOWS)
#include <WinSock2.h>
#include <ws2tcpip.h>

#define CLOSE(s)        closesocket(s)
#define POLL(ps, len)   WSAPoll(ps, len, -1)

#else
#include <sys/socket.h>
#include <sys/poll.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netdb.h>


#define CLOSE(s)        close(s)
#define POLL(ps, len)   poll(ps, len, -1)
#endif 





//=====================================================================================|
//          TYPES
//=====================================================================================|
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef __uint128_t u128;
typedef int8_t s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;
typedef __int128_t s128;




//=====================================================================================|
//          DEFINES
//=====================================================================================|
#define APP_NAME        "BerSabeh"      // Application name


#if __LITTLE_ENDIAN
#define HTONS(x)        htons(x)
#define HTONL(x)        htonl(x)
#define HTONLL(x)       (((u64)htonl((x) & 0xFFFFFFFF) << 32) | htonl((x) >> 32))

#define NTOHS(x)        ntohs(x)
#define NTOHL(x)        ntohl(x)
#define NTOHLL(x)       (((u64)ntol((x) & 0xFFFFFFFF) << 32) | ntohl((x) >> 32))
#else
#define HTONS(x)        (x)
#define HTONL(x)        (x)
#define HTONLL(x)       (x)

#define NTOHS(x)        (x)
#define NTOHL(x)        (x)
#define NTOHLL(x)       (x)
#endif


#define iZero(buf, len) memset(buf, 0, len)
#define iCpy(d, s, l)   memcpy(d, s, l)



#define SERV_PORT       7777                /* default pre-kooked server port */
#define LISTENQ         32                  /* default max number of listening descriptors */
#define SA              struct sockaddr     /* short hand notation for socket address structures */
#define MAXLINE         1024                /* default size of buffer used during intranetwork buf. */



enum Boolean {FALSE, TRUE};


#ifndef MAXPATH
#define MAXPATH     260     /* dir max length in Windows systems */
#endif


#if defined(WINDOWS)
#define PATH_SEP        '\\'
#else
#define PATH_SEP        '/'
#endif


#define DUMP_TIME char buf[MAXPATH]; time_t curr_time; std::time(&curr_time); \
    char b[MAXPATH]; snprintf(b, MAXPATH, "\033[33m%s\033[37m", APP_NAME); \
    std::strftime(buf, MAXPATH, strcat(b," \033[34m%d-%b-%y, %T\033[37m"), localtime(&curr_time)); \
    std::cout << buf << ": "; 



#endif
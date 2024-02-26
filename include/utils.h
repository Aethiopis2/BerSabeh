/**
 * @file utils.h
 * @author Rediet Worku, Dr. aka Aethiopis II ben Zahab (PanaceaSolutionsEth@gmail.com)
 *
 * @brief contains prototypes and global objects used commonly through out by different apps. 
 *  These include ablity to read/write from/to configuration file and so on so forth.
 * @version 0.1
 * @date 2023-10-28
 * 
 * @copyright Copyright (c) 2024
 * 
 */
#ifndef UTILS_H
#define UTILS_H



//=====================================================================================|
//          INCLUDES
//=====================================================================================|
#include "basics.h"



//=====================================================================================|
//          DEFINES
//=====================================================================================|



//=====================================================================================|
//          TYPES
//=====================================================================================|
/**
 * @brief 
 *  a little system configuration container
 */
typedef struct SYSTEM_CONFIGURATION
{
    std::unordered_map<std::string, std::string> config;
} SYS_CONFIG, *SYS_CONFIG_PTR;





//=====================================================================================|
//          GLOBALS
//=====================================================================================|
extern SYS_CONFIG sys_config;              // an instance of system config globally visible to all





//=====================================================================================|
//          PROTOTYPES
//=====================================================================================|
int Init_Configuration(const std::string &filename);
void Dump_Hex(const char *buf, const size_t len);
std::vector<std::string> Split_String(const std::string &str, const char token);
std::string Console_Out(const std::string app_name);
std::string Replace_String(std::string str, const std::string patt, const std::string replace);
std::string Format_Numerics(const double num);


#endif
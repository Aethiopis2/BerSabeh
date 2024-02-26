/**
 * @file utils.cpp
 * @author your name (you@domain.com)
 * 
 * @brief contains defintion for utils.h file function prototypes
 * @version 0.1
 * @date 2023-07-28
 * 
 * @copyright Copyright (c) 2024
 * 
 */



//=====================================================================================|
//          INCLUDES
//=====================================================================================|
#include "utils.h"



//=====================================================================================|
//          DEFINES
//=====================================================================================|



//=====================================================================================|
//          TYPES
//=====================================================================================|



//=====================================================================================|
//          GLOBALS
//=====================================================================================|



//=====================================================================================|
//          FUNCTIONS
//=====================================================================================|
/**
 * @brief initalizes the global sys_config structure which contains a map of key-value 
 *  pairs that store various system related configuration.
 * 
 * @param filename a full filename + path containing the config file usually named "config.dat" 
 * 
 * @return int a 0 on success, a -1 if file open failed.
 */
int Init_Configuration(const std::string &filename)
{
    std::string sz_first, sz_second;
    std::ifstream config_file{filename.c_str()};

    if (!config_file)
        return -1;

    sys_config.config.clear();

    while (config_file >> quoted(sz_first) >> quoted(sz_second))
        sys_config.config[sz_first] = sz_second;

    config_file.close();
    
    // success
    return 0;
} // end Init_Configuration



//=====================================================================================|
/**
 * @brief Splits a string using the sz_token provided and returns them as vector of strings
 * 
 * @param str the string to split   
 * @param token the token that let's the program know at which points it must split
 * 
 * @return std::vector<std::string> a vector of strings split is returned
 */
std::vector<std::string> Split_String(const std::string &str, const char token)
{
    std::string s;      // get's the split one at a time
    std::istringstream istr(str.c_str());
    std::vector<std::string> vec;

    while (std::getline(istr, s, token))
        vec.push_back(s);

    return vec;
} // end Split_String



//=====================================================================================|
/**
 * @brief Prints the contents of buffer in hex notation along side it's ASCII form much 
 *  like hex viewer's do it.
 * 
 * @param buf the information to dump as hex and char arrary treated as a char array. 
 * @param len the length of the buffer above
 */
void Dump_Hex(const char *buf, const size_t len)
{
    size_t i, j;                    // loop var
    const size_t skip = 8;          // loop skiping value
    size_t remaining = skip;        // remaining rows


    // print header
    printf("\n      ");
    for (i = 0; i < 8; i++)
        printf("\033[36m%02X ", (uint16_t)i);
    printf("\033[37m\n");

    if (len < skip)
        remaining = len;
    
    for (i = 0; i < len; i+=skip)
    {   
        printf("\033[36m%04X:\033[37m ", (uint16_t)i);
        for (j = 0; j < remaining; j++)
            printf("%02X ", (uint8_t)buf[i+j]);

        if (remaining < skip) {
            // fill blanks
            for (j = 0; j < skip - remaining; j++)
                printf("   ");
        } // end if

        printf("\t\t");

        for (j = 0; j < remaining; j++)
            printf("%c. ", buf[i+j]);

        printf("\n");

        if (len - (i + j) < skip)
            remaining = len - (i + j);
    } // end for

    printf("\n");
} // end Dump_Hex



//=====================================================================================|
/**
 * @brief Formats the date and time for displaying on the screen; so that my applications
 *  could have a standard look and feel when run through the console.
 * 
 * @return std::string a formatted datetime
 */
std::string Console_Out(const std::string app_name)
{
    time_t curr_time;
    char buf[MAXPATH];

    std::time(&curr_time);
    snprintf(buf, MAXPATH, "\033[33m%s\033[37m", app_name.c_str());
    std::strftime(buf + strlen(buf), MAXPATH, " \033[34m%d-%b-%y, %T\033[37m: ", 
        localtime(&curr_time));

    return buf;
} // end display_time



//=====================================================================================|
/**
 * @brief Neatly replaces the string patt from str using the string replace. It basically
 *  splits the string in half and insert's the replace string in between.
 * 
 * @param str the original string
 * @param patt the pattern to replace
 * @param replace the replacemebt string
 * 
 * @return std::string a new string containing the replaced string in between on succes
 *  alas the orignal string str is returned
 */
std::string Replace_String(std::string str, const std::string patt, const std::string replace)
{
    size_t pos;
    if (( pos = str.find(patt)) != std::string::npos)
    {
        str = str.substr(0, pos) + replace +  
            str.substr(pos + patt.length(), str.length() - pos);
    } // end find

    return str;
} // end Replace_String



//=====================================================================================|
/**
 * @brief Format's the numerical value with comma's. Useful in financial apps where its
 *  common to format numerics in such manner.
 * 
 * @param num the number to format
 * 
 * @return std::string a formatted numerical string 
 */
std::string Format_Numerics(const double num)
{
    std::ostringstream ostream;
    ostream << std::setprecision(2) << std::fixed << num;

    std::string s{ostream.str()};
    int pos = s.find(".");

    for (int i = pos - 3; i >= 1; i -= 3)
    {
        char t = s[i];

        for (int j = i + 1; j < (int)s.length(); j++)
        {
            char k = s[j];
            s[j] = t;
            t = k;
        } // end nested for

        s[i] = ',';
        s += t;
    } // end for

    return s;
} // end Format_Numerics
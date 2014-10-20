/*******************************************************************************
** ATOP Redundancy Concentrator Project version control information:          **
** $URL: http://10.0.0.185/repos/redundant_concentrator/trunk/smartGateway/smartgateway.h $
** $Rev:: 48                    $:  Revision of last commit                   **
** $Author:: simonhuang         $:  Author of last commit                     **
** $Date:: 2014-10-14 16:08:06 #$:  Date of last commit                       **
*******************************************************************************/


#ifndef __COMMON_H__
#define __COMMON_H__

#include <stdio.h>

#if 0
typedef signed   char   int8;
typedef unsigned char   uint8;

typedef signed   short  int16;
typedef unsigned short  uint16;

typedef signed   long   int32;
typedef unsigned long   uint32;
#endif


#if !defined (PJ_DEBUG)
#define PJ_DEBUG
#endif

#if defined (PJ_DEBUG)
#define PRINT_LEVEL_DEF     DEBUG_LEVEL
//extern int dbg_level;
//#define PRINT_LEVEL_DEF     dbg_level
#define DEBUG_LEVEL         0                        // Show all print message
#define INIT_LEVEL          1                        // Show init + running + err message
#define RUNNING_LEVEL       2                        // Show running + err message
#define ERROR_LEVEL         3                        // Only show err message

/* ps. just show the first certain characters of function name */
#define PRINTF_DBG(level, fmt, args...) \
{                                       \
    if ((DEBUG_KEY) && (level >= PRINT_LEVEL_DEF))       \
    {                                   \
        printf("[%d][%s] %-15.*s, %d: ", (int)time(NULL) % 1000, MODULE_NAME, 15, __FUNCTION__, __LINE__); \
        printf(fmt, ## args);           \
    }                                   \
}


#define PRINTF_DBG_1(level, fmt, args...) \
{                                       \
    if ((DEBUG_KEY) && (level >= PRINT_LEVEL_DEF))       \
    {                                   \
        printf(fmt, ## args);           \
    }                                   \
}


#define ASSERT(expr, str)   \
do                          \
{                           \
    if (!(expr))            \
    {                       \
        printf("[%d][%s] %-15.*s, %d: ASSERT! %s\n", (int)time(NULL) % 1000, MODULE_NAME, 15, __FUNCTION__, __LINE__, str); \
        while(1);           \
    }                       \
} while(0);


#else
#define PRINTF_DBG(level,fmt, args...)
#define PRINTF_DBG_1(level, fmt, args...)
#define ASSERT(expr, str)
#endif




#endif /* __COMMON_H__ */


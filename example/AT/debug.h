/******************************************************************************
    Copyright (C), 2008-2018, JABSCO ELECTRONIC Tech. Co., Ltd
 
    File Name    : debug.h
    Version      : 1.0
    Author       : JABSCO Video Server Software Group
    Created      : 2009-03-10
    Description  : 
    History      : 
                        created by lsf. 2009-03-10
******************************************************************************/

#ifndef _DEBUG_H
#define _DEBUG_H

#ifdef __cplusplus
extern "C"{
#endif 

#include <syslog.h>
#include <libgen.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

#ifndef is_float0
#define is_float0(v) (v >= -0.000001 && v <= 0.000001)
#endif

#ifndef MIN
#define MIN(a,b) ((a) < (b) ? (a) : (b))
#endif

#ifndef MAX
#define MAX(a,b) ((a) > (b) ? (a) : (b))
#endif

#ifndef FALSE
#define FALSE		0
#define TRUE		1
#endif

#ifndef SUCCESS
#define SUCCESS		0
#endif

#ifndef FAILURE
#define FAILURE		(-1)
#endif

#ifndef NULL
#define NULL		0L
#endif

#ifndef TYPEBOOL
#define TYPEBOOL
typedef unsigned BOOL;
#endif

#ifndef RETVOID
#define RETVOID free(NULL)
#endif

#ifndef BADF00D_LL 
#define BADF00D_LL (0x64303066646162LL)
#endif

#ifndef bb_if_fail
#define bb_if_fail(condi, fmt, args...) do {                                         \
  if (!(condi)) {                                                                   \
    printf("[%s:%5d] \x1b[%s;%sm" fmt "\n\x1b[0m\n", \
            (char *)__FILE__,__LINE__, __BG, __FG_R, ##args); \
  }                                                                                 \
} while (0)
#endif

#ifndef return_val_if_fail
#define return_val_if_fail(condi, ret) do {                                         \
  if (!(condi)) {                                                                   \
    fprintf(stdout, "\x1b[1;31m%s|%d| fail (" #condi ")\n\x1b[0m", __FILE__, __LINE__);           \
    return ret;                                                                     \
  }                                                                                 \
} while (0)
#endif

#ifndef goto_tag_if_fail
#define goto_tag_if_fail(condi, tag) do {                                           \
  if (!(condi)) {                                                                   \
    fprintf(stdout, "\x1b[1;31m%s|%d| fail (" #condi ")\n\x1b[0m", __FILE__, __LINE__);           \
    __from_line__ = __LINE__;                                                       \
    goto tag;                                                                       \
  }                                                                                 \
} while (0)
#endif

#define __FG_R "31"
#define __FG_G "32"
#define __FG_Y "33"
#define __BG   "0"


#define COLOR_R(fmt, args...) do { \
    printf("[%s:%5d] \x1b[%s;%sm" fmt "\n\x1b[0m\n", \
            (char *)__FILE__,__LINE__, __BG, __FG_R, ##args); \
} while(0)

#define COLOR_G(fmt, args...) do { \
    printf("[%s:%5d] \x1b[%s;%sm" fmt "\n\x1b[0m\n", \
            (char *)__FILE__,__LINE__, __BG, __FG_G, ##args); \
} while(0)

#define COLOR_Y(fmt, args...) do { \
    printf("[%s:%5d] \x1b[%s;%sm" fmt "\n\x1b[0m\n", \
            (char *)__FILE__,__LINE__, __BG, __FG_Y, ##args); \
} while(0)


#define DBG(fmt, args...) do { \
    fprintf(stdout, "[-DBG-] [%s:%5d] " fmt, (char *)__FILE__,__LINE__,## args);    \
} while(0)

#define ERR(fmt, args...) do { \
    fprintf(stderr, "[%s:%5d] \x1b[1;31m""[*ERR*]" fmt "\x1b[0m", (char *)__FILE__,__LINE__,## args);    \
} while(0)

#define WAR(fmt, args...) do { \
    fprintf(stdout, "[%s:%5d] \x1b[%s;%sm [-WAR-]" fmt "\x1b[0m\n", \
            (char *)__FILE__,__LINE__, __BG, __FG_Y, ##args); \
} while(0)

#define LOG(fmt, args...) do { \
    printf("[%s:%5d] \x1b[%s;%sm [-WAR-]" fmt "\x1b[0m\n", \
            (char *)__FILE__,__LINE__, __BG, __FG_Y, ##args); \
	log_record(1, 1, 1, "server", "[%s:%5d] " fmt, (char *)__FILE__,__LINE__,## args);\
} while(0)

#ifndef SYSLOG
#define SYSLOG(fmt, args...) do { \
    fprintf(stdout, "[%s:%5d] \x1b[%s;%sm [-SYS-]" fmt "\x1b[0m\n", \
            (char *)__FILE__,__LINE__, __BG, __FG_Y, ##args); \
    syslog(LOG_INFO,"[%s:%5d] " fmt, (char *)__FILE__,__LINE__,## args);            \
} while(0)
#endif

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(array)	((int)(sizeof(array) / sizeof(array[0])))
#endif

#ifdef __cplusplus
}
#endif 

#endif


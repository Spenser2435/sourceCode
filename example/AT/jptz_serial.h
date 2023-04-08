/**
 * Copyright (C) by Jabsco Company
 *
 * @File Name    : JPtzSerial.h
 * @Created Time : 2014-09-04
 * @Version      : 1.0
 * @Author       : cheby
 * @Description  :
 */

#ifndef __JPTZSERIAL_H__
#define __JPTZSERIAL_H__


#include <time.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <limits.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <termios.h>
#include <sys/time.h>


#ifdef __cplusplus
extern "C" {
#endif

    typedef enum {
        CLR_START,
        CLR_IN,
        CLR_OUT,
        CLR_IN_OUT
    } CLR_MODE;
    
    int serial_get_fd();

    int serial_exit();

    int serial_recv(char *buf, int buflen);

    int serial_send(char *buf, int buflen);

    int serial_select_recv(char *buf, int len, struct timeval tv);

    int serial_init();

    int com_read(char *buf, int len);

    int com_send(char *buf, int len);

    int Flush(CLR_MODE flag);

    int change_serial(int baud,int databits, int stopbits, int parity);

    int serial_set_speed(int fd, int speed);

    int serial_set_parity(int fd, int databits, int stopbits, int parity);

#ifdef __cplusplus
}
#endif
#endif

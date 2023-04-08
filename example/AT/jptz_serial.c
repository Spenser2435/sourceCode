/**
 * Copyright (C) by Jabsco Company
 *
 * @File Name    : JPtzSerial.cpp
 * @Created Time : 2014-09-04
 * @Version      : 1.0
 * @Author       : cheby
 * @Description  :
 */

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
#include <pthread.h>

#include "debug.h"
#include "jptz_serial.h"

#define SERIAL_NAME  "/dev/ttyS0"

static pthread_mutex_t sReadLock;
static pthread_mutex_t sWriteLock;
static int g_fd = -1;

//设置波特率
int serial_set_speed(int fd, int speed)
{
    int speed_arr[] = {B50, B75, B110, B134, B150, B200, B300, B600, B1200, B1800, B2400, B4800, B9600,
                       B19200, B38400, B57600, B115200
                      };
    int name_arr[] = {50,  75,  110,  134,  150,  200,  300,  600,  1200,  1800,  2400,  4800,  9600,
                      19200,  38400,  57600,  115200
                     };
    int i = 0;
    int ret = 0;
    struct termios options;

    if (tcgetattr(fd, &options) != 0) {
        DBG("fd: %d speed: %d, %s\n", fd, speed, strerror(errno));
        return FAILURE;
    }

    for (i = 0;  i < ARRAY_SIZE(speed_arr);  i++) {
        if (speed == name_arr[i]) {
            tcflush(fd, TCIOFLUSH);
            ret = cfsetispeed(&options, speed_arr[i]);
            return_val_if_fail(ret == SUCCESS, FAILURE);

            cfsetospeed(&options, speed_arr[i]);
            return_val_if_fail(ret == SUCCESS, FAILURE);

            if (tcsetattr(fd, TCSANOW, &options)) {
                DBG("fd: %d speed: %d, %s\n", fd, speed, strerror(errno));
                return FAILURE;
            }

            tcflush(fd, TCIOFLUSH);

            return SUCCESS;
        }
    }

    DBG("no speed find\n");

    return FAILURE;
}

int serial_set_parity(int fd, int databits, int stopbits, int parity)
{
    struct termios options;

    if (tcgetattr(fd, &options) != 0) {
        DBG("fd: %d %s\n", fd, strerror(errno));
        return FAILURE;
    }
    options.c_cflag &= ~CSIZE;

    // 设置数据位数
    switch (databits) {
        case 5: {
            options.c_cflag |= CS5;
            break;
        }

        case 6: {
            options.c_cflag |= CS6;
            break;
        }

        case 7: {
            options.c_cflag |= CS7;
            break;
        }

        case 8: {
            options.c_cflag |= CS8;
            break;
        }

        default: {
            printf("Unsupported data size\n");
            return FAILURE;
        }
    }

    // 设 置停止位
    switch (stopbits) {
        case 1: {
            options.c_cflag &= ~CSTOPB;
            break;
        }

        case 2: {
            options.c_cflag |= CSTOPB;
            break;
        }

        default: {
            printf("Unsupported stop bits\n");
            return FAILURE;
        }
    }

    // 设置奇偶校验位
    switch (parity) {
        case 'n':
        case 'N': {
            options.c_cflag &= ~PARENB; // Clear parity enable
            options.c_iflag &= ~INPCK;  // Enable parity checking
            break;
        }

        case 'o':
        case 'O': { // 设置为奇效验
            options.c_cflag |= (PARODD | PARENB);
            options.c_iflag |= INPCK;   // Disnable parity checking
            break;
        }

        case 'e':
        case 'E': { // 设置为偶效验
            options.c_cflag |= PARENB;  // Enable parity
            options.c_cflag &= ~PARODD;
            options.c_iflag |= INPCK;   // Disnable parity checking
            break;
        }

        case 'S':
        case 's': { // as no parity
            options.c_cflag &= ~PARENB;
            options.c_cflag &= ~CSTOPB;
            break;
        }

        default: {
            printf("Unsupported parity\n");
            return FAILURE;
        }
    }

    // Set input parity option
    if (parity != 'n') {
        options.c_iflag |= INPCK;
    }

    options.c_iflag &= ~(IXON | IXOFF | IXANY); // avoid 0x13 stop the termios
    options.c_iflag &= ~(ICRNL | IGNCR | INLCR);
    options.c_iflag &= ~(IGNBRK | BRKINT | PARMRK);
    options.c_oflag &= ~(ONLCR|OCRNL);
    //raw input mode
    options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
    //raw output
    options.c_oflag &= ~OPOST;

    tcflush(fd, TCIFLUSH); // Update the options and do it NOW
    options.c_cc[VTIME] = 1;
    options.c_cc[VMIN] = 0;

    if (tcsetattr(fd, TCSANOW, &options) != 0) {
        perror("SetupSerial 3");
        return FAILURE;
    }

    return SUCCESS;
}

int Flush(CLR_MODE flag)
{
    if (g_fd <= 0)
        return FAILURE;

    switch (flag) {
        case CLR_IN:
            tcflush (g_fd, TCIFLUSH);
            break;
        case CLR_OUT:
            tcflush (g_fd, TCOFLUSH);
            break;
        case CLR_IN_OUT:
            tcflush (g_fd, TCIOFLUSH);
            break;
        default:
            return FAILURE;
    }
    return SUCCESS;
}

int serial_get_fd()
{
    return g_fd;
}

int serial_exit()
{
    if (g_fd) {
        close(g_fd);
        g_fd = -1;
    }
    return SUCCESS;
}

int serial_select_recv(char *buf, int len, struct timeval tv)
{
    fd_set fdSet;
    int max;
    int ret = FAILURE;
    int readLen = -1;
    int waitUsec;

    if (g_fd <= 0)
        return -1;

    FD_ZERO (&fdSet);
    FD_SET (g_fd, &fdSet);
    max = g_fd + 1;
    ret = select (max, &fdSet, NULL, NULL, &tv);
    if (ret <= 0) {
        ERR ("Jco 485 serial port select error, ret = %d.\n", ret);
        return -1;
    }

    //waiting for trasnmitting data
    waitUsec = len * 10 * 1000000LL / 9600 + 1000;

    usleep (waitUsec);

    readLen = serial_recv(buf, len);
    return readLen;
}

int serial_recv(char *buf, int buflen)
{
    int readlen = 0;
    int r = 0;

    if ((buf == NULL) || (g_fd <= 0))
        return -1;

    pthread_mutex_lock (&sReadLock);
    do {
        r = read(g_fd, buf+readlen, buflen-readlen);
        if (r < 0) {
            if (errno == EINTR) {
                continue;
            } else {
                ERR("[%d] read error:%s \n",  g_fd, strerror(errno));
            }
        }

        readlen += r;
    } while(0);
    pthread_mutex_unlock (&sReadLock);

    return readlen;
}

int serial_send(char *buf, int buflen)
{
    int sendlen = 0;
    int r = 0;
    if ((buf == NULL) || (g_fd <= 0))
        return -1;

    pthread_mutex_lock (&sWriteLock);
    do {
        r = write(g_fd, buf+sendlen, buflen-sendlen);
        if (r < 0) {
            if (errno == EINTR || errno == EAGAIN) {
                continue;
            } else {
                ERR("[%d] write error:%s \n",  g_fd, strerror(errno));
            }
        } else if (0 == r) {
            break;
        }
        sendlen += r;
    } while(1);
    pthread_mutex_unlock (&sWriteLock);

    return sendlen == buflen ? SUCCESS : FAILURE;
}

int com_read(char *buf, int len)
{
    return serial_recv(buf, len);
}

int com_send(char *buf, int len)
{
    return serial_send(buf, len);
}

int change_serial(int baud,int databits, int stopbits, int parity)
{
    serial_set_speed(g_fd, baud);

    serial_set_parity(g_fd, databits, stopbits, parity);

    SYSLOG("change serial port baud:%d, databit:%d, stopbit:%d, parity:%c\n",
           baud, databits, stopbits, parity);

    return 0;
}



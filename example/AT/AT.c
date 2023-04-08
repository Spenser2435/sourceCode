/* 
 *       Filename:  AT.c
 *    Description:  AT of Lierda
 *        Version:  1.0
 *        Created:  2022年10月24日 11时12分52秒
 *       Revision:  none
 *       Compiler:  gcc
 *         Author:  zhangjian (), 
 *   Organization:  
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

/* file */
#include <fcntl.h>
#include <sys/file.h>

/* socket() */
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>

#include "debug.h"
#include "jptz_serial.h"

#define SIM4G_AT_TTY_USB0             "/dev/tty4G"

static int __from_line__ = 0;

int sim4g_set_serial(int fd,int baud,int nbits,int parity,int stop)
{
    return_val_if_fail(SUCCESS == serial_set_speed(fd, baud), FAILURE);

    return_val_if_fail(SUCCESS == serial_set_parity(fd, nbits, stop, parity), FAILURE);

	return SUCCESS;
}

int sim4g_at_readfully(int fd, void* buf, int nbytes, struct timeval *tv)
{
    int nr = 0;
	int nread = 0;
    fd_set rw_set;			
    FD_ZERO(&rw_set);
    FD_SET(fd, &rw_set);
    struct timeval tv0;

	while ( nread < nbytes ) {
        memcpy(&tv0, tv, sizeof(tv0));
        nr = select(fd+1, &rw_set/*read*/, NULL, NULL, &tv0);
        //printf("nr:%d hi: %ld %ld\n", nr, tv0.tv_sec, tv0.tv_usec);

        if (nr < 0) {
            if ((errno == EINTR || errno == EAGAIN)) {
                usleep(1000);
                continue;
            } else {
                break;
            }
        } else if (nr == 0) {
            return nread;
        }

		int r = read(fd, (char*) buf + nread, nbytes - nread);

		if (0 > r) {
			if (errno == EINTR || errno == EAGAIN) {
				usleep( 10 * 1000 );
				continue;
			} else {
				return r;
			}
		} else if (0 == r) {
			break;
		}
		nread += r;
	}

	return nread;
}

int Writefully(int fd, const void *buf, int nbytes)
{
    int nwritten;

    nwritten = 0;
    while (nwritten < nbytes) {
        int r;

        r = write(fd, (char *)buf + nwritten, nbytes - nwritten);
        if (0 > r) {
            if (errno == EINTR || errno == EAGAIN) {
                usleep(10 * 1000);
                continue;
            } else {
                printf("%s error:%s %d\n", __FUNCTION__, strerror(errno), fd);
                return r;
            }
        } else if (0 == r) {
            break;
        }

        nwritten += r;
    }

    return nwritten;
}

int DumpFile(const char *dstFile, const char *buf, int len)
{
    int fddst = -1;
    int ret = FAILURE;

    fddst = open(dstFile, O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (-1 == fddst) {
        printf("open dst file %s fail\n", dstFile);
        ret = FAILURE;
        goto cleanup;
    }

    ret = Writefully(fddst, buf, len);
    if (0 > ret || len != ret) {
        printf("%s at write_fully fail, %d %d\n", __FUNCTION__, len, ret);
        ret = FAILURE;
        goto cleanup;
    }

    ret = SUCCESS;

cleanup:
    if (0 < fddst) {
        fsync(fddst);
        close(fddst);
        fddst = -1;
    }

    return ret;
}

#define TEE_FILE "/tmp/tee"

/* stdout 作为多命令操作时，重定向整体日志
 * tee    作为单个命令的返回 grep 判断
 *
 **/
int main(int argc, char *argv[])
{
    int i = 0;
    int tee = FALSE;
    int fd = -1;
    int ret = -1;
    int len = 0;
    char at[256] = {0};
    struct timeval timeout = {.tv_sec = 0, .tv_usec = 500*1000 };
    char *sec = getenv("L");

    if (NULL != sec) {
        timeout.tv_sec = atoi(sec);
    }

    if (argc != 2 && argc != 3) {
        printf("usage: %s <AT-query> [t]\n", argv[0]);
        printf("       with t, to write at response to file /tmp/tee\n");
        return FAILURE;
    }

    if (argc == 3 && argv[2][0] == 't') {
        tee = TRUE;
        remove(TEE_FILE);
    }

    len = strlen(argv[1]);

    strncpy(at, argv[1], sizeof(at) - 1);

    for (i = len - 1; i >= 0; i--) {
        if (at[i] == '\r' && at[i] == '\n' && at[i] == ' ') {
            at[i] = '\0';
        } else {
            break;
        }
    }

    at[++i] = '\r';
    at[++i] = '\n';
    len = strlen(at);

    char *buf = at + strlen(at);

    if ((fd = open(SIM4G_AT_TTY_USB0, O_RDWR)) < 0) {
        sleep(1);
        if ((fd = open(SIM4G_AT_TTY_USB0, O_RDWR)) < 0) {
            printf("open %s fail: %s\n", SIM4G_AT_TTY_USB0, strerror(errno));
            return FAILURE;
        }
    }

    ret = sim4g_set_serial(fd, 115200, 8, 'S', 1);
    goto_tag_if_fail(ret == SUCCESS, __exit);

    {
        int size = 0;
        int nr = 0;
        
        tcflush(fd,TCIOFLUSH);

        size = write(fd, at, i+1);
        goto_tag_if_fail(size > 0, __exit);
        
        nr = sim4g_at_readfully(fd, buf, sizeof(at)-len-1, &timeout);

        // 超时再重试一次
        if (nr <= 0) {
            timeout.tv_sec = 1;
            timeout.tv_usec = 300*1000;
            nr = sim4g_at_readfully(fd, buf, sizeof(at)-len-1, &timeout);
            goto_tag_if_fail(nr > 0, __exit);
        }

        printf("____ %s----\n", at);

        if (tee) {
            DumpFile(TEE_FILE, buf, nr);
        }
    }

    if (fd > 0) {
        close(fd);
    }
    return 0;

__exit:
    if (fd > 0) {
        close(fd);
    }
    printf("____ %s @%d\n", argv[1], __from_line__);
    return FAILURE;
}

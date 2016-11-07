#if !defined(__UTIL_H__)
#define __UTIL_H__

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/types.h>

int x_file_exists(const char *fname);
int x_file_isdir(const char *fpath);
int dodaemon(void);
int util_gettime_in_sec(const char* buf);
int util_getsize_in_byte(const char* buf);
int get_free_port(int port_wanted);
int set_sock_nonblock(int sockfd);

#endif /*__UTIL_H__*/

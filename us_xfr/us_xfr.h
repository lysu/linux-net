#include <errno.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>

#define SV_SOCK_PATH "/tmp/us_xfr"

#define BUF_SIZE 100

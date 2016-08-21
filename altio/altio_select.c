#include <sys/select.h>
#include <sys/time.h>

#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#define GN_NONNEG 01
#define GN_GT_0 02
#define GN_ANY_BASE 0100
#define GN_BASE_8 0200
#define GN_BASE_16 0400

static void usageError(const char *progName) {
    fprintf(stderr, "Usage: %s {timeout|-} fd-num[rw]...\n", progName);
    fprintf(stderr, "    - means infinite timeout; \n");
    fprintf(stderr, "    w = monitor for read \n");
    fprintf(stderr, "    r = monitor for write \n");
    fprintf(stderr, "    e.g.: %s - 0rw 1w\n", progName);
    exit(EXIT_FAILURE);
}

static void gnFail(const char *fname, const char *msg, const char *arg,
                   const char *name) {
    fprintf(stderr, "%s error", fname);
    if (name != NULL) fprintf(stderr, " (in %s)", name);
    fprintf(stderr, ": %s\n", msg);
    if (arg != NULL && *arg != '\0')
        fprintf(stderr, "        offending text: %s\n", arg);

    exit(EXIT_FAILURE);
}

static long getNum(const char *fname, const char *arg, int flags,
                   const char *name) {
    long res;
    char *endptr;
    int base;

    if (arg == NULL || *arg == '\0')
        gnFail(fname, "null or empty string", arg, name);

    base = (flags & GN_ANY_BASE) ? 0 : (flags & GN_BASE_8)
                                           ? 8
                                           : (flags & GN_BASE_16) ? 16 : 10;

    errno = 0;
    res = strtol(arg, &endptr, base);
    if (errno != 0) gnFail(fname, "strtol() failed", arg, name);

    if (*endptr != '\0') gnFail(fname, "nonnumeric characters", arg, name);

    if ((flags & GN_NONNEG) && res < 0)
        gnFail(fname, "negative value not allowed", arg, name);

    if ((flags & GN_GT_0) && res <= 0)
        gnFail(fname, "value must be > 0", arg, name);

    return res;
}

long getLong(const char *arg, int flags, const char *name) {
    return getNum("getLong", arg, flags, name);
}

int main(int argc, char *argv[]) {
    fd_set readfds, writefds;
    int ready, nfds, fd, numRead, j;
    struct timeval timeout;
    struct timeval *pto;
    char buf[10];

    if (argc < 2 || strcmp(argv[1], "--help") == 0) {
        usageError(argv[0]);
    }

    if (strcmp(argv[1], "-") == 0) {
        pto = NULL;
    } else {
        pto = &timeout;
        timeout.tv_sec = getLong(argv[1], 0, "timeout");
        timeout.tv_usec = 0;
    }

    nfds = 0;
    FD_ZERO(&readfds);
    FD_ZERO(&writefds);

    for (j = 2; j < argc; j++) {
        numRead = sscanf(argv[j], "%d%2[rw]", &fd, buf);
        if (numRead != 2) {
            usageError(argv[0]);
        }
        if (fd >= FD_SETSIZE) {
            perror("file descript exceed limit");
            abort();
        }

        if (fd >= nfds) nfds = fd + 1;
        if (strchr(buf, 'r') != NULL) FD_SET(fd, &readfds);
        if (strchr(buf, 'w') != NULL) FD_SET(fd, &writefds);
    }

    ready = select(nfds, &readfds, &writefds, NULL, pto);

    if (ready == -1) {
        perror("select");
        abort();
    }

    printf("ready = %d\n", ready);

    for (fd = 0; fd < nfds; fd++) {
        printf("%d: %s%s\n", fd, FD_ISSET(fd, &readfds) ? "r" : "",
               FD_ISSET(fd, &writefds) ? "w" : "");
    }

    if (pto != NULL)
        printf("timeout after select %ld.%03ld\n", (long)timeout.tv_sec,
               (long)timeout.tv_usec / 10000);

    exit(EXIT_SUCCESS);
}

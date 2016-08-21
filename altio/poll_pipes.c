#include <poll.h>
#include <time.h>

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

int getInt(const char *arg, int flags, const char *name) {
    long res;

    res = getNum("getInt", arg, flags, name);

    if (res > INT_MAX || res < INT_MIN)
        gnFail("getInt", "integer out of range", arg, name);

    return (int)res;
}

int main(int argc, char *argv[]) {
    int numPipes, j, ready, randPipe, numWrites;
    int(*pfds)[2];
    struct pollfd *pollFd;

    if (argc < 2 || strcmp(argv[1], "--help") == 0) {
        perror("param wrong");
        abort();
    }

    numPipes = getInt(argv[1], GN_GT_0, "num_pipes");

    pfds = calloc(numPipes, sizeof(int[2]));
    if (pfds == NULL) {
        perror("mem error");
        abort();
    }
    pollFd = calloc(numPipes, sizeof(struct pollfd));
    if (pollFd == NULL) {
        perror("malloc");
        abort();
    }

    for (j = 0; j < numPipes; j++) {
        if (pipe(pfds[j]) == -1) {
            perror("pipe");
            abort();
        }
    }

    numWrites = (argc > 2) ? getInt(argv[2], GN_GT_0, "num-writes") : 1;

    srandom((int)time(NULL));
    for (j = 0; j < numWrites; j++) {
        randPipe = random() % numPipes;
        printf("write to fd: %3d, read fd: %3d\n", pfds[randPipe][1],
               pfds[randPipe][0]);
        if (write(pfds[randPipe][1], "a", 1) == -1) {
            perror("write");
            abort();
        }
    }

    for (j = 0; j < numPipes; j++) {
        pollFd[j].fd = pfds[j][0];
        pollFd[j].events = POLLIN;
    }

    ready = poll(pollFd, numPipes, -1);
    if (ready == -1) {
        perror("poll");
        abort();
    }

    printf("poll return %d\n", ready);

    for (j = 0; j < numPipes; j++) {
        if (pollFd[j].revents & POLLIN) {
            printf("Readable %d %3d\n", j, pollFd[j].fd);
        }
    }

    exit(EXIT_SUCCESS);
}

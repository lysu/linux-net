#include <fcntl.h>
#include <sys/epoll.h>

#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#define MAX_BUF 1000
#define MAX_EVENTS 5

int main(int argc, char *argv[]) {
    int epfd, ready, fd, s, j, numOpenFds;
    struct epoll_event ev;
    struct epoll_event evlist[MAX_EVENTS];
    char buf[MAX_BUF];

    if (argc < 2 || strcmp(argv[1], "--help") == 0) {
        perror("%s file...");
        abort();
    }

    epfd = epoll_create(argc - 1);
    if (epfd == -1) {
        perror("epoll_create");
        abort();
    }

    for (j = 1; j < argc; j++) {
        fd = open(argv[j], O_RDONLY);
        if (fd == -1) {
            perror("open");
            abort();
        }
        printf("Opened \"%s\" on fd %d\n", argv[j], fd);

        ev.events = EPOLLIN; /* Only interested in input events */
        ev.data.fd = fd;
        if (epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &ev) == -1) {
            perror("epoll_ctl");
            abort();
        }
    }

    numOpenFds = argc - 1;

    while (numOpenFds > 0) {
        printf("About to epoll_wait()\n");
        ready = epoll_wait(epfd, evlist, MAX_EVENTS, -1);
        if (ready == -1) {
            if (errno == EINTR)
                continue;
            else {
                perror("epoll_wait");
                abort();
            }
        }

        printf("Ready: %d\n", ready);

        for (j = 0; j < ready; j++) {
            printf("  fd=%d; events: %s%s%s\n", evlist[j].data.fd,
                   (evlist[j].events & EPOLLIN) ? "EPOLLIN " : "",
                   (evlist[j].events & EPOLLHUP) ? "EPOLLHUP " : "",
                   (evlist[j].events & EPOLLERR) ? "EPOLLERR " : "");

            if (evlist[j].events & EPOLLIN) {
                s = read(evlist[j].data.fd, buf, MAX_BUF);
                if (s == -1) {
                    perror("read");
                    abort();
                }
                printf("    read %d bytes: %.*s\n", s, s, buf);
            } else if (evlist[j].events & (EPOLLHUP | EPOLLERR)) {
                printf("    closing fd %d\n", evlist[j].data.fd);
                if (close(evlist[j].data.fd) == -1) {
                    perror("close");
                    abort();
                }
                numOpenFds--;
            }
        }
    }

    printf("All file descriptors closed; bye\n");
    exit(EXIT_SUCCESS);
}

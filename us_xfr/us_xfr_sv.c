#include "us_xfr.h"

#define BACKLOG 5

int main(void) {
    struct sockaddr_un addr;
    int sfd, cfd;
    ssize_t numRead;
    char buf[BUF_SIZE];

    sfd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sfd == -1) {
        perror("socket");
        abort();
    }

    if (remove(SV_SOCK_PATH) == -1 && errno != ENOENT) {
        perror("remove path");
        abort();
    }

    memset(&addr, 0, sizeof(struct sockaddr_un));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, SV_SOCK_PATH, sizeof(addr.sun_path) - 1);

    if (bind(sfd, (struct sockaddr *)&addr, sizeof(struct sockaddr_un)) == -1) {
        perror("bind");
        abort();
    }

    if (listen(sfd, BACKLOG) == -1) {
        perror("listen");
        abort();
    }

    for (;;) {
        cfd = accept(sfd, NULL, NULL);
        if (cfd == -1) {
            perror("accept");
            abort();
        }

        while ((numRead = read(cfd, buf, BUF_SIZE)) > 0) {
            if (write(STDOUT_FILENO, buf, numRead) != numRead) {
                perror("partial write failure");
                abort();
            }
        }

        if (numRead == -1) {
            perror("read");
            abort();
        }

        if (close(cfd) == -1) {
            perror("close");
            abort();
        }
    }
}

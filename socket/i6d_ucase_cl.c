#include "i6d_ucase.h"

int main(int argc, char* argv[]) {
    struct sockaddr_in6 svaddr;
    int sfd, j;
    size_t msgLen;
    ssize_t numBytes;
    char resp[BUF_SIZE];

    if (argc < 3 || strcmp(argv[1], "--help") == 0) {
        printf("%s host-address msg...\n", argv[0]);
        abort();
    }

    sfd = socket(AF_INET6, SOCK_DGRAM, 0);
    if (sfd == -1) {
        perror("socket");
        abort();
    }

    memset(&svaddr, 0, sizeof(struct sockaddr_in6));
    svaddr.sin6_family = AF_INET6;
    svaddr.sin6_port = htons(PORT_NUM);
    if (inet_pton(AF_INET6, argv[1], &svaddr.sin6_addr) <= 0) {
        printf("inet_pton failed for address '%s'", argv[1]);
        abort();
    }

    for (j = 2; j < argc; j++) {
        msgLen = strlen(argv[j]);
        if (sendto(sfd, argv[j], msgLen, 0, (struct sockaddr*)&svaddr,
                   sizeof(struct sockaddr_in6)) != (ssize_t)msgLen) {
            perror("sendto");
            abort();
        }

        numBytes = recvfrom(sfd, resp, BUF_SIZE, 0, NULL, NULL);
        if (numBytes == -1) {
            perror("recvfrom");
            abort();
        }

        printf("Response %d: %.*s\n", j - 1, (int)numBytes, resp);
    }

    exit(EXIT_SUCCESS);
}

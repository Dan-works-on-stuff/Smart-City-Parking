#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <cstring>

int main() {
    int sockpair[2];
    socketpair(AF_UNIX, SOCK_STREAM, 0, sockpair);

    if (fork() == 0) {
        close(sockpair[0]);
        while (true) {
            char buffer[256];
            memset(buffer, 0, sizeof(buffer));
            std::cin.getline(buffer, 256);
            write(sockpair[1], buffer, strlen(buffer));
            if (strcmp(buffer, "quit") == 0)
                break;

            memset(buffer, 0, sizeof(buffer));
            read(sockpair[1], buffer, sizeof(buffer));
            std::cout << buffer;
        }
        close(sockpair[1]);
    } else {
        close(sockpair[1]);
        while (true) {
            char buffer[256];
            memset(buffer, 0, sizeof(buffer));
            read(sockpair[0], buffer, sizeof(buffer));
            std::cout << buffer;
        }
        close(sockpair[0]);
    }

    return 0;
}

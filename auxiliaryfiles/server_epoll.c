#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/epoll.h>

#define BUF_SIZE 1024
#define MAX_EVENTS 20
#define MAX_CLIENTS 20

int main(int argc, char *argv[])
{
    if (argc != 2) {
        printf("Usage: %s <port>\n", argv[0]);
        return -1;
    }

    int port = atoi(argv[1]);
    int sock, new_sock, epfd, ret;
    struct sockaddr_in address;
    char *message = "world";
    char buffer[BUF_SIZE] = {0};

    struct epoll_event event, events[MAX_EVENTS];

    // Create Socket
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        printf("Socket creation error\n");
        return -1;
    }

    // Set address
    address.sin_family = AF_INET;
    address.sin_port = htons(port);
    address.sin_addr.s_addr = INADDR_ANY;

    // Bind
    if (bind(sock, (struct sockaddr *)&address, sizeof(address)) < 0) {
        printf("Bind Failed\n");
        close(sock);
        return -1;
    }

    // Listen
    if (listen(sock, MAX_CLIENTS) < 0) {
        printf("Listen Failed\n");
        close(sock);
        return -1;
    }

    // Create epoll instance
    if ((epfd = epoll_create1(0)) < 0) {
        printf("Epoll creation failed\n");
        close(sock);
        return -1;
    }

    // Add server socket to epoll
    event.data.fd = sock;
    event.events = EPOLLIN;
    if (epoll_ctl(epfd, EPOLL_CTL_ADD, sock, &event) < 0) {
        printf("Failed to add socket to epoll\n");
        close(sock);
        return -1;
    }

    while (1) {
        int num_fds = epoll_wait(epfd, events, MAX_EVENTS, -1);
        if (num_fds < 0) {
            printf("Epoll wait error\n");
            break;
        }

        for (int i = 0; i < num_fds; i++) {
            // For Server
            if (events[i].data.fd == sock) 
            {
                socklen_t addrlen = sizeof(address);
                if ((new_sock = accept(sock, (struct sockaddr *)&address, &addrlen)) < 0) {
                    printf("Accept Failed\n");
                    continue;
                }

                // Add new clients
                event.data.fd = new_sock;
                event.events = EPOLLIN;
                epoll_ctl(epfd, EPOLL_CTL_ADD, new_sock, &event);

            } 
            // For Clients
            else 
            {
                int new_sock = events[i].data.fd;
                ret = recv(new_sock, buffer, BUF_SIZE, 0);
                // Client Disconnect
                if (ret == 0) {
                    printf("Client disconnected, socket fd: %d\n", new_sock);
                } 
                // Check and Response
                else if (ret > 0) {
                    buffer[ret] = '\0';
                    printf("Received: %s\n", buffer);

                    if (strcmp(buffer, "hello") == 0) {
                        send(new_sock, message, strlen(message), 0);
                        printf("Sent: %s\n", message);
                    }
                }
                close(new_sock);
                epoll_ctl(epfd, EPOLL_CTL_DEL, new_sock, NULL);
            }
        }
    }

    close(sock);
    close(epfd);
    return 0;
}
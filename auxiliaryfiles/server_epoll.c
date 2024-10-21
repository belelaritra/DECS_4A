#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/epoll.h>

#define BUF_SIZE 1024
#define MAX_CLIENTS 10
#define PORT 8080

int main() {
    int sock, new_sock, epoll_fd, event_count;
    struct sockaddr_in address;
    char *message = "world";
    char buffer[BUF_SIZE];
    
    // Create socket
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Socket creation error");
        return -1;
    }

    // Set up the address structure
    address.sin_family = AF_INET;
    address.sin_port = htons(PORT);
    address.sin_addr.s_addr = INADDR_ANY;

    // Bind the socket
    if (bind(sock, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("Bind failed");
        close(sock);
        return -1;
    }

    // Listen for incoming connections
    if (listen(sock, 3) < 0) {
        perror("Listen failed");
        close(sock);
        return -1;
    }

    // Create epoll instance
    if ((epoll_fd = epoll_create1(0)) < 0) {
        perror("Epoll creation failed");
        close(sock);
        return -1;
    }

    // Add the listening socket to the epoll instance
    struct epoll_event event;
    event.events = EPOLLIN;
    event.data.fd = sock;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, sock, &event) < 0) {
        perror("Epoll_ctl failed");
        close(sock);
        close(epoll_fd);
        return -1;
    }

    // Event loop
    while (1) {
        struct epoll_event events[MAX_CLIENTS];

        // Wait for events
        event_count = epoll_wait(epoll_fd, events, MAX_CLIENTS, -1);
        if (event_count < 0) {
            perror("Epoll_wait failed");
            break;
        }

        // Iterate through the events
        for (int i = 0; i < event_count; i++) {
            // Check if it's the listening socket
            if (events[i].data.fd == sock) {
                socklen_t addrlen = sizeof(address);
                if ((new_sock = accept(sock, (struct sockaddr *)&address, &addrlen)) < 0) {
                    perror("Accept failed");
                    continue;
                }

                // Add the new socket to the epoll instance
                event.events = EPOLLIN;
                event.data.fd = new_sock;
                if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, new_sock, &event) < 0) {
                    perror("Epoll_ctl failed for new socket");
                    close(new_sock);
                    continue;
                }
            } else {
                // Handle client data
                new_sock = events[i].data.fd;
                memset(buffer, 0, BUF_SIZE);
                int ret = recv(new_sock, buffer, BUF_SIZE - 1, 0);
                if (ret <= 0) {
                    // Client disconnected or error occurred
                    if (ret == 0) {
                        printf("Client disconnected, socket fd: %d\n", new_sock);
                    } else {
                        perror("recv error");
                    }
                    close(new_sock);
                    epoll_ctl(epoll_fd, EPOLL_CTL_DEL, new_sock, NULL);
                } else {
                    buffer[ret] = '\0'; // Null-terminate the buffer
                    printf("Received: %s\n", buffer);

                    // Respond if message is "hello"
                    if (strcmp(buffer, "hello") == 0) {
                        send(new_sock, message, strlen(message), 0);
                        printf("Sent: %s\n", message);
                    }

                    // Close the connection after sending the response
                    close(new_sock);
                    epoll_ctl(epoll_fd, EPOLL_CTL_DEL, new_sock, NULL);
                }
            }
        }
    }

    // Clean up
    close(sock);
    close(epoll_fd);
    return 0;
}

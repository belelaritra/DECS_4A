#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>


#define BUF_SIZE 1024

int main(int argc, char *argv[])
{
    if (argc != 2) {
        printf("Usage: %s <port>\n", argv[0]);
        return -1;
    }

    int port = atoi(argv[1]);

    int sock, ret, new_sock;
    struct sockaddr_in address;
    char *message = "world";
    char buffer[BUF_SIZE] = {0};
    
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) == 0) 
    {
        printf("Socket creation error\n");
        return -1;
    }

    address.sin_family = AF_INET;
    address.sin_port = htons(port);
    address.sin_addr.s_addr = INADDR_ANY;

    if (bind(sock, (struct sockaddr *)&address, sizeof(address)) < 0) 
    {
        printf("Bind Failed\n");
        return -1;
    }

    if (listen(sock, 3) < 0) 
    {
        printf("Listen Failed\n");
        return -1;
    }

    while (1) 
    {
        socklen_t addrlen = sizeof(address); 
        if ((new_sock = accept(sock, (struct sockaddr *)&address, &addrlen)) < 0) 
        {
            printf("Accept Failed\n");
            continue;
        }

        ret = recv(new_sock, buffer, BUF_SIZE, 0);
        if (ret == -1)
        {
            printf("Failed to receive message\n");
            continue;
        }
        printf("Received: %s\n", buffer);

        if (strcmp(buffer, "hello") == 0) 
        {
            ret = send(new_sock, message, strlen(message), 0);
            if (ret == -1)
            {
                printf("Failed to send message\n");
                continue;
            }
            printf("Sent: %s\n", message);
            close(new_sock);
        }
    }

    return 0;
}

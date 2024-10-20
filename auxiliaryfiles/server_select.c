#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/select.h>

#define BUF_SIZE 1024
#define MAX_CLIENTS 10

int main(int argc, char *argv[])
{
    if (argc != 2) {
        printf("Usage: %s <port>\n", argv[0]);
        return -1;
    }

    int port = atoi(argv[1]);

    int sock, new_sock, max_sd, activity, ret;
    struct sockaddr_in address;
    char *message = "world";
    char buffer[BUF_SIZE] = {0};
    
    fd_set read_fds; // Set of file descriptors to monitor
    int client_List[MAX_CLIENTS] = {0}; // Array to hold client sockets

    // Create socket
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) == 0) 
    {
        printf("Socket creation error\n");
        return -1;
    }

    // Set up the address structure
    address.sin_family = AF_INET;
    address.sin_port = htons(port);
    address.sin_addr.s_addr = INADDR_ANY;

    // Bind the socket
    if (bind(sock, (struct sockaddr *)&address, sizeof(address)) < 0) 
    {
        printf("Bind Failed\n");
        return -1;
    }

    // Listen for incoming connections
    if (listen(sock, 3) < 0) 
    {
        printf("Listen Failed\n");
        return -1;
    }

    while (1) 
    {
        // Clear the socket set
        FD_ZERO(&read_fds);
        
        // Add master socket to the set
        FD_SET(sock, &read_fds);
        max_sd = sock;

        // Add child sockets to set
        for (int i = 0; i < MAX_CLIENTS; i++) 
        {
            // Socket descriptor
            new_sock = client_List[i];

            // If valid socket descriptor then add to reading list
            if (new_sock > 0)
                FD_SET(new_sock, &read_fds);

            // Keep track of the maximum socket descriptor
            if (new_sock > max_sd)
                max_sd = new_sock;
        }

        // Wait for an activity on one of the sockets
        activity = select(max_sd + 1, &read_fds, NULL, NULL, NULL);
        
        if (activity < 0) 
        {
            printf("Select error\n");
        }

        if (FD_ISSET(sock, &read_fds)) 
        {
            socklen_t addrlen = sizeof(address);
            if ((new_sock = accept(sock, (struct sockaddr *)&address, &addrlen)) < 0) 
            {
                printf("Accept Failed\n");
                continue;
            }

            // Add new socket to array of sockets
            for (int i = 0; i < MAX_CLIENTS; i++) 
            {
                // If position is empty
                if (client_List[i] == 0) 
                {
                    client_List[i] = new_sock;
                    // printf("Adding to list of sockets as %d\n", i);
                    break;
                }
            }
        }

        // Check all clients for data to read
        for (int i = 0; i < MAX_CLIENTS; i++) 
        {
            new_sock = client_List[i];

            // If valid socket descriptor, then check for activity
            if (FD_ISSET(new_sock, &read_fds)) 
            {
                ret = recv(new_sock, buffer, BUF_SIZE, 0);
                if (ret == 0) 
                {
                    // Client disconnected
                    printf("Client disconnected, socket fd: %d\n", new_sock);
                    close(new_sock);
                    client_List[i] = 0;
                } 
                else if (ret > 0) 
                {
                    buffer[ret] = '\0'; // Null-terminate the buffer
                    printf("Received: %s\n", buffer);

                    // Respond if message is "hello"
                    if (strcmp(buffer, "hello") == 0) 
                    {
                        send(new_sock, message, strlen(message), 0);
                        printf("Sent: %s\n", message);
                    }
                }
            }
        }
    }

    return 0;
}

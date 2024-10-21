#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/select.h>

#define BUF_SIZE 1024
#define MAX_CLIENTS 20

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
    
    // Read_FD SET
    fd_set read_fds;
    // Initialize Clients to Zero : Empty
    int client_List[MAX_CLIENTS] = {0};

    // Socket Creation
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) == 0) 
    {
        printf("Socket creation error\n");
        return -1;
    }

    // Update Address 
    address.sin_family = AF_INET;
    address.sin_port = htons(port);
    address.sin_addr.s_addr = INADDR_ANY;

    // Bind
    if (bind(sock, (struct sockaddr *)&address, sizeof(address)) < 0) 
    {
        printf("Bind Failed\n");
        close(sock);
        return -1;
    }

    // Listen
    if (listen(sock, 3) < 0) 
    {
        printf("Listen Failed\n");
        close(sock);
        return -1;
    }

    while (1) 
    {
        // Clear Read_FD SET
        FD_ZERO(&read_fds);
        
        // Add Server Socket FD in SET
        FD_SET(sock, &read_fds);
        max_sd = sock;

        // Traverse over Clients & if Client Val NonZero -> Not Empty
        for (int i = 0; i < MAX_CLIENTS; i++) 
        {
            new_sock = client_List[i];

            // If NonZero
            if (new_sock > 0)
                FD_SET(new_sock, &read_fds);

            // Update Max_SD
            if (new_sock > max_sd)
                max_sd = new_sock;
        }

        // Check Activity from Read_FD SET
        activity = select(max_sd + 1, &read_fds, NULL, NULL, NULL);
        
        if (activity < 0) 
        {
            printf("Select error\n");
        }

        // For Server
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

        // For Clients
        for (int i = 0; i < MAX_CLIENTS; i++) 
        {
            new_sock = client_List[i];

            // If Activity from this client
            if (FD_ISSET(new_sock, &read_fds)) 
            {
                ret = recv(new_sock, buffer, BUF_SIZE, 0);
                // Client Disconnected
                if (ret == 0) 
                {
                    printf("Client disconnected, socket fd: %d\n", new_sock);
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

                close(new_sock);
                client_List[i] = 0;
            }
        }
    }

    close(sock);
    return 0;
}

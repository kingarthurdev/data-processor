#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netdb.h>

#define BUF_SIZE 1024

// Function to resolve hostname to IP
void resolve_hostname(const char *hostname, struct sockaddr_in *server_addr) {
    struct hostent *host_info = gethostbyname(hostname);
    if (host_info == NULL) {
        perror("gethostbyname failed");
        exit(1);
    }

    server_addr->sin_family = AF_INET;
    memcpy(&(server_addr->sin_addr.s_addr), host_info->h_addr_list[0], host_info->h_length);
}

// Function to send HTTP GET request
int send_get_request(const char *hostname, const char *path) {
    int sockfd;
    struct sockaddr_in server_addr;
    char buffer[BUF_SIZE];
    char request[BUF_SIZE];
    int n;

    // Resolve the hostname to an IP address
    resolve_hostname(hostname, &server_addr);

    // Create socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("Socket creation failed");
        return -1;
    }

    // Connect to the server
    if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connection failed");
        close(sockfd);
        return -1;
    }

    // Format the HTTP GET request
    snprintf(request, sizeof(request), "GET %s HTTP/1.1\r\nHost: %s\r\nConnection: close\r\n\r\n", path, hostname);

    // Send the GET request
    if (send(sockfd, request, strlen(request), 0) < 0) {
        perror("Send failed");
        close(sockfd);
        return -1;
    }

    // Read the response
    printf("Response:\n");
    while ((n = recv(sockfd, buffer, sizeof(buffer) - 1, 0)) > 0) {
        buffer[n] = '\0';
        printf("%s", buffer);
    }

    if (n < 0) {
        perror("Receive failed");
        close(sockfd);
        return -1;
    }

    // Close the socket
    close(sockfd);
    return 0;
}

int main() {
    const char *hostname = "webhook.site";
    const char *path = "/5b6e416a-ecc6-4c2d-a5d1-4d38966c1c3d";  // Set the correct path for your webhook URL

    printf("Pinging webhook URL: %s\n", hostname);

    if (send_get_request(hostname, path) == 0) {
        printf("\nSuccessfully pinged the webhook.\n");
    } else {
        printf("\nFailed to ping the webhook.\n");
    }

    return 0;
}

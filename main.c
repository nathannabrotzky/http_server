#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>
#include <stdint.h>

int main() {
    WSADATA wsaData; 
    SOCKET listenSocket;
    SOCKET clientSocket;
    struct sockaddr_in serverAddr;
    struct sockaddr_in clientAddr;
    int clientAddrLen = sizeof(clientAddr);
    char recvBuf[4096];
    //int bytesReceived;

    // Initialize Winsock
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        printf("WSAStartup failed: %d\n", WSAGetLastError());
        return 1;
    }

    // Create a socket
    listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listenSocket == INVALID_SOCKET) {
        printf("Socket creation failed.\n");
        WSACleanup();
        return 1;
    }

    // Bind the socket
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(8080); // Listen on port 8080

    if (bind(listenSocket, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        printf("Bind failed.\n");
        closesocket(listenSocket);
        WSACleanup();
        return 1;
    }

    // Listen for connections
    if (listen(listenSocket, SOMAXCONN) == SOCKET_ERROR) {
        printf("Listen failed.\n");
        closesocket(listenSocket);
        WSACleanup();
        return 1;
    }

    printf("Server listening on port 8080...\n");

    // Accept a client connection
    clientSocket = accept(listenSocket, (struct sockaddr *)&clientAddr, &clientAddrLen);
    if (clientSocket == INVALID_SOCKET) {
        printf("Accept failed.\n");
        closesocket(listenSocket);
        WSACleanup();
        return 1;
    }

    printf("Client connected.\n");

    int bytesReceived = recv(clientSocket, recvBuf, sizeof(recvBuf) - 1, 0);
    if (bytesReceived > 0) {
        recvBuf[bytesReceived] = '\0';
        printf("Received request:\n%s\n", recvBuf);

        // Check if it's a GET request for "/" or "/index.html"
        if (strncmp(recvBuf, "GET / ", 6) == 0 || strncmp(recvBuf, "GET /index.html", 15) == 0) {
            FILE *file = fopen("index.html", "rb");
            if (file) {
                // Get file size
                fseek(file, 0, SEEK_END);
                long fileSize = ftell(file);
                fseek(file, 0, SEEK_SET);

                // Allocate buffer for file content
                char *fileBuffer = malloc(fileSize);
                fread(fileBuffer, 1, fileSize, file);
                fclose(file);

                // Send HTTP response header
                char header[256];
                sprintf(header,
                    "HTTP/1.1 200 OK\r\n"
                    "Content-Type: text/html\r\n"
                    "Content-Length: %ld\r\n"
                    "Connection: close\r\n"
                    "\r\n", fileSize);
                send(clientSocket, header, strlen(header), 0);

                // Send file content
                send(clientSocket, fileBuffer, fileSize, 0);
                free(fileBuffer);
            } else {
                const char *notFound =
                    "HTTP/1.1 404 Not Found\r\n"
                    "Content-Type: text/plain\r\n"
                    "Content-Length: 13\r\n"
                    "Connection: close\r\n"
                    "\r\n"
                    "404 Not Found";
                send(clientSocket, notFound, strlen(notFound), 0);
            }
        }
    }

    closesocket(clientSocket);
    closesocket(listenSocket);
    WSACleanup();

    return 0;
}
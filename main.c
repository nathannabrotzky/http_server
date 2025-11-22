#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>
#include <stdint.h>

int init_winsock();
int create_server(SOCKET *listener);
int main_loop(SOCKET *listener, SOCKET *client);
const char* get_mime_type(const char* path);
void serve_file(SOCKET client, const char *rootDir, const char *requestPath);

int main() {
    SOCKET listenSocket;
    SOCKET clientSocket;

    // Initialize Winsock
    init_winsock();

    // Create the listening socket
    create_server(&listenSocket);

    // Main server behavior
    main_loop(&listenSocket,&clientSocket);

    //Cleanup
    closesocket(clientSocket);
    closesocket(listenSocket);
    WSACleanup();

    return 0;
}

int init_winsock() {
    WSADATA wsaData; 
    if (WSAStartup(MAKEWORD(2,2), &wsaData) != 0) {
        printf("WSAStartup failed: %d\n", WSAGetLastError());
        return 1;
    }
    return 0;
}

int create_server(SOCKET *listener) {
    struct sockaddr_in server;
    //Create a listening socket
    *listener = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (*listener == INVALID_SOCKET) {
        printf("Socket creation failed.\n");
        WSACleanup();
        return 1;
    }

    // initialize the server
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(8080); // Listen on port 8080

    // bind the listening socket to the server
    if (bind(*listener, (struct sockaddr *)&server, sizeof(server)) == SOCKET_ERROR) {
        printf("Bind failed.\n");
        closesocket(*listener);
        WSACleanup();
        return 1;
    }
    return 0;
}

int main_loop(SOCKET *listener, SOCKET *client) {
    struct sockaddr_in clientSrc;
    int clientAddrLen = sizeof(clientSrc);
    char recvBuf[4096];
    int bytesReceived;

    // Call listen ONCE
    if (listen(*listener, SOMAXCONN) == SOCKET_ERROR) {
        printf("Listen failed: %d\n", WSAGetLastError());
        closesocket(*listener);
        WSACleanup();
        return 1;
    }

    printf("Server listening on port 8080...\n");

    while (1) {
        *client = accept(*listener, (struct sockaddr *) &clientSrc, &clientAddrLen);
        if (*client == INVALID_SOCKET) {
            printf("Accept failed: %d\n", WSAGetLastError());
            continue;
        }

        printf("Client connected.\n");

        // Handle client request
        bytesReceived = recv(*client, recvBuf, sizeof(recvBuf) - 1, 0);
        if (bytesReceived > 0) {
            recvBuf[bytesReceived] = '\0';
            printf("Received request:\n%s\n", recvBuf);

            char method[8], path[256], protocol[16];
            sscanf(recvBuf, "%s %s %s", method, path, protocol);

            if (strcmp(method, "GET") == 0) {
                serve_file(*client, "C:\\Users\\nabro\\Desktop\\Portfolio Projects\\C\\http_server\\wwwroot", path);
            }
        }

        // Close client after response
        closesocket(*client);
        printf("Client disconnected.\n");
    }


    return 0;
}

const char* get_mime_type(const char* path) {
    if (strstr(path, ".html")) return "text/html";
    if (strstr(path, ".css"))  return "text/css";
    if (strstr(path, ".js"))   return "application/javascript";
    if (strstr(path, ".png"))  return "image/png";
    if (strstr(path, ".jpg") || strstr(path, ".jpeg")) return "image/jpeg";
    if (strstr(path, ".gif"))  return "image/gif";
    return "application/octet-stream"; // fallback
}

void serve_file(SOCKET client, const char *rootDir, const char *requestPath) {
    char filePath[512];

    // Default to index.html if root requested
    if (strcmp(requestPath, "/") == 0) {
        snprintf(filePath, sizeof(filePath), "%s/index.html", rootDir);
    } else {
        const char *filename = requestPath[0] == '/' ? requestPath + 1 : requestPath;

        // Prevent directory traversal
        if (strstr(filename, "..")) {
            const char *badReq =
                "HTTP/1.1 400 Bad Request\r\n"
                "Content-Type: text/plain\r\n"
                "Content-Length: 11\r\n"
                "Connection: close\r\n"
                "\r\n"
                "Bad Request";
            send(client, badReq, strlen(badReq), 0);
            return;
        }

        snprintf(filePath, sizeof(filePath), "%s/%s", rootDir, filename);
    }

    FILE *file = fopen(filePath, "rb");
    if (file) {
        fseek(file, 0, SEEK_END);
        long fileSize = ftell(file);
        fseek(file, 0, SEEK_SET);

        char *buffer = malloc(fileSize);
        fread(buffer, 1, fileSize, file);
        fclose(file);

        const char* mime = get_mime_type(filePath);

        char header[256];
        sprintf(header,
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: %s\r\n"
            "Content-Length: %ld\r\n"
            "Connection: keep-alive\r\n"
            "\r\n", mime, fileSize);

        send(client, header, strlen(header), 0);
        send(client, buffer, fileSize, 0);
        free(buffer);
    } else {
        const char *notFound =
            "HTTP/1.1 404 Not Found\r\n"
            "Content-Type: text/plain\r\n"
            "Content-Length: 13\r\n"
            "Connection: close\r\n"
            "\r\n"
            "404 Not Found";
        send(client, notFound, strlen(notFound), 0);
    }
}



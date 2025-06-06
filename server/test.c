#include <winsock2.h>
#include <windows.h>
#include <stdio.h>
#include <direct.h>
#include "server.h"
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")

// Get the absolute path to the web directory
char *get_web_root()
{
    static char web_root[MAX_PATH];
    static char full_path[MAX_PATH];
    char exe_path[MAX_PATH];
    char drive[MAX_PATH];
    char dir[MAX_PATH];

    // Get the path of the executable
    GetModuleFileName(NULL, exe_path, MAX_PATH);

    // Split the path into drive and directory
    _splitpath(exe_path, drive, dir, NULL, NULL);

    // Construct the path to web directory
    sprintf(web_root, "%s%s../web", drive, dir);

    // Convert to absolute path
    GetFullPathName(web_root, MAX_PATH, full_path, NULL);

    printf("Executable path: %s\n", exe_path);
    printf("Web root path: %s\n", full_path);

    return full_path;
}

void send_http_response(SOCKET client_socket, const char *content_type, const char *body, int body_length)
{
    char header[1024];
    int header_length = sprintf(header,
                                "HTTP/1.1 200 OK\r\n"
                                "Content-Type: %s\r\n"
                                "Content-Length: %d\r\n"
                                "Connection: close\r\n"
                                "\r\n",
                                content_type, body_length);

    send(client_socket, header, header_length, 0);
    send(client_socket, body, body_length, 0);
}

void send_404(SOCKET client_socket)
{
    const char *not_found = "404 Not Found";
    char header[1024];
    int header_length = sprintf(header,
                                "HTTP/1.1 404 Not Found\r\n"
                                "Content-Type: text/plain\r\n"
                                "Content-Length: %d\r\n"
                                "Connection: close\r\n"
                                "\r\n",
                                strlen(not_found));

    send(client_socket, header, header_length, 0);
    send(client_socket, not_found, strlen(not_found), 0);
}

void handle_request(SOCKET client_socket, const char *request)
{
    // Get the web root path
    const char *web_root = get_web_root();

    // Simple request parsing - just get the path
    char path[MAX_PATH];
    strcpy(path, web_root); // Start with web root

    printf("\n=== New Request ===\n");
    printf("Raw request: %s\n", request); // Print the entire request

    if (strncmp(request, "GET ", 4) == 0)
    {
        const char *path_start = request + 4;
        const char *path_end = strchr(path_start, ' ');
        if (path_end)
        {
            int path_len = path_end - path_start;
            if (path_len > 0)
            {
                // Convert forward slashes to backslashes and copy the path
                char *temp = (char *)malloc(path_len + 1);
                strncpy(temp, path_start, path_len);
                temp[path_len] = '\0';

                printf("Requested URL path: %s\n", temp); // Debug print

                // Remove any query parameters
                char *query = strchr(temp, '?');
                if (query)
                    *query = '\0';

                // If it's just "/", serve index.html
                if (strcmp(temp, "/") == 0)
                {
                    strcat(path, "\\index.html");
                    printf("Serving index.html for root path\n");
                }
                else
                {
                    // For other paths, append the requested path
                    // Skip the leading slash
                    if (temp[0] == '/')
                    {
                        strcat(path, "\\");
                        strcat(path, temp + 1);
                    }
                    else
                    {
                        strcat(path, "\\");
                        strcat(path, temp);
                    }
                    printf("Constructed file path: %s\n", path);
                }
                free(temp);
            }
        }
    }

    printf("Final file path: %s\n", path); // Debug print

    // Try to open and read the file
    FILE *file = fopen(path, "rb");
    if (file)
    {
        printf("Successfully opened file: %s\n", path); // Debug print
        // Get file size
        fseek(file, 0, SEEK_END);
        long file_size = ftell(file);
        fseek(file, 0, SEEK_SET);

        // Read file content
        unsigned char *content = (unsigned char *)malloc(file_size);
        if (content)
        {
            size_t bytes_read = fread(content, 1, file_size, file);

            // Determine content type based on file extension
            const char *content_type = "text/html";    // Default content type
            const char *file_ext = strrchr(path, '.'); // Get the file extension

            if (file_ext)
            {
                printf("File extension: %s\n", file_ext); // Debug print
                if (strcmp(file_ext, ".css") == 0)
                {
                    content_type = "text/css";
                    printf("CSS file detected! Setting content type to: %s\n", content_type);
                }
                else if (strcmp(file_ext, ".js") == 0)
                {
                    content_type = "application/javascript";
                    printf("JavaScript file detected! Setting content type to: %s\n", content_type);
                }
                else if (strcmp(file_ext, ".png") == 0)
                    content_type = "image/png";
                else if (strcmp(file_ext, ".jpg") == 0 || strcmp(file_ext, ".jpeg") == 0)
                    content_type = "image/jpeg";
            }

            // Send headers
            char header[1024];
            int header_length = sprintf(header,
                                        "HTTP/1.1 200 OK\r\n"
                                        "Content-Type: %s\r\n"
                                        "Content-Length: %ld\r\n"
                                        "Connection: close\r\n"
                                        "\r\n",
                                        content_type, bytes_read);

            printf("Sending response with content type: %s\n", content_type); // Debug print
            printf("Response headers:\n%s\n", header);                        // Print the full headers

            send(client_socket, header, header_length, 0);
            send(client_socket, (char *)content, bytes_read, 0);

            free(content);
        }
        fclose(file);
    }
    else
    {
        printf("File not found: %s (Error: %d)\n", path, GetLastError()); // Added error code
        send_404(client_socket);
    }
    printf("=== End Request ===\n\n");
}

void launch(struct server *Server)
{
    char buffer[1024];
    int address_length = sizeof(Server->address);
    SOCKET new_socket;

    printf("Server started on port %d\n", Server->port);
    printf("Serving files from %s\n", get_web_root());

    while (1)
    {
        printf("==== Waiting For Connection ====\n");
        new_socket = accept(Server->socket, (SOCKADDR *)&Server->address, &address_length);

        if (new_socket == INVALID_SOCKET)
        {
            printf("Accept failed with error: %d\n", WSAGetLastError());
            continue;
        }

        printf("==== Connection Accepted ====\n");

        int bytes_received = recv(new_socket, buffer, sizeof(buffer) - 1, 0);
        if (bytes_received > 0)
        {
            buffer[bytes_received] = '\0'; // Null terminate the received data
            printf("==== Request Received ====\n");
            printf("%s\n", buffer);

            handle_request(new_socket, buffer);
        }

        closesocket(new_socket);
    }

    // Cleanup (this will only be reached if the loop is broken)
    closesocket(Server->socket);
    WSACleanup();
}

int main()
{
    struct server Server = server_constructor(AF_INET, SOCK_STREAM, 0, INADDR_ANY, 80, 100, launch);
    Server.launch(&Server);
    return 0;
}
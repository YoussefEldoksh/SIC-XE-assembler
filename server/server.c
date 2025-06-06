#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "server.h"

struct server server_constructor(int domain, int protocol, int service, ULONG ip_interface, int port, int backlog, void (*launch)(struct server *))
{
    struct server server;

    // Initialize Winsock
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
    {
        perror("WSAStartup failed...\n");
        exit(1);
    }

    server.domain = domain;
    server.protocol = protocol;
    server.service = service;
    server.ip_interface = ip_interface;
    server.port = port;
    server.backlog = backlog;

    server.address.sin_family = domain;
    server.address.sin_port = htons(port);
    server.address.sin_addr.s_addr = htonl(ip_interface);

    server.socket = socket(domain, protocol, service);

    if (server.socket == INVALID_SOCKET)
    {
        perror("Failed to create socket...\n");
        WSACleanup();
        exit(1);
    }

    if (bind(server.socket, (SOCKADDR *)&server.address, sizeof(server.address)) == SOCKET_ERROR)
    {
        perror("Failed to bind socket...\n");
        closesocket(server.socket);
        WSACleanup();
        exit(1);
    }

    if (listen(server.socket, server.backlog) == SOCKET_ERROR)
    {
        perror("Failed to start Listening...\n");
        closesocket(server.socket);
        WSACleanup();
        exit(1);
    }

    server.launch = launch;
    return server;
}
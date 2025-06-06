#ifndef server_h
#define server_h

#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")

struct server
{
    int domain;
    int protocol;
    int service;
    ULONG ip_interface;
    int port;
    int backlog;
    SOCKET socket;

    SOCKADDR_IN address;

    void (*launch)(struct server *);
};

struct server server_constructor(int domain, int protocol, int service, ULONG ip_interface, int port, int backlog, void (*launch)(struct server *));

#endif

#ifndef _SOCKET_INCLUDED
#define _SOCKET_INCLUDED

#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "Ws2_32.lib")

SOCKET cre_soc_connect(const char *ip_str, const char *port_str);

#endif
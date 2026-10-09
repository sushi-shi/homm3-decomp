/* winsock.h - Windows Sockets over the BSD socket calls Loki's port used
 * (work/loki-game only). */
#ifndef HOMM3_LOKI_WINSOCK_H
#define HOMM3_LOKI_WINSOCK_H

#include <windows.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>

typedef int SOCKET;
#define INVALID_SOCKET (-1)
#define SOCKET_ERROR (-1)
#define closesocket close
#define ioctlsocket ioctl

typedef struct WSAData {
    WORD wVersion;
    WORD wHighVersion;
    char szDescription[257];
    char szSystemStatus[129];
    unsigned short iMaxSockets;
    unsigned short iMaxUdpDg;
    char* lpVendorInfo;
} WSADATA, *LPWSADATA;

extern "C" {
int WSAStartup(WORD version, LPWSADATA data);
int WSACleanup(void);
int WSAGetLastError(void);
}

#endif

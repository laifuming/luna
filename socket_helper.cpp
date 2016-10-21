﻿#ifdef _MSC_VER
#include <Winsock2.h>
#include <Ws2tcpip.h>
#include <windows.h>
#endif
#include <string.h>
#include "tools.h"
#include "socket_helper.h"

#if defined(__linux) || defined(__APPLE__)
void SetSocketNoneBlock(socket_t nSocket)
{
    int     nOption  = 0;

    nOption = fcntl(nSocket, F_GETFL, 0);
    fcntl(nSocket, F_SETFL, nOption | O_NONBLOCK);
}
#endif

#ifdef _MSC_VER
void SetSocketNoneBlock(socket_t nSocket)
{
    u_long  ulOption = 1;
    ioctlsocket(nSocket, FIONBIO, &ulOption);
}
#endif

bool make_ip_addr(sockaddr_storage& addr, const char ip[], int port)
{
	memset(&addr, 0, sizeof(addr));

	if (strchr(ip, ':') || ip[0] == '\0')
	{
		sockaddr_in6* ipv6 = (sockaddr_in6*)&addr;
		ipv6->sin6_family = AF_INET6;
		ipv6->sin6_port = htons(port);
		ipv6->sin6_addr = in6addr_any;
		return ip[0] == '\0' || inet_pton(AF_INET6, ip, &ipv6->sin6_addr) == 1;
	}

	sockaddr_in* ipv4 = (sockaddr_in*)&addr;
	ipv4->sin_family = AF_INET;
	ipv4->sin_port = htons(port);
	return inet_pton(AF_INET, ip, &ipv4->sin_addr) == 1;
}


// http://beej.us/guide/bgnet/output/html/multipage/getaddrinfoman.html
// http://man7.org/linux/man-pages/man3/getaddrinfo.3.html
socket_t ConnectSocket(const char szIP[], int nPort)
{
	socket_t nResult = INVALID_SOCKET;
	int nRetCode = false;
	socket_t nSocket = INVALID_SOCKET;
	//hostent* pHost = NULL;
	sockaddr_storage addr;

	// 名字解析,弄成另外一个单独的接口?
	// getaddrinfo
	//pHost = gethostbyname(szIP);
	//FAILED_JUMP(pHost);

	nRetCode = make_ip_addr(addr, szIP, nPort);
	FAILED_JUMP(nRetCode);

	nSocket = socket(addr.ss_family, SOCK_STREAM, IPPROTO_IP);
	FAILED_JUMP(nSocket != INVALID_SOCKET);

	SetSocketNoneBlock(nSocket);

	nRetCode = connect(nSocket, (sockaddr*)&addr, sizeof(addr));
	if (nRetCode == SOCKET_ERROR)
	{
		nRetCode = GetSocketError();

#ifdef _MSC_VER
		FAILED_JUMP(nRetCode == WSAEWOULDBLOCK);
#endif

#if defined(__linux) || defined(__APPLE__)
		FAILED_JUMP(nRetCode == EINPROGRESS);
#endif
	}

	nResult = nSocket;
Exit0:
	if (nResult == INVALID_SOCKET)
	{
		if (nSocket != INVALID_SOCKET)
		{
			CloseSocketHandle(nSocket);
			nSocket = INVALID_SOCKET;
		}
	}
	return nResult;
}

// nTimeout: 单位ms,传入-1表示阻塞到永远
bool CheckSocketWriteable(socket_t nSocket, int nTimeout)
{
	timeval timeoutValue = { nTimeout / 1000, 1000 * (nTimeout % 1000) };
	fd_set writeSet;

	FD_ZERO(&writeSet);
	FD_SET(nSocket, &writeSet);

	int nRetCode = select((int)nSocket + 1, NULL, &writeSet, NULL, &timeoutValue);
	return (nRetCode == 1);
}
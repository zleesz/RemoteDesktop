#pragma once
#include <WinSock2.h>
#include <ws2tcpip.h>
#include <string>
#include <sstream>

class socket_util
{
public:
	static bool get_peer_ip_port(int fd, std::string* ip, unsigned short *port)
	{
		// discovery client information
		struct sockaddr_in addr;
		socklen_t addrlen = sizeof(addr);
		if(getpeername(fd, (struct sockaddr*)&addr, &addrlen) == -1)
		{
			fprintf(stderr,"discovery client information failed, fd=%d, errno=%d(%#x).\n", fd, errno, errno);
			return false;
		}

		char buf[16] = {0};
		//If no error occurs, inet_ntoa returns a character pointer to a static buffer   
		//containing the text address in standard ".'' notation  
		strcpy(buf, inet_ntoa(addr.sin_addr));
		*port = ntohs(addr.sin_port);
		*ip = buf;
		return true;
	}
	static bool get_peer_ip_portstr(int fd, std::string* str)
	{
		// discovery client information
		struct sockaddr_in addr;
		socklen_t addrlen = sizeof(addr);
		if(getpeername(fd, (struct sockaddr*)&addr, &addrlen) == -1)
		{
			fprintf(stderr,"discovery client information failed, fd=%d, errno=%d(%#x).\n", fd, errno, errno);
			return false;
		}
		char buf[16] = {0};
		//If no error occurs, inet_ntoa returns a character pointer to a static buffer   
		//containing the text address in standard ".'' notation  
		strcpy(buf, inet_ntoa(addr.sin_addr));
		int port = ntohs(addr.sin_port);

		fprintf(stdout, "get peer ip of client ip=%s, port=%d, fd=%d\n", buf, port, fd);
		std::ostringstream oss;
		oss << buf << ":" << port;
		*str = oss.str();
		return true;
	}
	static int inet_pton (int af, const char* src, void* dst)
	{
		switch (af)
		{
		case AF_INET:
			{
				struct sockaddr_in sa;
				int len = sizeof(sa);
				sa.sin_family = AF_INET;
				if (!WSAStringToAddress ((LPTSTR)src, af, NULL, (LPSOCKADDR)&sa, &len))
				{
					memcpy (dst, &sa.sin_addr, sizeof(struct in_addr));
					return 1;
				}
				else
					return -1;
			}
		case AF_INET6:
			{
				struct sockaddr_in6 sa;
				int len = sizeof(sa);
				sa.sin6_family = AF_INET6;
				if (!WSAStringToAddress ((LPTSTR)src, af, NULL, (LPSOCKADDR)&sa, &len))
				{
					memcpy (dst, &sa.sin6_addr, sizeof(struct in6_addr));
					return 1;
				}
				else
					return -1;
			}
		}
		return -1;
	}

	static char* inet_ntop (int af, const void* src, char* dst, size_t dstlen)
	{
		switch (af)
		{
		case AF_INET:
			{
				struct sockaddr_in sa;
				DWORD len = dstlen;
				sa.sin_family = AF_INET;
				memcpy (&sa.sin_addr, src, sizeof(struct in_addr));
				if (!WSAAddressToString ((LPSOCKADDR)&sa, (DWORD)sizeof(sa), NULL, (LPTSTR)dst, &len))
					return dst;
				else
					return NULL;
			}
		case AF_INET6:
			{
				struct sockaddr_in6 sa;
				DWORD len = dstlen;
				sa.sin6_family = AF_INET6;
				memcpy (&sa.sin6_addr, src, sizeof(struct in6_addr));
				if (!WSAAddressToString ((LPSOCKADDR)&sa, (DWORD)sizeof(sa), NULL, (LPTSTR)dst, &len))
					return dst;
				else
					return NULL;
			}
		}
		return NULL;
	}
};

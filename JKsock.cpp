#include "JKsock.h"

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>  //inet_itoa
#include <netinet/in.h> //sockaddr_in
#include <netinet/tcp.h>
#include <netdb.h>
#define closesocket close
#endif

#include <cstring> /// memset
#include <string>
#include <stdexcept>

#define JKSOCK_DEBUG 1 
#ifdef JKSOCK_DEBUG
#pragma message("JKSock Debug mode compiled in")
#include <cstdio>
#define JKsocklog(fmt,...) fprintf(stdout,"JKock: " fmt,##__VA_ARGS__)
#else
#define JKsocklog(fmt,...)
#endif

#ifdef _WIN32
#pragma comment(lib, "ws2_32.lib") 
#endif



class _init_winsock2_2_class
{
public:
	_init_winsock2_2_class()
	{
		/// Windows Platform need WinSock2.DLL initialization.
#ifdef _WIN32
		WORD wd;
		WSAData wdt;
		wd = MAKEWORD(2, 2);
		int ret = WSAStartup(wd, &wdt);

		JKsocklog("WSAStartup() Returns: %d\n", ret);

		if (ret<0)
		{
			JKsocklog("WSAGetLastError: %d\n", WSAGetLastError());
			throw std::runtime_error("Unable to load winsock2.dll. ");
		}
#endif
	}
	~_init_winsock2_2_class()
	{
		/// Windows Platform need WinSock2.DLL clean up.
#ifdef _WIN32
		WSACleanup();
		JKsocklog("WSACleanup() called.");
#endif
	}
} _init_winsock2_2_obj;//comment by jidzh@2018/04/27,del it and defined by user?


JKsock::JKsock():m_socket(-1),m_af(AF_INET)
{
	//讲道理socket pf ，addrin af
	m_socket = socket(m_af, SOCK_STREAM, 0);
	if(m_socket == -1)//
	{
		throw  std::runtime_error("Unable to Create Socket. ");
	}
	JKsocklog("socket =%d func =%s\n", m_socket,__func__);
}
JKsock::JKsock(int af,int typre,int protocl):m_socket(-1),m_af(af)
{
	m_socket = socket(af, typre, protocl);
	JKsocklog("sock::~sock() %p\n", this);
	if(m_socket == -1)//
	{
		throw  std::runtime_error("Unable to Create Socket. ");
	}
}

JKsock::~JKsock()
{
	JKsocklog("sock::~sock() %p\n", this);

	//delete by 
	if (m_socket != -1)
	{
		JKsocklog("Socket closed: [%d] in %p\n",m_socket, this);
		closesocket(m_socket);
		m_socket = -1;
	}
}

bool JKsock::getlocal(std::string& IpStr,unsigned int& port)
{
	//在AF_INET下，sockaddr_in和sockaddr 内存布局一致
	struct sockaddr_in addrin;
	socklen_t addrlen =sizeof(addrin);

	if(0 != getsockname(m_socket,(struct sockaddr*)&addrin,&addrlen))
	{
		return false;
	}

	//af =AF_INET6
	//const char *inet_ntop(int af, const void *src, char *dst, socklen_t cnt);
	IpStr = inet_ntoa(addrin.sin_addr);
	port  = addrin.sin_port;
	return true;
}
bool JKsock::getpeer(std::string& IpStr,unsigned int& port)
{
	//在AF_INET下，sockaddr_in和sockaddr 内存布局一致
	struct sockaddr_in addrin;
	socklen_t addrlen =sizeof(addrin);

	if(0 != getpeername(m_socket,(struct sockaddr*)&addrin,&addrlen))
	{
		return false;
	}

	//af =AF_INET6
	//const char *inet_ntop(int af, const void *src, char *dst, socklen_t cnt);
	IpStr = inet_ntoa(addrin.sin_addr);
	port  = addrin.sin_port;
	return true;
}
int JKsock::bind(std::string host, unsigned short port)
{
	JKsocklog("%s\n", __func__);
	sockaddr_in saddr;

	unsigned long host_IP = 0;//INADDR_ANY
	if ((host_IP = inet_addr(host.c_str())) == INADDR_NONE)
	{
		/* 转换失败，表明是主机名,需通过主机名获取ip */
		DNSParse(host, host_IP);
		//memmove(&dest_addr.sin_addr, pHost->h_addr_list[0], pHost->h_length);  
	}
	memset(&saddr, 0, sizeof(saddr));
	saddr.sin_addr.s_addr = host_IP;
	saddr.sin_port = htons(port);
	saddr.sin_family = AF_INET;
	return ::bind(m_socket, (sockaddr*)&saddr, sizeof(saddr));
}

int JKsock::Connect(const std::string ServerName, const unsigned short port)
{
	JKsocklog("%s\n", __func__);
	unsigned long _out_IP = 0;//INADDR_ANY
	if ((_out_IP = inet_addr(ServerName.c_str())) == INADDR_NONE)
	{
		/* 转换失败，表明是主机名,需通过主机名获取ip */
		DNSParse(ServerName, _out_IP);
		//memmove(&dest_addr.sin_addr, pHost->h_addr_list[0], pHost->h_length);  
	}
	sockaddr_in sa;
	sa.sin_family = AF_INET;
	sa.sin_port = htons(port);
	sa.sin_addr.s_addr = _out_IP;

	if (0 != connect(m_socket, (sockaddr *)&sa, sizeof(sockaddr_in)))
	{
		//int ret = WSAGetLastError();
		JKsocklog("%s eeror\n", __func__);
		return false;
	}
	std::string strIP;
	unsigned int nport;
	getpeer(strIP,nport);
	JKsocklog("connect success peer =%s:%d\n",strIP.c_str(),nport);
	return true;
}

int JKsock::send(const void * Buffer, int Length)
{
	return ::send(m_socket, (const char*)Buffer, Length, 0);;
}

int JKsock::recv(void * Buffer, int MaxToRecv)
{
	return ::recv(m_socket, (char*)Buffer, MaxToRecv, 0);
}

int JKsock::sendto(const std::string& IPStr, unsigned short Port,const void* Buffer, int Length)
{
	return ::sendto(m_socket, (const char*)Buffer, Length, 0, (const sockaddr*)paddr, addrsz);
}
int JKsock::recvfrom(std::string& fromIP, unsigned short fromPort,void* Buffer, int MaxToRecvs)
{
	return ::recvfrom(m_socket, (char*)Buffer, Length, 0, (sockaddr*)&saddr, &saddrlen);
}
int JKsock::getsendtime(int & _out_Second, int & _out_uSecond)
{
	struct timeval outtime;
	socklen_t _not_used_t;
	int ret = getsockopt(m_socket, SOL_SOCKET, SO_SNDTIMEO, (char*)&outtime, &_not_used_t);
	if (ret != 0) return ret;
	/// We don't know why, but on Windows, 1 Second means 1000.
#ifdef _WIN32
	_out_Second = outtime.tv_sec / 1000;
	_out_uSecond = outtime.tv_usec;
#else
	_out_Second = outtime.tv_sec;
	_out_uSecond = outtime.tv_usec;
#endif
	return ret;
}

int JKsock::getrecvtime(int & _out_Second, int & _out_uSecond)
{
	struct timeval outtime;
	socklen_t _not_used_t;
	int ret = getsockopt(m_socket, SOL_SOCKET, SO_RCVTIMEO, (char*)&outtime, &_not_used_t);
	if (ret !=0) return ret;
	/// We don't know why, but on Windows, 1 Second means 1000.
#ifdef _WIN32
	_out_Second = outtime.tv_sec / 1000;
	_out_uSecond = outtime.tv_usec;
#else
	_out_Second = outtime.tv_sec;
	_out_uSecond = outtime.tv_usec;
#endif

	return ret;
}

int JKsock::setsendtime(int Second)
{
	struct timeval outtime;
	/// We don't know why, but on Windows, 1 Second means 1000.
#ifdef _WIN32
	outtime.tv_sec = Second * 1000;
	outtime.tv_usec = 0;
#else
	outtime.tv_sec = Second;
	outtime.tv_usec = 0;
#endif

	return setsockopt(m_socket, SOL_SOCKET, SO_SNDTIMEO, (const char*)&outtime, sizeof(outtime));
	return 0;
}

int JKsock::setrecvtime(int Second)
{
	struct timeval outtime;
	/// We don't know why, but on Windows, 1 Second means 1000.
#ifdef _WIN32
	outtime.tv_sec = Second * 1000;
	outtime.tv_usec = 0;
#else
	outtime.tv_sec = Second;
	outtime.tv_usec = 0;
#endif
	return setsockopt(m_socket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&outtime, sizeof(outtime));
}

int DNSParse(const std::string & HostName, unsigned long& _out_IP)
{
	/// Use getaddrinfo instead
	struct addrinfo hints;
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;

	//struct addrinfo* result = nullptr;
	struct addrinfo* result = 0;

	int ret = getaddrinfo(HostName.c_str(), NULL, &hints, &result);
	if (ret != 0)
	{
		return -1;/// API Call Failed.
	}
	for (struct addrinfo* ptr = result; ptr != NULL; ptr = ptr->ai_next)
	{
		switch (ptr->ai_family)
		{
		case AF_INET:
			sockaddr_in * addr = (struct sockaddr_in*) (ptr->ai_addr);
			_out_IP = addr->sin_addr.s_addr;
			freeaddrinfo(result);
			return 0;
			break;
		}
	}
	/// Unknown error.
	freeaddrinfo(result);
	return -2;
}

JKServer::JKServer()
{
	
}
JKServer::~JKServer()
{

}
int JKServer::bind(unsigned short port)
{
	JKsocklog("%s\n", __func__);
	sockaddr_in saddr;
	memset(&saddr, 0, sizeof(saddr));
	saddr.sin_addr.s_addr = 0;
	saddr.sin_port = htons(port);
	saddr.sin_family = AF_INET;
	return ::bind(m_socket, (sockaddr*)&saddr, sizeof(saddr));
}
int JKServer::bind(std::string host, unsigned short port)
{
	JKsocklog("%s\n", __func__);
	sockaddr_in saddr;

	unsigned long host_IP = 0;//INADDR_ANY
	if ((host_IP = inet_addr(host.c_str())) == INADDR_NONE)
	{
		/* 转换失败，表明是主机名,需通过主机名获取ip */
		DNSParse(host, host_IP);
		//memmove(&dest_addr.sin_addr, pHost->h_addr_list[0], pHost->h_length);  
	}
	memset(&saddr, 0, sizeof(saddr));
	saddr.sin_addr.s_addr = host_IP;
	saddr.sin_port = htons(port);
	saddr.sin_family = AF_INET;
	return ::bind(m_socket, (sockaddr*)&saddr, sizeof(saddr));
}
int JKServer::listen(int MaxCount)
{
	JKsocklog("%s\n", __func__);
	return::listen(m_socket, MaxCount);
}

JKsock* JKServer::accept()
{
	JKsocklog("%s\n", __func__);
	sockaddr_in saddr;
	socklen_t saddrsz = sizeof(saddr);
	int ret = ::accept(m_socket, (sockaddr*)(&saddr), &saddrsz);

	JKsock *pJKsock =NULL;
	if(ret == -1) 
	{
		JKsocklog("%s error\n", __func__);
	}
	else
	{
		pJKsock=  new JKsock(ret);
		std::string strIP;
		unsigned int nport;
		getpeer(strIP,nport);
		JKsocklog("accept success peer =%s:%d\n",strIP.c_str(),nport);
	}
	return pJKsock;
	
}

JKClient::JKClient()
{

}

JKClient::~JKClient()
{

}


bool JKClient::Connect(const std::string ServerName, const unsigned int port)
{
	JKsocklog("%s\n", __func__);
	unsigned long _out_IP = 0;//INADDR_ANY
	if ((_out_IP = inet_addr(ServerName.c_str())) == INADDR_NONE)
	{
		/* 转换失败，表明是主机名,需通过主机名获取ip */
		DNSParse(ServerName, _out_IP);
		//memmove(&dest_addr.sin_addr, pHost->h_addr_list[0], pHost->h_length);  
	}
	sockaddr_in sa;
	sa.sin_family = AF_INET;
	sa.sin_port = htons(port);
	sa.sin_addr.s_addr = _out_IP;

	if (0 != connect(m_socket, (sockaddr *)&sa, sizeof(sockaddr_in)))
	{
		//int ret = WSAGetLastError();
		JKsocklog("%s eeror\n", __func__);
		return false;
	}
	std::string strIP;
	unsigned int nport;
	getpeer(strIP,nport);
	JKsocklog("connect success peer =%s:%d\n",strIP.c_str(),nport);
	return true;
}
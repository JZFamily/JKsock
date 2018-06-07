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
#include <memory.h>
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

//add by jzf@2018/06/05,copy from libevent to resolve ipv6
//comment by jzf@2018/06/05 ,it has define in socket lib
//struct sockaddr_storage {
//	union {
//		struct sockaddr ss_sa;
//		struct sockaddr_in ss_sin;
//		struct sockaddr_in6 ss_sin6;
//		char ss_padding[128];
//	} ss_union;
//};


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
	JKsocklog("sock::sock() %p\n", this);
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

bool JKsock::getlocal(std::string& IpStr, unsigned short& port)
{
	//在AF_INET下，sockaddr_in和sockaddr 内存布局一致
	struct sockaddr_storage ss;
	socklen_t sslen =sizeof(ss);

	memset(&ss, 0, sizeof(sockaddr_storage));
	if(0 != getsockname(m_socket,(struct sockaddr*)&ss,&sslen))
	{
		return false;
	}

	//af =AF_INET6,if error occus ,ntop return null
	//const char *inet_ntop(int af, const void *src, char *dst, socklen_t cnt);
	//IpStr = inet_ntoa(addrin.sin_addr);
	if(!getnameByaddrin(IpStr,port,&ss))
	{
		return false;
	}
	return true;
}
bool JKsock::getpeer(std::string& IpStr,unsigned short& port)
{
	//在AF_INET下，sockaddr_in和sockaddr 内存布局一致
	//struct sockaddr_in addrin;
	//socklen_t addrlen =sizeof(addrin);

	sockaddr_storage ss;
	socklen_t sslen =sizeof(ss);

	memset(&ss, 0, sizeof(sockaddr_storage));
	if(0 != getpeername(m_socket,(struct sockaddr*)&ss,&sslen))
	{
		JKsocklog("getpeername error %s\n", __func__);
		return false;
	}
/*
	//af =AF_INET6
	//const char *inet_ntop(int af, const void *src, char *dst, socklen_t cnt);
	IpStr = inet_ntoa(addrin.sin_addr);
	port  = addrin.sin_port;
	
*/
	if(!getnameByaddrin(IpStr,port,&ss))
	{
		return false;
	}
	return true;
}

bool  JKsock::getnameByaddrin(std::string& _out_IpStr, unsigned short& _out_Port,const void *_in_pAddrin)
{
	if (m_af == AF_INET)
	{
		char dst[INET_ADDRSTRLEN] = { 0 };
		struct sockaddr_in* sin = (struct sockaddr_in*)_in_pAddrin;
		if (NULL == inet_ntop(m_af, &(sin->sin_addr), dst, sizeof(dst)))
		{
			JKsocklog("inet_ntop error %s\n",__func__);
			return false;
		}
		_out_IpStr = dst;

		_out_Port = ntohs(sin->sin_port);
	}
	else if (m_af == AF_INET6)
	{
		char dst[INET6_ADDRSTRLEN] = { 0 };
		struct sockaddr_in6* sin6 = (struct sockaddr_in6*)_in_pAddrin;
		if (NULL == inet_ntop(m_af, &(sin6->sin6_addr), dst, sizeof(dst)))
		{
			JKsocklog("inet_ntop error %s\n", __func__);
			return false;
		}
		_out_IpStr = dst;
		_out_Port = ntohs(sin6->sin6_port);
	}
	return true;
}
bool JKsock::getsockaddrinByname(const std::string _in_HostName, const unsigned short _in_port, \
						const void*_inOut_dstaddrin)
{
	//success 1, r 0, fail -1
	int ret = -1;

	if(m_af == AF_INET)
	{
		struct sockaddr_in *sin = (struct sockaddr_in *)_inOut_dstaddrin;
		sin->sin_family =AF_INET;
		sin->sin_port = htons(_in_port);
		ret =inet_pton(m_af, _in_HostName.c_str(),&(sin->sin_addr));
	}
	else if(m_af ==AF_INET6)
	{
		struct sockaddr_in6 *sin6 = (struct sockaddr_in6 *)_inOut_dstaddrin;
		sin6->sin6_family =AF_INET6;
		sin6->sin6_port = htons(_in_port);
		ret = inet_pton(m_af, _in_HostName.c_str(), &(sin6->sin6_addr));//in6addr_any
	}

	if(1==ret)
	{
		return true;
	}
	else
	{
		if(!DNSParse(_in_HostName,(const sockaddr_storage *)_inOut_dstaddrin))
		{
			return false;
		}
	}
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
int JKsock::set_reuse()
{
	int on = 1;
	return setsockopt(m_socket, SOL_SOCKET, SO_REUSEADDR,(const char *)&on,sizeof(on));
}
int JKsock::bind(unsigned short port)
{
	JKsocklog("%s\n", __func__);
	//sockaddr_in saddr;
	sockaddr_storage ss;
	memset(&ss, 0, sizeof(sockaddr_storage));

	socklen_t sslen = sizeof(ss);

	if (m_af == AF_INET)
	{
		struct sockaddr_in * sin = (sockaddr_in *)&ss;
		sin->sin_family = AF_INET;
		sin->sin_addr.s_addr = INADDR_ANY;
		sin->sin_port = htons(port);
	}
	else if (m_af == AF_INET6)
	{
		struct sockaddr_in6 * sin6 = (sockaddr_in6 *)&ss;
		sin6->sin6_family = AF_INET6;
		sin6->sin6_addr = in6addr_any;
		sin6->sin6_port = htons(port);
	}
	return ::bind(m_socket, (sockaddr*)&ss, sizeof(ss));
}
int JKsock::bind(std::string host, unsigned short port)
{
	JKsocklog("%s\n", __func__);
	//sockaddr_in saddr;
	sockaddr_storage ss;
	memset(&ss, 0, sizeof(sockaddr_storage));
	
	socklen_t sslen =sizeof(ss);
	
	getsockaddrinByname(host,port,&ss);
/*
	if ((host_IP = inet_addr(host.c_str())) == INADDR_NONE)
	{
		//转换失败，表明是主机名,需通过主机名获取ip
		DNSParse(host, host_IP);
		//memmove(&dest_addr.sin_addr, pHost->h_addr_list[0], pHost->h_length);  
	}
	memset(&saddr, 0, sizeof(saddr));
	saddr.sin_addr.s_addr = host_IP;
	saddr.sin_port = htons(port);
	saddr.sin_family = AF_INET;
*/
	return ::bind(m_socket, (sockaddr*)&ss, sizeof(ss));
}

int JKsock::Connect(const std::string ServerName, const unsigned short port)
{
	JKsocklog("%s\n", __func__);
	sockaddr_storage ss;
	memset(&ss, 0, sizeof(sockaddr_storage));
	
	socklen_t sslen =sizeof(ss);
	getsockaddrinByname(ServerName,port,&ss);
	if (0 != connect(m_socket, (sockaddr *)&ss, sizeof(ss)))
	{
		//int ret = WSAGetLastError();
		JKsocklog("%s eeror\n", __func__);
		return false;
	}
	std::string strIP;
	unsigned short nport;
	getpeer(strIP,nport);
	JKsocklog("connect success peer =%s:%u\n",strIP.c_str(),nport);
	getlocal(strIP, nport);
	JKsocklog("connect success local =%s:%u\n", strIP.c_str(), nport);
	return true;
}

int JKsock::send(const void * Buffer, int Length)
{
	return ::send(m_socket, (const char*)Buffer, Length, 0);
}

int JKsock::recv(void * Buffer, int MaxToRecv)
{
	int ret =::recv(m_socket, (char*)Buffer, MaxToRecv, 0);
#ifdef JKSOCK_DEBUG
	std::string strIP;
	unsigned short nport;
	getpeer(strIP,nport);
	JKsocklog("func =%s peer =%s:%u\n",__func__, strIP.c_str(), nport);
#endif
	return  ret;
}

int JKsock::sendto(const std::string& IPStr,const unsigned short& Port,const void* Buffer, int Length)
{
	sockaddr_storage ss;
	socklen_t sslen =sizeof(ss);
	memset(&ss, 0, sizeof(sockaddr_storage));

	getsockaddrinByname(IPStr,Port,&ss);
	return ::sendto(m_socket, (const char*)Buffer, Length, 0, (const sockaddr*)&ss, sslen);
}

//int JKsock::recvfrom(const unsigned short fromPort, void* Buffer, int Length)
//{
//	JKsocklog("%s\n", __func__);
//	sockaddr_storage ss;
//	memset(&ss, 0, sizeof(sockaddr_storage));
//
//	socklen_t sslen = sizeof(ss);
//
//	if (m_af == AF_INET)
//	{
//		struct sockaddr_in * sin = (sockaddr_in *)&ss;
//		sin->sin_family = AF_INET;
//		sin->sin_addr.s_addr = INADDR_ANY;
//		sin->sin_port = htons(fromPort);
//	}
//	else if (m_af == AF_INET6)
//	{
//		struct sockaddr_in6 * sin6 = (sockaddr_in6 *)&ss;
//		sin6->sin6_family = AF_INET6;
//		sin6->sin6_addr = in6addr_any;
//		sin6->sin6_port = htons(fromPort);
//	}
//	
//	int ret =::recvfrom(m_socket, (char*)Buffer, Length, 0, (sockaddr*)&ss, &sslen);
//#ifdef JKSOCK_DEBUG
//	std::string strIP;
//	unsigned short nport;
//	getnameByaddrin(strIP,nport,&ss);
//	JKsocklog("func =%s peer =%s:%u\n",__func__, strIP.c_str(), nport);
//#endif
//	return  ret;
//}
int JKsock::recvfrom(const std::string& fromIP, unsigned short& fromPort,void* Buffer, int _in_MaxToRecvs)
{
	sockaddr_storage ss;
	socklen_t sslen =sizeof(ss);

	memset(&ss, 0, sizeof(sockaddr_storage));
	getsockaddrinByname(fromIP,fromPort,&ss);
	
	int ret=::recvfrom(m_socket, (char*)Buffer, _in_MaxToRecvs, 0, (sockaddr*)&ss, &sslen);
#ifdef JKSOCK_DEBUG
	std::string strIP;
	unsigned short nport;
	getnameByaddrin(strIP,nport,&ss);
	JKsocklog("func =%s peer =%s:%u\n",__func__, strIP.c_str(), nport);
#endif
	return ret;
}
int JKsock::recvfrom(struct sockaddr_storage& ss, const void* Buffer, int _in_MaxToRecvs)
{
	socklen_t sslen = sizeof(ss);

	int ret = ::recvfrom(m_socket, (char*)Buffer, _in_MaxToRecvs, 0, (sockaddr*)&ss, &sslen);
#ifdef JKSOCK_DEBUG
	std::string strIP;
	unsigned short nport;
	getnameByaddrin(strIP, nport, &ss);
	JKsocklog("func =%s peer =%s:%u\n", __func__, strIP.c_str(), nport);
#endif
	return ret;
}
int DNSParse(const std::string & HostName, const sockaddr_storage * ss)
{
	/// Use getaddrinfo instead
	struct addrinfo hints;
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = ((struct sockaddr*)ss) ->sa_family;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = 0;

	//struct addrinfo* result = nullptr;
	struct addrinfo* result = 0;

	int ret = getaddrinfo(HostName.c_str(), NULL, &hints, &result);
	if (ret != 0)
	{
		return -1;/// API Call Failed.
	}
	sockaddr_in * addr;
	sockaddr_in6 * addr6;
	struct  sockaddr_in6 *sin6;
	struct  sockaddr_in *sin;
	for (struct addrinfo* ptr = result; ptr != NULL; ptr = ptr->ai_next)
	{
		switch (ptr->ai_family)
		{
		case AF_INET:
			//sockaddr_in * 
				addr = (struct sockaddr_in*) (ptr->ai_addr);
			//struct  sockaddr_in *
				sin = (struct  sockaddr_in *)ss;
			memcpy((in_addr *)&sin->sin_addr, (in_addr *)& addr->sin_addr,sizeof(in_addr));
			//freeaddrinfo(result);
			//return 0;
			break;
		case AF_INET6:
			//sockaddr_in6 * 
				addr6 = (struct sockaddr_in6*) (ptr->ai_addr);
			//struct  sockaddr_in6 *
				sin6 = (struct  sockaddr_in6 *)ss;
			memcpy((in6_addr *)&sin6->sin6_addr, (in6_addr *)& addr6->sin6_addr,sizeof(in6_addr));

			break;
		}
	}
	/// Unknown error.
	freeaddrinfo(result);
	return 0;
	freeaddrinfo(result);
	return -2;
}

JKServer::JKServer()
{
	
}
JKServer::~JKServer()
{

}

int JKServer::listen(int MaxCount)
{
	JKsocklog("%s\n", __func__);
	return::listen(m_socket, MaxCount);
}

JKsock* JKServer::accept()
{
	JKsocklog("%s\n", __func__);
	sockaddr *saddr;
	socklen_t saddrsz;
	int ret = ::accept(m_socket, (sockaddr*)(&saddr), &saddrsz);

	JKsock *pJKsock =NULL;
	if(ret == -1) 
	{
		JKsocklog("%s error\n", __func__);
	}
	else
	{
		pJKsock=  new JKsock(ret,saddr->sa_family);

#ifdef JKSOCK_DEBUG

		std::string strIP;
		unsigned short nport;
		getpeer(strIP, nport);
		JKsocklog("accept success peer =%s:%d\n", strIP.c_str(), nport);
#endif //  JKDEBUG
	}
	return pJKsock;
	
}

JKClient::JKClient()
{

}

JKClient::~JKClient()
{

}

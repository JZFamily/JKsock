/*
create by JZFamily@2018/04/27,Modify the KiritoTRw gsock
note by JZFamily@2018/05/21,
	one socket is one link,try modify it
	顺便兼容下ipv6
note by JZFamily@2018/06/03,
	对于socket，
		linu下支持2-3层
		win是在3层基础上进行,2层要NDIS了
	所以，jksock只关心三层网络层的编程，物理层不刻意支持。
note by JZFamily@2018/06/04,
	一个服务器= 监听socket+与客户端连接的socket.so change the serversock design
*/
#pragma once
#include <string>
struct sockaddr_storage;
//DNS解析
int DNSParse(const std::string & HostName, const sockaddr_storage * ss);

class JKsock
{
public:
	JKsock();
	JKsock(int af,int typre,int protocl);
	JKsock(/*socket*/const int socket,const int af):m_socket(socket),m_af(af)   { }
	virtual ~JKsock();
public:
	JKsock( const JKsock&) =delete;
	JKsock& operator = (const JKsock&)  =delete;
	//operator int(){return m_socket;}
	//JKsock operator =(int value) { m_socket = value; }
protected:
	int	m_socket;
	//add by jzf@2018/06/05 
	const int  m_af;
public:

	//-----they should be called after connect or accpet
	//if success return 0
	bool getlocal(std::string& IpStr, unsigned short& port);
	bool getpeer(std::string& IpStr, unsigned short& port);
	
	bool getsockaddrinByname(const std::string _in_HostName, const unsigned short _in_port, \
						const void*_inOut_dstaddrin);
	bool getnameByaddrin(std::string& _out_IpStr, unsigned short& _out_Port,const void *_in_pAddrin);
	//int getsockname(int sockfd, struct sockaddr *localaddr, /*socklen_t*/ int *addrlen);  
	//int getpeername(int sockfd, struct sockaddr *peeraddr, /*socklen_t*/  int *addrlen);
	
//-----
	int getsendtime(int& _out_Second, int& _out_uSecond);
	int getrecvtime(int& _out_Second, int& _out_uSecond);
	
	//comment by jzf@2018/06/04 阻塞io，出现异常中断导致阻塞
	int setsendtime(int Second);
	int setrecvtime(int Second);
	
	//comment by jzf@2018/06/04, 1)after reset listen socket ,bind error. 
	//							 2)udp打洞
	int set_reuse();
	//add by jzf@2018/06/01,
	//void closesocket();
public:
	int bind(unsigned short port);
	int bind(std::string host, unsigned short port);
	int Connect(const std::string ServerName, const unsigned short port);
//--if server these function has some error
	virtual int send(const void* Buffer, int Length);
	virtual int recv(void* Buffer, int MaxToRecv);
	int sendto(const std::string& IPStr, const unsigned short& Port,
				const void* Buffer, int Length);
	int recvfrom(const std::string& fromIP, unsigned short& fromPort,
				void* Buffer, int MaxToRecvs);
	int recvfrom(const unsigned short fromPort, void* Buffer, int MaxToRecvs);
//----

};

class JKServer :public JKsock
{

public:
	JKServer();
	~JKServer();
public:
    int bind(unsigned short port);
	int listen(int MaxCount);
	JKsock *accept();
};

class JKClient:public JKsock
{
public:
	JKClient();
	~JKClient();
};
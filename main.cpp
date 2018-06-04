
#include"JKsock.h"
#include<thread>
#include<iostream>

void func(JKsock *pjksock)
{
	int ret;
	char recvbuf[1024] = {0};
	pjksock->setsendtime(30);
	while (true)
	{
		ret =pjksock->recv(recvbuf,sizeof(recvbuf));
		if (ret ==0)
		{
			std::cout << "remote socket close on recv\n";
			delete pjksock;
			break;
		}
		else if(ret == -1)
		{
			std::cout << "socket error on recv\n";
			delete pjksock;
			break;
		}
		else
		{
			std::cout << recvbuf << std::endl;
		}
		
		ret =pjksock->send(recvbuf,sizeof(recvbuf));
		if (ret == 0)
		{
			std::cout << "remote socket close on send\n";
			delete pjksock;
			break;
		}
		else if (ret == -1)
		{
			std::cout << "socket error on send\n";
			delete pjksock;
			break;
		}
		memset(recvbuf,0,1024);
	}
}
int main()
{
	JKServer server;
	server.bind(46667);
	server.listen(5);
	JKsock *pjksock;
	while (pjksock =server.accept())
	{
		 std::thread t(func, pjksock);
		t.detach();
	}
}

/* int main()
{
	JKClient client;
	char recvbuf[1024] = { 0 };
	std::string sendbuf;
	int ret;
	if (client.Connect("140.143.224.173", 46667))
	{
		while (true)
		{
			
			std::cin >> sendbuf;
			ret =client.send(sendbuf.c_str(), sendbuf.size());
					if (ret == 0)
					{
						std::cout << "remote socket close on send\n";
						// pjksock;
						break;
					}
					else if (ret == -1)
					{
						std::cout << "socket error on send\n";
						//delete pjksock;
						break;
					}
			client.recv(recvbuf, sizeof(recvbuf));
			if (ret == 0)
			{
				std::cout << "remote socket close on send\n";
				// pjksock;
				break;
			}
			else if (ret == -1)
			{
				std::cout << "socket error on send\n";
				//delete pjksock;
				break;
			}
			std::cout << recvbuf << std::endl;
			memset(recvbuf, 0, 1024);
		}

	}

} */
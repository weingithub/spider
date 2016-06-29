#include   "sockbase.h"
#include  <sys/socket.h>
#include <arpa/inet.h>
#include   <iostream>

using namespace std;

CSocket::CSocket()
{
	
}

CSocket::~CSocket()
{
	
}

int 
CSocket::CreateSockServerTcp(unsigned short uBindPort, int nNum)
{
	int sockserverfd;
	
	//�����׽���
	sockserverfd = socket(AF_INET,(int)SOCK_STREAM,0);
	
	if (-1 == sockserverfd)
    {
        return sockserverfd;
    }
	
	//�����׽��ֳɹ�,�������׽��ֵ�ַ�ṹ
	sockaddr_in servaddr;
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = INADDR_ANY;
	servaddr.sin_port = htons(uBindPort);
	
	//���׽���
	int nRet = bind(sockserverfd, (sockaddr *)&servaddr, static_cast<socklen_t>(sizeof (servaddr)));
	
	if (-1 == nRet)
	{
		close(sockserverfd);
		sockserverfd = -1;
		return sockserverfd;
	}	
	
	//���׽��ֳɹ�,���ڶ��׽������ü���
	nRet = listen(sockserverfd, nNum);
	
	if( -1 == nRet)
	{
		close(sockserverfd);
        sockserverfd = -1;
		return sockserverfd;
	}
	
	return sockserverfd;
}
	
int 
CSocket::ConnectSockClientTcp(string strServIP, unsigned short uBindPort )
{
	int sockclientfd;
	
	//�����׽���
	sockclientfd = socket(AF_INET,(int)SOCK_STREAM,0);
	
	if (-1 == sockclientfd)
    {
        return sockclientfd;
    }
	
	//�����׽��ֳɹ�,�������׽��ֵ�ַ�ṹ
	sockaddr_in servaddr;
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = inet_addr(strServIP.c_str());
	servaddr.sin_port = htons(uBindPort);
	
	int nRet = connect(sockclientfd, (sockaddr *)&servaddr, sizeof(servaddr));
	if( -1 == nRet)
	{
        close(sockclientfd);
		sockclientfd = -1;
        return sockclientfd;
	}
	
	return sockclientfd;
}

int 
CSocket::SmallToBigEndian(unsigned char *pData, unsigned int uDataLen)
{
	unsigned char *pStart = pData;
    unsigned char *pEnd   = pData + uDataLen - 1;
    unsigned char cTmp;
    
    while(pEnd > pStart)
    {
        cTmp    = *pStart;
        *pStart = *pEnd;
        *pEnd   = cTmp;

        ++pStart;
        --pEnd;
    }
	
	return 0;
}

int 
CSocket::BigToSmallEndian(unsigned char *pData, unsigned int uDataLen)
{
	return SmallToBigEndian(pData, uDataLen);
}


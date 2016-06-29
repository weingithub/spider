#ifndef  _SOCK_BASE_H_
#define  _SOCK_BASE_H_

#include <string>

#define SERVER_PORT 50000

class CSocket
{
public:
	CSocket();
	~CSocket();	

	typedef struct stThreadParam
	{
		void * pParam;
		int   sockfd;
	}TThreadParam, *LPThreadParam;

	int CreateSockServerTcp(unsigned short uBindPort = 10086, int nNum = 10);
	
	int ConnectSockClientTcp(std::string strServIP, unsigned short  uBindPort = 10086);
	
	int SmallToBigEndian(unsigned char *pData, unsigned int uDataLen);
	
	int BigToSmallEndian(unsigned char *pData, unsigned int uDataLen);
};

#endif


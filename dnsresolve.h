#ifndef _DNS_RESOLVE_HEADER_H_
#define  _DNS_RESOLVE_HEADER_H_

#include <map>
#include <vector>
#include <string.h>
#include "appdefine.h"

#define PRINT(n) cout<<#n<<" ="<<(int)n<<endl

#pragma pack (push)
#pragma pack(1) // 按照1字节方式进行对齐

struct DNSHeader
{
	uint16_t  id;
    //网络字节序中，高位在低地址，因此，位域中，设计时，在同一个字节内的字段要进行翻转
	uint8_t rd:1;
	uint8_t tc:1;
	uint8_t aa:1;
	uint8_t opcode:4;
	uint8_t qr:1;

	//使用位域时，涉及大小端问题，怎么解决?  翻转
	uint8_t rcode:4;	
	uint8_t zero:3;
	uint8_t ra:1;
    
	uint16_t qcount;
	uint16_t rcount;
	uint16_t arcount;
	uint16_t ercount;
    
    bool print();
};

typedef struct DNSDomainName
{
    uint8_t length;
    char content[1];
}*LPDomainName;

typedef struct DNSQuestion
{
    uint16_t qtype;
    uint16_t qclass;
}*LPQuestion;

typedef struct DNSResource
{
    uint16_t rtype;
    uint16_t rclass;
    uint32_t ttl;
    uint16_t length;
    char   content[1];
}*LPResource;

#pragma pack(pop)    //恢复对齐状态

using namespace std;

//是否和内存对齐有关系

class DNSResolve
{
public:	
	DNSResolve();
	~DNSResolve();
	
	enum
	{
        QCLASS_IN = 1,
		QUERY_TYPE_A = 1,
		QUERY_TYPE_NS = 2,
		QUERY_TYPE_CNAME = 5,
        QUERY_TYPE_AAAA = 28,
        QUERY_TYPE_ANY = 255,
	};
    
    enum
    {
        START_WITH_POINTER_ENDPOINTER = 1,   //指针开头，指针结尾
        START_WITH_CHAR_ENDPOINTER = 2,   //字符开头，指针结尾
        START_WITH_CHAR_ENDCHAR,          //字符开头，字符结尾     
    };
    
    typedef struct DNSHostInfo
    {
        string host;
        string ip;
        
        uint16_t type;
        uint16_t qclass;
        uint32_t ttl;
        
        DNSHostInfo():
            type(0),
            qclass(0),
            ttl(0)
        {
        }
        
        ~DNSHostInfo()
        {
           
        }
    }TDNSHostInfo, *LPDNSHostInfo;
    
    int Print();
	
	int Resolve(string url, string & ip);
	
	int ParseDNSServer();
	
	int BuildRequest(string url, uint16_t querytype, uint8_t * pdata);
	
	int SendReq(string url, string &ip);
	
	int ParseResponse(uint8_t * pdata, vector<string> & vctIP);
   
    int ParseRecord(uint8_t *pdata, uint32_t & offset, bool isQuestion = false);
    
    int ParseName(uint8_t * pdata, uint32_t & offset, string & name);
private:
	int SmallToBigEndian(uint8_t *pData, uint32_t uDataLen);
	int BigToSmallEndian(uint8_t *pData, uint32_t uDataLen);	
    int Init();  //从文件中读取数据到map对象中
	int Output();  

private:
    vector<LPDNSHostInfo>  m_HostInfo;   
	vector<string>	m_dnsserver; 
	map<string, string> m_url_to_ip_old;
    map<string, string> m_url_to_ip;
    
    uint16_t    m_identifyid;
};


#endif


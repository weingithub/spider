#include <iostream>
#include <fstream>
#include <ctime>
#include <cstdlib>
#include <sys/socket.h>
#include <errno.h>
#include <arpa/inet.h>

#include "dnsresolve.h"
#include "common.h"

using namespace std;

#define QR_TYPE_QUERY (0)
#define QR_TYPE_RESPNSE (1)

bool DNSHeader::print()
{
    cout<<"打印DNS报文头"<<endl;
    PRINT(id);
    //网络字节序中，高位在低地址，因此，位域中，设计时，在同一个字节内的字段要进行翻转
	PRINT(rd);
	PRINT(tc);
	PRINT(aa);
	PRINT(opcode);
	PRINT(qr);

	//使用位域时，涉及大小端问题，怎么解决?  翻转
	PRINT(rcode);	
	PRINT(zero);
	PRINT(ra);
    
	PRINT(qcount);
	PRINT(rcount);
	PRINT(arcount);
	PRINT(ercount);
}

DNSResolve::DNSResolve()
{
	ParseDNSServer();

	Init();
}

DNSResolve::~DNSResolve()
{
	cout<<"进行析构"<<endl;
	Output();
    
    LPDNSHostInfo pdnsInfo = NULL;
    
    for(vector<LPDNSHostInfo>::iterator iter = m_HostInfo.begin(); iter != m_HostInfo.end(); ++iter)
    {
        pdnsInfo = (LPDNSHostInfo)*iter;
        
        delete pdnsInfo;
    }
    
}

int DNSResolve::Print()
{
   return 0;
}

int DNSResolve::Init()
{
    //read data from file
	cout<<"Init start"<<endl;
    ifstream inputfile;
    inputfile.open(DNSIPFILE, ios::in);  //打开文件
    
	if (NULL == inputfile)
	{
		cerr<<"open file failed. reason:"<<strerror(errno)<<endl;
		return 1;
	}
	
    string dnsrecord;
    
    string dns, ip;
    
    while(!inputfile.eof())
    {
        getline(inputfile, dnsrecord); 
        //cout<<"what fuck"<<endl;
        if (dnsrecord.empty() || dnsrecord[0] == '\n')
        {
            continue;
        }

        //去除头尾的空格符
        Common::Str_trip(dnsrecord);
        
        int pos = dnsrecord.find('=');
        
        if (string::npos == pos)
        {
            continue;
        }
        
        dns = dnsrecord.substr(0, pos);   
        ip = dnsrecord.substr(pos + 1);
        
        m_url_to_ip_old.insert(make_pair(dns, ip) );
    }
    
    inputfile.close();
    
	cout<<"Init over"<<endl;
    return R_SUCCESS;
}

int DNSResolve::Output()
{
    //write data to file
	cout<<"Output start"<<endl;
    ofstream outputfile;
    outputfile.open(DNSIPFILE, ios::app);  //打开文件
    
    for (map<string, string>::iterator siter = m_url_to_ip.begin(); siter != m_url_to_ip.end(); ++siter)
    {
        outputfile<<siter->first<<"="<<siter->second <<endl;
    }
    
    outputfile.close();
    
	cout<<"Output over"<<endl;
	
    return R_SUCCESS;
}

int DNSResolve::ParseDNSServer()
{
	//打开/etc/resolv.conf 文件
	const char * serverpath = "/etc/resolv.conf";
	
	ifstream inputfile(serverpath, ios::in);
	
	if (!inputfile.is_open() )
	{
		m_dnsserver.push_back("8.8.8.8");
		inputfile.close();
		return 0;
	}
	
	string strdnsip;
	
	while (!inputfile.eof() )
	{
        strdnsip.clear();
		getline(inputfile, strdnsip);
		
		if (strdnsip.empty() || strdnsip[0] == '#'  || strdnsip[0] == '\n')  //跳过注释和空行
		{
			continue;
		}
		
		if (string::npos == strdnsip.find("nameserver"))  //跳过不含nameserver的行
		{
			continue; 
		}
		
		int pos = strdnsip.find(' ');
		
		if (string::npos == pos)  //跳过不符合要求的行
		{
			continue;
		}
		
		strdnsip = strdnsip.substr(pos + 1);
		m_dnsserver.push_back(strdnsip);
	}
	
	if (m_dnsserver.empty() )
	{
		m_dnsserver.push_back("8.8.8.8");
	}
	
    cout<<"ParseDNSServer.集合的长度是:"<<m_dnsserver.size()<<endl;
	inputfile.close();

	return 0;
}

int DNSResolve::Resolve(string url, string & ip)
{
    //栈被破坏
	map<string, string>::iterator siter;

	if (m_url_to_ip_old.end() != (siter = m_url_to_ip_old.find(url) ) )
	{
		ip = siter->second;
		
		return 0;
	}

	//发送报文
    ip.clear();
    
	SendReq(url, ip);
    
    if (ip.empty())
    {
        cerr<<"DNS解析失败"<<endl;
        return 1;
    }
    else
    {
        cout<<"DNS解析成功:\n域名\t"<<url<<"\nIP\t"<<ip<<endl;
    }
	
	return 0;
}

int DNSResolve::SendReq(string url, string &ip)
{
    //先组装DNS报文
	uint8_t szBuf[1000] = {0};
	
	uint16_t querytype = QUERY_TYPE_A;
    
	int len = BuildRequest(url, querytype, szBuf);
	
    if (-1 == len)
    {
        return -1;
    }

	int sockfd;
    
    //判断dns地址是ipv4还是ipv6地址
    string dnsserver = m_dnsserver[0];
    //string dnsserver = "121.22.88.3";
    //string dnsserver = "2001:470:0:45::2";
    sockaddr * addr;
    struct sockaddr_in servaddr;
    struct sockaddr_in6 servaddr6;
    
    int addrlen = 0;
    
    if (string::npos == dnsserver.find(':'))  //ipv4
    {
	
        sockfd = socket(AF_INET, SOCK_DGRAM, 0);
        
        if (sockfd < 0)
        {
            cerr<<"create socket fail"<<endl;
            return 1;    
        }
        
        bzero(&servaddr, sizeof(servaddr) );
        
        servaddr.sin_family = AF_INET;
        
        uint16_t port = 53;
        
        servaddr.sin_port = htons(port);
        
        int res = inet_pton(AF_INET, dnsserver.c_str(), &servaddr.sin_addr);
        
        if (res < 0)
        {
            cerr<<"[convert ip addr from text to binary from] fail"<<endl;
            return 1;    
        }
        
        addr = (sockaddr *)&servaddr;
        addrlen = sizeof(servaddr);
    }
    else
    {
        sockfd = socket(AF_INET6, SOCK_DGRAM, 0);
        
        if (sockfd < 0)
        {
            cerr<<"create socket fail"<<endl;
            return 1;    
        }
        
        bzero(&servaddr6, sizeof(servaddr6) );
        
        servaddr6.sin6_family  = AF_INET6;
        
        uint16_t port = 53;
        
        servaddr6.sin6_port = htons(port);
        
        int res = inet_pton(AF_INET6, dnsserver.c_str(), &servaddr6.sin6_addr);

        if (res < 0)
        {
            cerr<<"[convert ip addr from text to binary from] fail"<<endl;
            return 1;    
        }
        
        addr = (sockaddr *)&servaddr6;
        addrlen = sizeof(servaddr6);
    }

    cout<<"套接字描述符是:"<<sockfd<<endl;
	int nlen = sendto(sockfd, szBuf, len, 0, addr, addrlen);
	
	uint8_t recvline[1000] = {0};
	
	if (-1 == nlen)
	{
		cerr<<"发送数据失败"<<"原因,"<<strerror(errno)<<endl;
		return 1;
	}
	else
	{
		cout<<"发送数据成功，长度是:"<<nlen<<endl;
	}
	
	//等待接收数据
	
	struct timeval tv_out;
    tv_out.tv_sec = 30;//等待30秒
    tv_out.tv_usec = 0;
	
	setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO,&tv_out, sizeof(tv_out));

	nlen = recvfrom(sockfd, (char *)recvline, 1000, 0, NULL, NULL);
	
	if (-1 == nlen)
	{
		cerr<<"接收失败，超时"<<endl;
	}
	else
	{
		cout<<"数据接收完毕"<<",接收长度是:"<<nlen<<endl;
	}
	
	recvline[nlen] = 0;
    
    //解析响应
    vector<string> vctIP;
    ParseResponse(recvline, vctIP);

    if (0 == vctIP.size())
    {
        return 1;    
    }
    
    ip = vctIP[0];
    m_url_to_ip.insert(make_pair(url, vctIP[0]));
    
	return 0;
}


int DNSResolve::ParseResponse(uint8_t * pdata, std::vector<std::string> &vctIP)
{
    DNSHeader * pheader = (DNSHeader *)pdata;
    
    //大小端转换
	BigToSmallEndian((unsigned char *)&pheader->id, sizeof(pheader->id) );  
	BigToSmallEndian((unsigned char *)&pheader->qcount, sizeof(pheader->qcount) );
    BigToSmallEndian((unsigned char *)&pheader->rcount, sizeof(pheader->rcount) );
    BigToSmallEndian((unsigned char *)&pheader->arcount, sizeof(pheader->arcount) );
    BigToSmallEndian((unsigned char *)&pheader->ercount, sizeof(pheader->ercount) );
    
    switch(pheader->rcode)
    {
        case 1: cerr<<"格式出错"<<endl; return 1;
        case 2: cerr<<"问题在域名服务器上"<<endl; return 2;
		case 3: cerr<<"域参照问题"<<endl; return 3;
		case 4: cerr<<"查询类型不支持"<<endl; return 4;
		case 5: cerr<<"在管理上被禁止"<<endl; return 5;
        
        default : break;
    }
    
    //pheader->print();
    
    //分析DNS报文头
    if (m_identifyid != pheader->id)
    {
        cerr<<"标识id不对应"<<endl;
        return 1;
    }
    
    uint32_t offset = sizeof(DNSHeader);
    
    //分析QUESTION
    for (uint16_t i = 0; i < pheader->qcount; ++i)
    {
        ParseRecord(pdata, offset, true);
    }
    
    m_HostInfo.clear();
    
    //分析ANSWER
    for (uint16_t i = 0; i < pheader->rcount; ++i)
    {
        ParseRecord(pdata, offset);
    }
    
    for (uint16_t i = 0; i < m_HostInfo.size(); ++i)
    {
        //cout<<m_HostInfo[i].type<<","<<m_HostInfo[i].qclass<<","<<m_HostInfo[i].ip<<endl;
        if ((QUERY_TYPE_A == m_HostInfo[i]->type || QUERY_TYPE_AAAA == m_HostInfo[i]->type ) && QCLASS_IN == m_HostInfo[i]->qclass)
        {
            vctIP.push_back(m_HostInfo[i]->ip);
        }
    }
    
    return 0;
}

int DNSResolve::ParseRecord(uint8_t *pdata, uint32_t & offset, bool isQuestion)
{
    //先解析域名名称
    string hostname;
    
    int ret = ParseName(pdata, offset, hostname);

    //分析接下来的数据
    if (isQuestion)
    {
        //查询类型+查询类
        //需要吗
        offset += sizeof(DNSQuestion);
    }
    else
    {   
        LPResource pResour = (LPResource)(pdata + offset);
        
        //大小端转换
        BigToSmallEndian((unsigned char *)&pResour->rtype, sizeof(pResour->rtype) );  
        BigToSmallEndian((unsigned char *)&pResour->rclass, sizeof(pResour->rclass) );
        BigToSmallEndian((unsigned char *)&pResour->ttl, sizeof(pResour->ttl) );
        BigToSmallEndian((unsigned char *)&pResour->length, sizeof(pResour->length) );
        
        LPDNSHostInfo hostinfo = new TDNSHostInfo;
        
        hostinfo->host = hostname;
        hostinfo->type = pResour->rtype;
        hostinfo->qclass = pResour->rclass;
        hostinfo->ttl = pResour->ttl;
        
        offset += (sizeof(DNSResource) - 1);  //不包含资源数据内容
        //判断类型
        if (QUERY_TYPE_A == pResour->rtype)
        {
            uint32_t numericip = *(uint32_t *)(pResour->content);  //ip值是字符串对应的整型表示，直接取来用即可，不必进行大小端转换
            //ipv4地址
            struct in_addr addr;
            addr.s_addr = numericip;
            char buf[16] = {0};
            inet_ntop(AF_INET, &addr, buf, sizeof(buf));
            
            //cout<<"IP 是:"<<buf<<endl;
            hostinfo->ip = buf;
            
            offset += pResour->length;
        }
        else if (QUERY_TYPE_AAAA == pResour->rtype)
        {
            //ipv6地址
            //16字节的地址
            //直接转换成字符串形式即可
            string ipv6addr;
            char buf[5] = {0};
            
            for (int i = 0; i < pResour->length;)
            {
                memset(buf, 0, sizeof(buf));
                uint8_t c1 = *(pResour->content+i);
                uint8_t c2 = *(pResour->content+i+1);
                sprintf(buf, "%02X%02X", c1, c2);
                
                i+=2;
                
                if (ipv6addr.empty())
                {
                    ipv6addr = buf;
                }
                else
                {
                    ipv6addr += (string(":") + buf);
                }
            }
 
            hostinfo->ip = ipv6addr;
            offset += pResour->length;
        }
        else if (QUERY_TYPE_CNAME == pResour->rtype || QUERY_TYPE_NS == pResour->rtype)
        {
            //解析名称
            string dname;  //域名的别名
            
            ParseName(pdata, offset, dname);
            
            //cout<<"别名是:"<<dname<<",偏移量是:"<<offset<<endl;
            hostinfo->ip = dname;
        }
        
        m_HostInfo.push_back(hostinfo);
    }
    
    return 0;
}

int DNSResolve::ParseName(uint8_t * pdata, uint32_t & offset, string & hostname)
{
    string url;
    //cout<<"[ParseName] 偏移量是:"<<offset<<endl;
    LPDomainName pName = (LPDomainName)(pdata + offset);
    char szName[64] = {0};
    
    uint32_t start = offset;
    bool isPointer = false;
    
    //指针会出现在任意地方,当相同的字符串在同一个报文中出现并且是能到结束，就会出现指针...   20160606
    //如何判断 起始+指针这种情况?
    
    //存在三种情况:
    //    START_WITH_POINTER_ENDPOINTER = 1,   //指针开头，指针结尾.其实就一个指针
    //    START_WITH_CHAR_ENDPOINTER = 2,   //字符开头，指针结尾
    //    START_WITH_CHAR_ENDCHAR,          //字符开头，字符结尾   
    uint32_t firstchar = 0;
    //----TODO
    while(pName->length != 0)
    {
        memset(szName, 0, sizeof(szName) );
        
        if (pName->length >= 0xC0)  //指针，高两位11，用于标识指针，低14位是指针的实际地址的偏移量，从报文头0开始计算
        {
            uint16_t len = *(uint16_t *)(pdata + start);  //获得cname的第一个字节的地址,注意大小端转换，超过1个字节时
            BigToSmallEndian((unsigned char *)&len, sizeof(uint16_t)); 
            start = len & 0x3FFF;  //去掉最高两个的空位，获得完整的偏移地址.
            pName = (LPDomainName)(pdata + start);
            isPointer = true;
            continue;
        }

        memcpy(szName, pName->content, pName->length);
        url += string(szName);
        url += ".";
    
        //cout<<"url:"<<url<<endl;
        if ( !isPointer)
        {
            firstchar += (sizeof(pName->length) + pName->length);
        }
        
        start += (sizeof(pName->length) + pName->length);
        //cout<<"start length:"<<start<<endl;
        pName = (LPDomainName)(pdata + start);
    }
    
    url[url.size() - 1] = 0;

    hostname = url;
    
    //start < offset  说明含有指针; 0 == firstchar  说明没有字符串开头
    if (start < offset && 0 == firstchar)   //指针开头，指针结尾
    {
        offset += 2;
    }
    else if (start < offset)  //字符开头，指针结尾
    {
        offset += (2 + firstchar);
    }
    else //字符开头，字符结尾
    {
        offset += (firstchar + sizeof(uint8_t)) ; //别忘了以0结尾的结束字符
    }
    
    return 0;
}
	
int DNSResolve::BuildRequest(string url, uint16_t querytype, uint8_t * pdata)
{
    if (url.empty() )
    {
        cerr<<"[BuildRequest] param error"<<endl;
        return -1;
    }
	//产生随机值
	srand((unsigned)time(0));
	uint16_t id = rand();  //2字节的标志位
	
    m_identifyid = id;
	DNSHeader * pheader = (DNSHeader *)pdata;
	pheader->id = id;
	pheader->qr = QR_TYPE_QUERY;
	pheader->opcode = 0;
	pheader->aa = 0; 
	pheader->tc = 0;
	pheader->rd = 1;  //先改成0
	pheader->ra = 0;;
	pheader->zero = 0;
	pheader->rcode = 0;  	
	pheader->qcount = 1;
	pheader->rcount = 0;
	pheader->arcount = 0;
	pheader->ercount = 0;
	
	//大小端转换
	SmallToBigEndian((unsigned char *)&pheader->id, sizeof(pheader->id) );  
	SmallToBigEndian((unsigned char *)&pheader->qcount, sizeof(pheader->qcount) );
	
	int offset = sizeof(DNSHeader);
	cout<<"id:"<<id<<endl;
	
	//分析url
	vector<string> vcturl;
	
	//cout<<"url是:"<<url<<endl;
	Common::ParseStringToVct(url.c_str(), ".", vcturl);
	
	for (int i = 0; i < vcturl.size(); ++i)
	{
		uint8_t usize = vcturl[i].size();

		memcpy(pdata+offset, &usize, 1);
		
		offset += sizeof usize;
		//cout<<"temp:"<<vcturl[i]<<",长度:"<<(int)usize<<endl;
		memcpy(pdata+offset,  vcturl[i].c_str(), usize);
		offset += usize;
	}
	
	uint8_t zero = 0;
	
	memcpy(pdata+offset, &zero, 1);
	
	offset += 1;
	
	memcpy(pdata + offset, &querytype, 2); 
    //大小端转换
    SmallToBigEndian((unsigned char *)pdata+offset, sizeof(querytype) ); 
	offset += sizeof querytype;
    
	uint16_t queryclass = QCLASS_IN;
	
	memcpy(pdata + offset, &queryclass, 2);
	//大小端转换
    SmallToBigEndian((unsigned char *)pdata+offset, sizeof(queryclass) );  
	offset += sizeof(queryclass);
	
	return offset;
}
	
int DNSResolve::SmallToBigEndian(uint8_t *pData, uint32_t uDataLen)
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
 
int DNSResolve::BigToSmallEndian(uint8_t *pData, uint32_t uDataLen)
{
    return SmallToBigEndian(pData, uDataLen);
}	


/*
int main(int argc, char **argv)
{
    string hostname;
    
    if (argc == 1)
    {
       hostname = "www.baidu.com"; 
    }
    else
    {
        hostname = argv[1];
    }
    
	//cout<<sizeof(DNSHeader)<<endl;
	
	DNSResolve dns;
	
	string ip;
	
	dns.Resolve(hostname, ip);
	
	return 0;
}
*/
#include <iostream>
#include <fstream>
#include <string.h>
#include <set>
#include <sys/socket.h>
#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <openssl/md5.h>

#include "spider.h"

using namespace std;

const int MAX_THREAD_NUM = 20;
const int MAX_DEEPTH = 6;
const int DEFAULT_PAGE_BUF_SIZE = 1048576;   //1M
const int MAX_LENGTH = 1000;

SuperSpider::SuperSpider():m_pRedisContext(NULL)
{
    cout<<"SuperSpider构造函数"<<endl;
    pthread_mutex_init(&redis_mutex, NULL);
}

SuperSpider::~SuperSpider()
{
    cout<<"SuperSpider 准备析构"<<endl;
    pthread_mutex_destroy(&redis_mutex); 
}

    
int SuperSpider::Init(string url, uint8_t type)
{
	m_root = url;
    m_type = type;
    
    struct timeval timeout = {2, 0};    //2s的超时时间
     
    m_pRedisContext = (redisContext*) redisConnectWithTimeout("127.0.0.1", 6379, timeout);
    
    if ( (NULL == m_pRedisContext) || (m_pRedisContext->err) )
    {
        if (m_pRedisContext)
        {
            cerr << "connect error:" << m_pRedisContext->errstr << endl;
        }
        else
        {
            cerr << "connect error: can't allocate redis context." << endl;
        }

        return 1;
    }

    //设置redis的根路径初始值
    InitRedisRoot();
    
    return 0;
}

int SuperSpider::InitRedisRoot()
{
    string strmd5;
    GetMD5(m_root, strmd5);
    
    vector<string> vctcmd;
    
    char szkey[100] = {0};
    char szcmd[MAX_LENGTH] = {0};
       
    sprintf(szkey, "%s_%d", m_root.c_str(), 1);
    
    sprintf(szcmd, "del %s ", szkey);
    vctcmd.push_back(szcmd);
       
    memset(szcmd, 0, sizeof(szcmd));
    sprintf(szcmd, "hmset %s_%d content %s href %s", strmd5.c_str(), 1, m_root.c_str(), m_root.c_str());
    vctcmd.push_back(szcmd);
       
    memset(szcmd, 0, sizeof(szcmd));
    sprintf(szcmd, "sadd %s %s|%d", szkey, strmd5.c_str(), 1);
    vctcmd.push_back(szcmd);

    ExecRedisMultiCommand(vctcmd);

	return 0;
}

int SuperSpider::Run()
{
    if (m_root.empty())
	{
		cerr<<"no root url"<<endl;
		return 1;
	}
	
	m_dns.Print();
    
    uint8_t start = 1;
    
	AddTargetToList(start);
	
    cout<<"运行完毕"<<endl;
	return 0;
}

int SuperSpider::AddTargetToList(uint8_t deepth)
{
	//创建线程
	//使用pthread_join等待
    
    if (deepth > MAX_DEEPTH )  //大于爬虫的深度，停止递归
    {
        return 0;
    }
    
    char szkey[100] = {0};
    sprintf(szkey, "%s_%d", m_root.c_str(), deepth);
    cout<<"调用AddTargetToList, 当前key是:"<<szkey<<endl;

    //判断指定key是否存在
    char szcmd[MAX_LENGTH] = {0};
 
    //进行循环前，删除下一个可能存在的set
    memset(szcmd, 0, sizeof(szcmd));
    sprintf(szcmd, "del  %s_%d", m_root.c_str(), deepth + 1); 
    
    vector<string>  vctresult;
    
    int ret = ExecRedisCommand(szcmd, vctresult);
        
    if (0 != ret)
    {
        return ret;
    } 
    
    vctresult.clear();
    
    //获取当前层的所有下一个要访问的url
    
    ret = GetCurrentResult(szkey, vctresult);
   
    if (0 == vctresult.size())
    {
        return 0;
    }
    
    //获取该key的所有键值
	int current = 0;
	pthread_t threads[MAX_THREAD_NUM] = {0};

    //深度递归，如何实现爬虫的宽度递归??
    //链表的数据结构，貌似不合适宽度递归，要用宽度递归，只需记录接下来待访问的网址集合，访问完之后就清除集合
    //进入子线程前，记录当前novel type
    int oldtype = m_type;
    
	for(int i = 0; i < vctresult.size(); ++i)  //爬取的深度控制
	{
		if (current < MAX_THREAD_NUM)  //线程数目控制
		{
				//创建线程
				pthread_t  threadT;
				pthread_attr_t attr;
				
				pthread_attr_init(&attr);
				pthread_attr_setscope(&attr, PTHREAD_SCOPE_PROCESS);
				pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE ); 

				LPThreadParam tParam = new TThreadParam;
				tParam->pParam = this;
                
                LPUrlInfo purlInfo =  new TUlrInfo;
				
				purlInfo->deepth = deepth;
                purlInfo->parsetype = oldtype;
                purlInfo->hostname = vctresult[i];
                
                tParam->purlInfo = (void *) purlInfo;
				//cout<<"参数:"<<tParam->urlinfo->hostname<<",深度是:"<<tParam->urlinfo->deepth<<endl;
				
				if (0!= pthread_create(&threads[current], &attr, AccessTargetThread, tParam))
				{
					cerr<<"创建线程失败"<<endl;
				}	
				else
				{
					cout<<"创建线程成功"<<endl;
				}	
				
				current++;
		}
		else
		{
			WaitThread(threads, current);
			current = 0;
		}
	}
    
    //等待线程执行完毕
    vctresult.clear();
    
    WaitThread(threads, current);
    
    AddTargetToList(deepth + 1);
    
    return 0 ;
}

int SuperSpider::GetCurrentResult(const char * pkey, vector<string> & vctresult)
{
    char szcmd[MAX_LENGTH] = {0};
     
    vector<string> vcttemp;
    
    memset(szcmd, 0, sizeof(szcmd));
    sprintf(szcmd, "smembers  %s", pkey); //查询该key的所有元素
    
    int ret = ExecRedisCommand(szcmd, vcttemp);
        
    if (0 != ret)
    {
        return ret;
    } 
    
    if (0 == vcttemp.size())  //如果没有值，则递归到此结束
    {
        return 0;
    }
    
    for(int i = 0; i < vcttemp.size(); ++i)
    {
        //结果的形式是md5|个数
        int pos = vcttemp[i].find('|');
        
        string strmd5 = vcttemp[i].substr(0, pos);
        int nums = atoi(vcttemp[i].substr(pos + 1).c_str());
        
        //获取实际内容中的链接
        for(int j = 0; j < nums; ++j)
        {
            memset(szcmd, 0, sizeof(szcmd));
            sprintf(szcmd, "hget %s_%d href", strmd5.c_str(), j+1); //查询hash中的href
            
            ExecRedisCommand(szcmd, vctresult);
        }
    }

    cout<<"当前key:"<<pkey<<",结果个数:"<<vctresult.size()<<endl;
    
    return 0;    
}

int SuperSpider::WaitThread(pthread_t * thread, int num)
{
	cout<<"等待子线程结束"<<endl;
	
	for(int t = 0; t < num; t++)
        pthread_join(thread[t], NULL);
	
	return 0;
}

void * SuperSpider::AccessTargetThread(void * vParam)
{
	LPThreadParam pThread = (LPThreadParam)vParam;
	
	SuperSpider * spider = (SuperSpider * )pThread->pParam;
	LPUrlInfo purl = (LPUrlInfo) pThread->purlInfo;
    
	assert(NULL != spider);
	assert(NULL != purl);
    
	//先睡眠1s
	sleep(1);
    
	spider->GetResponse(purl->hostname, purl);

    //释放内存
    delete pThread;
    
	return (void*)0;
}

int SuperSpider::GetResponse(string url, LPUrlInfo purl)
{
	string host;
	string resource;
	string filename;
    
	ParseURL(url, host, resource, filename);

    //m_dns.Print();
    
	string ip;
    
	int ret = m_dns.Resolve(host, ip);
	
	if (0 != ret)
	{
		cerr<<"解析DNS失败:"<<host<<",原始url是:"<<purl->hostname<<endl;
		return ret;
	}
	
	//建立TCP连接
	CSocket sock;
	uint16_t port = 80;
	int sockfd = sock.ConnectSockClientTcp(ip, port);
	
	if (-1 == sockfd)
	{
		cerr<<"创建客户端套接字失败"<<endl;
		return 1;
	}
	
	//组装请求报文
	
	string request = "GET " + resource + " HTTP/1.1\r\nHost:" + host + "\r\nConnection:Close\r\nUser-Agent: Mozilla/4.0\r\n\r\n";
	
    //设置超时时间，防止无限期阻塞
	struct timeval tv;
	tv.tv_sec = 60;	// 设置读超时
	tv.tv_usec = 200;
	
    (void)setsockopt( sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv) );
    
	//发送数据
	if( -1 == send( sockfd, request.c_str(), request.size(), 0 ) )
	{
		cerr << "send error" <<"reason:"<<strerror(errno)<<endl;
		close( sockfd );
		return 1;
	}

	//等待接收数据
	int m_nContentLength = DEFAULT_PAGE_BUF_SIZE;
	uint8_t * pageBuf = new uint8_t[m_nContentLength];
    memset(pageBuf, 0, m_nContentLength);
	
	int bytesRead = 0;
	ret = 1;
	
    while(ret > 0)
    {
        ret = recv(sockfd, (char *)pageBuf + bytesRead, m_nContentLength - bytesRead, 0);
        
        if(ret > 0)
        {
            bytesRead += ret;
        }

		if( m_nContentLength - bytesRead < 10)
        {
			cerr << "\n memorry not enough! data recv had been truncated"<<endl;
			break;
		}
        
    }

    cout<<"收到的网页大小是:"<<bytesRead<<endl;

    if (0 != bytesRead)
    {
        string path = "output/";
        filename = path + filename;
        //解析报文
        ParseResponse(pageBuf, bytesRead, purl, url, filename);
    }
    
	close( sockfd );
    
    delete[] pageBuf;
	return 0;
}


int SuperSpider::ParseResponse(uint8_t * pdata, int len, LPUrlInfo pCurrent, string url, string filename)
{
	int offset = 0;
	//解析报文头
    string data((char *) pdata);
    
	int status = ParseResponseHeader(data, offset);
    
    
    //先将返回报文写入文件
    
	//cout<<"文件名称是:"<<filename<<endl;
    string path = filename.substr(0, filename.find_last_of('/'));
    
    cout<<"路径是:"<<path<<endl;
    
    if (access(path.c_str(), F_OK))  //路径不存在。创建路径
    {
        char szcmd[100] = {0};
        sprintf(szcmd, "mkdir -p %s", path.c_str());
        system(szcmd);
        cout<<"创建目录成功"<<endl;
    }
    
	ofstream output(filename.c_str(), ios::binary | ios::out);
	
	if (NULL == output)
	{
		cerr<<"打开文件失败"<<endl;
		return 1;
	}
	
	output.write((char *) pdata + offset, len - offset);
	output.close();
    
    //生成新文件
	
	bool isJump = false;
    
	switch(status)
	{
		case HTTP_MOVE_FOREVER:
		case HTTP_MOVE_TEMPORY: isJump = true; break;
		
		default : break;
	}
	
	if (isJump)  //跳转，获得最新的地址，然后再次访问
	{
		//从响应头中获得新地址
		string header = data.substr(0, offset);
		int pos = header.find("Location:");
		
		if (string::npos == pos)
		{
			cerr<<"wrong response about 301"<<endl;
			return 1;
		}
		
		int endpos = header.find("\r\n", pos + 1);
		
		if (string::npos == endpos)
		{
			cerr<<"wrong response about 301"<<endl;
			return 1;
		}
		
		int startpos = pos + strlen("Location:");
		string jumpurl = data.substr(startpos, endpos - startpos);
		
		Common::Str_trip(jumpurl);  //去除首尾的空白符

		if (jumpurl[0] == '/')  //如果以/开头，则是相对地址，相对于上次的url
		{
			jumpurl = url + jumpurl;
		} 
		
		cout<<"新跳转的URL是:"<<jumpurl<<endl;
        GetResponse(jumpurl, pCurrent);
	}
    
    //解析HTML
    CHtmlParse * pParse = NULL;
    
    switch(pCurrent->parsetype)
    {
        case NOVEL_INDEX: 
        case NOVEL_BOOK_INDEX:
        case NOVEL_BOOK_DIRCTORY:
        case NOVEL_BOOK_HREF:   pParse = new QidianHtmlParse;
  
        default : break;
    }
    
    if (NULL == pParse)
    {
        cerr<<"类型错误，导致无法解析"<<endl;
        return 1;
    }
    
    vector<LPHtmlInfo>  vctHtmlInfo;
    
    pParse->ParseHtml((char *)pdata + offset, pCurrent->parsetype, vctHtmlInfo);
    
    char szkey[50] = {0};
    sprintf(szkey, "%s_%d ", m_root.c_str(), pCurrent->deepth + 1);

    InsertHtmlInfoToRedis(szkey, vctHtmlInfo, url);
    
	return 0;
}

int SuperSpider::InsertHtmlInfoToRedis(const char * pkey, vector<LPHtmlInfo> & vctHtmlInfo, string & url)
{
    string md5str;
    
    //对url做md5运算
    GetMD5(url, md5str);
    
    vector<string> vctkey;
    vector<string> vctcontent;
    vector<string> vcthref;
      
    string strcmd;
    char szTemp[100] = {0};
    int start = 0;
    set<string> seturl;
    LPHtmlInfo pHtml = NULL;
    string hostname;
    
    for (int i = 0; i < vctHtmlInfo.size(); ++i)
    {
        pHtml = vctHtmlInfo[i];
        
        hostname = pHtml->pHref;
        
        if (hostname[0] == '/')  //相对路径
        {
            hostname = url + hostname;
        }
        
        int pos = hostname.find("http://");
        
        if (string::npos != pos)
        {
            hostname = hostname.substr(pos + strlen("http://"));
        }
        
        if (seturl.end() == seturl.find(hostname))
        {
            if (m_type != pHtml->nexttype)
            {
                m_type = pHtml->nexttype;
            }
            
            seturl.insert(hostname);
            
            ++start;
            
            // vector<string> vctkey;
            // vector<string> vctcontent;
            // vector<string> vcthref;
            
            memset(szTemp, 0, sizeof(szTemp));
            //暂时遇到中文中有空格，导致命令参数对应不上问题            
            sprintf(szTemp, "%s_%d", md5str.c_str(), start);
            
            vctkey.push_back(szTemp);
            vcthref.push_back(hostname);
            vctcontent.push_back(pHtml->pContent != NULL ? pHtml->pContent : hostname.c_str());
        }
    }
    
    //先将实际内容写入redis中，如果成功，再将映射的信息写入redis
    if (0 == vctkey.size())
    {
        return 0;
    }
    
    //批量执行
    int ret = ExecSpecialRedisMultiCommand(vctkey, vctcontent, vcthref);
    
    if (ret)
    {
        return ret;
    }
    
    char setval[50] = {0};
    
    sprintf(setval, "%s|%d", md5str.c_str(), start);
    
    char szcmd[300] = {0};
    sprintf(szcmd, "sadd %s %s", pkey, setval);
    vector<string>  vctresult;
    
    ret = ExecRedisCommand(szcmd, vctresult);
            
    return ret;        
}

int SuperSpider::ExecSpecialRedisMultiCommand(vector<string> & vctkey, vector<string> & vctcontent, vector<string> & vcthref)
{
    redisReply* reply = NULL;
    //问题如下
    //当中途遇到错误时，是怎么处理的?剩下的几个命令是否会继续执行
    //加锁
    pthread_mutex_lock(&redis_mutex); 
    
    for(int i = 0; i < vctkey.size(); ++i)
    {
       redisAppendCommand(m_pRedisContext,  "hmset %s content %s href %s", vctkey[i].c_str(), vctcontent[i].c_str(), vcthref[i].c_str());
    }
    
    int ret = 0;
    
    for(int i = 0; i < vctkey.size(); ++i)
    {
        ret = redisGetReply(m_pRedisContext, (void**)&reply);
        
        if (REDIS_REPLY_ERROR == reply->type)
        {
            cerr<<"执行批量命令:"<<vcthref[i]<<"出错，错误原因是:"<<reply->str<<endl;
            ret = 1;
        }
        
        freeReplyObject(reply);   
    }
    
    //解锁
    pthread_mutex_unlock(&redis_mutex); 
    
    return ret;    
}

int SuperSpider::ExecRedisMultiCommand(vector<string> & vctcmd)
{
    redisReply* reply = NULL;
    //问题如下
    //当中途遇到错误时，是怎么处理的?剩下的几个命令是否会继续执行
    //加锁
    pthread_mutex_lock(&redis_mutex); 
    
    for(int i = 0; i < vctcmd.size(); ++i)
    {
       redisAppendCommand(m_pRedisContext, vctcmd[i].c_str());
    }
    
    int ret = 0;
    
    for(int i = 0; i < vctcmd.size(); ++i)
    {
        ret = redisGetReply(m_pRedisContext, (void**)&reply);
        
        if (REDIS_REPLY_ERROR == reply->type)
        {
            cerr<<"执行批量命令:"<<vctcmd[i]<<"出错，错误原因是:"<<reply->str<<endl;
            ret = 1;
        }
        
        freeReplyObject(reply);   
    }
    
    //解锁
    pthread_mutex_unlock(&redis_mutex); 
    
    return ret;
}

int SuperSpider::ExecRedisCommand(const char * pcommand, vector<string> & vctresult)
{
    //加锁
    pthread_mutex_lock(&redis_mutex); 

    redisReply * reply = (redisReply*) redisCommand(m_pRedisContext, pcommand);
    
    int ret = 0;
    
    if (NULL == reply)
    {
        cerr<<"执行command报错。错误代号是:"<<m_pRedisContext->err<<",错误内容是:"<<m_pRedisContext->errstr<<endl;    
        ret = 1;
    }
    else
    {
        ret = GetValue(reply, vctresult);
        
        if (0 != ret && R_REDIS_NIL != ret)
        {
            cerr<<"执行单个命令:"<<pcommand<<",结果:"<<ret<<"错误原因:"<<m_pRedisContext->errstr<<endl;
        }
    }
   
    //解锁
    pthread_mutex_unlock(&redis_mutex); 
    
    return ret;
}

int SuperSpider::GetValue(redisReply * preply, vector<string> & vctresult, int flag)
{
    if (NULL == preply)
    {
        return 0;
    }
    
    /*
        #define REDIS_REPLY_STRING 1
        #define REDIS_REPLY_ARRAY 2
        #define REDIS_REPLY_INTEGER 3
        #define REDIS_REPLY_NIL 4
        #define REDIS_REPLY_STATUS 5
        #define REDIS_REPLY_ERROR 6
    */
    int ret = 0;
    
    switch (preply->type)
    {
        case REDIS_REPLY_STRING:
        case REDIS_REPLY_STATUS: vctresult.push_back(preply->str);break;
        case REDIS_REPLY_ERROR:   ret= R_REDIS_ERROR; break;
        case REDIS_REPLY_INTEGER: 
        {
            char sznum[20] = {0};
            sprintf(sznum, "%lld", preply->integer);
            vctresult.push_back(sznum); break;
        }
        case REDIS_REPLY_NIL:  ret = R_REDIS_NIL; break;
        case REDIS_REPLY_ARRAY:
        {
            for (int i = 0; i < preply->elements; ++i)
            {
                GetValue(preply->element[i], vctresult, 1);
            }
            
            break;
        }
        
        default:break;
    }
    
    if (1 != flag)  //加上标志位，区分是否数组产生的递归，因为释放数组时，会自动释放数组内的元素，而无须再重复释放
    {
        freeReplyObject(preply);   
    }
    
    return ret;
}

int SuperSpider::GetMD5(string & data, string &md5value)
{
    md5value.clear();
    
    unsigned char md[16];
    char tmp[3]={0};
    char sztemp[33] = {0};
    
    MD5((unsigned char *)data.c_str(), data.length(), md);
        
    for( int  i = 0; i < 16; i++ )
    {
        sprintf(tmp, "%02X", md[i]);
        strcat(sztemp, tmp);
    }
    
    md5value = sztemp;
    
    return 0;
}

int SuperSpider::ParseResponseHeader(string &data, int &offset)
{
	//content-length  之后的数据，从\r\n起始\
	//查询以\r\n起始的内容
	int pos = data.find("\r\n");
	
	string httpRespon = data.substr(0, pos);
	cout<<"响应的第一行内容是:"<<httpRespon<<endl;
	
	vector<string> vctstr;
	Common::ParseStringToVct(httpRespon.c_str(), " ", vctstr);
	cout<<"响应码:"<<vctstr[1]<<endl;
	
	pos = data.find("\r\n\r\n");
	cout<<"报文正文内容是起始:"<<pos<<endl;
	//返回响应码
	offset = pos + strlen("\r\n\r\n");
	return atoi(vctstr[1].c_str());
}

int SuperSpider::ParseURL(string url, string &host, string &resource, string & filename)
{
	//处理url前段，去掉http://.  www.xx.com这样的形式
	int pos = 0;
	
	if (string::npos != url.find(':'))
	{
		pos = url.find("://");
		if (string::npos == pos)
		{
			cerr<<"错误的url格式:"<<url<<endl;
			return 1;
		}
		else
		{
			url = url.substr(pos+strlen("://"));
		}
	}
	
	//再处理url后段
	pos = url.find('/');
	
	if (string::npos != pos)
	{
		host = url.substr(0, pos);
		resource = url.substr(pos);
        
        //获的最后的/地址
        int endpos = url.find_last_of('/');
        
        filename = url.substr(endpos + 1);
	}
	else   //不包含/
	{
        filename = url;
		host = url;
		resource = "/";
	}
	
	//cout<<"host is:"<<host<<",get url:"<<resource<<endl;
	
	return 0;
}

int SuperSpider::SetRootUrl(string url)
{
    m_root = url;
}

int main(int argc, char **argv)
{
    //增加选项
    
	string rooturl;
    
    char ch;
    uint8_t type = 0;
    
	while((ch = getopt(argc, argv, "u:t:")) != EOF)
    {
        switch(ch)
        {
            case 'u': rooturl = optarg;
            case 't': type = atoi(optarg);
            default :break;
        }
    }
	
	SuperSpider spider;
 
	spider.Init(rooturl, type);  //类的结构被破坏,这是为什么?因为内存对齐问题
	spider.Run();
	
	return 0;
}

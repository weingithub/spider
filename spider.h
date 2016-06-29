#ifndef _SPIDER_H
#define _SPIDER_H

#include <cstring>
#include <iostream>
#include <pthread.h>
#include "appdefine.h"
#include "sockbase.h"
#include "common.h"
#include "dnsresolve.h"
#include "htmlparse.h"
#include <hiredis/hiredis.h>

using namespace std;

class SuperSpider
{
public:    
    SuperSpider();
    ~SuperSpider();    
    
    typedef struct stUrlInfo
    {
        uint8_t threading;
        uint8_t parsetype;
        uint16_t deepth;
        
        string hostname;
        
        stUrlInfo():
            threading(0),
            parsetype(0),
            deepth(0)
        {
            
        }
        
        ~stUrlInfo()
        {
            //cout<<"析构URL"<<endl;
        }
    }TUlrInfo, *LPUrlInfo;

	typedef struct stThreadParam
	{
		void * pParam;
        void * purlInfo;
	}TThreadParam, *LPThreadParam;
	
	enum
	{
		HTTP_OK = 200,
		HTTP_MOVE_FOREVER = 301,
		HTTP_MOVE_TEMPORY = 302,
	};
	
	static void * AccessTargetThread(void * vParam);
	    
    int Init(string url, uint8_t type);  //初始化
    int Run();   //启动
    int AddTargetToList(uint8_t deepth);  //从上层递归
    int SetRootUrl(string url);
	
	int PrintLink();
	
private:
    int WaitThread(pthread_t * thread, int num);
	int ParseURL(string url, string &host, string &resource, string & filename);
	int GetResponse(string url, LPUrlInfo purlInfo);	
	int ParseResponse(uint8_t * data, int len, LPUrlInfo pCurrent, string url, string filename);
	int ParseResponseHeader(string &data, int &offset);
    
    //redis使用相关
    
	int ExecRedisMultiCommand(vector<string> vctcmd, vector<redisReply *> &vctReply);
    int ExecRedisCommand(const char * pcommand, vector<string> & vctresult);
    int GetValue(redisReply * preply, vector<string> & vctresult, int flag = 0);
    //int InsertHostToRedis(const char * pHost, const char * pContent, int deep);

private:
	DNSResolve m_dns;
    string  m_root;
    uint8_t m_type;
    redisContext * m_pRedisContext;
};

#endif

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
	int InitRedisRoot();
    
private:
    int WaitThread(pthread_t * thread, int num);
    
    //报文相关
	int ParseURL(string url, string &host, string &resource, string & filename);
	int GetResponse(string url, LPUrlInfo purlInfo);	
	int ParseResponse(uint8_t * data, int len, LPUrlInfo pCurrent, string url, string filename);
	int ParseResponseHeader(string &data, int &offset);
    
    //redis使用相关
	int ExecRedisMultiCommand(vector<string> & vctcmd);   //批量执行，不需要获得每一个redis的返回结果
    int ExecSpecialRedisMultiCommand(vector<string> & vctkey, vector<string> & vctcontent, vector<string> & vcthref);
    int ExecRedisCommand(const char * pcommand, vector<string> & vctresult);
    int GetValue(redisReply * preply, vector<string> & vctresult, int flag = 0);
    int InsertHtmlInfoToRedis(const char * pkey, vector<LPHtmlInfo> & vctHtml, string & url);
    int GetCurrentResult(const char * pkey, vector<string> &vctresult);
    
    //加密相关
    int GetMD5(string & data, string &md5value);
private:
	DNSResolve m_dns;
    string  m_root;
    uint8_t m_type;
    redisContext * m_pRedisContext;
    pthread_mutex_t  redis_mutex;
};

#endif

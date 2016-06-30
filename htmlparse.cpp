#include <iostream>
#include <set>
#include <cstdio>

#include "htmlparse.h"

using namespace std;

int QidianHtmlParse::ParseHtml(const char * pdata, uint8_t type, vector<LPHtmlInfo>  & vctResult)
{
    CDocument doc;
    doc.parse(pdata);
    
    int ret = 0;
    
    switch(type)
    {
        case  NOVEL_INDEX : ret = ParseBookList(doc, vctResult); break;
        case  NOVEL_BOOK_INDEX : ret = ParseBookIndex(doc, vctResult); break;
        case  NOVEL_BOOK_DIRCTORY : ret = ParseBookDirectory(doc, vctResult); break;
        case  NOVEL_BOOK_HREF : ret = ParseBookHref(doc, vctResult); break;
        
        default: break;
    }
    
    return ret;
}

//解析首页，获取首页当中隐藏的小说链接
int QidianHtmlParse::ParseBookList(CDocument & doc, vector<LPHtmlInfo>  & vctResult)
{
    char szCondition[100] = {0};
    string contentvalue;
    string herfvalue;

    char szCut[10] = "Book/";
    memset(szCondition, 0, sizeof(szCondition));
    sprintf(szCondition, "body div a[href*=Book]");
    CSelection c = doc.find(szCondition);

    int result = c.nodeNum();
    //cout<<"查询到的节点数目是:"<<result<<endl;

    if (0 == result)
    {
        return 0;
    }

    int book = 0;

    //如何才能去重?
    //以href做key
    set<string> setHref;
    LPHtmlInfo pHtmlInfo = NULL;
    
    for (int i = 0; i < c.nodeNum(); ++i)
    {
        contentvalue = c.nodeAt(i).text();
        Common::Str_trip(contentvalue);

        //获取链接的值
        herfvalue = c.nodeAt(i).attribute("href");
        Common::Str_trip(herfvalue);

        int pos = herfvalue.find(szCut);
        
        if (string::npos == pos  || contentvalue.empty())
        {
            continue;
        }
        else
        {
            int next = pos + strlen(szCut);
            
            if ((herfvalue[next] - '0') > 9 || (herfvalue[next] - '0') < 0)
            {
                continue;
            }
        }
        
        if (setHref.end() != setHref.find(herfvalue))
        {
            continue;
        }
        
        setHref.insert(herfvalue);
        
        pHtmlInfo = new THtmlInfo;
        pHtmlInfo->nexttype = NOVEL_BOOK_INDEX;
        MEMCPY(pHtmlInfo->pHref, herfvalue.c_str(), herfvalue.length());
        MEMCPY(pHtmlInfo->pContent, contentvalue.c_str(), contentvalue.length());
        vctResult.push_back(pHtmlInfo);
         
        /* 
        cout<<"第"<<++book<<"个结果是:";
        cout << contentvalue; // some link
        cout << ",链接是:"<<herfvalue<<endl; // some link
        */
    }
}

//从小说链接的内容中解析出目录链接
int QidianHtmlParse::ParseBookIndex(CDocument & doc, vector<LPHtmlInfo>  & vctResult)
{
    char szCondition[100] = {0};
     
    memset(szCondition, 0, sizeof(szCondition));
    sprintf(szCondition, "body div a[stat-type=read]");
    CSelection c = doc.find(szCondition);
    
    int result = c.nodeNum();
    //cout<<"查询到的节点数目是:"<<result<<endl;
    
    if (0 == result)
    {
        return 0;
    }
    
    string contentvalue;
    string hrefvalue;
    LPHtmlInfo pHtmlInfo = NULL;
    
    for (int i = 0; i < c.nodeNum(); ++i)
    {
        //获取链接的值
        contentvalue = c.nodeAt(i).attribute("title");
        Common::Str_trip(contentvalue);

        hrefvalue = c.nodeAt(i).attribute("href");
        Common::Str_trip(hrefvalue);

        if (contentvalue.empty() || hrefvalue.empty() )
        {
            continue;
        }
        
        pHtmlInfo = new THtmlInfo;
        
        pHtmlInfo->nexttype = NOVEL_BOOK_DIRCTORY;
        MEMCPY(pHtmlInfo->pHref, hrefvalue.c_str(), hrefvalue.length());
        MEMCPY(pHtmlInfo->pContent, contentvalue.c_str(), contentvalue.length());
        
        vctResult.push_back(pHtmlInfo);
        
   
        cout<<"title:"<<contentvalue<<endl;
        cout<<"href:"<<hrefvalue<<endl;

    }
    
    return 0;
}

//解析目录，获得所有章节的链接
int QidianHtmlParse::ParseBookDirectory(CDocument & doc, vector<LPHtmlInfo>  & vctResult)  
{
    char szCondition[100] = {0};
    int i = 0;

    string contentvalue;
    string herfvalue;
    
    int book = 0;
    
    memset(szCondition, 0, sizeof(szCondition));
    sprintf(szCondition, "body div a[itemprop=url]");
    CSelection c = doc.find(szCondition);

    int result = c.nodeNum();
    //cout<<"查询到的节点数目是:"<<result<<endl;

    if (0 == result)
    {
        return 0;
    }

    char szCut[10] = "?";
    
    book = 0;
    LPHtmlInfo pHtmlInfo = NULL;

    for (int i = 0; i < c.nodeNum(); ++i)
    {
        contentvalue = c.nodeAt(i).text();
        Common::Str_trip(contentvalue);

        //获取链接的值
        herfvalue = c.nodeAt(i).attribute("href");
        Common::Str_trip(herfvalue);
 
        if (string::npos != herfvalue.find(szCut))
        {
            continue;
        }
        
        if (contentvalue.empty() || herfvalue.empty())
        {
            continue;
        }

        pHtmlInfo = new THtmlInfo;
        
        pHtmlInfo->nexttype = NOVEL_BOOK_HREF;
        MEMCPY(pHtmlInfo->pHref, herfvalue.c_str(), herfvalue.length());
        MEMCPY(pHtmlInfo->pContent, contentvalue.c_str(), contentvalue.length());
        
        vctResult.push_back(pHtmlInfo);
  
        cout<<"第"<<++book<<"个结果是:";
        cout << contentvalue; // some link
        cout << ",链接是:"<<herfvalue<<endl; // some link
  
    }
   
    return 0;
}

//从章节链接的内容中，解析获得章节内容的链接
int QidianHtmlParse::ParseBookHref(CDocument & doc, vector<LPHtmlInfo>  & vctResult)
{
    char szCondition[100] = {0};
    int i = 0;
    
    string herfvalue;
    string contentvalue;
    
    int book = 0;
    char szCut[10] = ".txt";
    
    memset(szCondition, 0, sizeof(szCondition));
    sprintf(szCondition, "body div script[src]");
    CSelection c = doc.find(szCondition);
    
    int result = c.nodeNum();
  
    if (0 == result)
    {
        return 0;
    }
        
    LPHtmlInfo pHtmlInfo = NULL;
    
    //先搜索到链接
    for (int i = 0; i < c.nodeNum(); ++i)
    {
        herfvalue = c.nodeAt(i).attribute("src");
        Common::Str_trip(herfvalue);

        if (string::npos == herfvalue.find(szCut))
        {
            continue;
        }
         
        pHtmlInfo = new THtmlInfo;
         
        MEMCPY(pHtmlInfo->pHref, herfvalue.c_str(), herfvalue.length());    
        
        break;
    }
    
    if (NULL == pHtmlInfo)  //如果没搜索到小说内容链接，则不进行下面的搜索
    {
        return 0;    
    }
    
    //再搜索章节名称
    memset(szCondition, 0, sizeof(szCondition));
    sprintf(szCondition, "body div.story_title h1");
    
    c = doc.find(szCondition);
    
    result = c.nodeNum();
        
    if (0 == result)
    {
        return 1;
    }
    
    contentvalue = c.nodeAt(i).text();
    Common::Str_trip(contentvalue);
    MEMCPY(pHtmlInfo->pContent, contentvalue.c_str(), contentvalue.length());  
        
    vctResult.push_back(pHtmlInfo);
    
    return 0;
}



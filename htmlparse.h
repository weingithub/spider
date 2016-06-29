#ifndef _HTML_PARSE_H_
#define _HTML_PARSE_H_

#include "Document.h"
#include "Node.h"
#include "appdefine.h"
#include "common.h"

typedef struct stHtmlInfo
{
    uint8_t nexttype;
    char * pContent;
    char * pHref;
    
    stHtmlInfo():
        nexttype(0),
        pContent(NULL),
        pHref(NULL)
    {
        
    }
    
    ~stHtmlInfo()
    {
        if (NULL != pContent) {delete[] pContent; pContent = NULL;}
        if (NULL != pHref) {delete[] pHref; pHref = NULL;}
    }
}THtmlInfo, *LPHtmlInfo;

enum
{
    NOVEL_INDEX = 0X1,  //小说网站首页
    NOVEL_BOOK_INDEX,  //某小说首页
    NOVEL_BOOK_DIRCTORY, //某小说目录
    NOVEL_BOOK_HREF,  //某小说链接
    NOVEL_BOOK_CONTENT, //某小说内容
};

class CHtmlParse
{
public:
    virtual int ParseHtml(const char * pdata, uint8_t type, std::vector<LPHtmlInfo>  & vctResult) = 0;
    
private:
};

class QidianHtmlParse : public CHtmlParse
{
public:
    int ParseHtml(const char * pdata, uint8_t type, std::vector<LPHtmlInfo>  & vctResult);
    
private:
    //从章节链接的内容中，解析获得章节内容的链接
    int ParseBookHref(CDocument & doc, std::vector<LPHtmlInfo>  & vctResult);    
    //解析目录，获得所有章节的链接
    int ParseBookDirectory(CDocument & doc, std::vector<LPHtmlInfo>  & vctResult);
    //从小说链接的内容中解析出目录链接
    int ParseBookIndex(CDocument & doc, std::vector<LPHtmlInfo>  & vctResult);
    //解析首页，获取首页当中隐藏的小说链接
    int ParseBookList(CDocument & doc, std::vector<LPHtmlInfo>  & vctResult);
    
};

#endif

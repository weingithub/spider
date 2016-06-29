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
    NOVEL_INDEX = 0X1,  //С˵��վ��ҳ
    NOVEL_BOOK_INDEX,  //ĳС˵��ҳ
    NOVEL_BOOK_DIRCTORY, //ĳС˵Ŀ¼
    NOVEL_BOOK_HREF,  //ĳС˵����
    NOVEL_BOOK_CONTENT, //ĳС˵����
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
    //���½����ӵ������У���������½����ݵ�����
    int ParseBookHref(CDocument & doc, std::vector<LPHtmlInfo>  & vctResult);    
    //����Ŀ¼����������½ڵ�����
    int ParseBookDirectory(CDocument & doc, std::vector<LPHtmlInfo>  & vctResult);
    //��С˵���ӵ������н�����Ŀ¼����
    int ParseBookIndex(CDocument & doc, std::vector<LPHtmlInfo>  & vctResult);
    //������ҳ����ȡ��ҳ�������ص�С˵����
    int ParseBookList(CDocument & doc, std::vector<LPHtmlInfo>  & vctResult);
    
};

#endif

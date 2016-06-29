#include "common.h"
#include "appdefine.h"

using namespace std;

int Common::Str_trip(string & strsrc)
{
    if (strsrc.empty())
    {
        return 0;
    }
    
    int start = 0, end = 0;  //起始位置和结束位置
      
    int pos = 0;
   
    while ( strsrc[pos] && strchr(" \t\n\r", strsrc[pos]) )
    {
        ++pos;
    }
        
    start = pos;
        
    pos = strsrc.size() - 1;  //倒置
    while(pos >= start && strsrc[pos] == ' ')
    {
        --pos;
    }
        
    end = pos;
        
    strsrc = strsrc.substr(start, end - start + 1);
        
    return 0;
}

int Common::ParseStringToVct(const char *pPreaseString, const char *pszSep, vector<string> &vctDestStr)
{
    if (NULL == pPreaseString || NULL == pszSep || 0 == strlen(pPreaseString) || 0 == strlen(pszSep))
    {
        cerr<<"PreaseStringToVct param is invalid"<<endl;
        return 1;
    }
    
    vctDestStr.clear();
    
    unsigned int upsSize = strlen(pPreaseString);
    
	char *pSrcString = new char[upsSize + 1];
	
	if(pSrcString == NULL)
	{
		return 2;
	}

	memset(pSrcString, 0, upsSize + 1);
	memcpy(pSrcString, pPreaseString, upsSize);

	unsigned int uSepLen = strlen(pszSep);
	char *pString = pSrcString;
	char *pStart = pSrcString;
	char *pEnd = pSrcString;
	
	pStart = strstr(pString, pszSep);
	
	if (NULL == pStart) 
	{
		vctDestStr.push_back(pString);
		return 0;
	}
	
	int firstLen = pStart - pString;
	char * pFirst = new char [firstLen+1];
	memset(pFirst, 0, firstLen+1);
	
	strncpy(pFirst, pString, firstLen);
	pFirst[firstLen] = 0;
	//cout<<"first is:"<<pFirst<<", length is:"<<strlen(pFirst)<<", should length is:"<<firstLen<<endl;
	vctDestStr.push_back(pFirst);
	
	delete[] pFirst;
	
	pStart += uSepLen;
	pEnd = pStart;
	
	while(NULL != pStart)
	{
		pEnd = strstr(pEnd, pszSep);
		
		if (NULL != pEnd)
		{
			*pEnd = '\0';
			pEnd += uSepLen;
			vctDestStr.push_back(pStart);
		}
		else
		{
			vctDestStr.push_back(pStart);
		}
		pStart = pEnd;
	}
	
	
	delete[] pSrcString;
	
	return 0;
}


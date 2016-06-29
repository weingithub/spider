#ifndef _COMMON_H_
#define _COMMON_H_
#include <iostream>
#include <cstring>
#include <vector>

namespace Common
{
    int Str_trip(std::string & strsrc);
	int ParseStringToVct(const char *pPreaseString, const char *pszSep, std::vector<std::string> &vctDestStr); 
}

#endif

#ifndef _APPDEFIND_H_
#define _APPDEFIND_H_

#define R_SUCCESS  0
#define R_ERROR    1

#define R_REDIS_ERROR  9
#define R_REDIS_NIL  10

//SOCKETœ‡πÿ¥ÌŒÛ
#define R_SOCK_ERROR_CREATE  100

#define DNSIPFILE  "dns_record.conf"

typedef  unsigned char  uint8_t;
typedef  unsigned short uint16_t;
typedef  unsigned int   uint32_t;

#ifdef __x86_64__
    typedef  unsigned  long uint64_t;
#else
    typedef  unsigned  long long  uint64_t;
#endif

#define MEMCPY(pDstData, pSrcData, sLen)        \
{												\
    if (NULL == pSrcData)                       \
    {                                           \
        pDstData = new char[1];                 \
        memset(pDstData, 0, 1);                 \
    }                                           \
    else                                        \
    {                                           \
        pDstData = new char[sLen + 1];          \
        memset(pDstData, 0, sLen + 1);          \
        memcpy(pDstData, pSrcData, sLen);       \
    }											\
}		

#endif

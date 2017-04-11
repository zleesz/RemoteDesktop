#pragma once
#include "def.h"

class ICompressBase
{
public:
	virtual RD_COMPRESS_MODE GetMode()
	{
		return RDCM_RAW;
	}
	virtual bool encode(char* src, unsigned int srclen, char** dst, unsigned int* dstlen)
	{
		*dst = src;
		*dstlen = srclen;
		return true;
	}
	virtual bool decode(char* src, unsigned int srclen, char** dst, unsigned int* dstlen)
	{
		*dst = src;
		*dstlen = srclen;
		return true;
	}
};

#pragma once
#include "def.h"
#include "compress_base.h"
#include "compress_zip.h"

class CCompressFactory
{
public:
	CCompressFactory(void);
	~CCompressFactory(void);

public:
	static bool CreateCompress(RD_COMPRESS_MODE mode, ICompressBase** pCompress)
	{
		switch (mode)
		{
		case RDCM_RAW:
			*pCompress = new ICompressBase();
			return true;
		case RDCM_ZIP:
			*pCompress = new CCompressZip();
			return true;
		default:
			break;
		}
		return false;
	}
};

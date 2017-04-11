#pragma once
#include "compress_base.h"
#include "..\zlib\zlib.h"

#define COMPRESS_REALLOC_SIZE 10240

class CCompressZip : public ICompressBase
{
public:
	RD_COMPRESS_MODE GetMode()
	{
		return RDCM_ZIP;
	}
	
	bool encode(char* src, unsigned int srclen, char** dst, unsigned int* dstlen)
	{
		z_stream c_stream;
		int err = 0;
		c_stream.zalloc = NULL;
		c_stream.zfree = NULL;
		c_stream.opaque = NULL;
		if (deflateInit2(&c_stream, Z_DEFAULT_COMPRESSION, Z_DEFLATED, 47, 8, Z_DEFAULT_STRATEGY) != Z_OK)
		{
			return false;
		}
		*dstlen = srclen;
		*dst = new char[*dstlen];
		ZeroMemory(*dst, *dstlen);

		c_stream.next_in  = (Bytef*)src;
		c_stream.avail_in  = srclen;
		c_stream.next_out = (Bytef*)*dst;
		c_stream.avail_out  = *dstlen;
		while (c_stream.avail_in != 0 && c_stream.total_out < *dstlen)
		{
			if (deflate(&c_stream, Z_NO_FLUSH) != Z_OK)
			{
				delete *dst;
				*dst = NULL;
				*dstlen = 0;
				return false;
			}
		}
		if (c_stream.avail_in != 0)
		{
			delete *dst;
			*dst = NULL;
			*dstlen = 0;
			return false;
		}
		for(;;)
		{
			if ((err = deflate(&c_stream, Z_FINISH)) == Z_STREAM_END)
			{
				break;
			}
			if (err != Z_OK)
			{
				delete *dst;
				*dst = NULL;
				*dstlen = 0;
				return false;
			}
		}
		if (deflateEnd(&c_stream) != Z_OK)
		{
			delete *dst;
			*dst = NULL;
			*dstlen = 0;
			return false;
		}
		*dstlen = c_stream.total_out;
		return true;
	}
	bool decode(char* src, unsigned int srclen, char** dst, unsigned int* dstlen)
	{
		int err = 0;
		z_stream d_stream = {0};
		static char dummy_head[2] =
		{
			0x8 + 0x7 * 0x10,
			(((0x8 + 0x7 * 0x10) * 0x100 + 30) / 31 * 31) & 0xFF,
		};
		d_stream.zalloc = (alloc_func) 0;
		d_stream.zfree = (free_func) 0;
		d_stream.opaque = (voidpf) 0;
		d_stream.next_in = (Bytef*)src;
		d_stream.avail_in = 0;
		*dstlen = srclen;
		*dst = new char[*dstlen];
		if (*dst == NULL)
		{
			return false;
		}
		ZeroMemory(*dst, *dstlen);
		d_stream.next_out = (Bytef*)*dst;
		if (inflateInit2(&d_stream, 47) != Z_OK)
		{
			delete *dst;
			*dst = NULL;
			*dstlen = 0;
			return false;
		}
		while (d_stream.total_in < srclen)
		{
			if (d_stream.total_out == *dstlen)
			{
				uLong offset = d_stream.next_out - (Bytef*)*dst;
				char* pnew = new char[*dstlen + COMPRESS_REALLOC_SIZE];
				if (pnew == NULL)
				{
					delete *dst;
					*dst = NULL;
					*dstlen = 0;
					return false;
				}
				memcpy(pnew, *dst, *dstlen);
				delete *dst;
				*dstlen += COMPRESS_REALLOC_SIZE;
				*dst = pnew;
				ZeroMemory(*dst + *dstlen - COMPRESS_REALLOC_SIZE, COMPRESS_REALLOC_SIZE);
				d_stream.next_out = (Bytef*)*dst + offset;
			}
			d_stream.avail_in = d_stream.avail_out = 1; /* force small buffers */
			if ((err = inflate(&d_stream, Z_NO_FLUSH)) == Z_STREAM_END)
			{
				break;
			}
			if (err != Z_OK)
			{
				if (err == Z_DATA_ERROR)
				{
					d_stream.next_in = (Bytef*) dummy_head;
					d_stream.avail_in = sizeof(dummy_head);
					if ((err = inflate(&d_stream, Z_NO_FLUSH)) != Z_OK)
					{
						delete *dst;
						*dst = NULL;
						*dstlen = 0;
						return false;
					}
				}
				else
				{
					delete *dst;
					*dst = NULL;
					*dstlen = 0;
					return false;
				}
			}
		}
		if (inflateEnd(&d_stream) != Z_OK)
		{
			delete *dst;
			*dst = NULL;
			*dstlen = 0;
			return false;
		}
		*dstlen = d_stream.total_out;
		return true;
	}
};

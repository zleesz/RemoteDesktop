#pragma once

extern "C"{
#include "jpeglib.h"
};

class JPEG
{
public:
	static bool Decompress(unsigned char* data, const unsigned int dataSize, BitmapInfo* pBitmapInfo, unsigned char** ppDestBuffer)
	{
		struct jpeg_decompress_struct cinfo;
		struct jpeg_error_mgr jerr;

		cinfo.err = jpeg_std_error(&jerr);
		jpeg_create_decompress(&cinfo);

		//从内存读取
		jpeg_mem_src(&cinfo, data, dataSize);

		jpeg_read_header(&cinfo, TRUE);
		jpeg_start_decompress(&cinfo);

		int row_stride = cinfo.output_width * cinfo.output_components;

		ZeroMemory(&pBitmapInfo->bmiHeader, sizeof(BITMAPINFOHEADER));
		pBitmapInfo->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
		pBitmapInfo->bmiHeader.biWidth = cinfo.output_width;
		pBitmapInfo->bmiHeader.biHeight = cinfo.output_height;
		pBitmapInfo->bmiHeader.biPlanes = 1;
		pBitmapInfo->bmiHeader.biBitCount = 24;
		pBitmapInfo->bmiHeader.biCompression = BI_RGB;
		pBitmapInfo->bmiHeader.biSizeImage = row_stride * cinfo.output_height;

		*ppDestBuffer = new unsigned char[pBitmapInfo->bmiHeader.biSizeImage];

		unsigned char* buffer = new unsigned char[cinfo.output_width * cinfo.output_components];
		memset(buffer, 0, cinfo.output_width * cinfo.output_components);
		while (cinfo.output_scanline < cinfo.output_height)  
		{  
			int line = cinfo.output_scanline;//当前行数  
			jpeg_read_scanlines(&cinfo, &buffer, 1); //执行该操作读取第line行数据，cinfo.output_scanline将加一，指向下一个要扫描的行  
			for (size_t i = 0; i < cinfo.output_width; i++)
			{
				(*ppDestBuffer)[line * row_stride + i * cinfo.output_components]= buffer[i * 3];
				(*ppDestBuffer)[line * row_stride + i * cinfo.output_components + 1] = buffer[i * 3 + 1];
				(*ppDestBuffer)[line * row_stride + i * cinfo.output_components + 2] = buffer[i * 3 + 2];
			}
		}
		delete[] buffer;

		jpeg_finish_decompress(&cinfo);
		jpeg_destroy_decompress(&cinfo);
		return true;
	}

	static bool Compress(unsigned int width, unsigned int height, WORD wPixelBytes, unsigned char* pSrc, int quality, unsigned char** pDest, unsigned int* pDestLength)
	{
		assert(wPixelBytes == 3);

		struct jpeg_compress_struct cinfo;
		struct jpeg_error_mgr jerr;
		JSAMPROW row_pointer[1];
		cinfo.err = jpeg_std_error(&jerr);  
		jpeg_create_compress (&cinfo);
		jpeg_mem_dest(&cinfo, pDest, (unsigned long*)pDestLength);

		// 设置压缩参数，主要参数有图像宽、高、色彩通道数（１：索引图像，３：其他）
		// 色彩空间（JCS_GRAYSCALE表示灰度图，JCS_RGB表示彩色图像），压缩质量
		cinfo.image_width = width;
		cinfo.image_height =  height;
		cinfo.input_components = 3;
		cinfo.in_color_space = JCS_RGB;

		jpeg_set_defaults(&cinfo);
		jpeg_set_quality(&cinfo, quality, TRUE);
		jpeg_start_compress(&cinfo, TRUE);

		while (cinfo.next_scanline < cinfo.image_height)
		{
			row_pointer[0] = &pSrc[cinfo.next_scanline * width * wPixelBytes];
			jpeg_write_scanlines(&cinfo, row_pointer, 1);
		}
		jpeg_finish_compress(&cinfo);
		jpeg_destroy_compress(&cinfo);
		return true;
	}
};

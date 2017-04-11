#pragma once

class Util
{
public:
	static void SaveBitmap(DWORD height, DWORD width, WORD wPixelBytes, unsigned char* pdata, TCHAR* pszFilePath)
	{
		BITMAPFILEHEADER   bmfHeader = {0};
		BITMAPINFOHEADER   bi = {0};

		bi.biSize = sizeof(BITMAPINFOHEADER);
		bi.biWidth = width;
		bi.biHeight = height;
		bi.biPlanes = 1;
		bi.biBitCount = wPixelBytes * 8;
		bi.biCompression = BI_RGB;
		bi.biSizeImage = 0;
		bi.biXPelsPerMeter = 0;
		bi.biYPelsPerMeter = 0;
		bi.biClrUsed = 0;
		bi.biClrImportant = 0;
		DWORD dwBmpSize = width * height * wPixelBytes;

		// Add the size of the headers to the size of the bitmap to get the total file size
		DWORD dwSizeofDIB = dwBmpSize + sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
		//Offset to where the actual bitmap bits start.
		bmfHeader.bfOffBits = (DWORD)sizeof(BITMAPFILEHEADER) + (DWORD)sizeof(BITMAPINFOHEADER);

		//Size of the file
		bmfHeader.bfSize = dwSizeofDIB;

		//bfType must always be BM for Bitmaps
		bmfHeader.bfType = 0x4D42; //BM

		HANDLE hFile = CreateFile(pszFilePath, GENERIC_WRITE, 0,  NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
		DWORD dwBytesWritten = 0;
		WriteFile(hFile, (LPSTR)&bmfHeader, sizeof(BITMAPFILEHEADER), &dwBytesWritten, NULL);
		WriteFile(hFile, (LPSTR)&bi, sizeof(BITMAPINFOHEADER), &dwBytesWritten, NULL);
		WriteFile(hFile, (LPSTR)pdata, dwBmpSize, &dwBytesWritten, NULL);
		CloseHandle(hFile);
	}
};
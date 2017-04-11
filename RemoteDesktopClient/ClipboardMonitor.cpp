#include "stdafx.h"
#include "ClipboardMonitor.h"

CClipboardMonitor::CClipboardMonitor(void)
{
}

CClipboardMonitor::~CClipboardMonitor(void)
{
}

void CClipboardMonitor::OnClipboardChanged()
{
	HBITMAP hBitmap = NULL;
	if ( NULL == hBitmap )
	{
		return;
	}
	if (!::OpenClipboard(::GetActiveWindow()))
	{
		::DeleteObject(hBitmap);
		return;
	}
	EmptyClipboard();
	HANDLE hBmp = SetClipboardData(CF_BITMAP, hBitmap); 
	hBmp;
	::DeleteObject(hBitmap);
	CloseClipboard();
	if (!::OpenClipboard(::GetActiveWindow())) 
		return; 

	if(IsClipboardFormatAvailable(CF_UNICODETEXT))
	{
		HANDLE hglb = GetClipboardData(CF_UNICODETEXT); 
		std::wstring str((WCHAR*)GlobalLock(hglb));
		GlobalUnlock(hglb);

	}
	else if(IsClipboardFormatAvailable(CF_TEXT))
	{
		HANDLE hglb = GetClipboardData(CF_UNICODETEXT); 
		std::string str((char*)GlobalLock(hglb));
		GlobalUnlock(hglb);
	}
	CloseClipboard();
}
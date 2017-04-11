#include "StdAfx.h"
#include "ScreenBitmap.h"
#include "WindowHooks.h"
#include <assert.h>
#include "..\common\utils.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#define CAPTURE_DESKTOP_TIMERID		1
CScreenBitmap* CScreenBitmap::s_pScreenBitmap = NULL;

CScreenBitmap::CScreenBitmap(void) : m_pBuffer(NULL), m_pScreenBitmapEvent(NULL), m_uThreadId(0)
{
	tool.Log(_T("CScreenBitmap::CScreenBitmap"));
	HWND hDesktopWnd = ::GetDesktopWindow();
	::GetWindowRect(hDesktopWnd, &m_rcScreen);

	WORD wBitCount = 24;
	m_wPixelBytes = wBitCount / 8;

	m_BitmapInfo.bmiHeader.biHeight = m_rcScreen.Height();
	m_BitmapInfo.bmiHeader.biWidth = m_rcScreen.Width();
	m_BitmapInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	m_BitmapInfo.bmiHeader.biPlanes = 1;
	m_BitmapInfo.bmiHeader.biBitCount = wBitCount;
	m_BitmapInfo.bmiHeader.biCompression = BI_RGB;
	m_BitmapInfo.bmiHeader.biSizeImage = 0;
	m_BitmapInfo.bmiHeader.biXPelsPerMeter = 0;
	m_BitmapInfo.bmiHeader.biYPelsPerMeter = 0;
	m_BitmapInfo.bmiHeader.biClrUsed = 0;
	m_BitmapInfo.bmiHeader.biClrImportant = 0;

	m_nScanPointRowCount = (m_rcScreen.Height() - 1) / SCAN_MODIFY_RECT_UNIT + 1;
	m_nScanPointColumnCount = (m_rcScreen.Width() - 1) / SCAN_MODIFY_RECT_UNIT + 1;
}

CScreenBitmap::~CScreenBitmap(void)
{
	tool.Log(_T("CScreenBitmap::~CScreenBitmap"));
}

unsigned __stdcall CScreenBitmap::DoStartCapture(void* pParam)
{
	tool.Log(_T("CScreenBitmap::DoStartCapture enter."));
	CAutoAddReleasePtr<CScreenBitmap> spThis;
	spThis.Attach((CScreenBitmap*)pParam);

	MSG msg;
	while (::GetMessage(&msg, NULL, 0, 0) != -1)
	{
		bool bQuit = false;
		switch (msg.message)
		{
		case WM_TIMER:
			spThis->CaptureDesktop();
			break;
		case WM_CLOSE:
			bQuit = false;
			break;
		default:
			break;
		}
		if (bQuit)
			break;
	}

	tool.Log(_T("CScreenBitmap::DoStartCapture leave."));
	return 0;
}

void CScreenBitmap::StartCapture(IScreenBitmapEvent* pScreenBitmapEvent)
{
	tool.Log(_T("CScreenBitmap::StartCapture pScreenBitmapEvent:0x%08X"), pScreenBitmapEvent);
	if (m_uThreadId != 0)
	{
		tool.Log(_T("CaptureScreen is already started!"));
		return;
	}

	m_pScreenBitmapEvent = pScreenBitmapEvent;
	Create(HWND_MESSAGE);
	SetTimer(CAPTURE_DESKTOP_TIMERID, CAPTURE_DESKTOP_FRAME);
	//CaptureDesktop();

	// 子线程拥有一个引用计数
	AddRef();
	HANDLE hThread = (HANDLE)_beginthreadex(NULL, 0, &DoStartCapture, (LPVOID)this, 0, &m_uThreadId);
	CloseHandle(hThread);

	assert(hThread != NULL);
}

void CScreenBitmap::StopCapture()
{
	tool.Log(_T("CScreenBitmap::StopCapture"));
	if (m_uThreadId != 0)
	{
		PostThreadMessage(m_uThreadId, WM_CLOSE, 0, 0);
		m_uThreadId = 0;
		KillTimer(CAPTURE_DESKTOP_TIMERID);
		DestroyWindow();
	}
	m_lock.Lock();
	if (m_pBuffer)
	{
		delete[] m_pBuffer;
		m_pBuffer = NULL;
	}
	m_lock.Unlock();
	m_pScreenBitmapEvent = NULL;
	::SetRectEmpty(&m_rcScreen);
}

bool CScreenBitmap::IsCapturing() const
{
	return m_uThreadId != 0;
}

CRect CScreenBitmap::GetRect() const
{
	return m_rcScreen;
}

void CScreenBitmap::GetBitmapInfo(BitmapInfo* pBitmapInfo)
{
	m_lock.Lock();
	*pBitmapInfo = m_BitmapInfo;
	m_lock.Unlock();
}

unsigned char* CScreenBitmap::GetBuffer(unsigned int x, unsigned int y)
{
	unsigned char* pBuffer = NULL;
	if (m_pBuffer != NULL)
	{
		m_lock.Lock();
		LONG bmWidthBytes = m_BitmapInfo.bmiHeader.biWidth * m_wPixelBytes;
		pBuffer = m_pBuffer + (y * bmWidthBytes + x * m_wPixelBytes);
		m_lock.Unlock();
	}
	return pBuffer;
}

WORD CScreenBitmap::GetPixelBytes() const
{
	return m_wPixelBytes;
}

LRESULT CScreenBitmap::OnTimer(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	if (m_uThreadId != 0)
	{
		PostThreadMessage(m_uThreadId, WM_TIMER, 0, 0);
	}
	return 0;
}

LRESULT CScreenBitmap::OnModified(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/)
{
	unsigned int nModifiedBlockCount = (unsigned int)wParam;
	unsigned int* pnModifiedBlocks = (unsigned int*)lParam;
	m_pScreenBitmapEvent->OnModified(nModifiedBlockCount, pnModifiedBlocks);
	delete[] pnModifiedBlocks;
	return 0;
}

LRESULT CScreenBitmap::OnFirstBitmap(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	m_pScreenBitmapEvent->OnFirstBitmap(&m_BitmapInfo, m_wPixelBytes, m_pBuffer);
	return 0;
}

void CScreenBitmap::CaptureDesktop()
{
	unsigned char* pLastBuffer = m_pBuffer;

	HWND hDesktopWnd = ::GetDesktopWindow();
	HDC hDC = ::GetDC(hDesktopWnd);
	HDC hMemDC = ::CreateCompatibleDC(hDC);
	HBITMAP hBitmap = ::CreateCompatibleBitmap(hDC, m_rcScreen.Width(), m_rcScreen.Height());

	HBITMAP hOldBitmap = (HBITMAP)SelectObject(hMemDC, hBitmap);
	::BitBlt(hMemDC, 0, 0, m_rcScreen.Width(), m_rcScreen.Height(), hDC, 0, 0, SRCCOPY);

	BITMAP bmp;
	::GetObject(hBitmap, sizeof(BITMAP), (LPSTR)&bmp);

	m_lock.Lock();
	BitmapInfo bitmapInfo = m_BitmapInfo;
	m_lock.Unlock();

	unsigned char* pNewBuffer = NULL;
	if (::GetDIBits(hDC, hBitmap, 0, 1, NULL, (BITMAPINFO*)&bitmapInfo, DIB_RGB_COLORS))
	{
		pNewBuffer = new unsigned char[bitmapInfo.bmiHeader.biSizeImage];
		ZeroMemory(pNewBuffer, bitmapInfo.bmiHeader.biSizeImage);
		::GetDIBits(hDC, hBitmap, 0, bmp.bmHeight, pNewBuffer, (LPBITMAPINFO)&bitmapInfo, DIB_RGB_COLORS);
	}

	::SelectObject(hMemDC, hOldBitmap);
	::DeleteObject(hBitmap);
	::ReleaseDC(hDesktopWnd, hDC);
	::DeleteDC(hMemDC);

	m_lock.Lock();
	m_BitmapInfo = bitmapInfo;
	m_lock.Unlock();

	if (m_hWnd && pLastBuffer)
	{
		unsigned int nModifiedBlockCount = 0;
		unsigned int* pnModifiedBlocks = NULL;
		GetModifiedBlocks(pNewBuffer, pLastBuffer, &nModifiedBlockCount, &pnModifiedBlocks);
		m_lock.Lock();
		delete[] pLastBuffer;
		pLastBuffer = NULL;
		m_pBuffer = pNewBuffer;
		m_lock.Unlock();
		if (m_pScreenBitmapEvent && nModifiedBlockCount > 0)
		{
			//KillTimer(CAPTURE_DESKTOP_TIMERID);
			//Util::SaveBitmap(m_rcScreen.Height(), m_rcScreen.Width(), m_wPixelBytes, pNewBuffer, _T("E:\\workspaces\\GitHub\\RemoteDesktop\\Debug\\new.bmp"));
			PostMessage(WM_SCREENBITMAP_MODIFIED, (WPARAM)nModifiedBlockCount, (LPARAM)pnModifiedBlocks);
		}
		else if (pnModifiedBlocks != NULL)
		{
			delete[] pnModifiedBlocks;
		}
	}
	else if (m_hWnd)
	{
		m_lock.Lock();
		m_pBuffer = pNewBuffer;
		//Util::SaveBitmap(m_rcScreen.Height(), m_rcScreen.Width(), m_wPixelBytes, pNewBuffer, _T("E:\\workspaces\\GitHub\\RemoteDesktop\\Debug\\old.bmp"));
		m_lock.Unlock();
		SendMessage(WM_SCREENBITMAP_FIRSTBITMAP);
	}
}

void CScreenBitmap::GetModifiedBlocks(unsigned char* pNewBuffer, unsigned char* pLastBuffer, unsigned int* pnModifiedBlockCount, unsigned int** ppnModifiedBlocks)
{
	tool.Log(_T("GetModifiedBlocks start"));
	unsigned int nRowPixelByteCount = m_wPixelBytes * m_BitmapInfo.bmiHeader.biWidth;
	
	bool* pbModifyMask = new bool[m_nScanPointRowCount * m_nScanPointColumnCount];
	memset(pbModifyMask, 0, sizeof(bool) * m_nScanPointRowCount * m_nScanPointColumnCount);

	unsigned int nScanUnitPixelByteCount = SCAN_MODIFY_RECT_UNIT * m_wPixelBytes;
	unsigned int nLastScanPixelByteCount = nRowPixelByteCount - (m_nScanPointColumnCount - 1) * nScanUnitPixelByteCount;
	unsigned int nScanStartPixelByte = 0;
	*pnModifiedBlockCount = 0;
	for (unsigned int row = 0; row < m_nScanPointRowCount; row++)
	{
		// 一行一行每32像素扫描，如果不一样，标记这一方块改变
		unsigned int nScanOneRowStart = row * SCAN_MODIFY_RECT_UNIT;
		for (unsigned int unitRow = nScanOneRowStart; unitRow < nScanOneRowStart + SCAN_MODIFY_RECT_UNIT && unitRow < (unsigned int)m_BitmapInfo.bmiHeader.biHeight; unitRow++)
		{
			for (unsigned int column = 0; column < m_nScanPointColumnCount; column++)
			{
				unsigned int nModifyIndex = row * m_nScanPointColumnCount + column;
				unsigned int nScanPixelByteCount;
				if (column == m_nScanPointColumnCount - 1)
					nScanPixelByteCount = nLastScanPixelByteCount;
				else
					nScanPixelByteCount = nScanUnitPixelByteCount;

				if (!pbModifyMask[nModifyIndex] && memcmp(pNewBuffer + nScanStartPixelByte, pLastBuffer + nScanStartPixelByte, nScanPixelByteCount) != 0)
				{
					// 不一样，标记
					pbModifyMask[nModifyIndex] = true;
					(*pnModifiedBlockCount)++;
				}
				nScanStartPixelByte += nScanPixelByteCount;
			}
		}
	}
	tool.Log(_T("GetModifiedBlocks nModifiedBlockCount:%u"), *pnModifiedBlockCount);
	if (*pnModifiedBlockCount > 0)
	{
		int nModifiedBlockIndex = 0;
		*ppnModifiedBlocks = new unsigned int[*pnModifiedBlockCount];
		for (unsigned int i = 0; i < m_nScanPointRowCount * m_nScanPointColumnCount; i++)
		{
			if (pbModifyMask[i])
			{
				(*ppnModifiedBlocks)[nModifiedBlockIndex++] = i;
			}
		}
	}
	delete[] pbModifyMask;
	tool.Log(_T("GetModifiedBlocks end"));
}

bool CScreenBitmap::ClipBitmapRect(CRect* pClipRect, unsigned char** ppClipBuffer, unsigned int* pLength)
{
	tool.Log(_T("CScreenBitmap::ClipBitmapRect pClipRect:0x%08X"), pClipRect);
	if (pClipRect == NULL || ::IsRectEmpty(pClipRect))
		return false;
	if (pClipRect->left > m_rcScreen.Width() || pClipRect->top > m_rcScreen.Height())
		return false;

	if (pClipRect->left < 0) pClipRect->left = 0;
	if (pClipRect->top < 0) pClipRect->top = 0;
	if (pClipRect->right > m_rcScreen.Width()) pClipRect->right = m_rcScreen.Width();
	if (pClipRect->bottom > m_rcScreen.Height()) pClipRect->bottom = m_rcScreen.Height();

	unsigned int nClipRowPixelByteCount = pClipRect->Width() * m_wPixelBytes;
	*pLength = pClipRect->Width() * pClipRect->Height() * m_wPixelBytes;
	tool.Log(_T("CScreenBitmap::ClipBitmapRect nClipRowPixelByteCount:%d, length:%u"), nClipRowPixelByteCount, *pLength);
	*ppClipBuffer = new unsigned char[*pLength];
	for (unsigned int row = pClipRect->top; row < (unsigned int)pClipRect->bottom; row++)
	{
		m_lock.Lock();
		unsigned char* pBuffer = GetBuffer(pClipRect->left, row);
		memcpy((*ppClipBuffer) + (row - pClipRect->top) * nClipRowPixelByteCount, pBuffer, nClipRowPixelByteCount);
		m_lock.Unlock();
	}
	return true;
}

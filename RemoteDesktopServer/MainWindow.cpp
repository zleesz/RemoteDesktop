#include "stdafx.h"
#include "resource.h"
#include "MainWindow.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

CMainWindow::CMainWindow(void)
{
	m_spScreenBitmap = CScreenBitmap::GetInstance();
}

CMainWindow::~CMainWindow(void)
{
}

HWND CMainWindow::Create(CRect& rcBound, int nCmdShow)
{
	if (m_hWnd)
	{
		return m_hWnd;
	}
	DWORD dwStyle = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX;
	DWORD dwStyleEx = WS_EX_APPWINDOW;

	CWindowImpl<CMainWindow>::Create(GetDesktopWindow(), rcBound, NULL, dwStyle, dwStyleEx);
	ShowWindow(nCmdShow);
	m_server.Start(this);
	return m_hWnd;
}

void CMainWindow::Destroy()
{
	if (m_hWnd)
	{
		DestroyWindow();
	}
}

LRESULT CMainWindow::OnClose(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	Destroy();
	return 0;
}

LRESULT CMainWindow::OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	HICON hSmallIcon = (HICON)::LoadImage(::GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_ICON1), IMAGE_ICON,
		::GetSystemMetrics(SM_CXSMICON), ::GetSystemMetrics(SM_CYSMICON), LR_DEFAULTCOLOR);

	SendMessageW(WM_SETICON,(WPARAM)ICON_SMALL,(LPARAM) hSmallIcon);

	HICON hBigIcon = (HICON)::LoadImage(::GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_ICON1), IMAGE_ICON,
		::GetSystemMetrics(SM_CXICON), ::GetSystemMetrics(SM_CYICON), LR_DEFAULTCOLOR);

	SendMessageW(WM_SETICON,(WPARAM)ICON_BIG,(LPARAM) hBigIcon);
	return 0;
}

LRESULT CMainWindow::OnDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	tool.Log(_T("CMainWindow::OnDestroy"));
	if (m_spScreenBitmap)
	{
		m_spScreenBitmap->StopCapture();
		m_spScreenBitmap.Release();
	}
	m_server.Stop();
	PostQuitMessage(0);
	return 0;
}

HRESULT CMainWindow::OnEraseBackground(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	return TRUE;
}

LRESULT CMainWindow::OnPaint(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	PAINTSTRUCT ps;
	HDC hDC = ::BeginPaint(m_hWnd, &ps);

	CRect rcClient;
	GetClientRect(&rcClient);
	HDC hMemDC = ::CreateCompatibleDC(hDC);
	HBITMAP hMemBmp = ::CreateCompatibleBitmap(hDC, rcClient.Width(), rcClient.Height());
	HGDIOBJ hOldBmp = ::SelectObject(hMemDC, hMemBmp);

// 	unsigned char *bitmapBits = m_bmpScreen.GetBuffer(0, 0);
// 	if (bitmapBits)
// 	{
// 		BitmapInfo* bitmapInfo = NULL;
// 		m_bmpScreen.GetBitmapInfo(&bitmapInfo);
// 
// 		::SetStretchBltMode(hMemDC, STRETCH_HALFTONE);
// 		::StretchDIBits(hMemDC, 0, 0, rcClient.Width(), rcClient.Height(), 0, 0, bitmapInfo->bmiHeader.biWidth, bitmapInfo->bmiHeader.biHeight, bitmapBits, (LPBITMAPINFO)bitmapInfo, DIB_RGB_COLORS, SRCCOPY);
// 	}

	::BitBlt(hDC, 0, 0, rcClient.Width(), rcClient.Height(), hMemDC, 0, 0, SRCCOPY);
	::SelectObject(hMemDC, hOldBmp);
	::DeleteObject(hMemBmp);
	::DeleteDC(hMemDC);

	::EndPaint(m_hWnd, &ps);
	return 0;
}

void CMainWindow::OnStart()
{
}

void CMainWindow::OnStop()
{
	
}

void CMainWindow::OnConnect(CClientConnection* pClientConnection)
{
	tool.Log(_T("CMainWindow::OnConnect pClientConnection:0x%08X"), pClientConnection);
	if (!m_spScreenBitmap->IsCapturing())
	{
		// 有客户端连接过来了，如果还没启动截屏，就去启动
		m_spScreenBitmap->StartCapture(this);
	}
}

void CMainWindow::OnDisconnect(CClientConnection* pClientConnection, RD_ERROR_CODE errorCode)
{
	tool.Log(_T("CMainWindow::OnDisconnect pClientConnection:0x%08X, errorCode:0x%08X"), pClientConnection, errorCode);
	if (!m_server.HasClientConnected() && m_spScreenBitmap->IsCapturing())
	{
		// 已经没有客户端连接了，可以停止截屏
		m_spScreenBitmap->StopCapture();
	}
}
void CMainWindow::OnFirstBitmap(BitmapInfo* pBitmapInfo, WORD wPixelBytes, unsigned char *bitmapBits)
{
	tool.Log(_T("CMainWindow::OnFirstBitmap width:%d, height:%d, wPixelBytes:%d"), pBitmapInfo->bmiHeader.biWidth, pBitmapInfo->bmiHeader.biHeight, wPixelBytes);
	if (!m_server.HasClientConnected())
		return;

	m_server.OnScreenFirstBitmap(pBitmapInfo, wPixelBytes, bitmapBits);
}

void CMainWindow::OnModified(unsigned int nModifiedBlockCount, unsigned int* pnModifiedBlocks)
{
	tool.Log(_T("CMainWindow::OnModified nModifiedBlockCount:%d"), nModifiedBlockCount);
	if (!m_server.HasClientConnected())
		return;

	m_server.OnScreenModified(nModifiedBlockCount, pnModifiedBlocks);
}

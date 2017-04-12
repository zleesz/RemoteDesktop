#include "stdafx.h"
#include "MainWindow.h"
#include "resource.h"

CMainWindow::CMainWindow(void)
{
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
	PostQuitMessage(0);
	return 0;
}

LRESULT CMainWindow::OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	//m_btnSetting.Load("settings_normal.png", "settings_hover.png", "settings_down.png");
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
	HDC hDCMem = ::CreateCompatibleDC(hDC);
	HBITMAP hMemBmp = ::CreateCompatibleBitmap(hDC, rcClient.Width(), rcClient.Height());
	HGDIOBJ hOldBmp = ::SelectObject(hDCMem, hMemBmp);

	::BitBlt(hDC, 0, 0, rcClient.Width(), rcClient.Height(), hDCMem, 0, 0, SRCCOPY);
	::SelectObject(hDCMem, hOldBmp);
	::DeleteObject(hMemBmp);
	::DeleteDC(hDCMem);

	::EndPaint(m_hWnd, &ps);
	return 0;
}

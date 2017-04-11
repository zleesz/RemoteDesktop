#include "stdafx.h"
#include "MainWindow.h"
#include "PngLoader.h"
#include "resource.h"
#include "..\common\utils.h"

CMainWindow::CMainWindow(void) : m_pBuffer(NULL), m_nScanPointRowCount(0), m_nScanPointColumnCount(0)
{
	ZeroMemory((void*)&m_bitmapInfo, sizeof(BitmapInfo));
}

CMainWindow::~CMainWindow(void)
{
	if (m_pBuffer)
	{
		delete[] m_pBuffer;
		m_pBuffer = NULL;
	}
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
	m_remoteClient.Connect(this);
	return m_hWnd;
}

void CMainWindow::Destroy()
{
	if (m_pBuffer)
	{
		delete[] m_pBuffer;
		m_pBuffer = NULL;
	}
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
	m_remoteClient.Stop();
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

	// »­Ô¶³Ì×ÀÃæ
	DrawRemoteDesktop(hDCMem);

	::BitBlt(hDC, 0, 0, rcClient.Width(), rcClient.Height(), hDCMem, 0, 0, SRCCOPY);
	::SelectObject(hDCMem, hOldBmp);
	::DeleteObject(hMemBmp);
	::DeleteDC(hDCMem);

	::EndPaint(m_hWnd, &ps);
	return 0;
}

void CMainWindow::DrawRemoteDesktop(HDC hDC)
{
	if (m_pBuffer)
	{
		CRect rcClient;
		GetClientRect(&rcClient);

		int left = 0, top = 0, width = 0, height = 0;
		if (rcClient.Width() * m_bitmapInfo.bmiHeader.biHeight > m_bitmapInfo.bmiHeader.biWidth * rcClient.Height())
		{
			height = rcClient.Height();
			width = m_bitmapInfo.bmiHeader.biWidth * height / m_bitmapInfo.bmiHeader.biHeight;
			left = (rcClient.Width() - width) / 2;
		}
		else
		{
			width = rcClient.Width();
			height = m_bitmapInfo.bmiHeader.biHeight * width / m_bitmapInfo.bmiHeader.biWidth;
			top = (rcClient.Height() - height) / 2;
		}

		::SetStretchBltMode(hDC, STRETCH_HALFTONE);
		::StretchDIBits(hDC, left, top, width, height, 0, 0, m_bitmapInfo.bmiHeader.biWidth, m_bitmapInfo.bmiHeader.biHeight, m_pBuffer, (LPBITMAPINFO)&m_bitmapInfo, DIB_RGB_COLORS, SRCCOPY);
	}
}

void CMainWindow::GetBitmapInfo(BitmapInfo* pBitmapInfo)
{
	*pBitmapInfo = m_bitmapInfo;
}

unsigned char* CMainWindow::GetBuffer(unsigned int x, unsigned int y)
{
	unsigned char* pBuffer = NULL;
	if (m_pBuffer != NULL)
	{
		LONG bmWidthBytes = m_bitmapInfo.bmiHeader.biWidth * m_wPixelBytes;
		pBuffer = m_pBuffer + (y * bmWidthBytes + x * m_wPixelBytes);
	}
	return pBuffer;
}

WORD CMainWindow::GetPixelBytes() const
{
	return m_wPixelBytes;
}

void CMainWindow::OnConnected()
{
	
}

void CMainWindow::OnStateChanged(RD_CONNECTION_STATE state)
{
	
}

void CMainWindow::OnGetLogonResult(int nResult)
{

}

void CMainWindow::OnGetTransferBitmap(BitmapInfo& bitmapInfo, WORD wPixelBytes, unsigned char* pBuffer)
{
	if (m_pBuffer)
	{
		delete[] m_pBuffer;
		m_pBuffer = NULL;
	}
	m_wPixelBytes = wPixelBytes;
	m_bitmapInfo = bitmapInfo;
	m_pBuffer = new unsigned char[m_bitmapInfo.bmiHeader.biSizeImage];
	memcpy(m_pBuffer, pBuffer, m_bitmapInfo.bmiHeader.biSizeImage);

	m_nScanPointRowCount = (m_bitmapInfo.bmiHeader.biHeight - 1) / SCAN_MODIFY_RECT_UNIT + 1;
	m_nScanPointColumnCount = (m_bitmapInfo.bmiHeader.biWidth - 1) / SCAN_MODIFY_RECT_UNIT + 1;

	Invalidate(TRUE);
}

void CMainWindow::OnGetTransferModifyBitmap(unsigned int nModifiedBlockCount, unsigned int* pnModifiedBlocks, unsigned int nBufferSize, unsigned char* pBuffer)
{
	if (m_pBuffer && nModifiedBlockCount > 0)
	{
		static bool b = false;
		if (!b)
		{
			b = true;
			//Util::SaveBitmap(SCAN_MODIFY_RECT_UNIT, nModifiedBlockCount * SCAN_MODIFY_RECT_UNIT, m_wPixelBytes, pBuffer, _T("E:\\workspaces\\GitHub\\RemoteDesktop\\Debug\\modified_recive.bmp"));
		}
		unsigned int nModifiedRowBytes = nModifiedBlockCount * SCAN_MODIFY_RECT_UNIT * m_wPixelBytes;
		unsigned int nRowBytes = m_bitmapInfo.bmiHeader.biWidth * m_wPixelBytes;
		for (unsigned int i = 0; i < nModifiedBlockCount; i++)
		{
			unsigned int nBlockIndex = pnModifiedBlocks[i];
			unsigned int nBlockRow = nBlockIndex / m_nScanPointColumnCount;
			unsigned int nBlockColumn = nBlockIndex % m_nScanPointColumnCount;

			CRect rcOneModified;
			rcOneModified.left = nBlockColumn * SCAN_MODIFY_RECT_UNIT;
			rcOneModified.top = nBlockRow * SCAN_MODIFY_RECT_UNIT;
			rcOneModified.right = (nBlockColumn == m_nScanPointColumnCount - 1) ? m_bitmapInfo.bmiHeader.biWidth : (rcOneModified.left + SCAN_MODIFY_RECT_UNIT);
			rcOneModified.bottom = (nBlockRow == m_nScanPointRowCount - 1) ? m_bitmapInfo.bmiHeader.biHeight : (rcOneModified.top + SCAN_MODIFY_RECT_UNIT);

			unsigned int nColumnStartPos = i * SCAN_MODIFY_RECT_UNIT * m_wPixelBytes;
			unsigned int nRowPixelByteCount = rcOneModified.Width() * m_wPixelBytes;
			for (int j = 0; j < rcOneModified.Height(); j++)
			{
				unsigned char* pClipBuffer = GetBuffer(rcOneModified.left, rcOneModified.top + j);
				memcpy(pClipBuffer, pBuffer + j * nModifiedRowBytes + nColumnStartPos, nRowPixelByteCount);
			}
		}
		
		Invalidate(TRUE);
	}
}

void CMainWindow::OnGetDisconnect(int nDisconnectCode)
{

}

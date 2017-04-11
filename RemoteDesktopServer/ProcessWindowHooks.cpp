#include "StdAfx.h"
#include "ProcessWindowHooks.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#define  UPDATE_RECT_DELAY_TIMERID 1
#define  UPDATE_RECT_DELAY_INTERVAL 40

CProcessWindowHooks::CProcessWindowHooks(void) : m_hThread(NULL), m_hEvent(NULL)
{
	m_hEvent = ::CreateEvent(NULL, TRUE, FALSE, NULL);
}

CProcessWindowHooks::~CProcessWindowHooks(void)
{
}

HWND CProcessWindowHooks::GetHWND()
{
	if (m_hWnd == NULL)
	{
		::WaitForSingleObject(m_hEvent, INFINITE);
	}
	return m_hWnd;
}

bool CProcessWindowHooks::Start()
{
	if (m_hThread != NULL)
	{
		tool.Log(_T("thread is already started!"));
		return true;
	}
	unsigned threadID = 0;
	m_hThread = (HANDLE)_beginthreadex(NULL, 0, &DoHooks, (LPVOID)this, 0, &threadID);
	CloseHandle(m_hThread);

	if (m_hThread == NULL)
		return false;

	::WaitForSingleObject(m_hEvent, INFINITE);

	return true;
}

void CProcessWindowHooks::Stop()
{
	::WaitForSingleObject(m_hEvent, INFINITE);
	if (m_hWnd)
	{
		PostMessage(WM_CLOSE);
	}
}

unsigned __stdcall CProcessWindowHooks::DoHooks(void* pParam)
{
	CProcessWindowHooks* pThis = (CProcessWindowHooks*)pParam;
	pThis->Create(HWND_MESSAGE);
	::SetEvent(pThis->m_hEvent);
	::CloseHandle(pThis->m_hEvent);
	pThis->m_hEvent = NULL;
	MSG msg;
	while (::PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE))
	{
		if (::GetMessage(&msg, NULL, 0, 0) != -1)
		{
			::TranslateMessage(&msg);
			::DispatchMessage(&msg);
		}
	}
	return 0;
}

LRESULT CProcessWindowHooks::OnTimer(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
// 	NotifyHooksRegion(updates[1-activeRgn]);
// 	if (updates[activeRgn].is_empty())
// 		KillTimer(UPDATE_RECT_DELAY_TIMERID);
// 	activeRgn = 1-activeRgn;
// 	updates[activeRgn].clear();
	return 0;
}

LRESULT CProcessWindowHooks::OnHooksWindowChanged(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	// An entire window has (potentially) changed
// 	HWND hWnd = (HWND) msg.lParam;
// 	if (::IsWindow(hWnd) && ::IsWindowVisible(hWnd) && !::IsIconic(hWnd) && ::GetWindowRect(hWnd, &wrect) && !IsRectEmpty(&wrect))
// 	{
// 			updates[activeRgn].assign_union(Rect(wrect.left, wrect.top,
// 				wrect.right, wrect.bottom));
// 			SetTimer(UPDATE_RECT_DELAY_TIMERID, UPDATE_RECT_DELAY_INTERVAL);
// 	}
	return 0;
}

LRESULT CProcessWindowHooks::OnHooksWindowClientAreaChanged(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
// 	// The client area of a window has (potentially) changed
// 	hwnd = (HWND) msg.lParam;
// 	if (IsWindow(hwnd) && IsWindowVisible(hwnd) && !IsIconic(hwnd) &&
// 		GetClientRect(hwnd, &wrect) && !IsRectEmpty(&wrect))
// 	{
// 		POINT pt = {0,0};
// 		if (ClientToScreen(hwnd, &pt)) {
// 			updates[activeRgn].assign_union(Rect(wrect.left+pt.x, wrect.top+pt.y,
// 				wrect.right+pt.x, wrect.bottom+pt.y));
// 			SetTimer(UPDATE_RECT_DELAY_TIMERID, UPDATE_RECT_DELAY_INTERVAL);
// 		}
// 	}
	return 0;
}

LRESULT CProcessWindowHooks::OnHooksWindowBorderChanged(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
// 	hwnd = (HWND) msg.lParam;
// 	if (IsWindow(hwnd) && IsWindowVisible(hwnd) && !IsIconic(hwnd) &&
// 		GetWindowRect(hwnd, &wrect) && !IsRectEmpty(&wrect))
// 	{
// 		Region changed(Rect(wrect.left, wrect.top, wrect.right, wrect.bottom));
// 		RECT crect;
// 		POINT pt = {0,0};
// 		if (GetClientRect(hwnd, &crect) && ClientToScreen(hwnd, &pt) &&
// 			!IsRectEmpty(&crect))
// 		{
// 			changed.assign_subtract(Rect(crect.left+pt.x, crect.top+pt.y,
// 				crect.right+pt.x, crect.bottom+pt.y));
// 		}
// 		if (!changed.is_empty()) {
// 			updates[activeRgn].assign_union(changed);
// 			SetTimer(UPDATE_RECT_DELAY_TIMERID, UPDATE_RECT_DELAY_INTERVAL);
// 		}
// 	}
	return 0;
}

LRESULT CProcessWindowHooks::OnHooksHooksRectangleChanged(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
// 	Rect r = Rect(LOWORD(msg.wParam), HIWORD(msg.wParam),
// 		LOWORD(msg.lParam), HIWORD(msg.lParam));
// 	if (!r.is_empty()) {
// 		updates[activeRgn].assign_union(r);
// 		SetTimer(UPDATE_RECT_DELAY_TIMERID, UPDATE_RECT_DELAY_INTERVAL);
// 	}
	return 0;
}

LRESULT CProcessWindowHooks::OnHooksHooksCursorChanged(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	//NotifyHooksCursor((HCURSOR)msg.lParam);
	return 0;
}

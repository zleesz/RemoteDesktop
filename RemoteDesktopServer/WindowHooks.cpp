#include "StdAfx.h"
#include "WindowHooks.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

HWND CWindowHooks::s_hOwnerWnd				= NULL;
DWORD CWindowHooks::s_dwHookThreadId		= 0;
HHOOK CWindowHooks::s_hCallWndProc			= NULL;
HHOOK CWindowHooks::s_hCallWndProcRet		= NULL;
HHOOK CWindowHooks::s_hGetMessage			= NULL;
HHOOK CWindowHooks::s_hDialogMessage		= NULL;
CWindowHooks* CWindowHooks::s_pWindowHooks	= NULL;

CWindowHooks::CWindowHooks(void)
{
}

CWindowHooks::~CWindowHooks(void)
{
}

bool CWindowHooks::Install(HWND hOwnerWnd, DWORD dwHookThreadId)
{
	s_hOwnerWnd = hOwnerWnd;
	s_dwHookThreadId = dwHookThreadId;

	HMODULE hModule = ::GetModuleHandle(NULL);
	s_hCallWndProc = ::SetWindowsHookEx(WH_CALLWNDPROC, HookCallWndProc, hModule, s_dwHookThreadId);
	s_hCallWndProcRet = ::SetWindowsHookEx(WH_CALLWNDPROCRET, HookCallWndProcRet, hModule, s_dwHookThreadId);
	s_hGetMessage = ::SetWindowsHookEx(WH_GETMESSAGE, HookGetMessage, hModule, s_dwHookThreadId);
	s_hDialogMessage = ::SetWindowsHookEx(WH_SYSMSGFILTER, HookDialogMessage, hModule, s_dwHookThreadId);

	if (!s_hCallWndProc || !s_hCallWndProcRet || !s_hGetMessage || !s_hDialogMessage)
	{
		Uninstall();
		return false;
	}

	return true;
}

void CWindowHooks::Uninstall()
{
	if (s_hOwnerWnd == 0)
		return;

	if (s_hCallWndProc)
	{
		UnhookWindowsHookEx(s_hCallWndProc);
		s_hCallWndProc = NULL;
	}
	if (s_hCallWndProcRet)
	{
		UnhookWindowsHookEx(s_hCallWndProcRet);
		s_hCallWndProcRet = NULL;
	}
	if (s_hGetMessage)
	{
		UnhookWindowsHookEx(s_hGetMessage);
		s_hGetMessage = NULL;
	}
	if (s_hDialogMessage)
	{
		UnhookWindowsHookEx(s_hDialogMessage);
		s_hDialogMessage = NULL;
	}

	s_hOwnerWnd = 0;
	s_dwHookThreadId = 0;
}

LRESULT CALLBACK CWindowHooks::HookCallWndProc(int nCode, WPARAM wParam, LPARAM lParam)
{
	if (nCode == HC_ACTION)
	{
		CWPSTRUCT* info = (CWPSTRUCT*) lParam;
		ProcessWindowMessage(info->message, info->hwnd, info->wParam, info->lParam);
	}
	return CallNextHookEx(s_hCallWndProc, nCode, wParam, lParam);
}

LRESULT CALLBACK CWindowHooks::HookCallWndProcRet(int nCode, WPARAM wParam, LPARAM lParam)
{
	if (nCode == HC_ACTION)
	{
		CWPRETSTRUCT* info = (CWPRETSTRUCT*) lParam;
		ProcessWindowMessage(info->message, info->hwnd, info->wParam, info->lParam);
	}
	return CallNextHookEx(s_hCallWndProcRet, nCode, wParam, lParam);
}

LRESULT CALLBACK CWindowHooks::HookGetMessage(int nCode, WPARAM wParam, LPARAM lParam)
{
	if (nCode == HC_ACTION)
	{
		if (wParam & PM_REMOVE)
		{
			MSG* msg = (MSG*) lParam;
			ProcessWindowMessage(msg->message, msg->hwnd, msg->wParam, msg->lParam);
		}
	}
	return CallNextHookEx(s_hGetMessage, nCode, wParam, lParam);
}

LRESULT CALLBACK CWindowHooks::HookDialogMessage(int nCode, WPARAM wParam, LPARAM lParam)
{
	if (nCode == HC_ACTION)
	{
		MSG* msg = (MSG*) lParam;
		ProcessWindowMessage(msg->message, msg->hwnd, msg->wParam, msg->lParam);
	}
	return CallNextHookEx(s_hDialogMessage, nCode, wParam, lParam);
}

void CWindowHooks::ProcessWindowMessage(UINT msg, HWND wnd, WPARAM wParam, LPARAM lParam)
{
	if (!IsWindowVisible(wnd))
		return;
	switch (msg) {

		// -=- Border update events
	case WM_NCPAINT:
	case WM_NCACTIVATE:
		NotifyWindowBorder(wnd, msg);
		break;

		// -=- Client area update events
	case BM_SETCHECK:
	case BM_SETSTATE:
	case EM_SETSEL:
	case WM_CHAR:
	case WM_ENABLE:
	case WM_KEYUP:
	case WM_LBUTTONUP:
	case WM_MBUTTONUP:
	case WM_PALETTECHANGED:
	case WM_RBUTTONUP:
	case WM_SYSCOLORCHANGE:
	case WM_SETTEXT:
	case WM_SETFOCUS:
		NotifyWindowClientArea(wnd, msg);
		break;
	case WM_HSCROLL:
	case WM_VSCROLL:
		if (((int) LOWORD(wParam) == SB_THUMBTRACK) || ((int) LOWORD(wParam) == SB_ENDSCROLL))
			NotifyWindow(wnd, msg);
		break;

	case WM_WINDOWPOSCHANGING:
	case WM_DESTROY:
		{
			RECT wrect;
			if (GetWindowRect(wnd, &wrect)) {
				NotifyRectangle(&wrect);
			}
		}
		break;

	case WM_WINDOWPOSCHANGED:
		NotifyWindow(wnd, msg);
		break;

	case WM_PAINT:
		// *** could improve this
		NotifyWindowClientArea(wnd, msg);
		break;

		// Handle pop-up menus appearing
	case 482:
		NotifyWindow(wnd, 482);
		break;

		// Handle pop-up menus having items selected
	case 485:
		{
			HANDLE prop = GetProp(wnd, (LPCTSTR) MAKELONG(GetPopupSelectionATOM(), 0));
			if (prop != (HANDLE) wParam) {
				NotifyWindow(wnd, 485);
				SetProp(wnd,
					(LPCTSTR) MAKELONG(GetPopupSelectionATOM(), 0),
					(HANDLE) wParam);
			}
		}
		break;
	case WM_NCMOUSEMOVE:
	case WM_MOUSEMOVE:
		// todo cursor
// 		if (enable_cursor_shape) {
// 			HCURSOR new_cursor = GetCursor();
// 			if (new_cursor != cursor) {
// 				cursor = new_cursor;
// 				NotifyCursor(cursor);
// 			}
// 		}
		break;
	};
}

bool CWindowHooks::NotifyHookOwner(UINT event, WPARAM wParam, LPARAM lParam)
{
	if (s_hOwnerWnd)
	{
		return PostMessage(s_hOwnerWnd, event, wParam, lParam) != 0;
	}
	return false;
}

bool CWindowHooks::NotifyWindow(HWND hwnd, UINT msg)
{
	return NotifyHookOwner(GetWMHooksWindowChanged(), msg, (LPARAM)hwnd);
}

bool CWindowHooks::NotifyWindowBorder(HWND hwnd, UINT msg)
{
	return NotifyHookOwner(GetWMHooksWindowBorderChanged(), msg, (LPARAM)hwnd);
}

bool CWindowHooks::NotifyWindowClientArea(HWND hwnd, UINT msg)
{
	return NotifyHookOwner(GetWMHooksWindowClientAreaChanged(), msg, (LPARAM)hwnd);
}

bool CWindowHooks::NotifyRectangle(RECT* rect)
{
	WPARAM w = MAKELONG((SHORT)rect->left, (SHORT)rect->top);
	LPARAM l = MAKELONG((SHORT)rect->right, (SHORT)rect->bottom);
	return NotifyHookOwner(GetWMHooksRectangleChanged(), w, l);
}

bool CWindowHooks::NotifyCursor(HCURSOR cursor)
{
	return NotifyHookOwner(GetWMHooksCursorChanged(), 0, (LPARAM)cursor);
}

UINT CWindowHooks::GetWMHooksWindowChanged()
{
	static UINT WM_Hooks_WindowChanged = RegisterWindowMessage(_T("RD.WM_Hooks.WindowChanged"));
	return WM_Hooks_WindowChanged;
}

UINT CWindowHooks::GetWMHooksWindowClientAreaChanged()
{
	static UINT WM_Hooks_WindowClientAreaChanged = RegisterWindowMessage(_T("RD.WM_Hooks.WindowClientAreaChanged"));
	return WM_Hooks_WindowClientAreaChanged;
}

UINT CWindowHooks::GetWMHooksWindowBorderChanged()
{
	static UINT WM_Hooks_WindowBorderChanged = RegisterWindowMessage(_T("RD.WM_Hooks.WindowBorderChanged"));
	return WM_Hooks_WindowBorderChanged;
}

UINT CWindowHooks::GetWMHooksRectangleChanged()
{
	static UINT WM_Hooks_RectangleChanged = RegisterWindowMessage(_T("RD.WM_Hooks.RectangleChanged"));
	return WM_Hooks_RectangleChanged;
}

UINT CWindowHooks::GetWMHooksCursorChanged()
{
	static UINT WM_Hooks_CursorChanged = RegisterWindowMessage(_T("RD.WM_Hooks.CursorChanged"));
	return WM_Hooks_CursorChanged;
}

ATOM CWindowHooks::GetPopupSelectionATOM()
{
	static ATOM ATOM_Popup_Selection = GlobalAddAtom(_T("RD.WM_Hooks.PopupSelectionAtom"));
	return ATOM_Popup_Selection;
}

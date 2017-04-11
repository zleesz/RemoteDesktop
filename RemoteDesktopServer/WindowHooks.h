#pragma once

class CWindowHooks
{
public:
	CWindowHooks(void);
	virtual ~CWindowHooks(void);

public:
	static CWindowHooks* GetInstance()
	{
		if (s_pWindowHooks == NULL)
		{
			s_pWindowHooks = new CWindowHooks;
		}
		return s_pWindowHooks;
	}
	static void DestroyInstance()
	{
		if (s_pWindowHooks)
		{
			delete s_pWindowHooks;
		}
	}
	bool Install(HWND hOwnerWnd, DWORD dwHookThreadId);
	void Uninstall();

private:
	static LRESULT CALLBACK HookCallWndProc(int nCode, WPARAM wParam, LPARAM lParam);
	static LRESULT CALLBACK HookCallWndProcRet(int nCode, WPARAM wParam, LPARAM lParam);
	static LRESULT CALLBACK HookGetMessage(int nCode, WPARAM wParam, LPARAM lParam);
	static LRESULT CALLBACK HookDialogMessage(int nCode, WPARAM wParam, LPARAM lParam);
	static void ProcessWindowMessage(UINT msg, HWND wnd, WPARAM wParam, LPARAM lParam);

	static bool NotifyHookOwner(UINT event, WPARAM wParam, LPARAM lParam);
	static bool NotifyWindow(HWND hwnd, UINT msg);
	static bool NotifyWindowBorder(HWND hwnd, UINT msg);
	static bool NotifyWindowClientArea(HWND hwnd, UINT msg);
	static bool NotifyRectangle(RECT* rect);
	static bool NotifyCursor(HCURSOR cursor);

public:
	static UINT GetWMHooksWindowChanged();
	static UINT GetWMHooksWindowClientAreaChanged();
	static UINT GetWMHooksWindowBorderChanged();
	static UINT GetWMHooksRectangleChanged();
	static UINT GetWMHooksCursorChanged();
	static ATOM GetPopupSelectionATOM();

private:
	static HWND		s_hOwnerWnd;
	static DWORD	s_dwHookThreadId;
	static HHOOK	s_hCallWndProc;
	static HHOOK	s_hCallWndProcRet;
	static HHOOK	s_hGetMessage;
	static HHOOK	s_hDialogMessage;

	static CWindowHooks*	s_pWindowHooks;
};

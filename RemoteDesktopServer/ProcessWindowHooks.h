#pragma once
#include "WindowHooks.h"

class CProcessWindowHooks :
	public CWindowImpl<CProcessWindowHooks>
{
public:
	CProcessWindowHooks(void);
	virtual ~CProcessWindowHooks(void);

	BEGIN_MSG_MAP(CProcessWindowHooks)
		MESSAGE_HANDLER(WM_TIMER, OnTimer)
		MESSAGE_HANDLER(CWindowHooks::GetWMHooksWindowChanged(), OnHooksWindowChanged)
		MESSAGE_HANDLER(CWindowHooks::GetWMHooksWindowClientAreaChanged(), OnHooksWindowClientAreaChanged)
		MESSAGE_HANDLER(CWindowHooks::GetWMHooksWindowBorderChanged(), OnHooksWindowBorderChanged)
		MESSAGE_HANDLER(CWindowHooks::GetWMHooksRectangleChanged(), OnHooksHooksRectangleChanged)
		MESSAGE_HANDLER(CWindowHooks::GetWMHooksCursorChanged(), OnHooksHooksCursorChanged)
	END_MSG_MAP()

protected:
	LRESULT OnTimer(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	LRESULT OnHooksWindowChanged(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	LRESULT OnHooksWindowClientAreaChanged(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	LRESULT OnHooksWindowBorderChanged(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	LRESULT OnHooksHooksRectangleChanged(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	LRESULT OnHooksHooksCursorChanged(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);

private:
	static unsigned __stdcall DoHooks(void* pParam);

public:
	HWND GetHWND();
	bool Start();
	void Stop();

private:
	HANDLE	m_hThread;
	HANDLE	m_hEvent;
};

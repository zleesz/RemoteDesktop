#pragma once
#include <atlwin.h>
#include <atltypes.h>
#include "TrackerHttpServer.h"

class CMainWindow :
	public CWindowImpl<CMainWindow>,
	public ITrackerEvent
{
public:
	CMainWindow(void);
	~CMainWindow(void);

	BEGIN_MSG_MAP(CMainWindow)
		MESSAGE_HANDLER(WM_CLOSE, OnClose)
		MESSAGE_HANDLER(WM_CREATE, OnCreate)
		MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
		MESSAGE_HANDLER(WM_ERASEBKGND, OnEraseBackground)
		MESSAGE_HANDLER(WM_PAINT, OnPaint)
	END_MSG_MAP()

protected:
	LRESULT OnClose(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	LRESULT OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	LRESULT OnDestroy(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	LRESULT OnEraseBackground(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	LRESULT OnPaint(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);

	virtual void OnPeerAvailable(const char* peer, unsigned short port);
	virtual void OnPeerLive(const char* peer, unsigned short port);
	virtual void OnPeerOffline(const char* peer, unsigned short port);

public:
	HWND Create(CRect& rcBound, int nCmdShow);
	void Destroy();

private:
	CTrackerHttpServer* m_pTrackerHttpServer;
};

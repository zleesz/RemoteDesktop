#pragma once
#include <atlwin.h>
#include <atltypes.h>

#include "RemoteServer.h"
#include "ScreenBitmap.h"

class CMainWindow :
	public CWindowImpl<CMainWindow>,
	public IRemoteServerEvent,
	public IScreenBitmapEvent
{
public:
	CMainWindow(void);
	virtual ~CMainWindow(void);

	BEGIN_MSG_MAP(CMainWindow)
		MESSAGE_HANDLER(WM_CLOSE, OnClose)
		MESSAGE_HANDLER(WM_CREATE, OnCreate)
		MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
		MESSAGE_HANDLER(WM_PAINT, OnPaint)
		MESSAGE_HANDLER(WM_ERASEBKGND, OnEraseBackground)
	END_MSG_MAP()

protected:
	LRESULT OnClose(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	LRESULT OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	LRESULT OnDestroy(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	LRESULT OnPaint(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	LRESULT OnEraseBackground(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);

public:
	HWND Create(CRect& rcBound, int nCmdShow);
	void Destroy();

	virtual void OnStart();
	virtual void OnStop();
	virtual void OnConnect(CClientConnection* pClientConnection);
	virtual void OnDisconnect(CClientConnection* pClientConnection, RD_ERROR_CODE errorCode);

	virtual void OnFirstBitmap(BitmapInfo* pBitmapInfo, WORD wPixelBytes, unsigned char *bitmapBits);
	virtual void OnModified(unsigned int nModifiedBlockCount, unsigned int* pnModifiedBlocks);

private:
	CRemoteServer						m_RemoteServer;
	CAutoAddReleasePtr<CScreenBitmap>	m_spScreenBitmap;
};

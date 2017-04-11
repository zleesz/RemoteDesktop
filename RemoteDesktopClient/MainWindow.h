#pragma once
#include <atlwin.h>
#include <atltypes.h>
#include "SimpleButton.h"

#include "RemoteClient.h"

class CMainWindow :
	public CWindowImpl<CMainWindow>,
	public IRemoteClientEvent
{
public:
	CMainWindow(void);
	virtual ~CMainWindow(void);

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

public:
	HWND Create(CRect& rcBound, int nCmdShow);
	void Destroy();

	virtual void OnConnected();
	virtual void OnStateChanged(RD_CONNECTION_STATE state);
	virtual void OnGetLogonResult(int nResult);
	virtual void OnGetTransferBitmap(BitmapInfo& bitmapInfo, WORD wPixelBytes, unsigned char* pBuffer);
	virtual void OnGetTransferModifyBitmap(unsigned int nModifiedBlockCount, unsigned int* pnModifiedBlocks, unsigned int nBufferSize, unsigned char* pBuffer);
	virtual void OnGetDisconnect(int nDisconnectCode);

	void GetBitmapInfo(BitmapInfo* pBitmapInfo);
	unsigned char* GetBuffer(unsigned int x, unsigned int y);
	WORD GetPixelBytes() const;

private:
	void DrawRemoteDesktop(HDC hDC);

private:
	CSimpleButton	m_btnSetting;
	CRemoteClient	m_remoteClient;
	BitmapInfo		m_bitmapInfo;
	unsigned char*	m_pBuffer;
	WORD			m_wPixelBytes;
	unsigned int	m_nScanPointRowCount;
	unsigned int	m_nScanPointColumnCount;
};

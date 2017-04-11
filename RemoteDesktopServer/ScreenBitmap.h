#pragma once
#include <atlwin.h>
#include <atltypes.h>
#include "..\common\def.h"
#include "..\common\lock.h"
#include "..\common\AutoAddReleasePtr.h"

#define WM_SCREENBITMAP_MODIFIED		(WM_USER+1)
#define WM_SCREENBITMAP_FIRSTBITMAP		(WM_USER+2)

class IScreenBitmapEvent
{
public:
	virtual void OnFirstBitmap(BitmapInfo* pBitmapInfo, WORD wPixelBytes, unsigned char *bitmapBits) = 0;
	virtual void OnModified(unsigned int nModifiedBlockCount, unsigned int* pnModifiedBlocks) = 0;
};

class CScreenBitmap :
	public CAddReleaseRef,
	public CWindowImpl<CScreenBitmap>
{
public:
	CScreenBitmap(void);
	~CScreenBitmap(void);

public:
	static CScreenBitmap* GetInstance()
	{
		if (s_pScreenBitmap == NULL)
		{
			s_pScreenBitmap = new CScreenBitmap();
			s_pScreenBitmap->AddRef();
		}
		return s_pScreenBitmap;
	}

	BEGIN_MSG_MAP(CScreenBitmap)
		MESSAGE_HANDLER(WM_TIMER, OnTimer)
		MESSAGE_HANDLER(WM_SCREENBITMAP_MODIFIED, OnModified)
		MESSAGE_HANDLER(WM_SCREENBITMAP_FIRSTBITMAP, OnFirstBitmap)
	END_MSG_MAP()

private:
	static unsigned __stdcall DoStartCapture(void * pParam);
	LRESULT OnTimer(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	LRESULT OnModified(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	LRESULT OnFirstBitmap(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	void CaptureDesktop();
	void GetModifiedBlocks(unsigned char* pNewBuffer, unsigned char* pLastBuffer, unsigned int* pnModifiedBlockCount, unsigned int** ppnModifiedBlocks);

public:
	void StartCapture(IScreenBitmapEvent* pScreenBitmapEvent);
	void StopCapture();
	bool IsCapturing() const;
	CRect GetRect() const;
	void GetBitmapInfo(BitmapInfo* pBitmapInfo);
	unsigned char* GetBuffer(unsigned int x, unsigned int y);
	WORD GetPixelBytes() const;
	bool ClipBitmapRect(CRect* pClipRect, unsigned char** ppClipBuffer, unsigned int* pLength);

private:
	unsigned				m_uThreadId;
	CRect					m_rcScreen;
	BitmapInfo				m_BitmapInfo;
	unsigned char*			m_pBuffer;
	WORD					m_wPixelBytes;
	IScreenBitmapEvent*		m_pScreenBitmapEvent;
	static CScreenBitmap*	s_pScreenBitmap;
	unsigned int			m_nScanPointRowCount;
	unsigned int			m_nScanPointColumnCount;
	CLock					m_lock;
};

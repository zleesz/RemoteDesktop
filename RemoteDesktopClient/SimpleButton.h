#pragma once
#include <Windows.h>

class CSimpleButton
{
public:
	CSimpleButton(void);
	virtual ~CSimpleButton(void);

public:
	void Load(const char* pszNormal, const char* pszHover, const char* pszDown, const char* pszDisable = NULL);
	void Render(HDC hDC);

private:
	HBITMAP m_hNormal;
	HBITMAP m_hHover;
	HBITMAP m_hDown;
	HBITMAP m_hDisable;

	BOOL m_bDisable;
};

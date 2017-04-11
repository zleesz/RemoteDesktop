#include "stdafx.h"
#include "SimpleButton.h"
#include "PngLoader.h"

CSimpleButton::CSimpleButton(void) :
	m_hNormal(NULL),
	m_hHover(NULL),
	m_hDown(NULL),
	m_hDisable(NULL),
	m_bDisable(FALSE)
{
}

CSimpleButton::~CSimpleButton(void)
{
}

void CSimpleButton::Load(const char* pszNormal, const char* pszHover, const char* pszDown, const char* pszDisable)
{
	if (pszNormal)
	{
		int nWidth = 0, nHeight = 0;
		unsigned char *cbData = NULL;
		long nRet = PngLoader::ReadPngData("op.png", &nWidth, &nHeight, &cbData);
		if (nRet > 0)
		{
			m_hNormal = ::CreateBitmap(nWidth, nHeight, 32, 1, cbData);
		}
	}
	if (pszHover)
	{
		int nWidth = 0, nHeight = 0;
		unsigned char *cbData = NULL;
		long nRet = PngLoader::ReadPngData("op.png", &nWidth, &nHeight, &cbData);
		if (nRet > 0)
		{
			m_hHover = ::CreateBitmap(nWidth, nHeight, 32, 1, cbData);
		}
	}
	if (pszDown)
	{
		int nWidth = 0, nHeight = 0;
		unsigned char *cbData = NULL;
		long nRet = PngLoader::ReadPngData("op.png", &nWidth, &nHeight, &cbData);
		if (nRet > 0)
		{
			m_hDown = ::CreateBitmap(nWidth, nHeight, 32, 1, cbData);
		}
	}
	if (pszDisable)
	{
		int nWidth = 0, nHeight = 0;
		unsigned char *cbData = NULL;
		long nRet = PngLoader::ReadPngData("op.png", &nWidth, &nHeight, &cbData);
		if (nRet > 0)
		{
			m_hDisable = ::CreateBitmap(nWidth, nHeight, 32, 1, cbData);
		}
	}
}

void CSimpleButton::Render(HDC hDC)
{

}

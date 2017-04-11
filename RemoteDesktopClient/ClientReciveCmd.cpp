#include "stdafx.h"
#include "ClientReciveCmd.h"
#include "RemoteClient.h"
#include "..\common\jpeg.h"

//////////////////////////////////////////////////////////////////////////
// CClientReciveCmdLogonResult
//////////////////////////////////////////////////////////////////////////
CClientReciveCmdLogonResult::CClientReciveCmdLogonResult()
{

}

CClientReciveCmdLogonResult::CClientReciveCmdLogonResult(CommandData& commandData, IClientReciveCmdLogonResultEvent* pClientReciveCmdLogonResultEvent) : m_nResult(-1)
{
	Parse(commandData, pClientReciveCmdLogonResultEvent);
}

CClientReciveCmdLogonResult::~CClientReciveCmdLogonResult()
{
}

RD_CMD_CODE CClientReciveCmdLogonResult::GetCode()
{
	return RDCC_LOGON_RESULT;
}

bool CClientReciveCmdLogonResult::Parse(CommandData& commandData, void* pCallBack)
{
	stream_buffer buffer((void*)commandData.data.c_str(), commandData.data.length());
	int ret = buffer.ReadInt32(m_nResult);
	CLIENT_RECIVE_CMD_RETURN_IFFAILED(ret);

	CRemoteClient* pRemoteClient = (CRemoteClient*)pCallBack;
	IClientReciveCmdLogonResultEvent* pClientReciveCmdLogonResultEvent = static_cast<IClientReciveCmdLogonResultEvent*>(pRemoteClient);
	if (pClientReciveCmdLogonResultEvent)
	{
		pClientReciveCmdLogonResultEvent->OnGetLogonResult(m_nResult);
	}

	return true;
}

int CClientReciveCmdLogonResult::GetResult()
{
	return m_nResult;
}

//////////////////////////////////////////////////////////////////////////
// CClientReciveCmdTransferBitmap
//////////////////////////////////////////////////////////////////////////
CClientReciveCmdTransferBitmap::CClientReciveCmdTransferBitmap() : m_pBuffer(NULL), m_strSuffix(BITMAP_SUFFIX_BMP)
{

}

CClientReciveCmdTransferBitmap::CClientReciveCmdTransferBitmap(CommandData& commandData, IClientReciveCmdTransferBitmapEvent* pClientReciveCmdTransferBitmapEvent) : m_pBuffer(NULL), m_strSuffix(BITMAP_SUFFIX_BMP)
{
	Parse(commandData, pClientReciveCmdTransferBitmapEvent);
}

RD_CMD_CODE CClientReciveCmdTransferBitmap::GetCode()
{
	return RDCC_TRANSFER_BITMAP;
}

CClientReciveCmdTransferBitmap::~CClientReciveCmdTransferBitmap()
{
	if (m_pBuffer)
	{
		delete[] m_pBuffer;
		m_pBuffer = NULL;
	}
}

bool CClientReciveCmdTransferBitmap::Parse(CommandData& commandData, void* pCallBack)
{
	CRemoteClient* pRemoteClient = (CRemoteClient*)pCallBack;
	IClientReciveCmdTransferBitmapEvent* pClientReciveCmdTransferBitmapEvent = static_cast<IClientReciveCmdTransferBitmapEvent*>(pRemoteClient);
	if (pClientReciveCmdTransferBitmapEvent)
	{
		pClientReciveCmdTransferBitmapEvent->GetSuffix(m_strSuffix);
	}

	stream_buffer buffer((void*)commandData.data.c_str(), commandData.data.length());

	if (m_strSuffix == BITMAP_SUFFIX_BMP)
	{
		uint32_t len = sizeof(BitmapInfo);
		int ret = buffer.ReadStringUTF8((char*)&m_bitmapInfo, len);
		CLIENT_RECIVE_CMD_RETURN_IFFAILED(ret);

		ret = buffer.ReadUInt16((uint16_t&)m_wPixelBytes);
		CLIENT_RECIVE_CMD_RETURN_IFFAILED(ret);
		
		len = 0;
		ret = buffer.ReadUInt32(len);
		CLIENT_RECIVE_CMD_RETURN_IFFAILED(ret);

		m_pBuffer = new unsigned char[len];
		ret = buffer.ReadStringUTF8((char*)m_pBuffer, len);
		CLIENT_RECIVE_CMD_RETURN_IFFAILED(ret);
	}
	else if (m_strSuffix == BITMAP_SUFFIX_JPEG)
	{
		int ret = buffer.ReadUInt16((uint16_t&)m_wPixelBytes);
		CLIENT_RECIVE_CMD_RETURN_IFFAILED(ret);

		uint32_t len = 0;
		ret = buffer.ReadUInt32(len);
		CLIENT_RECIVE_CMD_RETURN_IFFAILED(ret);

		m_pBuffer = new unsigned char[len];
		ret = buffer.ReadStringUTF8((char*)m_pBuffer, len);
		CLIENT_RECIVE_CMD_RETURN_IFFAILED(ret);

		JPEG::Decompress(m_pBuffer, len, &m_bitmapInfo, &m_pBuffer);
	}

	if (pClientReciveCmdTransferBitmapEvent)
	{
		pClientReciveCmdTransferBitmapEvent->OnGetTransferBitmap(m_bitmapInfo, m_wPixelBytes, m_pBuffer);
	}
	return true;
}

BitmapInfo CClientReciveCmdTransferBitmap::GetBitmapInfo()
{
	return m_bitmapInfo;
}

unsigned char* CClientReciveCmdTransferBitmap::GetBuffer()
{
	return m_pBuffer;
}

WORD CClientReciveCmdTransferBitmap::GetPixelBytes()
{
	return m_wPixelBytes;
}

//////////////////////////////////////////////////////////////////////////
// CClientReciveCmdTransferModifyBitmap
//////////////////////////////////////////////////////////////////////////
CClientReciveCmdTransferModifyBitmap::CClientReciveCmdTransferModifyBitmap() : m_pBuffer(NULL), m_nModifiedBlockCount(0), m_pnModifiedBlocks(NULL)
{

}

CClientReciveCmdTransferModifyBitmap::CClientReciveCmdTransferModifyBitmap(CommandData& commandData, IClientReciveCmdTransferModifyBitmapEvent* pClientReciveCmdTransferModifyBitmapEvent) : m_pBuffer(NULL), m_nModifiedBlockCount(0), m_pnModifiedBlocks(NULL)
{
	Parse(commandData, pClientReciveCmdTransferModifyBitmapEvent);
}

CClientReciveCmdTransferModifyBitmap::~CClientReciveCmdTransferModifyBitmap()
{
	if (m_pBuffer)
	{
		delete[] m_pBuffer;
		m_pBuffer = NULL;
	}
	if (m_pnModifiedBlocks)
	{
		delete[] m_pnModifiedBlocks;
		m_pnModifiedBlocks = NULL;
	}
}

RD_CMD_CODE CClientReciveCmdTransferModifyBitmap::GetCode()
{
	return RDCC_TRANSFER_MODIFY_BITMAP;
}

bool CClientReciveCmdTransferModifyBitmap::Parse(CommandData& commandData, void* pCallBack)
{
	CRemoteClient* pRemoteClient = (CRemoteClient*)pCallBack;
	IClientReciveCmdTransferBitmapEvent* pClientReciveCmdTransferBitmapEvent = static_cast<IClientReciveCmdTransferBitmapEvent*>(pRemoteClient);
	if (pClientReciveCmdTransferBitmapEvent)
	{
		pClientReciveCmdTransferBitmapEvent->GetSuffix(m_strSuffix);
	}

	stream_buffer buffer((void*)commandData.data.c_str(), commandData.data.length());

	int ret = buffer.ReadInt32((int32_t&)m_nModifiedBlockCount);
	CLIENT_RECIVE_CMD_RETURN_IFFAILED(ret);

	m_pnModifiedBlocks = new unsigned int[m_nModifiedBlockCount];
	ZeroMemory(m_pnModifiedBlocks, m_nModifiedBlockCount * sizeof(unsigned int));

	uint32_t nlen = m_nModifiedBlockCount * sizeof(unsigned int);
	ret = buffer.ReadStringUTF8((char*)m_pnModifiedBlocks, (uint32_t&)nlen);
	CLIENT_RECIVE_CMD_RETURN_IFFAILED(ret);

	ret = buffer.ReadUInt32(m_nBufferSize);
	CLIENT_RECIVE_CMD_RETURN_IFFAILED(ret && m_nBufferSize > 0);

	m_pBuffer = new unsigned char[m_nBufferSize];
	ret = buffer.ReadStringUTF8((char*)m_pBuffer, m_nBufferSize);
	CLIENT_RECIVE_CMD_RETURN_IFFAILED(ret);

	if (m_strSuffix == BITMAP_SUFFIX_JPEG)
	{
		BitmapInfo bitmapInfo;
		JPEG::Decompress(m_pBuffer, m_nBufferSize, &bitmapInfo, &m_pBuffer);
		m_nBufferSize = bitmapInfo.bmiHeader.biSizeImage;
	}

	IClientReciveCmdTransferModifyBitmapEvent* pClientReciveCmdTransferModifyBitmapEvent = static_cast<IClientReciveCmdTransferModifyBitmapEvent*>(pRemoteClient);
	if (pClientReciveCmdTransferModifyBitmapEvent)
	{
		pClientReciveCmdTransferModifyBitmapEvent->OnGetTransferModifyBitmap(m_nModifiedBlockCount, m_pnModifiedBlocks, m_nBufferSize, m_pBuffer);
	}

	return true;
}

unsigned int CClientReciveCmdTransferModifyBitmap::GetModifiedBlockCount()
{
	return m_nModifiedBlockCount;
}

unsigned int* CClientReciveCmdTransferModifyBitmap::GetModifiedBlocks()
{
	return m_pnModifiedBlocks;
}

unsigned int CClientReciveCmdTransferModifyBitmap::GetBufferSize()
{
	return m_nBufferSize;
}

unsigned char* CClientReciveCmdTransferModifyBitmap::GetBuffer()
{
	return m_pBuffer;
}

//////////////////////////////////////////////////////////////////////////
// CClientReciveCmdDisconnect
//////////////////////////////////////////////////////////////////////////
CClientReciveCmdDisconnect::CClientReciveCmdDisconnect() : m_nDisconnectCode(0)
{

}

CClientReciveCmdDisconnect::CClientReciveCmdDisconnect(CommandData& commandData, IClientReciveCmdDisconnectEvent* pClientReciveCmdDisconnectEvent) : m_nDisconnectCode(0)
{
	Parse(commandData, pClientReciveCmdDisconnectEvent);
}

CClientReciveCmdDisconnect::~CClientReciveCmdDisconnect()
{

}

RD_CMD_CODE CClientReciveCmdDisconnect::GetCode()
{
	return RDCC_DISCONNECT;
}

bool CClientReciveCmdDisconnect::Parse(CommandData& commandData, void* pCallBack)
{
	stream_buffer buffer((void*)commandData.data.c_str(), commandData.data.length());
	int ret = buffer.ReadInt32((int32_t&)m_nDisconnectCode);
	CLIENT_RECIVE_CMD_RETURN_IFFAILED(ret);

	CRemoteClient* pRemoteClient = (CRemoteClient*)pCallBack;
	IClientReciveCmdDisconnectEvent* pClientReciveCmdDisconnectEvent = static_cast<IClientReciveCmdDisconnectEvent*>(pRemoteClient);
	if (pClientReciveCmdDisconnectEvent)
	{
		pClientReciveCmdDisconnectEvent->OnGetDisconnect(m_nDisconnectCode);
	}

	return true;
}

int CClientReciveCmdDisconnect::GetDisconnectCode()
{
	return m_nDisconnectCode;
}

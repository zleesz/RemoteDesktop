#include "stdafx.h"
#include "ServerReciveCmd.h"
#include "ClientConnection.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////////
// CServerReciveCmdProtocalVersion
//////////////////////////////////////////////////////////////////////////
CServerReciveCmdProtocalVersion::CServerReciveCmdProtocalVersion()
{

}

CServerReciveCmdProtocalVersion::CServerReciveCmdProtocalVersion(CommandData& commandData, IServerReciveCmdProtocalVersionEvent* pServerReciveCmdProtocalVersionEvent) : m_strProtocalVersion(RD_PROTOCAL_VERSION)
{
	Parse(commandData, pServerReciveCmdProtocalVersionEvent);
}

CServerReciveCmdProtocalVersion::~CServerReciveCmdProtocalVersion()
{
}

RD_CMD_CODE CServerReciveCmdProtocalVersion::GetCode()
{
	return RDCC_PROTOCOL_VERSION;
}

bool CServerReciveCmdProtocalVersion::Parse(CommandData& commandData, void* pCallBack)
{
	stream_buffer buffer((void*)commandData.data.c_str(), commandData.data.length());
	uint32_t nLength = 0;
	int ret = buffer.ReadUInt32(nLength);
	SERVER_RECIVE_CMD_RETURN_IFFAILED(ret);
	ret = buffer.ReadStringUTF8(m_strProtocalVersion, nLength);
	SERVER_RECIVE_CMD_RETURN_IFFAILED(ret);

	nLength = 0;
	ret = buffer.ReadUInt32(nLength);
	SERVER_RECIVE_CMD_RETURN_IFFAILED(ret);
	ret = buffer.ReadStringUTF8(m_strClientVersion, nLength);
	SERVER_RECIVE_CMD_RETURN_IFFAILED(ret);

	CClientConnection* pClientConnection = (CClientConnection*)pCallBack;
	pClientConnection->AddRef();
	IServerReciveCmdProtocalVersionEvent* pServerReciveCmdProtocalVersionEvent = static_cast<IServerReciveCmdProtocalVersionEvent*>(pClientConnection);
	if (pServerReciveCmdProtocalVersionEvent)
	{
		pServerReciveCmdProtocalVersionEvent->OnGetProtocalVersion(m_strProtocalVersion, m_strClientVersion);
	}
	pClientConnection->Release();
	return true;
}

std::string CServerReciveCmdProtocalVersion::GetProtocalVersion()
{
	return m_strProtocalVersion;
}
std::string CServerReciveCmdProtocalVersion::GetClientVersion()
{
	return m_strClientVersion;
}

//////////////////////////////////////////////////////////////////////////
// CServerReciveCmdLogon
//////////////////////////////////////////////////////////////////////////
CServerReciveCmdLogon::CServerReciveCmdLogon()
{

}

CServerReciveCmdLogon::CServerReciveCmdLogon(CommandData& commandData, IServerReciveCmdLogonEvent* pServerReciveCmdLogonEvent)
{
	Parse(commandData, pServerReciveCmdLogonEvent);
}

RD_CMD_CODE CServerReciveCmdLogon::GetCode()
{
	return RDCC_LOGON;
}

bool CServerReciveCmdLogon::Parse(CommandData& commandData, void* pCallBack)
{
	stream_buffer buffer((void*)commandData.data.c_str(), commandData.data.length());
	uint32_t nLength = 0;
	int ret = buffer.ReadUInt32(nLength);
	SERVER_RECIVE_CMD_RETURN_IFFAILED(ret);

	ret = buffer.ReadStringUTF8(m_strUserName, nLength);
	SERVER_RECIVE_CMD_RETURN_IFFAILED(ret);

	nLength = 0;
	ret = buffer.ReadUInt32(nLength);
	SERVER_RECIVE_CMD_RETURN_IFFAILED(ret);

	ret = buffer.ReadStringUTF8(m_strPassword, nLength);
	SERVER_RECIVE_CMD_RETURN_IFFAILED(ret);

	CClientConnection* pClientConnection = (CClientConnection*)pCallBack;
	pClientConnection->AddRef();
	IServerReciveCmdLogonEvent* pServerReciveCmdLogonEvent = static_cast<IServerReciveCmdLogonEvent*>(pClientConnection);
	if (pServerReciveCmdLogonEvent)
	{
		pServerReciveCmdLogonEvent->OnGetLogon(m_strUserName, m_strPassword);
	}
	pClientConnection->Release();
	return true;
}

std::string CServerReciveCmdLogon::GetUserName()
{
	return m_strUserName;
}
std::string CServerReciveCmdLogon::GetPassword()
{
	return m_strPassword;
}

//////////////////////////////////////////////////////////////////////////
// CServerReciveCmdCompressMode
//////////////////////////////////////////////////////////////////////////
CServerReciveCmdCompressMode::CServerReciveCmdCompressMode()
{

}

CServerReciveCmdCompressMode::CServerReciveCmdCompressMode(CommandData& commandData, IServerReciveCmdCompressModeEvent* pServerReciveCmdCompressModeEvent) : m_mode(RDCM_RAW)
{
	Parse(commandData, pServerReciveCmdCompressModeEvent);
}

RD_CMD_CODE CServerReciveCmdCompressMode::GetCode()
{
	return RDCC_COMPRESS_MODE;
}

bool CServerReciveCmdCompressMode::Parse(CommandData& commandData, void* pCallBack)
{
	stream_buffer buffer((void*)commandData.data.c_str(), commandData.data.length());
	int ret = buffer.ReadInt32((int32_t&)m_mode);
	SERVER_RECIVE_CMD_RETURN_IFFAILED(ret);

	CClientConnection* pClientConnection = (CClientConnection*)pCallBack;
	pClientConnection->AddRef();
	IServerReciveCmdCompressModeEvent* pServerReciveCmdCompressModeEvent = static_cast<IServerReciveCmdCompressModeEvent*>(pClientConnection);
	if (pServerReciveCmdCompressModeEvent)
	{
		pServerReciveCmdCompressModeEvent->OnGetCompressMode(m_mode);
	}
	pClientConnection->Release();
	return true;
}

RD_COMPRESS_MODE CServerReciveCmdCompressMode::GetMode()
{
	return m_mode;
}

//////////////////////////////////////////////////////////////////////////
// CServerReciveCmdTransferBitmap
//////////////////////////////////////////////////////////////////////////
CServerReciveCmdTransferBitmap::CServerReciveCmdTransferBitmap()
{

}

CServerReciveCmdTransferBitmap::CServerReciveCmdTransferBitmap(CommandData& commandData, IServerReciveCmdTransferBitmapEvent* pServerReciveCmdTransferBitmapEvent)
{
	Parse(commandData, pServerReciveCmdTransferBitmapEvent);
}

RD_CMD_CODE CServerReciveCmdTransferBitmap::GetCode()
{
	return RDCC_TRANSFER_BITMAP;
}

bool CServerReciveCmdTransferBitmap::Parse(CommandData& commandData, void* pCallBack)
{
	stream_buffer buffer((void*)commandData.data.c_str(), commandData.data.length());
	uint32_t nLength = 0;
	int ret = buffer.ReadUInt32(nLength);
	SERVER_RECIVE_CMD_RETURN_IFFAILED(ret);

	ret = buffer.ReadStringUTF8(m_strSuffix, nLength);
	SERVER_RECIVE_CMD_RETURN_IFFAILED(ret);

	CClientConnection* pClientConnection = (CClientConnection*)pCallBack;
	pClientConnection->AddRef();
	IServerReciveCmdTransferBitmapEvent* pServerReciveCmdTransferBitmapEvent = static_cast<IServerReciveCmdTransferBitmapEvent*>(pClientConnection);
	if (pServerReciveCmdTransferBitmapEvent)
	{
		pServerReciveCmdTransferBitmapEvent->OnGetTransferBitmap(m_strSuffix);
	}
	pClientConnection->Release();
	return true;
}

std::string CServerReciveCmdTransferBitmap::GetSuffix()
{
	return m_strSuffix;
}

//////////////////////////////////////////////////////////////////////////
// CServerReciveCmdTransferBitmapResponse
//////////////////////////////////////////////////////////////////////////
CServerReciveCmdTransferBitmapResponse::CServerReciveCmdTransferBitmapResponse()
{

}

CServerReciveCmdTransferBitmapResponse::CServerReciveCmdTransferBitmapResponse(CommandData& commandData, IServerReciveCmdTransferBitmapResponseEvent* pServerReciveCmdTransferBitmapResponseEvent) : m_nResult(0)
{
	Parse(commandData, pServerReciveCmdTransferBitmapResponseEvent);
}

RD_CMD_CODE CServerReciveCmdTransferBitmapResponse::GetCode()
{
	return RDCC_TRANSFER_BITMAP_RESPONSE;
}

bool CServerReciveCmdTransferBitmapResponse::Parse(CommandData& commandData, void* pCallBack)
{
	stream_buffer buffer((void*)commandData.data.c_str(), commandData.data.length());
	int ret = buffer.ReadInt32((int32_t&)m_nResult);
	SERVER_RECIVE_CMD_RETURN_IFFAILED(ret);

	CClientConnection* pClientConnection = (CClientConnection*)pCallBack;
	pClientConnection->AddRef();
	IServerReciveCmdTransferBitmapResponseEvent* pServerReciveCmdTransferBitmapResponseEvent = static_cast<IServerReciveCmdTransferBitmapResponseEvent*>(pClientConnection);
	if (pServerReciveCmdTransferBitmapResponseEvent)
	{
		pServerReciveCmdTransferBitmapResponseEvent->OnGetTransferBitmapResult(m_nResult);
	}
	pClientConnection->Release();
	return true;
}

int CServerReciveCmdTransferBitmapResponse::GetResult()
{
	return m_nResult;
}

//////////////////////////////////////////////////////////////////////////
// CServerReciveCmdTransferMouseEvent
//////////////////////////////////////////////////////////////////////////
CServerReciveCmdTransferMouseEvent::CServerReciveCmdTransferMouseEvent()
{

}

CServerReciveCmdTransferMouseEvent::CServerReciveCmdTransferMouseEvent(CommandData& commandData, IServerReciveCmdTransferMouseEventEvent* pServerReciveCmdTransferMouseEventEvent) : m_dwFlags(MOUSEEVENTF_MOVE), m_pt(0, 0), m_dwData(0)
{
	Parse(commandData, pServerReciveCmdTransferMouseEventEvent);
}

RD_CMD_CODE CServerReciveCmdTransferMouseEvent::GetCode()
{
	return RDCC_TRANSFER_MOUSE_EVENT;
}

bool CServerReciveCmdTransferMouseEvent::Parse(CommandData& commandData, void* pCallBack)
{
	stream_buffer buffer((void*)commandData.data.c_str(), commandData.data.length());
	int ret = buffer.ReadUInt32((uint32_t&)m_dwFlags);
	SERVER_RECIVE_CMD_RETURN_IFFAILED(ret);

	ret = buffer.ReadInt32((int32_t&)m_pt.x);
	SERVER_RECIVE_CMD_RETURN_IFFAILED(ret);

	ret = buffer.ReadInt32((int32_t&)m_pt.y);
	SERVER_RECIVE_CMD_RETURN_IFFAILED(ret);

	ret = buffer.ReadUInt32((uint32_t&)m_dwData);
	SERVER_RECIVE_CMD_RETURN_IFFAILED(ret);

	CClientConnection* pClientConnection = (CClientConnection*)pCallBack;
	pClientConnection->AddRef();
	IServerReciveCmdTransferMouseEventEvent* pServerReciveCmdTransferMouseEventEvent = static_cast<IServerReciveCmdTransferMouseEventEvent*>(pClientConnection);
	if (pServerReciveCmdTransferMouseEventEvent)
	{
		pServerReciveCmdTransferMouseEventEvent->OnGetTransferMouseEvent(m_dwFlags, m_pt, m_dwData);
	}
	pClientConnection->Release();
	return true;
}

DWORD CServerReciveCmdTransferMouseEvent::GetFlags()
{
	return m_dwFlags;
}

CPoint CServerReciveCmdTransferMouseEvent::GetPoint()
{
	return m_pt;
}

DWORD CServerReciveCmdTransferMouseEvent::GetData()
{
	return m_dwData;
}

//////////////////////////////////////////////////////////////////////////
// CServerReciveCmdTransferKeyboardEvent
//////////////////////////////////////////////////////////////////////////
CServerReciveCmdTransferKeyboardEvent::CServerReciveCmdTransferKeyboardEvent()
{

}

CServerReciveCmdTransferKeyboardEvent::CServerReciveCmdTransferKeyboardEvent(CommandData& commandData, IServerReciveCmdTransferKeyboardEventEvent* pServerReciveCmdTransferKeyboardEventEvent) : m_bVk(0), m_dwFlags(0)
{
	Parse(commandData, pServerReciveCmdTransferKeyboardEventEvent);
}

RD_CMD_CODE CServerReciveCmdTransferKeyboardEvent::GetCode()
{
	return RDCC_TRANSFER_KEYBOARD_EVENT;
}

bool CServerReciveCmdTransferKeyboardEvent::Parse(CommandData& commandData, void* pCallBack)
{
	stream_buffer buffer((void*)commandData.data.c_str(), commandData.data.length());
	int ret = buffer.ReadUInt8((uint8_t&)m_bVk);
	SERVER_RECIVE_CMD_RETURN_IFFAILED(ret);

	ret = buffer.ReadUInt32((uint32_t&)m_dwFlags);
	SERVER_RECIVE_CMD_RETURN_IFFAILED(ret);

	CClientConnection* pClientConnection = (CClientConnection*)pCallBack;
	pClientConnection->AddRef();
	IServerReciveCmdTransferKeyboardEventEvent* pServerReciveCmdTransferKeyboardEventEvent = static_cast<IServerReciveCmdTransferKeyboardEventEvent*>(pClientConnection);
	if (pServerReciveCmdTransferKeyboardEventEvent)
	{
		pServerReciveCmdTransferKeyboardEventEvent->OnGetTransferKeyboardEvent(m_bVk, m_dwFlags);
	}
	pClientConnection->Release();
	return true;
}

BYTE CServerReciveCmdTransferKeyboardEvent::GetVirtualKeyCode()
{
	return m_bVk;
}

DWORD CServerReciveCmdTransferKeyboardEvent::GetFlags()
{
	return m_dwFlags;
}

//////////////////////////////////////////////////////////////////////////
// CServerReciveCmdTransferClipboard
//////////////////////////////////////////////////////////////////////////
CServerReciveCmdTransferClipboard::CServerReciveCmdTransferClipboard()
{

}

CServerReciveCmdTransferClipboard::CServerReciveCmdTransferClipboard(CommandData& commandData, IServerReciveCmdTransferClipboardEvent* pServerReciveCmdTransferClipboardEvent) : m_uFormat(CF_TEXT), m_pData(NULL), m_nLength(0)
{
	Parse(commandData, pServerReciveCmdTransferClipboardEvent);
}

CServerReciveCmdTransferClipboard::~CServerReciveCmdTransferClipboard()
{
	delete[] m_pData;
}

RD_CMD_CODE CServerReciveCmdTransferClipboard::GetCode()
{
	return RDCC_TRANSFER_CLIPBOARD;
}

bool CServerReciveCmdTransferClipboard::Parse(CommandData& commandData, void* pCallBack)
{
	stream_buffer buffer((void*)commandData.data.c_str(), commandData.data.length());
	int ret = buffer.ReadUInt32((uint32_t&)m_uFormat);
	SERVER_RECIVE_CMD_RETURN_IFFAILED(ret);

	ret = buffer.ReadUInt32((uint32_t&)m_nLength);
	SERVER_RECIVE_CMD_RETURN_IFFAILED(ret);

	m_pData = new char[m_nLength];
	ZeroMemory(m_pData, m_nLength);

	ret = buffer.ReadStringUTF8((char*)m_pData, m_nLength);
	SERVER_RECIVE_CMD_RETURN_IFFAILED(ret);

	CClientConnection* pClientConnection = (CClientConnection*)pCallBack;
	pClientConnection->AddRef();
	IServerReciveCmdTransferClipboardEvent* pServerReciveCmdTransferClipboardEvent = static_cast<IServerReciveCmdTransferClipboardEvent*>(pClientConnection);
	if (pServerReciveCmdTransferClipboardEvent)
	{
		pServerReciveCmdTransferClipboardEvent->OnGetTransferClipboardEvent(m_uFormat, m_pData, m_nLength);
	}
	pClientConnection->Release();
	return true;
}

unsigned int CServerReciveCmdTransferClipboard::GetFormat()
{
	return m_uFormat;
}

void* CServerReciveCmdTransferClipboard::GetData()
{
	return m_pData;
}

unsigned int CServerReciveCmdTransferClipboard::GetLength()
{
	return m_nLength;
}

//////////////////////////////////////////////////////////////////////////
// CServerReciveCmdTransferFile
//////////////////////////////////////////////////////////////////////////
CServerReciveCmdTransferFile::CServerReciveCmdTransferFile()
{

}

CServerReciveCmdTransferFile::CServerReciveCmdTransferFile(CommandData& commandData, IServerReciveCmdTransferFileEvent* pServerReciveCmdTransferFileEvent) : m_ullFileSize(0)
{
	Parse(commandData, pServerReciveCmdTransferFileEvent);
}

RD_CMD_CODE CServerReciveCmdTransferFile::GetCode()
{
	return RDCC_TRANSFER_FILE;
}

bool CServerReciveCmdTransferFile::Parse(CommandData& commandData, void* pCallBack)
{
	stream_buffer buffer((void*)commandData.data.c_str(), commandData.data.length());
	uint32_t length = 0;
	int ret = buffer.ReadUInt32(length);
	SERVER_RECIVE_CMD_RETURN_IFFAILED(ret);

	ret = buffer.ReadStringUTF8(m_strFilePath, length);
	SERVER_RECIVE_CMD_RETURN_IFFAILED(ret);

	ret = buffer.ReadUInt64(m_ullFileSize);
	SERVER_RECIVE_CMD_RETURN_IFFAILED(ret);

	CClientConnection* pClientConnection = (CClientConnection*)pCallBack;
	pClientConnection->AddRef();
	IServerReciveCmdTransferFileEvent* pServerReciveCmdTransferFileEvent = static_cast<IServerReciveCmdTransferFileEvent*>(pClientConnection);
	if (pServerReciveCmdTransferFileEvent)
	{
		pServerReciveCmdTransferFileEvent->OnGetTransferFile(m_strFilePath, m_ullFileSize);
	}
	pClientConnection->Release();
	return true;
}

std::string CServerReciveCmdTransferFile::GetFilePath()
{
	return m_strFilePath;
}

unsigned long long CServerReciveCmdTransferFile::GetFileSize()
{
	return m_ullFileSize;
}

//////////////////////////////////////////////////////////////////////////
// CServerReciveCmdTransferDirectory
//////////////////////////////////////////////////////////////////////////
CServerReciveCmdTransferDirectory::CServerReciveCmdTransferDirectory()
{

}

CServerReciveCmdTransferDirectory::CServerReciveCmdTransferDirectory(CommandData& commandData, IServerReciveCmdTransferDirectoryEvent* pServerReciveCmdTransferDirectoryEvent) : m_nFileCount(0), m_ullDirSize(0)
{
	Parse(commandData, pServerReciveCmdTransferDirectoryEvent);
}

RD_CMD_CODE CServerReciveCmdTransferDirectory::GetCode()
{
	return RDCC_TRANSFER_DIRECTORY;
}

bool CServerReciveCmdTransferDirectory::Parse(CommandData& commandData, void* pCallBack)
{
	stream_buffer buffer((void*)commandData.data.c_str(), commandData.data.length());
	uint32_t length = 0;
	int ret = buffer.ReadUInt32(length);
	SERVER_RECIVE_CMD_RETURN_IFFAILED(ret);

	ret = buffer.ReadStringUTF8(m_strDirPath, length);
	SERVER_RECIVE_CMD_RETURN_IFFAILED(ret);

	ret = buffer.ReadUInt32(m_nFileCount);
	SERVER_RECIVE_CMD_RETURN_IFFAILED(ret);

	ret = buffer.ReadUInt64(m_ullDirSize);
	SERVER_RECIVE_CMD_RETURN_IFFAILED(ret);

	CClientConnection* pClientConnection = (CClientConnection*)pCallBack;
	pClientConnection->AddRef();
	IServerReciveCmdTransferDirectoryEvent* pServerReciveCmdTransferDirectoryEvent = static_cast<IServerReciveCmdTransferDirectoryEvent*>(pClientConnection);
	if (pServerReciveCmdTransferDirectoryEvent)
	{
		pServerReciveCmdTransferDirectoryEvent->OnGetTransferDirectory(m_strDirPath, m_nFileCount, m_ullDirSize);
	}
	pClientConnection->Release();
	return true;
}

std::string CServerReciveCmdTransferDirectory::GetDirPath()
{
	return m_strDirPath;
}

unsigned int CServerReciveCmdTransferDirectory::GetFileCount()
{
	return m_nFileCount;
}

unsigned long long CServerReciveCmdTransferDirectory::GetDirSize()
{
	return m_ullDirSize;
}

//////////////////////////////////////////////////////////////////////////
// CServerReciveCmdDisconnect
//////////////////////////////////////////////////////////////////////////

CServerReciveCmdDisconnect::CServerReciveCmdDisconnect()
{

}

CServerReciveCmdDisconnect::CServerReciveCmdDisconnect(CommandData& commandData, IServerReciveCmdDisconnectEvent* pServerReciveCmdDisconnectEvent) : m_nDisconnectCode(0)
{
	Parse(commandData, pServerReciveCmdDisconnectEvent);
}

RD_CMD_CODE CServerReciveCmdDisconnect::GetCode()
{
	return RDCC_DISCONNECT;
}

bool CServerReciveCmdDisconnect::Parse(CommandData& commandData, void* pCallBack)
{
	stream_buffer buffer((void*)commandData.data.c_str(), commandData.data.length());
	int ret = buffer.ReadInt32(m_nDisconnectCode);
	SERVER_RECIVE_CMD_RETURN_IFFAILED(ret);

	CClientConnection* pClientConnection = (CClientConnection*)pCallBack;
	pClientConnection->AddRef();
	IServerReciveCmdDisconnectEvent* pServerReciveCmdDisconnectEvent = static_cast<IServerReciveCmdDisconnectEvent*>(pClientConnection);
	if (pServerReciveCmdDisconnectEvent)
	{
		pServerReciveCmdDisconnectEvent->OnGetDisconnect(m_nDisconnectCode);
	}
	pClientConnection->Release();
	return true;
}

int CServerReciveCmdDisconnect::GetDisconnectCode()
{
	return m_nDisconnectCode;
}
#pragma once
#include "..\common\def.h"
#include "..\common\cmd.h"
#include "..\common\stream_buffer.h"
#include <atltypes.h>

#define  SERVER_RECIVE_CMD_RETURN_IFFAILED(ret) do{if((ret) != S_OK){ return false;}}while(0)

class IServerReciveCmdBase
{
public:
	virtual ~IServerReciveCmdBase() {};
	virtual RD_CMD_CODE GetCode() = 0;
	virtual bool Parse(CommandData& commandData, void* pCallBack = NULL) = 0;
};

class IServerReciveCmdProtocalVersionEvent
{
public:
	virtual void OnGetProtocalVersion(std::string& strProtocalVersion, std::string& strClientVersion) = 0;
};

class CServerReciveCmdProtocalVersion : public IServerReciveCmdBase
{
public:
	CServerReciveCmdProtocalVersion();
	CServerReciveCmdProtocalVersion(CommandData& commandData, IServerReciveCmdProtocalVersionEvent* pServerReciveCmdProtocalVersionEvent = NULL);
	virtual ~CServerReciveCmdProtocalVersion();
	virtual RD_CMD_CODE GetCode();
	virtual bool Parse(CommandData& commandData, void* pCallBack = NULL);
	std::string GetProtocalVersion();
	std::string GetClientVersion();

private:
	std::string	m_strProtocalVersion;
	std::string m_strClientVersion;
};

class IServerReciveCmdLogonEvent
{
public:
	virtual void OnGetLogon(std::string& strUserName, std::string& strPassword) = 0;
};

class CServerReciveCmdLogon : public IServerReciveCmdBase
{
public:
	CServerReciveCmdLogon();
	CServerReciveCmdLogon(CommandData& commandData, IServerReciveCmdLogonEvent* pServerReciveCmdLogonEvent = NULL);
	virtual RD_CMD_CODE GetCode();
	virtual bool Parse(CommandData& commandData, void* pCallBack = NULL);
	std::string GetUserName();
	std::string GetPassword();

private:
	std::string m_strUserName;
	std::string m_strPassword;
};

class IServerReciveCmdCompressModeEvent
{
public:
	virtual void OnGetCompressMode(RD_COMPRESS_MODE mode) = 0;
};

class CServerReciveCmdCompressMode : public IServerReciveCmdBase
{
public:
	CServerReciveCmdCompressMode();
	CServerReciveCmdCompressMode(CommandData& commandData, IServerReciveCmdCompressModeEvent* pServerReciveCmdCompressModeEvent = NULL);
	virtual RD_CMD_CODE GetCode();
	virtual bool Parse(CommandData& commandData, void* pCallBack = NULL);
	RD_COMPRESS_MODE GetMode();

private:
	RD_COMPRESS_MODE m_mode;
};

class IServerReciveCmdTransferBitmapEvent
{
public:
	virtual void OnGetTransferBitmap(std::string& strSuffix) = 0;
};

class CServerReciveCmdTransferBitmap : public IServerReciveCmdBase
{
public:
	CServerReciveCmdTransferBitmap();
	CServerReciveCmdTransferBitmap(CommandData& commandData, IServerReciveCmdTransferBitmapEvent* pServerReciveCmdTransferBitmapEvent = NULL);
	virtual RD_CMD_CODE GetCode();
	virtual bool Parse(CommandData& commandData, void* pCallBack = NULL);
	std::string GetSuffix();

private:
	std::string	m_strSuffix;
};

class IServerReciveCmdTransferBitmapResponseEvent
{
public:
	virtual void OnGetTransferBitmapResult(int nResult) = 0;
};

class CServerReciveCmdTransferBitmapResponse : public IServerReciveCmdBase
{
public:
	CServerReciveCmdTransferBitmapResponse();
	CServerReciveCmdTransferBitmapResponse(CommandData& commandData, IServerReciveCmdTransferBitmapResponseEvent* pServerReciveCmdTransferBitmapResponseEvent = NULL);
	virtual RD_CMD_CODE GetCode();
	virtual bool Parse(CommandData& commandData, void* pCallBack = NULL);
	int GetResult();

private:
	int m_nResult;
};

class IServerReciveCmdTransferMouseEventEvent
{
public:
	virtual void OnGetTransferMouseEvent(DWORD dwFlags, CPoint& pt, DWORD dwData) = 0;
};

class CServerReciveCmdTransferMouseEvent : public IServerReciveCmdBase
{
public:
	CServerReciveCmdTransferMouseEvent();
	CServerReciveCmdTransferMouseEvent(CommandData& commandData, IServerReciveCmdTransferMouseEventEvent* pServerReciveCmdTransferMouseEventEvent = NULL);
	virtual RD_CMD_CODE GetCode();
	virtual bool Parse(CommandData& commandData, void* pCallBack = NULL);
	DWORD GetFlags();
	CPoint GetPoint();
	DWORD GetData();

private:
	DWORD	m_dwFlags;
	CPoint	m_pt;
	DWORD	m_dwData;
};

class IServerReciveCmdTransferKeyboardEventEvent
{
public:
	virtual void OnGetTransferKeyboardEvent(BYTE bVk, DWORD dwFlags) = 0;
};

class CServerReciveCmdTransferKeyboardEvent : public IServerReciveCmdBase
{
public:
	CServerReciveCmdTransferKeyboardEvent();
	CServerReciveCmdTransferKeyboardEvent(CommandData& commandData, IServerReciveCmdTransferKeyboardEventEvent* pServerReciveCmdTransferKeyboardEventEvent = NULL);
	virtual RD_CMD_CODE GetCode();
	virtual bool Parse(CommandData& commandData, void* pCallBack = NULL);
	BYTE GetVirtualKeyCode();
	DWORD GetFlags();

private:
	BYTE	m_bVk;
	DWORD	m_dwFlags;
};

class IServerReciveCmdTransferClipboardEvent
{
public:
	virtual void OnGetTransferClipboardEvent(unsigned int uFormat, void* pData, unsigned int nLength) = 0;
};

class CServerReciveCmdTransferClipboard : public IServerReciveCmdBase
{
public:
	CServerReciveCmdTransferClipboard();
	CServerReciveCmdTransferClipboard(CommandData& commandData, IServerReciveCmdTransferClipboardEvent* pServerReciveCmdTransferClipboardEvent = NULL);
	virtual ~CServerReciveCmdTransferClipboard();
	virtual RD_CMD_CODE GetCode();
	virtual bool Parse(CommandData& commandData, void* pCallBack = NULL);
	unsigned int GetFormat();
	void* GetData();
	unsigned int GetLength();

private:
	unsigned int	m_uFormat;
	void*			m_pData;
	unsigned int	m_nLength;
};

class IServerReciveCmdTransferFileEvent
{
public:
	virtual void OnGetTransferFile(std::string& strFilePath, unsigned long long ullFileSize) = 0;
};

class CServerReciveCmdTransferFile : public IServerReciveCmdBase
{
public:
	CServerReciveCmdTransferFile();
	CServerReciveCmdTransferFile(CommandData& commandData, IServerReciveCmdTransferFileEvent* pServerReciveCmdTransferFileEvent = NULL);
	virtual RD_CMD_CODE GetCode();
	virtual bool Parse(CommandData& commandData, void* pCallBack = NULL);
	std::string GetFilePath();
	unsigned long long GetFileSize();

private:
	std::string			m_strFilePath;
	unsigned long long	m_ullFileSize;
};

class IServerReciveCmdTransferDirectoryEvent
{
public:
	virtual void OnGetTransferDirectory(std::string& strDirPath, unsigned int nFileCount, unsigned long long ullDirSize) = 0;
};

class CServerReciveCmdTransferDirectory : public IServerReciveCmdBase
{
public:
	CServerReciveCmdTransferDirectory();
	CServerReciveCmdTransferDirectory(CommandData& commandData, IServerReciveCmdTransferDirectoryEvent* pServerReciveCmdTransferDirectoryEvent = NULL);
	virtual RD_CMD_CODE GetCode();
	virtual bool Parse(CommandData& commandData, void* pCallBack = NULL);
	std::string GetDirPath();
	unsigned int GetFileCount();
	unsigned long long GetDirSize();

private:
	std::string			m_strDirPath;
	unsigned int		m_nFileCount;
	unsigned long long	m_ullDirSize;
};

class IServerReciveCmdDisconnectEvent
{
public:
	virtual void OnGetDisconnect(int nDisconnectCode) = 0;
};

class CServerReciveCmdDisconnect : public IServerReciveCmdBase
{
public:
	CServerReciveCmdDisconnect();
	CServerReciveCmdDisconnect(CommandData& commandData, IServerReciveCmdDisconnectEvent* pServerReciveCmdDisconnectEvent = NULL);
	virtual RD_CMD_CODE GetCode();
	virtual bool Parse(CommandData& commandData, void* pCallBack = NULL);
	int GetDisconnectCode();

private:
	int	m_nDisconnectCode;
};

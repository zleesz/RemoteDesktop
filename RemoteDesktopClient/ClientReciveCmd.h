#pragma once
#include "..\common\def.h"
#include "..\common\cmd.h"
#include "..\common\stream_buffer.h"
#include <atltypes.h>

#define  CLIENT_RECIVE_CMD_RETURN_IFFAILED(ret) do{if((ret) != S_OK){ return false;}}while(0)

class IClientReciveCmdBase
{
public:
	virtual RD_CMD_CODE GetCode() = 0;
	virtual bool Parse(CommandData& commandData, void* pCallBack = NULL) = 0;
	virtual ~IClientReciveCmdBase() {}
};

class IClientReciveCmdLogonResultEvent
{
public:
	virtual void OnGetLogonResult(int nResult) = 0;
};

class CClientReciveCmdLogonResult : public IClientReciveCmdBase
{
public:
	CClientReciveCmdLogonResult();
	CClientReciveCmdLogonResult(CommandData& commandData, IClientReciveCmdLogonResultEvent* pClientReciveCmdLogonResultEvent = NULL);
	virtual ~CClientReciveCmdLogonResult();
	virtual RD_CMD_CODE GetCode();
	virtual bool Parse(CommandData& commandData, void* pCallBack = NULL);
	int GetResult();

private:
	int m_nResult;
};

class IClientReciveCmdTransferBitmapEvent
{
public:
	virtual void GetSuffix(std::string& strSuffix) = 0;
	virtual void OnGetTransferBitmap(BitmapInfo& bitmapInfo, WORD wPixelBytes, unsigned char* pBuffer) = 0;
};

class CClientReciveCmdTransferBitmap : public IClientReciveCmdBase
{
public:
	CClientReciveCmdTransferBitmap();
	CClientReciveCmdTransferBitmap(CommandData& commandData, IClientReciveCmdTransferBitmapEvent* pClientReciveCmdTransferBitmapEvent = NULL);
	virtual ~CClientReciveCmdTransferBitmap();
	virtual RD_CMD_CODE GetCode();
	virtual bool Parse(CommandData& commandData, void* pCallBack);
	BitmapInfo GetBitmapInfo();
	unsigned char* GetBuffer();
	WORD GetPixelBytes();

private:
	BitmapInfo		m_bitmapInfo;
	unsigned char*	m_pBuffer;
	std::string		m_strSuffix;
	WORD			m_wPixelBytes;
};

class IClientReciveCmdTransferModifyBitmapEvent
{
public:
	virtual void OnGetTransferModifyBitmap(unsigned int nModifiedBlockCount, unsigned int* pnModifiedBlocks, unsigned int nBufferSize, unsigned char* pBuffer) = 0;
};

class CClientReciveCmdTransferModifyBitmap : public IClientReciveCmdBase
{
public:
	CClientReciveCmdTransferModifyBitmap();
	CClientReciveCmdTransferModifyBitmap(CommandData& commandData, IClientReciveCmdTransferModifyBitmapEvent* pClientReciveCmdTransferModifyBitmapEvent = NULL);
	virtual ~CClientReciveCmdTransferModifyBitmap();
	virtual RD_CMD_CODE GetCode();
	virtual bool Parse(CommandData& commandData, void* pCallBack = NULL);
	unsigned int GetModifiedBlockCount();
	unsigned int* GetModifiedBlocks();
	unsigned int GetBufferSize();
	unsigned char* GetBuffer();

private:
	unsigned int	m_nModifiedBlockCount;
	unsigned int*	m_pnModifiedBlocks;
	unsigned int	m_nBufferSize;
	unsigned char*	m_pBuffer;
	std::string		m_strSuffix;
};

class IClientReciveCmdDisconnectEvent
{
public:
	virtual void OnGetDisconnect(int nDisconnectCode) = 0;
};

class CClientReciveCmdDisconnect : public IClientReciveCmdBase
{
public:
	CClientReciveCmdDisconnect();
	CClientReciveCmdDisconnect(CommandData& commandData, IClientReciveCmdDisconnectEvent* pClientReciveCmdDisconnectEvent = NULL);
	virtual ~CClientReciveCmdDisconnect();
	virtual RD_CMD_CODE GetCode();
	virtual bool Parse(CommandData& commandData, void* pCallBack = NULL);
	int GetDisconnectCode();

private:
	int m_nDisconnectCode;
};

#pragma once
#include <event2/event.h>
#include <event2/bufferevent.h>
#include <event2/buffer.h>
#include <event2/listener.h>
#include <event2/util.h>

#include "..\common\def.h"
#include "..\common\cmd.h"
#include "..\common\compress_base.h"
#include "..\common\lock.h"
#include "ServerReciveCmd.h"
#include "FileTransferDlg.h"

#include <queue>

class CClientConnection;

class IClientConnectionEvent
{
public:
	virtual void OnDisconnect(CClientConnection* pClient, RD_ERROR_CODE errorCode = RDEC_SUCCEEDED, DWORD dwExtra = 0) = 0;
};

class CClientConnection :
	public CAddReleaseRef,
	public IServerReciveCmdProtocalVersionEvent,
	public IServerReciveCmdLogonEvent,
	public IServerReciveCmdCompressModeEvent,
	public IServerReciveCmdTransferBitmapEvent,
	public IServerReciveCmdTransferBitmapResponseEvent,
	public IServerReciveCmdTransferMouseEventEvent,
	public IServerReciveCmdTransferKeyboardEventEvent,
	public IServerReciveCmdTransferClipboardEvent,
	public IServerReciveCmdTransferFileEvent,
	public IServerReciveCmdTransferDirectoryEvent,
	public IServerReciveCmdDisconnectEvent
{
public:
	CClientConnection(struct event_base *base, SOCKET socket, bufferevent* bufferev, IClientConnectionEvent* pClientConnectionEvent);
	~CClientConnection(void);

public:
	bufferevent* GetBufferEvent();
	RD_CONNECTION_STATE GetState() const;
	void Stop(RD_ERROR_CODE errorCode);
	int Send(const void *data, size_t size);
	int Send(CommandBase* pCommand);
	int AsynSend(CommandBase* pCommand);
	std::string GetIp() const;
	unsigned short GetPort() const;
	bool IsValid() const;
	bool CheckCmdCodeValid(RD_CMD_CODE code) const;
	bool operator == (const CClientConnection& p) const;

	void OnConnect();
	void OnReciveCommand(CommandData* pCommand);
	void OnError(RD_ERROR_CODE errorCode);
	void OnWrite();

	void OnScreenFirstBitmap(BitmapInfo* pBitmapInfo, WORD wPixelBytes, unsigned char *bitmapBits);
	void OnScreenModified(unsigned int nModifiedBlockCount, unsigned int* pnModifiedBlocks);

private:
	static void OnAsynSendCommand(evutil_socket_t fd, short event,void *arg);
	int AsynSendLogonResult(CCommandLogonResult* pCommand);
	int AsynSendTransferBitmap();
	int AsynSendTransterModifyBitmap();

protected:
	virtual void OnGetProtocalVersion(std::string& strProtocalVersion, std::string& strClientVersion);
	virtual void OnGetLogon(std::string& strUserName, std::string& strPassword);
	virtual void OnGetCompressMode(RD_COMPRESS_MODE mode);
	virtual void OnGetTransferBitmap(std::string& strSuffix);
	virtual void OnGetTransferBitmapResult(int nResult);
	virtual void OnGetTransferMouseEvent(DWORD dwFlags, CPoint& pt, DWORD dwData);
	virtual void OnGetTransferKeyboardEvent(BYTE bVk, DWORD dwFlags);
	virtual void OnGetTransferClipboardEvent(unsigned int uFormat, void* pData, unsigned int nLength);
	virtual void OnGetTransferFile(std::string& strFilePath, unsigned long long ullFileSize);
	virtual void OnGetTransferDirectory(std::string& strDirPath, unsigned int nFileCount, unsigned long long ullDirSize);
	virtual void OnGetDisconnect(int nDisconnectCode);

private:
	IClientConnectionEvent*		m_pClientConnectionEvent;
	struct event_base*			m_pEventBase;
	SOCKET						m_socket;
	std::string					m_strIp;
	unsigned short				m_uPort;
	bufferevent*				m_bufferev;

	struct event*				m_pAsynSendCommandEvent;
	std::queue<CommandBase*>	m_queueAsynCommand;
	CLock						m_lockCommandQueue;

	RD_ERROR_CODE				m_errorCode;

	RD_CONNECTION_STATE			m_state;
	ICompressBase*				m_pCompress;

	std::string					m_strClientVersion;
	std::string					m_strSuffix;
	int							m_nQuality;

	CFileTransferDlg*			m_pFileTransferDlg;

	unsigned int				m_nScanPointRowCount;
	unsigned int				m_nScanPointColumnCount;
	unsigned int				m_nModifiedBlockCount;
	unsigned int*				m_pnModifiedBlocks;

	bool						m_bSendBitmapReady;
};

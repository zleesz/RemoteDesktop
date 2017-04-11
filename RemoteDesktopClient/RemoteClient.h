#pragma once

#include "event2/bufferevent.h"
#include "event2/buffer.h"
#include "event2/listener.h"
#include "event2/util.h"
#include "event2/event.h"

#include "..\common\def.h"
#include "..\common\cmd.h"
#include "..\common\lock.h"

#include "ClientReciveCmd.h"

#include <queue>

#define WM_REMOTECLIENT_ONCONNECTED			(WM_USER+1)
#define WM_REMOTECLIENT_ONRECIVECOMMAND		(WM_USER+2)

class IRemoteClientEvent
{
public:
	virtual void OnConnected() = 0;
	virtual void OnStateChanged(RD_CONNECTION_STATE state) = 0;
	virtual void OnGetLogonResult(int nResult) = 0;
	virtual void OnGetTransferBitmap(BitmapInfo& bitmapInfo, WORD wPixelBytes, unsigned char* pBuffer) = 0;
	virtual void OnGetTransferModifyBitmap(unsigned int nModifiedBlockCount, unsigned int* pnModifiedBlocks, unsigned int nBufferSize, unsigned char* pBuffer) = 0;
	virtual void OnGetDisconnect(int nDisconnectCode) = 0;
};

class CRemoteClient :
	public CWindowImpl<CRemoteClient>,
	public IClientReciveCmdLogonResultEvent,
	public IClientReciveCmdTransferBitmapEvent,
	public IClientReciveCmdTransferModifyBitmapEvent,
	public IClientReciveCmdDisconnectEvent,
	public CLock
{
public:
	CRemoteClient(void);
	~CRemoteClient(void);

	BEGIN_MSG_MAP(CRemoteClient)
		MESSAGE_HANDLER(WM_REMOTECLIENT_ONCONNECTED, OnConnected)
		MESSAGE_HANDLER(WM_REMOTECLIENT_ONRECIVECOMMAND, OnReciveCommand)
	END_MSG_MAP()

protected:
	LRESULT OnConnected(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	LRESULT OnReciveCommand(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);

private:
	static void conn_writecb( struct bufferevent *bev, void *user_data );
	static void conn_readcb( struct bufferevent *bev, void *user_data );
	static unsigned __stdcall DoConnect(void* pParam);

	void InitCommandData();
	bool CheckCmdCodeValid(RD_CMD_CODE code);

public:
	bool Connect(IRemoteClientEvent* pRemoteClientEvent);
	void Stop();
	void DecodeBuffer(char* buf, uint32_t total);
	RD_CONNECTION_STATE	GetState();

	int Send(const void *data, size_t size);
	int Send(CommandBase* pCommand);
	int AsynSend(CommandBase* pCommand);

	virtual void GetSuffix(std::string& strSuffix);
	virtual void OnGetLogonResult(int nResult);
	virtual void OnGetTransferBitmap(BitmapInfo& bitmapInfo, WORD wPixelBytes, unsigned char* pBuffer);
	virtual void OnGetTransferModifyBitmap(unsigned int nModifiedBlockCount, unsigned int* pnModifiedBlocks, unsigned int nBufferSize, unsigned char* pBuffer);
	virtual void OnGetDisconnect(int nDisconnectCode);

private:
	static void OnAsynSendCommand(evutil_socket_t fd, short event,void *arg);

private:
	HANDLE						m_hThread;
	struct event_base*			m_pEventBase;
	struct bufferevent*			m_bufferev;
	IRemoteClientEvent*			m_pRemoteClientEvent;
	CommandData					m_Command;
	RD_CONNECTION_STATE			m_state;

	struct event*				m_pAsynSendCommandEvent;
	std::queue<CommandBase*>	m_queueAsynCommand;
	CLock						m_lockCommandQueue;
	const std::string			m_strSuffix;
};

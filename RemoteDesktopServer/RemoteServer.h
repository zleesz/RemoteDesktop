#pragma once
#include "ClientConnection.h"

#include "event2/event.h"
#include "event2/bufferevent.h"
#include "event2/buffer.h"
#include "event2/listener.h"
#include "event2/util.h"

#include "..\common\lock.h"
#include "..\common\def.h"
#include "..\common\cmd.h"
#include <map>
#include <queue>

#define WM_REMOTESERVER_ONCLIENTCONNECTED		(WM_USER+1)
#define WM_REMOTESERVER_ONRECIVECOMMAND			(WM_USER+2)
#define WM_REMOTESERVER_ONCLIENTDISCONNECTED	(WM_USER+3)
#define WM_REMOTESERVER_ONSERVERSTARTED			(WM_USER+5)
#define WM_REMOTESERVER_ONSERVERSTOPPED			(WM_USER+6)

class IRemoteServerEvent
{
public:
	virtual void OnStart() = 0;
	virtual void OnStop() = 0;
	virtual void OnConnect(CClientConnection* pClientConnection) = 0;
	virtual void OnDisconnect(CClientConnection* pClientConnection, RD_ERROR_CODE errorCode = RDEC_SUCCEEDED) = 0;
};

typedef	std::map<bufferevent*, CClientConnection*> MapBufferEvent;

class CRemoteServer :
	public CWindowImpl<CRemoteServer>,
	public IClientConnectionEvent
{
public:
	CRemoteServer(void);
	~CRemoteServer(void);

	BEGIN_MSG_MAP(CRemoteServer)
		MESSAGE_HANDLER(WM_REMOTESERVER_ONCLIENTCONNECTED, OnClientConnected)
		MESSAGE_HANDLER(WM_REMOTESERVER_ONRECIVECOMMAND, OnReciveCommand)
		MESSAGE_HANDLER(WM_REMOTESERVER_ONCLIENTDISCONNECTED, OnClientDisconnected)
		MESSAGE_HANDLER(WM_REMOTESERVER_ONSERVERSTARTED, OnServerStarted)
		MESSAGE_HANDLER(WM_REMOTESERVER_ONSERVERSTOPPED, OnServerStopped)
	END_MSG_MAP()

protected:
	LRESULT OnClientConnected(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	LRESULT OnReciveCommand(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	LRESULT OnClientDisconnected(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	LRESULT OnServerStarted(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	LRESULT OnServerStopped(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);

private:
	static unsigned __stdcall DoStart(void * pParam);
	static void conn_writecb( struct bufferevent *bev, void *user_data );
	static void conn_readcb( struct bufferevent *bev, void *user_data );
	static void conn_errorcb( struct bufferevent *bev, short events , void *user_data);
	static void listener_cb(struct evconnlistener *listener, evutil_socket_t fd, struct sockaddr *sa, int socklen, void *user_data);
	static void accept_error_cb(struct evconnlistener *listener, void *ctx);

	void InitCommandData();
	void DecodeBuffer(CClientConnection* pClientConnection, char* buf, uint32_t total);

public:
	bool Start(IRemoteServerEvent* pRemoteServerEvent);
	void Stop();
	void StopClient(CClientConnection* pClient, RD_ERROR_CODE errorCode = RDEC_SUCCEEDED);
	bool HasClientConnected();
	void OnScreenFirstBitmap(BitmapInfo* pBitmapInfo, WORD wPixelBytes, unsigned char *bitmapBits);
	void OnScreenModified(unsigned int nModifiedBlockCount, unsigned int* pnModifiedBlocks);

public:
	virtual void OnDisconnect(CClientConnection* pClient, RD_ERROR_CODE errorCode, DWORD dwExtra);

private:
	HANDLE						m_hThread;
	IRemoteServerEvent*			m_pRemoteServerEvent;
	struct event_base*			m_pEventBase;
	MapBufferEvent				m_mapClient;
	CLock						m_lockClient;
	CommandData					m_Command;
};

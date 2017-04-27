#include "StdAfx.h"
#include "RemoteServer.h"
#include <iostream>
#include <signal.h>
#include "..\common\socket_util.h"
#include "..\common\AutoAddReleasePtr.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#define TIMERID_P2PTRACKER_PEER_HOLD		0
#define TIMERID_PEER_CONNECTING_DELAY_CLOSE	1

CRemoteServer::CRemoteServer(void) : m_pEventBase(NULL), m_hThread(NULL), m_pRemoteServerEvent(NULL)
{
	InitCommandData();
	Create(HWND_MESSAGE);
}

CRemoteServer::~CRemoteServer(void)
{
}

void CRemoteServer::conn_writecb(struct bufferevent* bev, void* user_data)
{
	tool.Log(_T("CRemoteServer::conn_writecb bev:0x%08X"), bev);
	CRemoteServer* pThis = (CRemoteServer*)user_data;
	pThis->m_lockClient.Lock();
	MapBufferEvent::iterator it = pThis->m_mapClient.find(bev);
	if (it != pThis->m_mapClient.end())
	{
		it->second->OnWrite();
	}
	pThis->m_lockClient.Unlock();
	evbuffer* output = bufferevent_get_output(bev);
	int length = evbuffer_get_length(output);
	tool.Log(_T("CRemoteServer::conn_writecb length:%d"), length);
}

void CRemoteServer::conn_readcb(struct bufferevent *bev, void *user_data)
{
	tool.Log(_T("CRemoteServer::conn_readcb bev:0x%08X"), bev);
	CRemoteServer* pThis = (CRemoteServer*)user_data;

	CClientConnection* pClientConnection = NULL;
	pThis->m_lockClient.Lock();
	MapBufferEvent::iterator it = pThis->m_mapClient.find(bev);
	if (it != pThis->m_mapClient.end())
	{
		pClientConnection = it->second;
		pClientConnection->AddRef();
	}
	pThis->m_lockClient.Unlock();
	if (pClientConnection == NULL)
	{
		return;
	}

	struct evbuffer* input = bufferevent_get_input(bev);
	uint32_t total = evbuffer_get_length(input);
	if (total <= 0)
	{
		pClientConnection->Release();
		return;
	}

	char* buf = new char[total+1];
	memset(buf, 0, total+1);
	evbuffer_remove(input, buf, total);
	pThis->DecodeBuffer(pClientConnection, buf, total);
	delete[] buf;
	pClientConnection->Release();
}

void CRemoteServer::conn_eventcb(struct bufferevent* bev, short events, void* user_data)
{
	tool.Log(_T("CRemoteServer::conn_eventcb bev:0x%08X, events:%d"), bev, events);
	CRemoteServer* pThis = (CRemoteServer*)user_data;
	if (events & BEV_EVENT_EOF)
	{
		tool.Log(_T("Connection closed."));
	}
	else if (events & BEV_EVENT_ERROR)
	{
		tool.Log(_T("Got an error on the connection: %s"), _tcserror(errno));
	}
	else
	{
		return;
	}
	if (pThis)
	{
		CAutoAddReleasePtr<CClientConnection> spClientConnection;
		pThis->m_lockClient.Lock();
		MapBufferEvent::iterator it = pThis->m_mapClient.find(bev);
		if (it != pThis->m_mapClient.end())
		{
			spClientConnection = it->second;
			pThis->m_mapClient.erase(it);
		}
		pThis->m_lockClient.Unlock();
		pThis->SendMessage(WM_REMOTESERVER_ONCLIENTDISCONNECTED, (WPARAM)(spClientConnection.p), (LPARAM)events);
	}
	bufferevent_free(bev);
}

void CRemoteServer::listener_cb(struct evconnlistener* /*listener*/, evutil_socket_t fd, struct sockaddr* /*sa*/, int /*socklen*/, void* user_data)
{
	tool.Log(_T("CRemoteServer::listener_cb"));
	CRemoteServer* pThis = (CRemoteServer*)user_data;
	struct event_base* base = pThis->m_pEventBase;
	struct bufferevent* bev;
	//生成一个bufferevent，用于读或者写
	bev = bufferevent_socket_new(base, fd, BEV_OPT_CLOSE_ON_FREE);
	tool.Log(_T("bev:0x%08X"), bev);
	if (!bev) 
	{
		tool.Log(_T("CRemoteServer::listener_cb Error constructing bufferevent!"));
		return;
	}
	CAutoAddReleasePtr<CClientConnection> spClientConnection = new CClientConnection(base, fd, bev, pThis);
	if (spClientConnection == NULL || !spClientConnection->IsValid())
	{
		tool.Log(_T("CRemoteServer::listener_cb Error new CClientConnection is not valid!"));
		bufferevent_free(bev);
		return;
	}
	bool bNotExist = true;
	pThis->m_lockClient.Lock();
	MapBufferEvent::iterator it = pThis->m_mapClient.begin();
	for (; it != pThis->m_mapClient.end(); it++)
	{
		if (*(it->second) == *spClientConnection)
		{
			bNotExist = false;
			break;
		}
	}
	pThis->m_lockClient.Unlock();
	if (bNotExist)
	{
		spClientConnection->AddRef();
		pThis->m_lockClient.Lock();
		pThis->m_mapClient[bev] = spClientConnection;
		pThis->m_lockClient.Unlock();
	}
	else
	{
		tool.LogA("CRemoteServer::listener_cb Error already connect! ip : %s", spClientConnection->GetIp().c_str());
		bufferevent_free(bev);
		return;
	}

	evutil_make_socket_nonblocking(fd);
	bufferevent_setcb(bev, conn_readcb, conn_writecb, conn_eventcb, user_data);
	bufferevent_enable(bev, EV_WRITE | EV_READ);
	pThis->SendMessage(WM_REMOTESERVER_ONCLIENTCONNECTED, (WPARAM)(spClientConnection.p), 0);
}

void CRemoteServer::accept_error_cb(struct evconnlistener* listener, void* pParam)
{
	CRemoteServer* pThis = (CRemoteServer*)pParam;
	struct event_base *base = evconnlistener_get_base(listener);
	int err = EVUTIL_SOCKET_ERROR();
	tool.LogA("Got an error %d (%s) on the listener.", err, evutil_socket_error_to_string(err));

	pThis->m_P2PTrackerClient.SendOffline();
	pThis->m_P2PTrackerClient.Uninit();
	event_base_loopexit(base, NULL);
}

unsigned __stdcall CRemoteServer::DoStart(void* pParam)
{
	tool.Log(_T("CRemoteServer::DoStart pParam:0x%08X"), pParam);
	CRemoteServer* pThis = (CRemoteServer*)pParam;
	//创建event_base
	pThis->m_pEventBase = event_base_new();
	if (!pThis->m_pEventBase) 
	{
		tool.Log(_T("Could not initialize libevent!"));
		return 1;
	}

	// 先连接Tracker服务器，得到本地端口，再在这个端口上监听
	bool bInit = pThis->m_P2PTrackerClient.Init(pThis, pThis->m_pEventBase);
	if (bInit)
	{
		bool bTrack = pThis->m_P2PTrackerClient.SendOnline();
		if (bTrack)
		{
			event_base_dispatch(pThis->m_pEventBase);
		}
	}

	event_base_free(pThis->m_pEventBase);
	pThis->m_pEventBase = NULL;
	pThis->SendMessage(WM_REMOTESERVER_ONSERVERSTOPPED, 0, 0);
	return 0;
}

bool CRemoteServer::Start(IRemoteServerEvent* pRemoteServerEvent)
{
	tool.Log(_T("CRemoteServer::Start pRemoteServerEvent:0x%08X"), pRemoteServerEvent);
	if (m_hThread != NULL)
	{
		tool.Log(_T("libevent is already initialized!"));
		return true;
	}
	m_pRemoteServerEvent = pRemoteServerEvent;

	unsigned threadID = 0;
	m_hThread = (HANDLE)_beginthreadex(NULL, 0, &DoStart, (LPVOID)this, 0, &threadID);
	CloseHandle(m_hThread);

	if (m_hThread == NULL)
		return false;

	return true;
}

void CRemoteServer::Stop()
{
	tool.Log(_T("CRemoteServer::Stop"));
	m_lockClient.Lock();
	MapBufferEvent::iterator it = m_mapClient.begin();
	for (; it != m_mapClient.end(); it++)
	{
		it->second->Stop(RDEC_SUCCEEDED);
		it->second->Release();
	}
	m_mapClient.clear();
	m_lockClient.Unlock();
	if (m_pEventBase)
	{
		m_P2PTrackerClient.SendOffline();
		m_P2PTrackerClient.Uninit();
		if (m_hWnd)
		{
			KillTimer(TIMERID_P2PTRACKER_PEER_HOLD);
			KillTimer(TIMERID_PEER_CONNECTING_DELAY_CLOSE);
		}
		event_base_loopexit(m_pEventBase, NULL);
	}
	if (m_hWnd)
	{
		DestroyWindow();
	}
}

void CRemoteServer::DecodeBuffer(CClientConnection* pClientConnection, char* buf, uint32_t total)
{
	tool.Log(_T("CRemoteServer::DecodeBuffer pClientConnection:0x%08X, buf:0x%08X, total:%d"), pClientConnection, buf, total);
	if (m_Command.code == RDCC_INVALID)
	{
		stream_buffer buffer(buf, total);
		uint32_t ncode = RDCC_INVALID;
		if (FAILED(buffer.ReadUInt32(ncode)) || ncode == RDCC_INVALID)
		{
			return;
		}
		uint32_t nLength = 0;
		if (FAILED(buffer.ReadUInt32(nLength)) || nLength > CMD_ITEM_SIZE_MAX)
		{
			return;
		}
		uint32_t nReadLength = nLength;
		if (FAILED(buffer.ReadStringUTF8(m_Command.data, nReadLength)))
		{
			return;
		}
		m_Command.code = (RD_CMD_CODE)ncode;
		m_Command.length = nLength;
		if (nReadLength != nLength)
		{
			// 未读完全
			return;
		}
		else if (total > nLength + sizeof(uint32_t) * 2)
		{
			// 还有下一条命令
			// 先处理这条命令，再解析下一条
			SendMessage(WM_REMOTESERVER_ONRECIVECOMMAND, (WPARAM)pClientConnection, (LPARAM)&m_Command);
			InitCommandData();
			DecodeBuffer(pClientConnection, buf + nLength + sizeof(uint32_t) * 2, total - (nLength + sizeof(uint32_t) * 2));
			return;
		}
	}
	else
	{
		// 上次的命令还没接收完
		stream_buffer buffer(buf, total);
		std::string strNext;
		uint32_t nLeaveLength = m_Command.length - m_Command.data.length();
		uint32_t nReadLength = nLeaveLength;
		if (FAILED(buffer.ReadStringUTF8(strNext, nReadLength)))
		{
			InitCommandData();
			return;
		}
		m_Command.data.append(strNext);
		if (nReadLength != nLeaveLength)
		{
			// 未读完全
			return;
		}
		else if (total > nReadLength)
		{
			// 还有下一条命令
			// 先处理这条命令，再解析下一条
			SendMessage(WM_REMOTESERVER_ONRECIVECOMMAND, (WPARAM)pClientConnection, (LPARAM)&m_Command);
			InitCommandData();
			DecodeBuffer(pClientConnection, buf + nReadLength, total - nReadLength);
			return;
		}
	}
	// 到这里，一个命令接收完了
	SendMessage(WM_REMOTESERVER_ONRECIVECOMMAND, (WPARAM)pClientConnection, (LPARAM)&m_Command);
	InitCommandData();
}

bool CRemoteServer::HasClientConnected()
{
	return m_mapClient.size() > 0;
}

void CRemoteServer::StopClient(CClientConnection* pClient, RD_ERROR_CODE errorCode)
{
	tool.Log(_T("CRemoteServer::StopClient pClient:0x%08X, errorCode:%d"), pClient, errorCode);
	m_lockClient.Lock();
	MapBufferEvent::iterator it = m_mapClient.find(pClient->GetBufferEvent());
	if (it != m_mapClient.end())
	{
		it->second->Stop(errorCode);
		it->second->Release();
		bufferevent_free(it->first);
		m_mapClient.erase(it);
	}
	m_lockClient.Unlock();
}

void CRemoteServer::OnScreenFirstBitmap(BitmapInfo* pBitmapInfo, WORD wPixelBytes, unsigned char *bitmapBits)
{
	m_lockClient.Lock();
	MapBufferEvent::iterator it = m_mapClient.begin();
	for (; it != m_mapClient.end(); it++)
	{
		it->second->OnScreenFirstBitmap(pBitmapInfo, wPixelBytes, bitmapBits);
	}
	m_lockClient.Unlock();
}

void CRemoteServer::OnScreenModified(unsigned int nModifiedBlockCount, unsigned int* pnModifiedBlocks)
{
	m_lockClient.Lock();
	MapBufferEvent::iterator it = m_mapClient.begin();
	for (; it != m_mapClient.end(); it++)
	{
		it->second->OnScreenModified(nModifiedBlockCount, pnModifiedBlocks);
	}
	m_lockClient.Unlock();
}

void CRemoteServer::OnDisconnect(CClientConnection* pClient, RD_ERROR_CODE errorCode, DWORD dwExtra)
{
	tool.Log(_T("CRemoteServer::OnDisconnect pClient:0x%08X, errorCode:%d, dwExtra:0x%08X"), pClient, errorCode, dwExtra);
	StopClient(pClient, errorCode);
}

void CRemoteServer::InitCommandData()
{
	m_Command.code = RDCC_INVALID;
	m_Command.data.clear();
	m_Command.length = 0;
}

LRESULT CRemoteServer::OnClientConnected(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	CAutoAddReleasePtr<CClientConnection> spClientConnection = (CClientConnection*)wParam;
	tool.Log(_T("CRemoteServer::OnClientConnected pClient:0x%08X"), spClientConnection);
	spClientConnection->OnConnect();
	if (m_pRemoteServerEvent)
	{
		m_pRemoteServerEvent->OnConnect(spClientConnection);
	}
	return 0;
}

LRESULT CRemoteServer::OnReciveCommand(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/)
{
	CAutoAddReleasePtr<CClientConnection> spClientConnection = (CClientConnection*)wParam;
	tool.Log(_T("CRemoteServer::OnReciveCommand pClient:0x%08X"), spClientConnection);
	CommandData* pCommand = (CommandData*)lParam;
	spClientConnection->OnReciveCommand(pCommand);
	return 0;
}

LRESULT CRemoteServer::OnClientDisconnected(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/)
{
	CAutoAddReleasePtr<CClientConnection> spClientConnection = (CClientConnection*)wParam;
	tool.Log(_T("CRemoteServer::OnClientDisconnected pClient:0x%08X"), spClientConnection);
	RD_ERROR_CODE errorCode = (RD_ERROR_CODE)lParam;
	if (errorCode != RDEC_SUCCEEDED)
	{
		spClientConnection->OnError(errorCode);
	}
	if (m_pRemoteServerEvent)
	{
		m_pRemoteServerEvent->OnDisconnect(spClientConnection, errorCode);
	}
	return 0;
}

LRESULT CRemoteServer::OnServerStarted(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	tool.Log(_T("CRemoteServer::OnServerStarted"));
	if (m_pRemoteServerEvent)
	{
		m_pRemoteServerEvent->OnStart();
	}
	return 0;
}

LRESULT CRemoteServer::OnServerStopped(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	tool.Log(_T("CRemoteServer::OnServerStopped"));
	if (m_pRemoteServerEvent)
	{
		m_pRemoteServerEvent->OnStop();
	}
	return 0;
}

LRESULT CRemoteServer::OnTimer(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	if (wParam == TIMERID_P2PTRACKER_PEER_HOLD)
	{
		m_P2PTrackerClient.SendPing();
	}
	else if (wParam == TIMERID_PEER_CONNECTING_DELAY_CLOSE)
	{
	}
	return 0;
}

// 连接tracker服务器成功
void CRemoteServer::OnServerConnectSuccess(unsigned short tcp_port)
{
	tool.Log(_T("CRemoteServer::OnServerConnectSuccess tcp_port:%u"), tcp_port);
	struct sockaddr_in sin;
	memset(&sin, 0, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_port = tcp_port;
	sin.sin_addr.s_addr = inet_addr("127.0.0.1");

	struct evconnlistener* listener = evconnlistener_new_bind(m_pEventBase, listener_cb, (void*)this, LEV_OPT_REUSEABLE | LEV_OPT_CLOSE_ON_FREE | LEV_OPT_THREADSAFE, -1, (struct sockaddr*)&sin, sizeof(sin));

	if (!listener)
	{
		tool.Log(_T("Could not create a listener!"));
		return;
	}

	evconnlistener_set_error_cb(listener, accept_error_cb);

	SendMessage(WM_REMOTESERVER_ONSERVERSTARTED, 0, 0);
	SetTimer(TIMERID_P2PTRACKER_PEER_HOLD, RD_P2PTRACKER_PEER_HOLD_TIME);
}

// 连接tracker服务器失败
void CRemoteServer::OnServerConnectFailed()
{
	tool.Log(_T("CRemoteServer::OnServerConnectFailed!"));
	if (m_pEventBase)
	{
		event_base_loopexit(m_pEventBase, NULL);
	}
}

// 与tracker服务器断开连接
void CRemoteServer::OnServerDisconnect()
{
	tool.Log(_T("CRemoteServer::OnServerDisconnect!"));
	KillTimer(0);
}

static void conn_writecb2(struct bufferevent *bev, void *user_data)
{
	tool.Log(_T("CRemoteServer::conn_writecb2 bev:0x%08X"), bev);
	evbuffer* output = bufferevent_get_output(bev);
	int length = evbuffer_get_length(output);
	tool.Log(_T("CRemoteServer::conn_writecb2 length:%d"), length);
}

static void conn_readcb2(struct bufferevent *bev, void *user_data)
{
	tool.Log(_T("CRemoteServer::conn_readcb2 bev:0x%08X"), bev);
}

static void conn_eventcb2(struct bufferevent *bev, short events, void *user_data)
{
	tool.Log(_T("CRemoteServer::conn_eventcb2 bev:0x%08X, events:%d"), bev, events);
	CRemoteServer* pThis = (CRemoteServer*)user_data;
	if (events & BEV_EVENT_EOF)
	{
		tool.Log(_T("CRemoteServer::conn_eventcb2 Connection closed."));
	}
	else if (events & BEV_EVENT_ERROR)
	{
		tool.Log(_T("CRemoteServer::conn_eventcb2 Got an error on the connection: %s"), _tcserror(errno));
	}
}

bool CRemoteServer::OnPeerConnecting(PeerInfo& peer)
{
	tool.LogA("CRemoteServer::OnPeerConnecting ip:%s, port:%u", peer.ip.c_str(), peer.tcp_port);
	struct sockaddr_in sin;
	memset(&sin, 0, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = inet_addr(peer.ip.c_str());
	sin.sin_port = htons(peer.tcp_port);

	struct bufferevent* pBufferEvent = bufferevent_socket_new(m_pEventBase, -1, BEV_OPT_CLOSE_ON_FREE | LEV_OPT_THREADSAFE);
	bufferevent_setcb(pBufferEvent, conn_readcb2, conn_writecb2, conn_eventcb2, this);
	bufferevent_enable(pBufferEvent, EV_WRITE | EV_READ);
	if (bufferevent_socket_connect(pBufferEvent, (struct sockaddr*)&sin, sizeof(sin)) == 0)
	{
		tool.LogA("bufferevent_socket_connect success.");
		std::string strInvalidMessage("invalid message.");
		bufferevent_write(pBufferEvent, strInvalidMessage.c_str(), strInvalidMessage.length());

		SetTimer(TIMERID_PEER_CONNECTING_DELAY_CLOSE, 5000);
		return true;
	}
	else
	{
		bufferevent_free(pBufferEvent);
		return false;
	}
}

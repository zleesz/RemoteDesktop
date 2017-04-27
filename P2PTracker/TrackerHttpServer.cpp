#include "stdafx.h"
#include "TrackerHttpServer.h"
#include "HttpRequestCommandPeerTracker.h"

CTrackerHttpServer::CTrackerHttpServer(void) :
	m_pTrackerEvent(NULL),
	m_hThread(NULL),
	m_pEventBase(NULL)
{
	tool.Log(_T("CTrackerHttpServer::CTrackerHttpServer"));
	m_strIp = "127.0.0.1";
	m_uPort = 80;
}

CTrackerHttpServer::~CTrackerHttpServer(void)
{
	tool.Log(_T("CTrackerHttpServer::~CTrackerHttpServer"));
}

void CTrackerHttpServer::conn_writecb(struct bufferevent* bev, void* user_data)
{
	tool.Log(_T("CTrackerHttpServer::conn_writecb bev:0x%08X"), bev);
	CTrackerHttpServer* pThis = (CTrackerHttpServer*)user_data;
	pThis->m_lockClient.Lock();
	MapBufferEvent::iterator it = pThis->m_mapClient.find(bev);
	if (it != pThis->m_mapClient.end())
	{
		it->second->OnWrite();
	}
	pThis->m_lockClient.Unlock();
	evbuffer* output = bufferevent_get_output(bev);
	int length = evbuffer_get_length(output);
	tool.Log(_T("CTrackerHttpServer::conn_writecb length:%d"), length);
}

void CTrackerHttpServer::conn_readcb(struct bufferevent *bev, void *user_data)
{
	tool.Log(_T("CTrackerHttpServer::conn_readcb bev:0x%08X"), bev);
	CTrackerHttpServer* pThis = (CTrackerHttpServer*)user_data;

	CHttpClientConnection* pClientConnection = NULL;
	pThis->m_lockClient.Lock();
	MapBufferEvent::iterator it = pThis->m_mapClient.find(bev);
	if (it != pThis->m_mapClient.end())
	{
		pClientConnection = it->second;
		pClientConnection->AddRef();
	}
	pThis->m_lockClient.Unlock();
	if (pClientConnection)
	{
		pClientConnection->OnRead(bev);
		pClientConnection->Release();
	}
}

void CTrackerHttpServer::conn_eventcb(struct bufferevent* bev, short events, void* user_data)
{
	tool.Log(_T("CTrackerHttpServer::conn_eventcb bev:0x%08X, events:%d"), bev, events);
	CTrackerHttpServer* pThis = (CTrackerHttpServer*)user_data;
	if (events & BEV_EVENT_EOF)
	{
		tool.Log(_T("CTrackerHttpServer::conn_eventcb Connection closed."));
	}
	else if (events & BEV_EVENT_ERROR)
	{
		tool.Log(_T("CTrackerHttpServer::conn_eventcb Got an error on the connection: %s"), _tcserror(errno));
	}
	else
	{
		return;
	}
	if (pThis)
	{
		CAutoAddReleasePtr<CHttpClientConnection> spClientConnection;
		pThis->m_lockClient.Lock();
		MapBufferEvent::iterator it = pThis->m_mapClient.find(bev);
		if (it != pThis->m_mapClient.end())
		{
			spClientConnection = it->second;
			spClientConnection->Stop(RDEC_CONNECT_FAILED);
			pThis->m_mapClient.erase(it);
		}
		pThis->m_lockClient.Unlock();
	}
	bufferevent_free(bev);
}

void CTrackerHttpServer::listener_cb(struct evconnlistener* /*listener*/, evutil_socket_t fd, struct sockaddr* /*sa*/, int /*socklen*/, void* user_data)
{
	tool.Log(_T("CTrackerHttpServer::listener_cb"));
	CTrackerHttpServer* pThis = (CTrackerHttpServer*)user_data;
	struct event_base* base = pThis->m_pEventBase;
	struct bufferevent* bev;
	//生成一个bufferevent，用于读或者写
	bev = bufferevent_socket_new(base, fd, BEV_OPT_CLOSE_ON_FREE);
	tool.Log(_T("CTrackerHttpServer::listener_cb bev:0x%08X"), bev);
	if (!bev) 
	{
		tool.Log(_T("CTrackerHttpServer::listener_cb Error constructing bufferevent!"));
		return;
	}
	CAutoAddReleasePtr<CHttpClientConnection> spClientConnection = new CHttpClientConnection(base, fd, bev);
	if (spClientConnection == NULL || !spClientConnection->IsValid())
	{
		tool.Log(_T("CTrackerHttpServer::listener_cb Error new CClientConnection is not valid!"));
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
		tool.Log(_T("CTrackerHttpServer::listener_cb spClientConnection:0x%08X"), spClientConnection);
		spClientConnection->AddRef();
		pThis->m_lockClient.Lock();
		pThis->m_mapClient[bev] = spClientConnection;
		pThis->m_lockClient.Unlock();
	}
	else
	{
		tool.LogA("CTrackerHttpServer::listener_cb Error already connect! ip : %s", spClientConnection->GetIp().c_str());
		bufferevent_free(bev);
		return;
	}

	evutil_make_socket_nonblocking(fd);
	bufferevent_setcb(bev, conn_readcb, conn_writecb, conn_eventcb, user_data);
	bufferevent_enable(bev, EV_WRITE | EV_READ);
}

void CTrackerHttpServer::accept_error_cb(struct evconnlistener* listener, void* pParam)
{
	CTrackerHttpServer* pThis = (CTrackerHttpServer*)pParam;
	struct event_base *base = evconnlistener_get_base(listener);
	int err = EVUTIL_SOCKET_ERROR();
	tool.LogA("Got an error %d (%s) on the listener.", err, evutil_socket_error_to_string(err));

	event_base_loopexit(base, NULL);
}

unsigned __stdcall CTrackerHttpServer::DoStart(void* pParam)
{
	tool.Log(_T("CTrackerHttpServer::DoStart pParam:0x%08X"), pParam);
	CTrackerHttpServer* pThis = (CTrackerHttpServer*)pParam;

	// 创建event_base和evhttp
	pThis->m_pEventBase = event_base_new();
	if (!pThis->m_pEventBase) 
	{
		tool.Log(_T("Could not initialize libevent!"));
		return 1;
	}

	struct sockaddr_in sin;
	memset(&sin, 0, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_port = htons(pThis->m_uPort);
	sin.sin_addr.s_addr = inet_addr(pThis->m_strIp.c_str());

	struct evconnlistener* listener = evconnlistener_new_bind(pThis->m_pEventBase, listener_cb, (void*)pThis, LEV_OPT_REUSEABLE | LEV_OPT_CLOSE_ON_FREE | LEV_OPT_THREADSAFE, -1, (struct sockaddr*)&sin, sizeof(sin));

	if (!listener)
	{
		tool.Log(_T("Could not create a listener!"));
		return 2;
	}

	evconnlistener_set_error_cb(listener, accept_error_cb);
	event_base_dispatch(pThis->m_pEventBase);
	pThis->m_pEventBase = NULL;
	return 0;
}

bool CTrackerHttpServer::Start(ITrackerEvent* pTrackerEvent)
{
	tool.Log(_T("CRemoteServer::Start pTrackerEvent:0x%08X"), pTrackerEvent);
	if (m_hThread != NULL)
	{
		tool.Log(_T("libevent is already initialized!"));
		return true;
	}
	m_pTrackerEvent = pTrackerEvent;

	unsigned threadID = 0;
	m_hThread = (HANDLE)_beginthreadex(NULL, 0, &DoStart, (LPVOID)this, 0, &threadID);
	CloseHandle(m_hThread);

	if (m_hThread == NULL)
		return false;

	return true;
}

void CTrackerHttpServer::Stop()
{
	tool.Log(_T("CTrackerHttpServer::Stop"));
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
		event_base_loopexit(m_pEventBase, NULL);
	}
}

bool CTrackerHttpServer::PeerConnectPeer(CHttpClientConnection* pSrcHttpClientConnection, PeerInfo& d_peer, PeerInfo& s_peer)
{
	tool.LogA("CTrackerHttpServer::PeerConnectPeer d_peer:%s:%u, s_peer:%s:%u", d_peer.ip.c_str(), d_peer.tcp_port, s_peer.ip.c_str(), s_peer.tcp_port);
	CHttpClientConnection* pHttpClientConnection = NULL;
	m_lockClient.Lock();
	MapBufferEvent::iterator it = m_mapClient.begin();
	for (; it != m_mapClient.end(); it++)
	{
		if (it->second->GetIp() == d_peer.ip && it->second->GetPort() == d_peer.tcp_port)
		{
			pHttpClientConnection = it->second;
			pHttpClientConnection->AddRef();
			break;
		}
	}
	m_lockClient.Unlock();

	std::string strResponse;
	strResponse += "HTTP/1.1 200 OK\r\n";
	strResponse += "Connect-Peer: ";
	strResponse += s_peer.ip;
	strResponse += ":";

	char szPort[10] = {0};
	sprintf(szPort, "%u", s_peer.tcp_port);
	strResponse += szPort;
	strResponse += "\r\n";

	strResponse += "Content-Type: ";
	strResponse += HTTP_CONTENT_TYPE_TEXT_PLAIN;
	strResponse += "\r\n";
	strResponse += "Content-Length: 0\r\n\r\n";

	pHttpClientConnection->Send(strResponse.c_str(), strResponse.length());
	pHttpClientConnection->Release();
	return true;
}

bool CTrackerHttpServer::GetHttpClientConnection(CHttpClientConnection** ppHttpClientConnection, PeerInfo& peer)
{
	assert(*ppHttpClientConnection == NULL);
	*ppHttpClientConnection = NULL;
	m_lockClient.Lock();
	MapBufferEvent::iterator it = m_mapClient.begin();
	for (; it != m_mapClient.end(); it++)
	{
		if (it->second->GetIp() == peer.ip && it->second->GetPort() == peer.tcp_port)
		{
			(*ppHttpClientConnection) = it->second;
			(*ppHttpClientConnection)->AddRef();
			break;
		}
	}
	m_lockClient.Unlock();
	return *ppHttpClientConnection != NULL;
}
#pragma once
#include <event2/event.h>
#include <event2/bufferevent.h>
#include <event2/buffer.h>
#include <event2/listener.h>
#include <event2/util.h>
#include <event2/http.h>
#include <event2/http_struct.h>
#include <string>
#include <map>
#include "HttpClientConnection.h"

#include "..\common\AutoAddReleasePtr.h"
#include "..\common\lock.h"

typedef	std::map<bufferevent*, CHttpClientConnection*> MapBufferEvent;

class ITrackerEvent
{
public:
	virtual void OnPeerAvailable(const char* peer, unsigned short port) = 0;
	virtual void OnPeerLive(const char* peer, unsigned short port) = 0;
	virtual void OnPeerOffline(const char* peer, unsigned short port) = 0;
};

class CTrackerHttpServer :
	public CAddReleaseRef
{
public:
	CTrackerHttpServer(void);
	~CTrackerHttpServer(void);

	static CTrackerHttpServer* GetInstance()
	{
		static CTrackerHttpServer* s_pTrackerHttpServer = NULL;
		if (s_pTrackerHttpServer == NULL)
		{
			s_pTrackerHttpServer = new CTrackerHttpServer();
		}
		return s_pTrackerHttpServer;
	}

private:
	static unsigned __stdcall DoStart(void* pParam);

	static void conn_writecb( struct bufferevent *bev, void *user_data );
	static void conn_readcb( struct bufferevent *bev, void *user_data );
	static void conn_eventcb( struct bufferevent *bev, short events , void *user_data);
	static void listener_cb(struct evconnlistener *listener, evutil_socket_t fd, struct sockaddr *sa, int socklen, void *user_data);
	static void accept_error_cb(struct evconnlistener *listener, void *ctx);

public:
	bool Start(ITrackerEvent* pTrackerEvent);
	void Stop();
	bool PeerConnectPeer(CHttpClientConnection* pSrcHttpClientConnection, PeerInfo& d_peer, PeerInfo& s_peer);
	bool GetHttpClientConnection(CHttpClientConnection** ppHttpClientConnection, PeerInfo& peer);

private:
	ITrackerEvent*			m_pTrackerEvent;
	HANDLE					m_hThread;
	struct event_base*		m_pEventBase;
	std::string				m_strIp;
	unsigned short			m_uPort;

	MapBufferEvent			m_mapClient;
	CLock					m_lockClient;

};

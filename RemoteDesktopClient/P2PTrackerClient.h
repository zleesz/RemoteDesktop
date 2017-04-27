#pragma once

#include <event2/buffer.h>
#include <event2/event.h>
#include <event2/http.h>
#include <event2/http_struct.h>
#include <event2/bufferevent.h>
#include <event2/listener.h>
#include <event2/keyvalq_struct.h>

#include "..\common\def.h"
#include "..\common\asyn_dns.h"
#include "..\common\lock.h"

#include <queue>
#include <list>

class IP2PTrackerClientEvent
{
public:
	virtual void OnGetLivePeerList(std::list<PeerInfo>& listPeer) = 0;
	virtual void OnGetLivePeerFailed() = 0;
	virtual void OnConnectPeerResult(PeerInfo& peer, unsigned short port) = 0;
};

class CP2PTrackerClient
{
public:
	CP2PTrackerClient(void);
	virtual ~CP2PTrackerClient(void);

public:
	bool Init(IP2PTrackerClientEvent* pP2PTrackerClient, struct event_base* base);
	void Uninit();
	bool GetLivePeerList();
	bool ConnectPeer(PeerInfo& d_peer);
	void CloseClient();
	unsigned int GetIp();
	unsigned short GetPort();

private:
	static void HttpRequestDoneCallback(struct evhttp_request* req, void* arg);
	static void OnAsynSendHttp(evutil_socket_t fd, short event,void *arg);

	static void conn_writecb(struct bufferevent* bev, void* user_data);
	static void conn_readcb(struct bufferevent* bev, void* user_data);
	static void conn_eventcb(struct bufferevent* bev, short events, void* user_data);

	bool Connect();
	void DecodeBuffer();
	void DecodeFirstLine();
	void DecodeHeader();
	void DecodeOneHeader(std::string& strOneHeader);
	void DecodeBody();
	void Clear();

	void OnHttpResponse();
	bool SendHttpPackage(evhttp_cmd_type t, const char* path, struct evkeyvalq* headers, std::string& strBody);
	void OnConnectPeerResult();

private:
	typedef enum HTTP_BUFFER_STATE
	{
		HBS_READY		= 0,
		HBS_HEADER		= 1,
		HBS_BODY		= 2,
	}HTTP_BUFFER_STATE;

	typedef enum P2PTRACKER_STATE
	{
		P2PTS_INVALID		= 0,
		P2PTS_DNS			= 1,
		P2PTS_CONNECT		= 2,
		P2PTS_ONLINE		= 3,
		P2PTS_PING			= 4,
		P2PTS_OFFLINE		= 5,
	}P2PTRACKER_STATE;

	IP2PTrackerClientEvent*		m_pP2PTrackerClientEvent;
	struct event_base*			m_pEventBase;
	struct bufferevent*			m_pBufferEvent;
	struct event*				m_pAsynSendHttpEvent;
	unsigned int				m_ip;
	unsigned short				m_port;
	HTTP_BUFFER_STATE			m_nHttpBufferState;
	std::string					m_strResponseBuffer;
	int							m_nResponseCode;
	struct evkeyvalq			m_ResponseHeaders;
	std::string					m_strResponseBody;

	unsigned int				m_nPingFailedTimes;
	unsigned short				m_uLocalPort;

	typedef struct AsynHttpItem
	{
		std::string			strPath;
		std::string			strBody;
		struct evkeyvalq	RequestHeaders;
		AsynHttpItem()
		{
			TAILQ_INIT(&RequestHeaders);
		}
		~AsynHttpItem()
		{
			evhttp_clear_headers(&RequestHeaders);
		}
	}AsynHttpItem;
	std::queue<AsynHttpItem*>	m_queueAsynSendHttpItem;
	CLock						m_lockSendHttpItemQueue;
};

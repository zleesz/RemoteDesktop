#include "StdAfx.h"
#include "P2PTrackerClient.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

CP2PTrackerClient::CP2PTrackerClient(void) :
	m_pP2PTrackerClientEvent(NULL),
	m_pEventBase(NULL),
	m_pBufferEvent(NULL),
	m_pAsynSendHttpEvent(NULL),
	m_nHttpBufferState(HBS_READY),
	m_nResponseCode(0),
	m_nPingFailedTimes(0),
	m_uLocalPort(0)
{
	m_ip = inet_addr(RD_REMOTE_P2PTRACKER_SERVER_HOST);
	m_port = RD_REMOTE_P2PTRACKER_SERVER_PORT;
	TAILQ_INIT(&m_ResponseHeaders);
}

CP2PTrackerClient::~CP2PTrackerClient(void)
{
	tool.Log(_T("CP2PTrackerClient::~CP2PTrackerClient"));
}

bool CP2PTrackerClient::Connect()
{
	if (INADDR_NONE == m_ip)
	{
		asyn_dns dns;
		std::string ip;
		if (dns.syn_get_host_by_name(RD_REMOTE_P2PTRACKER_SERVER_HOST, ip))
		{
			tool.LogA("syn_get_host_by_name ip:%s", ip.c_str());
			m_ip = inet_addr(ip.c_str());
		}
		else
		{
			tool.LogA("syn_get_host_by_name failed!");
			return false;
		}
	}

	struct sockaddr_in sin;
	memset(&sin, 0, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = m_ip;
	sin.sin_port = htons(m_port);

	m_pBufferEvent = bufferevent_socket_new(m_pEventBase, -1, BEV_OPT_CLOSE_ON_FREE | LEV_OPT_THREADSAFE);
	bufferevent_setcb(m_pBufferEvent, conn_readcb, conn_writecb, conn_eventcb, this);
	bufferevent_enable(m_pBufferEvent, EV_WRITE | EV_READ);

	if (bufferevent_socket_connect(m_pBufferEvent, (struct sockaddr*)&sin, sizeof(sin)) == 0)
	{
		evutil_socket_t fd = bufferevent_getfd(m_pBufferEvent);
		int n = evutil_make_listen_socket_reuseable(fd);
		tool.Log(_T("CP2PTrackerClient::Connect evutil_make_listen_socket_reuseable n:%d"), n);

		sockaddr_in addr;
		memset(&addr, 0, sizeof(addr));
		int len = sizeof(addr);
		getsockname(fd, (sockaddr*)&addr, &len);

		m_uLocalPort = addr.sin_port;
		m_pAsynSendHttpEvent = event_new(m_pEventBase, -1, EV_SIGNAL|EV_PERSIST, OnAsynSendHttp, this);
		tool.Log(_T("connect success. m_pAsynSendPingEvent:0x%08X"), m_pAsynSendHttpEvent);
		return true;
	}
	else
	{
		tool.Log(_T("connect failed!"));
		bufferevent_free(m_pBufferEvent);
		m_pBufferEvent = NULL;
		return false;
	}
}

bool CP2PTrackerClient::Init(IP2PTrackerClientEvent* pP2PTrackerClient, struct event_base* base)
{
	tool.Log(_T("CP2PTrackerClient::Init IP2PTrackerClient:0x%08X, base:0x%08X"), pP2PTrackerClient, base);
	m_pP2PTrackerClientEvent = pP2PTrackerClient;
	m_pEventBase = base;

	return Connect();
}

void CP2PTrackerClient::Uninit()
{
	CloseClient();
	m_pEventBase = NULL;
}

void CP2PTrackerClient::CloseClient()
{
	if (m_pAsynSendHttpEvent)
	{
		event_del(m_pAsynSendHttpEvent);
		event_free(m_pAsynSendHttpEvent);
		m_pAsynSendHttpEvent = NULL;
	}
	if (m_pBufferEvent)
	{
		evutil_socket_t fd = bufferevent_getfd(m_pBufferEvent);
		bufferevent_free(m_pBufferEvent);
		m_pBufferEvent = NULL;
		evutil_closesocket(fd);
	}
	Clear();
	m_nPingFailedTimes = 0;
	m_lockSendHttpItemQueue.Lock();
	while (m_queueAsynSendHttpItem.size() > 0)
	{
		AsynHttpItem* pItem = m_queueAsynSendHttpItem.front();
		delete pItem;
		m_queueAsynSendHttpItem.pop();
	}
	m_lockSendHttpItemQueue.Unlock();
}

unsigned int CP2PTrackerClient::GetIp()
{
	return m_ip;
}

unsigned short CP2PTrackerClient::GetPort()
{
	return m_port;
}

void CP2PTrackerClient::conn_writecb(struct bufferevent* bev, void* /*user_data*/)
{
	tool.Log(_T("CP2PTrackerClient::conn_writecb"));
	struct evbuffer *output = bufferevent_get_output(bev);
	if (evbuffer_get_length (output) == 0)
	{
	}
}

void CP2PTrackerClient::conn_readcb(struct bufferevent* bev, void* user_data)
{
	tool.Log(_T("CP2PTrackerClient::conn_readcb"));
	CP2PTrackerClient* pThis = (CP2PTrackerClient*)user_data;
	struct evbuffer* input = bufferevent_get_input(bev);
	size_t total = evbuffer_get_length(input);
	if (total <= 0)
		return;

	char* buf = new char[total+1];
	memset(buf, 0, total+1);
	evbuffer_remove(input, buf, total);
	pThis->m_strResponseBuffer.append(buf, total);
	delete[] buf;
	pThis->DecodeBuffer();
}

void CP2PTrackerClient::conn_eventcb(struct bufferevent* bev, short events, void* user_data)
{
	tool.Log(_T("CP2PTrackerClient::conn_eventcb bev:0x%08X, events:%d"), bev, events);
	CP2PTrackerClient* pThis = (CP2PTrackerClient*)user_data;
	if (events & BEV_EVENT_EOF)
	{
		tool.Log(_T("Connection closed."));
	}
	else if (events & BEV_EVENT_ERROR)
	{
		tool.Log(_T("Got an error on the connection: %s"), _tcserror(errno));
	}
}

void CP2PTrackerClient::Clear()
{
	m_nHttpBufferState = HBS_READY;
	m_nResponseCode = 0;
	m_strResponseBuffer.clear();
	evhttp_clear_headers(&m_ResponseHeaders);
	m_strResponseBody.clear();
}

void CP2PTrackerClient::DecodeFirstLine()
{
	if (m_strResponseBuffer.find("HTTP/1.1") != 0)
	{
		Clear();
		return;
	}
	size_t pos = m_strResponseBuffer.find("\r\n");
	if (pos != std::string::npos)
	{
		size_t pos1 = m_strResponseBuffer.find(' ');
		m_nResponseCode = atoi(m_strResponseBuffer.c_str() + pos1);
		m_nHttpBufferState = HBS_HEADER;
		m_strResponseBuffer.erase(0, pos+2);
		tool.LogA("m_strResponseBuffer:\r\n%s", m_strResponseBuffer.c_str());
		DecodeHeader();
	}
}

void CP2PTrackerClient::DecodeHeader()
{
	// 判断是否完整的响应头
	size_t pos = m_strResponseBuffer.find("\r\n\r\n");
	if (pos == std::string::npos)
		return;
	size_t pos_begin = 0;
	size_t pos1 = m_strResponseBuffer.find("\r\n");
	while (pos1 <= pos)
	{
		std::string strOneHeader = m_strResponseBuffer.substr(pos_begin, pos1-pos_begin);;
		DecodeOneHeader(strOneHeader);
		pos_begin = pos1+2;
		pos1 = m_strResponseBuffer.find("\r\n", pos_begin);
	}
	m_strResponseBuffer.erase(0, pos+4);
	m_nHttpBufferState = HBS_BODY;
	DecodeBody();
}

void CP2PTrackerClient::DecodeOneHeader(std::string& strOneHeader)
{
	size_t pos = strOneHeader.find(':');
	if (pos > 0 && pos != std::string::npos)
	{
		size_t pos1 = strOneHeader.find_first_not_of(' ', pos+1);
		if (pos1 != std::string::npos)
		{
			std::string strKey = strOneHeader.substr(0, pos);
			std::string strValue = strOneHeader.substr(pos1);
			evhttp_add_header(&m_ResponseHeaders, strKey.c_str(), strValue.c_str());
		}
	}
}

void CP2PTrackerClient::DecodeBody()
{
	// 先只支持 Content-Length，不支持chunked
	const char* pContentLength = evhttp_find_header(&m_ResponseHeaders, "Content-Length");
	int length = 0;
	if (pContentLength && strlen(pContentLength) > 0)
	{
		length = atoi(pContentLength);
	}
	if (length > (int)m_strResponseBuffer.length())
	{
		// body没返回完全
		return;
	}
	else if (length == (int)m_strResponseBuffer.length())
	{
		m_strResponseBody = m_strResponseBuffer;
		OnHttpResponse();
		Clear();
	}
	else
	{
		m_strResponseBody = m_strResponseBuffer.substr(0, length);
		m_strResponseBuffer.erase(0, length);
		OnHttpResponse();

		std::string strNextResponseBuffer(m_strResponseBuffer);
		Clear();
		m_strResponseBuffer = strNextResponseBuffer;

		// 解析下个包
		DecodeFirstLine();
	}
}

void CP2PTrackerClient::DecodeBuffer()
{
	switch (m_nHttpBufferState)
	{
	case HBS_READY:
		DecodeFirstLine();
		break;
	case HBS_HEADER:
		DecodeHeader();
		break;
	case HBS_BODY:
		DecodeBody();
		break;
	default:
		break;
	}
}

void CP2PTrackerClient::OnHttpResponse()
{
	tool.LogA("CP2PTrackerClient::OnHttpResponse code:%d, body:%s", m_nResponseCode, m_strResponseBody.c_str());
	const char* pPeerResult = evhttp_find_header(&m_ResponseHeaders, "Connect-Peer-Result");
	if (pPeerResult)
	{
		// 客户端的打洞回包的响应
		tool.LogA("CP2PTrackerClient::OnHttpResponse Connect-Peer-Result: %s", pPeerResult);
		OnConnectPeerResult();
		return;
	}
	if (m_strResponseBody.find("result=0") == 0)
	{
		std::list<PeerInfo> listPeer;
		size_t pos = m_strResponseBody.find("peer=");
		if (pos != std::string::npos)
		{
			size_t pos_begin = pos + 5;
			size_t pos1 = m_strResponseBody.find(',', pos_begin);
			while (pos1 != std::string::npos)
			{
				std::string strPeerAddr = m_strResponseBody.substr(pos_begin, pos1 - pos_begin);
				size_t pos2 = strPeerAddr.find(':');
				if (pos2 != std::string::npos)
				{
					std::string ip = strPeerAddr.substr(0, pos2);
					unsigned short port = (unsigned short)atoi(strPeerAddr.substr(pos2+1).c_str());
					PeerInfo peer(ip, port);
					listPeer.push_back(peer);
				}
				pos_begin = pos1 + 1;
				pos1 = m_strResponseBody.find(',', pos_begin);
			}
		}
		m_pP2PTrackerClientEvent->OnGetLivePeerList(listPeer);
	}
	else
	{
		tool.LogA("CP2PTrackerClient::OnHttpResponse OnGetLivePeer failed!");
		m_pP2PTrackerClientEvent->OnGetLivePeerFailed();
	}
}

void CP2PTrackerClient::OnAsynSendHttp(evutil_socket_t /*fd*/, short /*event*/,void* arg)
{
	CP2PTrackerClient* pThis = (CP2PTrackerClient*)arg;

	if (pThis->m_pBufferEvent == NULL)
	{
		tool.Log(_T("CP2PTrackerClient::OnAsynSendHttp m_pBufferEvent == NULL!"));
		return;
	}

	pThis->m_lockSendHttpItemQueue.Lock();
	tool.Log(_T("CRemoteClient::OnAsynSendCommand, m_queueAsynCommand.size:%d"), pThis->m_queueAsynSendHttpItem.size());
	while (pThis->m_queueAsynSendHttpItem.size() > 0)
	{
		AsynHttpItem* pItem = pThis->m_queueAsynSendHttpItem.front();
		pThis->m_queueAsynSendHttpItem.pop();

		struct evkeyvalq headers;
		TAILQ_INIT(&headers);
		struct evkeyval* node = pItem->RequestHeaders.tqh_first;  
		while (node)
		{
			evhttp_add_header(&headers, node->key, node->value);
			node = node->next.tqe_next;
		}
		if (evhttp_find_header(&headers, "Host") == NULL)
		{
			evhttp_add_header(&headers, "Host", RD_REMOTE_P2PTRACKER_SERVER_HOST);
		}
		if (evhttp_find_header(&headers, "Connection") == NULL)
		{
			evhttp_add_header(&headers, "Connection", "Keep-Alive");
		}
		if (evhttp_find_header(&headers, "Content-Type") == NULL)
		{
			evhttp_add_header(&headers, "Content-Type", HTTP_CONTENT_TYPE_TEXT_PLAIN);
		}
		if (evhttp_find_header(&headers, "User-Agent") == NULL)
		{
			evhttp_add_header(&headers, "User-Agent", "P2PTrackerClient");
		}

		pThis->SendHttpPackage(EVHTTP_REQ_POST, pItem->strPath.c_str(), &headers, pItem->strBody);
		evhttp_clear_headers(&headers);
		delete pItem;
	}
	pThis->m_lockSendHttpItemQueue.Unlock();
}

bool CP2PTrackerClient::SendHttpPackage(evhttp_cmd_type t, const char* path, struct evkeyvalq* headers, std::string& strBody)
{
	std::string strPackage;
	if (t == EVHTTP_REQ_GET)
	{
		strPackage += "GET ";
	}
	else if (t == EVHTTP_REQ_POST)
	{
		strPackage += "POST ";
	}
	else
	{
		return false;
	}
	strPackage += path;
	strPackage += " HTTP/1.1\r\n";

	struct evkeyval* kv = headers->tqh_first;
	while (kv)
	{
		strPackage += kv->key;
		strPackage += ": ";
		strPackage += kv->value;
		strPackage += "\r\n";
		kv = kv->next.tqe_next;
	}
	if (evhttp_find_header(headers, "Content-Length") == NULL)
	{
		strPackage += "Content-Length: ";
		char s[100] = {0};
		_itoa(strBody.length(), s, 10);
		strPackage += s;
		strPackage += "\r\n";
	}
	strPackage += "\r\n";
	strPackage += strBody;
	int n = bufferevent_write(m_pBufferEvent, strPackage.c_str(), strPackage.length());
	tool.LogA("bufferevent_write n:%d, strPackage:%s", n, strPackage.c_str());
	return n == 0;
}

bool CP2PTrackerClient::GetLivePeerList()
{
	if (m_pBufferEvent == NULL || m_pAsynSendHttpEvent == NULL)
	{
		tool.Log(_T("CP2PTrackerClient::SendPing bufferevent invalid! m_pBufferEvent:0x%08X, m_pAsynSendHttpEvent:0x%08X"), m_pBufferEvent, m_pAsynSendHttpEvent);
		return false;
	}
	if (!m_pAsynSendHttpEvent)
	{
		tool.Log(_T("CP2PTrackerClient::SendPing m_pAsynSendHttpEvent == NULL!"));
		return false;
	}
	AsynHttpItem* pItem = new AsynHttpItem();
	pItem->strPath = "/p";
	pItem->strBody = "c=g";

	m_lockSendHttpItemQueue.Lock();
	m_queueAsynSendHttpItem.push(pItem);
	m_lockSendHttpItemQueue.Unlock();
	event_active(m_pAsynSendHttpEvent, EV_SIGNAL, 1);
	return true;
}

bool CP2PTrackerClient::ConnectPeer(PeerInfo& d_peer)
{
	if (m_pBufferEvent == NULL || m_pAsynSendHttpEvent == NULL)
	{
		tool.Log(_T("CP2PTrackerClient::SendPing bufferevent invalid! m_pBufferEvent:0x%08X, m_pAsynSendHttpEvent:0x%08X"), m_pBufferEvent, m_pAsynSendHttpEvent);
		return false;
	}
	if (!m_pAsynSendHttpEvent)
	{
		tool.Log(_T("CP2PTrackerClient::SendPing m_pAsynSendHttpEvent == NULL!"));
		return false;
	}
	AsynHttpItem* pItem = new AsynHttpItem();
	pItem->strPath = "/c";

	char szHeader[100] = {0};
	sprintf(szHeader, "d_peer=%s:%u", d_peer.ip.c_str(), d_peer.tcp_port);
	evhttp_add_header(&pItem->RequestHeaders, "Connect-Peer", szHeader);

	m_lockSendHttpItemQueue.Lock();
	m_queueAsynSendHttpItem.push(pItem);
	m_lockSendHttpItemQueue.Unlock();
	event_active(m_pAsynSendHttpEvent, EV_SIGNAL, 1);
	return true;
}

void CP2PTrackerClient::OnConnectPeerResult()
{
	const char* pPeerResult = evhttp_find_header(&m_ResponseHeaders, "Connect-Peer-Result");
	std::string input_buf_str("/?");
	input_buf_str += pPeerResult;

	struct evkeyvalq params;
	evhttp_parse_query(input_buf_str.c_str(), &params);
	const char* result = evhttp_find_header(&params, "result");
	const char* ppeer = evhttp_find_header(&params, "peer");
	tool.LogA("CP2PTrackerClient::OnConnectPeerResult result:%s, peer:%s", result, ppeer);
	if (result == NULL || strcmp(result, "0") != 0 || ppeer == NULL)
		return;

	std::string strPeer(ppeer);
	size_t pos = strPeer.find(':');
	if (pos == std::string::npos)
		return;

	PeerInfo peer;
	peer.ip = strPeer.substr(0, pos);
	peer.tcp_port = (unsigned short)atoi(strPeer.substr(pos+1).c_str());

	m_pP2PTrackerClientEvent->OnConnectPeerResult(peer, m_uLocalPort);
}

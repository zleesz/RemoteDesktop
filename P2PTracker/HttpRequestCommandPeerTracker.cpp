#include "stdafx.h"
#include "HttpRequestCommandPeerTracker.h"

#include <event2/event.h>
#include <event2/http.h>
#include <event2/buffer.h>
#include <event2/keyvalq_struct.h>

CHttpRequestCommandPeerTracker* CHttpRequestCommandPeerTracker::s_pHttpRequestCommandPeerTracker = NULL;

CHttpRequestCommandPeerTracker::CHttpRequestCommandPeerTracker(void)
{
	tool.Log(_T("CHttpRequestCommandPeerTracker::CHttpRequestCommandPeerTracker"));
}

CHttpRequestCommandPeerTracker::~CHttpRequestCommandPeerTracker(void)
{
	tool.Log(_T("CHttpRequestCommandPeerTracker::~CHttpRequestCommandPeerTracker"));
}

void CHttpRequestCommandPeerTracker::Init()
{
	tool.Log(_T("CHttpRequestCommandPeerTracker::Init"));
	Create(HWND_MESSAGE);
	SetTimer(0, RD_P2PTRACKER_PEER_HOLD_TIME);
}

void CHttpRequestCommandPeerTracker::Uninit()
{
	tool.Log(_T("CHttpRequestCommandPeerTracker::Uninit"));
	if (m_hWnd)
	{
		KillTimer(0);
		DestroyWindow();
	}
}

LRESULT CHttpRequestCommandPeerTracker::OnTimer(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	PeerMap::iterator it = m_mapPeer.begin();
	for (; it != m_mapPeer.end();)
	{
		if (it->second.state == RDPS_ONLINE)
		{
			it->second.offline_times++;
			if (it->second.offline_times > 5)
			{
				it->second.state = RDPS_PING_OFFLINE;
				tool.LogA("CHttpRequestCommandPeerTracker::OnTimer ip:%s, port:%u, RDPS_PING_OFFLINE", it->first.ip.c_str(), it->first.tcp_port);
				it = m_mapPeer.erase(it);
				continue;
			}
		}
		it++;
	}
	return 0;
}

void CHttpRequestCommandPeerTracker::Send200(bufferevent* bufferev, int result, std::string* strExtra)
{
	tool.Log(_T("CHttpRequestCommandPeerTracker::Send200 result:%d"), result);

	std::string strResponse;
	strResponse += "HTTP/1.1 200 OK\r\n";
	strResponse += "Content-Type: ";
	strResponse += HTTP_CONTENT_TYPE_TEXT_PLAIN;
	strResponse += "\r\n";
	strResponse += "Content-Length: ";

	char szBody[100] = {0};
	sprintf(szBody, "result=%d", result);

	char szSize[100] = {0};
	if (strExtra && strExtra->length() > 0)
	{
		sprintf(szSize, "%d", strlen(szBody) + 1 + strExtra->length());
	}
	else
	{
		sprintf(szSize, "%d", strlen(szBody));
	}

	strResponse += szSize;
	strResponse += "\r\n\r\n";
	strResponse += szBody;
	if (strExtra && strExtra->length() > 0)
	{
		strResponse += "&";
		strResponse += *strExtra;
	}

	tool.LogA("CHttpRequestCommandPeerTracker::Send200\r\n%s", strResponse.c_str());
	bufferevent_write(bufferev, strResponse.c_str(), strResponse.length());
}

bool CHttpRequestCommandPeerTracker::OnPeerOnline(CHttpClientConnection* pHttpClientConnection, const char* ip, unsigned short port, struct evkeyvalq* /*params*/)
{
	tool.LogA("CHttpRequestCommandPeerTracker::OnPeerOnline ip:%s, port:%u", ip, port);
	struct bufferevent* bufferev = pHttpClientConnection->GetBufferEvent();
	PeerInfo peer(ip, port);
	PeerMap::iterator it = m_mapPeer.find(peer);
	if (it == m_mapPeer.end())
	{
		PeerState state;
		state.online_tm = time(NULL);
		m_mapPeer.insert(std::make_pair(peer, state));
		it = m_mapPeer.find(peer);
	}
	tool.LogA("ip:%s, port:%u", it->first.ip.c_str(), it->first.tcp_port);
	it->second.state = RDPS_ONLINE;
	it->second.offline_times = 0;

	Send200(bufferev, 0);
	return true;
}

bool CHttpRequestCommandPeerTracker::OnPeerPing(CHttpClientConnection* pHttpClientConnection, const char* ip, unsigned short port, struct evkeyvalq* /*params*/)
{
	tool.LogA("CHttpRequestCommandPeerTracker::OnPeerPing ip:%s, port:%u", ip, port);
	struct bufferevent* bufferev = pHttpClientConnection->GetBufferEvent();
	PeerInfo peer(ip, port);
	PeerMap::iterator it = m_mapPeer.find(peer);
	if (it == m_mapPeer.end())
	{
		return false;
	}
	it->second.offline_times = 0;

	Send200(bufferev, 0);
	return true;
}

bool CHttpRequestCommandPeerTracker::OnPeerOffline(CHttpClientConnection* pHttpClientConnection, const char* ip, unsigned short port, struct evkeyvalq* /*params*/)
{
	tool.LogA("CHttpRequestCommandPeerTracker::OnPeerOffline ip:%s, port:%u", ip, port);
	struct bufferevent* bufferev = pHttpClientConnection->GetBufferEvent();
	PeerInfo peer(ip, port);
	PeerMap::iterator it = m_mapPeer.find(peer);
	if (it == m_mapPeer.end())
	{
		return false;
	}
	it->second.state = RDPS_OFFLINE;

	Send200(bufferev, 0);
	m_mapPeer.erase(it);
	return true;
}

bool CHttpRequestCommandPeerTracker::OnPeerGetLive(CHttpClientConnection* pHttpClientConnection, const char* ip, unsigned short port, struct evkeyvalq* /*params*/)
{
	tool.LogA("CHttpRequestCommandPeerTracker::OnPeerGetLive ip:%s, port:%u", ip, port);
	struct bufferevent* bufferev = pHttpClientConnection->GetBufferEvent();
	PeerList lstPeer;
	GetLivePeer(lstPeer);
	std::string buffer("peer=");
	PeerList::iterator it = lstPeer.begin();
	for (; it != lstPeer.end(); it++)
	{
		char peer[100] = {0};
		sprintf(peer, "%s:%u,", it->ip.c_str(), it->tcp_port);
		buffer.append(peer);
	}

	Send200(bufferev, 0, &buffer);
	return true;
}

bool CHttpRequestCommandPeerTracker::Execute(CHttpClientConnection* pHttpClientConnection, struct evkeyvalq* headers, std::string& strBody)
{
	tool.LogA("CHttpRequestCommandPeerTracker::Execute");

	const char* ip_header = evhttp_find_header(headers, "X_FORWARDED_FOR");
	const char* port_header = evhttp_find_header(headers, "X_FORWARDED_FOR_PORT");

	tool.LogA("client ip:%s, port:%s", ip_header, port_header);
	if (ip_header == NULL || port_header == NULL || strlen(ip_header) <= 0 || strlen(port_header) <= 0)
		return false;

	std::string input_buf_str("/?");
	input_buf_str += strBody;

	struct evkeyvalq params;
	evhttp_parse_query(input_buf_str.c_str(), &params);
	const char* c = evhttp_find_header(&params, "c");
	tool.LogA("param c=%s", c);
	if (c == NULL)
		return false;

	unsigned short port = (unsigned short)atoi(port_header);

	switch (c[0])
	{
	case 's':
		// start
		return OnPeerOnline(pHttpClientConnection, ip_header, port, &params);
	case 'p':
		// ping
		return OnPeerPing(pHttpClientConnection, ip_header, port, &params);
	case 'g':
		// get
		return OnPeerGetLive(pHttpClientConnection, ip_header, port, &params);
	case 'e':
		// end
		return OnPeerOffline(pHttpClientConnection, ip_header, port, &params);
	default:
		break;
	}
	return false;
}

void CHttpRequestCommandPeerTracker::GetLivePeer(PeerList& lstPeer)
{
	PeerMap::iterator it = m_mapPeer.begin();
	for (; it != m_mapPeer.end(); it++)
	{
		if (it->second.state == RDPS_ONLINE)
		{
			lstPeer.push_back(it->first);
		}
	}
}

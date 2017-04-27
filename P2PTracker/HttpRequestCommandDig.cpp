#include "stdafx.h"
#include "HttpRequestCommandDig.h"

CHttpRequestCommandDig* CHttpRequestCommandDig::s_pHttpRequestCommandDig = NULL;

CHttpRequestCommandDig::CHttpRequestCommandDig(void)
{
}

CHttpRequestCommandDig::~CHttpRequestCommandDig(void)
{
	tool.Log(_T("CHttpRequestCommandDig::~CHttpRequestCommandDig"));
}

void CHttpRequestCommandDig::Init()
{
	tool.Log(_T("CHttpRequestCommandDig::Init"));
}

void CHttpRequestCommandDig::Uninit()
{
	tool.Log(_T("CHttpRequestCommandDig::Uninit"));
}

void CHttpRequestCommandDig::Send200(CHttpClientConnection* pHttpClientConnection, int result, struct evkeyvalq* pheaders, std::string* strExtra)
{
	tool.Log(_T("CHttpRequestCommandPeerTracker::Send200 result:%d"), result);
	struct bufferevent* pBufferEvent = pHttpClientConnection->GetBufferEvent();

	std::string strResponse;
	strResponse += "HTTP/1.1 200 OK\r\n";

	struct evkeyvalq headers;
	TAILQ_INIT(&headers);
	if (pheaders)
	{
		struct evkeyval* node = pheaders->tqh_first;  
		while (node)
		{
			evhttp_add_header(&headers, node->key, node->value);
			node = node->next.tqe_next;
		}
	}
	if (evhttp_find_header(&headers, "Content-Type") == NULL)
	{
		evhttp_add_header(&headers, "Content-Type", HTTP_CONTENT_TYPE_TEXT_PLAIN);
	}
	struct evkeyval* node = headers.tqh_first;  
	while (node)
	{
		strResponse += node->key;
		strResponse += ":";
		strResponse += node->value;
		strResponse += "\r\n";
		node = node->next.tqe_next;
	}
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
	bufferevent_write(pBufferEvent, strResponse.c_str(), strResponse.length());
}

bool CHttpRequestCommandDig::Execute(CHttpClientConnection* pHttpClientConnection, struct evkeyvalq* headers, std::string& strBody)
{
	tool.LogA("CHttpRequestCommandDig::Execute, body:%s", strBody.c_str());
	const char* pConnectPeerResult = evhttp_find_header(headers, "Connect-Peer-Result");
	if (pConnectPeerResult)
	{
		tool.LogA("CHttpRequestCommandDig::Execute, pConnectPeerResult:%s", pConnectPeerResult);
		return OnRequestPeerConnectPeerResult(pHttpClientConnection, headers, strBody);
	}
	const char* pConnectPeer = evhttp_find_header(headers, "Connect-Peer");
	if (pConnectPeer)
	{
		tool.LogA("CHttpRequestCommandDig::Execute, pConnectPeer:%s", pConnectPeer);
		return OnRequestConnectPeer(pHttpClientConnection, headers, strBody);
	}
	return false;
}

bool CHttpRequestCommandDig::OnRequestConnectPeer(CHttpClientConnection* pHttpClientConnection, struct evkeyvalq* headers, std::string& /*strBody*/)
{
	const char* pConnectPeer = evhttp_find_header(headers, "Connect-Peer");
	std::string input_buf_str("/?");
	input_buf_str += pConnectPeer;

	struct evkeyvalq params;
	evhttp_parse_query(input_buf_str.c_str(), &params);
	const char* pd_peer = evhttp_find_header(&params, "d_peer");
	tool.LogA("param pd_peer=%s", pd_peer);
	if (pd_peer == NULL)
	{
		Send200(pHttpClientConnection, -1);
		return false;
	}
	std::string strPeer_d(pd_peer);
	size_t pos = strPeer_d.find(':');
	if (pos == std::string::npos)
	{
		Send200(pHttpClientConnection, -2);
		return false;
	}
	PeerInfo d_peer, s_peer;
	d_peer.ip = strPeer_d.substr(0, pos);
	d_peer.tcp_port = (unsigned short)atoi(strPeer_d.substr(pos+1).c_str());
	s_peer.ip = pHttpClientConnection->GetIp();
	s_peer.tcp_port = pHttpClientConnection->GetPort();

	CTrackerHttpServer* pTrackerHttpServer = CTrackerHttpServer::GetInstance();
	pTrackerHttpServer->PeerConnectPeer(pHttpClientConnection, d_peer, s_peer);
	return true;
}

bool CHttpRequestCommandDig::OnRequestPeerConnectPeerResult(CHttpClientConnection* pHttpClientConnection, struct evkeyvalq* headers, std::string& /*strBody*/)
{
	const char* pConnectPeerResult = evhttp_find_header(headers, "Connect-Peer-Result");
	std::string input_buf_str("/?");
	input_buf_str += pConnectPeerResult;

	struct evkeyvalq params;
	evhttp_parse_query(input_buf_str.c_str(), &params);
	const char* result = evhttp_find_header(&params, "result");
	const char* ppeer = evhttp_find_header(&params, "peer");
	tool.LogA("CHttpRequestCommandDig::OnRequestPeerConnectPeerResult result:%s, peer:%s", result, ppeer);
	if (result == NULL || ppeer == NULL)
		return false;

	std::string strPeer(ppeer);
	size_t pos = strPeer.find(':');
	if (pos == std::string::npos)
		return false;

	PeerInfo peer;
	peer.ip = strPeer.substr(0, pos);
	peer.tcp_port = (unsigned short)atoi(strPeer.substr(pos+1).c_str());

	CTrackerHttpServer* pTrackerHttpServer = CTrackerHttpServer::GetInstance();
	CHttpClientConnection* pDestHttpClientConnection = NULL;
	pTrackerHttpServer->GetHttpClientConnection(&pDestHttpClientConnection, peer);
	if (pDestHttpClientConnection == NULL)
		return false;

	PeerInfo destPeer;
	destPeer.ip = pHttpClientConnection->GetIp();
	destPeer.tcp_port = pHttpClientConnection->GetPort();

	std::string strHeader;
	strHeader += "result=";
	strHeader += result;
	strHeader += "&peer=";
	strHeader += destPeer.ip;
	strHeader += ":";
	char szPort[10] = {0};
	sprintf(szPort, "%u", destPeer.tcp_port);
	strHeader += szPort;
	struct evkeyvalq resultHeaders;
	TAILQ_INIT(&resultHeaders);
	evhttp_add_header(&resultHeaders, "Connect-Peer-Result", strHeader.c_str());
	tool.LogA("CHttpRequestCommandDig::OnRequestPeerConnectPeerResult Response Connect-Peer-Result:%s", strHeader.c_str());

	Send200(pDestHttpClientConnection, atoi(result), &resultHeaders);
	return true;
}
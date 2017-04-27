#include "stdafx.h"
#include "HttpClientConnection.h"

#include "..\common\socket_util.h"
#include "..\common\AutoAddReleasePtr.h"
#include "HttpRequestCommandPeerTracker.h"
#include "HttpRequestCommandDig.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

CHttpClientConnection::CHttpClientConnection(struct event_base* base, SOCKET socket, bufferevent* bufferev) :
	m_pEventBase(base),
	m_bufferev(bufferev),
	m_state(RDCS_INVALID),
	m_uPort(0),
	m_socket(socket),
	m_nHttpBufferState(HBS_READY)
{
	tool.Log(_T("CHttpClientConnection::CHttpClientConnection base:0x%08X, socket:%d, bufferev:0x%08X, pClientConnectionEvent:"), base, socket, bufferev);
	m_pAsynSendHttpEvent = event_new(m_pEventBase, -1, EV_SIGNAL|EV_PERSIST, OnAsynSendCommand, this);
	if (m_pAsynSendHttpEvent)
	{
		// event 引用了一个计数，删除event时再减
		AddRef();
	}
	TAILQ_INIT(&m_RequestHeaders);
	InitHttpRequestCommandMap();
}

CHttpClientConnection::~CHttpClientConnection(void)
{
	tool.Log(_T("CHttpClientConnection::~CHttpClientConnection"));
}

void CHttpClientConnection::Send404()
{
	std::string strResponse;
	strResponse += "HTTP/1.1 404 Not Found\r\n";
	strResponse += "Content-Type: ";
	strResponse += HTTP_CONTENT_TYPE_TEXT_PLAIN;
	strResponse += "\r\n";
	strResponse += "Content-Length: 0\r\n\r\n";
	Send(strResponse.c_str(), strResponse.length());
}

void CHttpClientConnection::InitHttpRequestCommandMap()
{
	CHttpRequestCommandPeerTracker* pHttpRequestCommandPeerTracker = CHttpRequestCommandPeerTracker::GetInstance();
	CHttpRequestCommandDig* pHttpRequestCommandDig = CHttpRequestCommandDig::GetInstance();
	pHttpRequestCommandPeerTracker->AddRef();
	pHttpRequestCommandDig->AddRef();
	m_mapHttpRequestCommand.insert(std::make_pair("/p", pHttpRequestCommandPeerTracker));
	m_mapHttpRequestCommand.insert(std::make_pair("/c", pHttpRequestCommandDig));
}

bufferevent* CHttpClientConnection::GetBufferEvent()
{
	return m_bufferev;
}

RD_CONNECTION_STATE CHttpClientConnection::GetState() const
{
	return m_state;
}

void CHttpClientConnection::OnAsynSendCommand(evutil_socket_t /*fd*/, short /*event*/, void* arg)
{
	CAutoAddReleasePtr<CHttpClientConnection> spThis((CHttpClientConnection*)arg);
	spThis->m_lockSendBodyQueue.Lock();
	tool.Log(_T("CHttpClientConnection::OnAsynSendCommand, m_queueAsynCommand.size:%d"), spThis->m_queueAsynSendBody.size());
	while (spThis->m_queueAsynSendBody.size() > 0)
	{
		std::string strBody = spThis->m_queueAsynSendBody.front();
		spThis->m_queueAsynSendBody.pop();
		//spThis->Send(spCommand);
	}
	spThis->m_lockSendBodyQueue.Unlock();
}

int CHttpClientConnection::Send(const void *data, size_t size)
{
	tool.Log(_T("bufferevent_write m_bufferev:0x%08X"), m_bufferev);
	if (m_bufferev == NULL)
		return -1;

	return bufferevent_write(m_bufferev, data, size);
}

std::string CHttpClientConnection::GetIp() const
{
	return m_strIp;
}

unsigned short CHttpClientConnection::GetPort() const
{
	return m_uPort;
}

void CHttpClientConnection::Stop(RD_ERROR_CODE errorCode)
{
	tool.Log(_T("CHttpClientConnection::Stop, errorCode:%d"), errorCode);
	m_bufferev = NULL;
	if (m_pAsynSendHttpEvent)
	{
		event_del(m_pAsynSendHttpEvent);
		event_free(m_pAsynSendHttpEvent);
		m_pAsynSendHttpEvent = NULL;
		Release();
	}
	HttpRequestCommandMap::iterator it = m_mapHttpRequestCommand.begin();
	for (; it != m_mapHttpRequestCommand.end(); it++)
	{
		it->second->Release();
	}
	m_mapHttpRequestCommand.clear();
}

bool CHttpClientConnection::IsValid() const
{
	if (m_socket == INVALID_SOCKET || m_bufferev == NULL)
	{
		return false;
	}
	return true;
}

bool CHttpClientConnection::operator == (const CHttpClientConnection& p) const
{
	return p.GetIp() == m_strIp && p.GetPort() == m_uPort;
}

void CHttpClientConnection::OnConnect()
{
	tool.Log(_T("CClientConnection::OnConnect"));
	m_state = RDCS_CONNECTED;
}

void CHttpClientConnection::OnError(RD_ERROR_CODE errorCode)
{
	tool.Log(_T("CClientConnection::OnError, events:%d"), errorCode);
	m_state = RDCS_DISCONNECTED;

	m_bufferev = NULL;
	if (m_pAsynSendHttpEvent)
	{
		event_del(m_pAsynSendHttpEvent);
		event_free(m_pAsynSendHttpEvent);
		m_pAsynSendHttpEvent = NULL;
		Release();
	}
}

void CHttpClientConnection::OnWrite()
{
}

void CHttpClientConnection::OnRead(bufferevent* bufferev)
{
	struct evbuffer* input = bufferevent_get_input(bufferev);
	size_t total = evbuffer_get_length(input);
	if (total <= 0)
	{
		return;
	}

	char* buf = new char[total+1];
	memset(buf, 0, total+1);
	evbuffer_remove(input, buf, total);
	m_strRequestBuffer.append(buf, total);
	delete[] buf;
	DecodeBuffer();
}

void CHttpClientConnection::Clear()
{
	m_nHttpBufferState = HBS_READY;
	m_strRequestMethod.clear();
	m_strRequestPath.clear();
	m_strRequestBuffer.clear();
	evhttp_clear_headers(&m_RequestHeaders);
	m_strResponseBody.clear();
}

void CHttpClientConnection::DecodeFirstLine()
{
	size_t pos = m_strRequestBuffer.find("\r\n");
	if (pos == std::string::npos)
	{
		return;
	}
	size_t pos1 = m_strRequestBuffer.find(' ');
	m_strRequestMethod = m_strRequestBuffer.substr(0, pos1);
	if (m_strRequestMethod != "GET" && m_strRequestMethod != "POST")
	{
		// 不支持其他的请求方法
		Send404();
		Clear();
		return;
	}
	size_t pos2 = m_strRequestBuffer.find(' ', pos1+1);
	if (pos2 != std::string::npos)
	{
		m_strRequestPath = m_strRequestBuffer.substr(pos1+1, pos2-pos1-1);
		m_nHttpBufferState = HBS_HEADER;
		m_strRequestBuffer.erase(0, pos+2);
		tool.LogA("m_strRequestBuffer:\r\n%s", m_strRequestBuffer.c_str());
		DecodeHeader();
	}
}

void CHttpClientConnection::DecodeHeader()
{
	// 判断是否完整的响应头
	size_t pos = m_strRequestBuffer.find("\r\n\r\n");
	if (pos == std::string::npos)
		return;
	size_t pos_begin = 0;
	size_t pos1 = m_strRequestBuffer.find("\r\n");
	while (pos1 <= pos)
	{
		std::string strOneHeader = m_strRequestBuffer.substr(pos_begin, pos1-pos_begin);;
		DecodeOneHeader(strOneHeader);
		pos_begin = pos1+2;
		pos1 = m_strRequestBuffer.find("\r\n", pos_begin);
	}
	m_strRequestBuffer.erase(0, pos+4);

	const char* ip_header = evhttp_find_header(&m_RequestHeaders, "X_FORWARDED_FOR");
	const char* port_header = evhttp_find_header(&m_RequestHeaders, "X_FORWARDED_FOR_PORT");
	tool.LogA("CHttpClientConnection::DecodeHeader ip_header:%s, port_header:%s", ip_header, port_header);
	if (ip_header && port_header)
	{
		m_strIp = ip_header;
		m_uPort = (unsigned short)atoi(port_header);
	}
	else
	{
		assert(false);
	}

	m_nHttpBufferState = HBS_BODY;
	DecodeBody();
}

void CHttpClientConnection::DecodeOneHeader(std::string& strOneHeader)
{
	size_t pos = strOneHeader.find(':');
	if (pos > 0 && pos != std::string::npos)
	{
		size_t pos1 = strOneHeader.find_first_not_of(' ', pos+1);
		if (pos1 != std::string::npos)
		{
			std::string strKey = strOneHeader.substr(0, pos);
			std::string strValue = strOneHeader.substr(pos1);
			evhttp_add_header(&m_RequestHeaders, strKey.c_str(), strValue.c_str());
		}
	}
}

void CHttpClientConnection::DecodeBody()
{
	// 先只支持 Content-Length，不支持chunked
	const char* pContentLength = evhttp_find_header(&m_RequestHeaders, "Content-Length");
	int length = 0;
	if (pContentLength && strlen(pContentLength) > 0)
	{
		length = atoi(pContentLength);
	}
	if (length > (int)m_strRequestBuffer.length())
	{
		// body没返回完全
		return;
	}
	else if (length == (int)m_strRequestBuffer.length())
	{
		m_strResponseBody = m_strRequestBuffer;
		OnHttpRequest();
		Clear();
	}
	else
	{
		m_strResponseBody = m_strRequestBuffer.substr(0, length);
		m_strRequestBuffer.erase(0, length);
		OnHttpRequest();

		std::string strNextResponseBuffer(m_strRequestBuffer);
		Clear();
		m_strRequestBuffer = strNextResponseBuffer;

		// 解析下个包
		DecodeFirstLine();
	}
}

void CHttpClientConnection::DecodeBuffer()
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

void CHttpClientConnection::OnHttpRequest()
{
	tool.LogA("CTrackerHttpServer::OnHttpRequest path:%s", m_strRequestPath.c_str());
	HttpRequestCommandMap::iterator it = m_mapHttpRequestCommand.find(m_strRequestPath);
	if (it == m_mapHttpRequestCommand.end())
	{
		tool.LogA("CTrackerHttpServer::OnHttpRequest file not found!");
		Send404();
		return;
	}

	bool bHandled = it->second->Execute(this, &m_RequestHeaders, m_strResponseBody);
	if (!bHandled)
	{
		Send404();
	}
}

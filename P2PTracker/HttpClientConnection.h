#pragma once
#include <event2/event.h>
#include <event2/bufferevent.h>
#include <event2/buffer.h>
#include <event2/listener.h>
#include <event2/util.h>
#include <event2/http.h>
#include <event2/keyvalq_struct.h>

#include "..\common\def.h"
#include "..\common\lock.h"
#include "..\common\AutoAddReleasePtr.h"
#include "HttpRequestCommand.h"

#include <queue>
#include <map>
#include <string>

class CHttpRequestCommandBase;

class CHttpClientConnection :
	public CAddReleaseRef
{
public:
	CHttpClientConnection(struct event_base* base, SOCKET socket, bufferevent* bufferev);
	virtual ~CHttpClientConnection(void);

public:
	bufferevent* GetBufferEvent();
	RD_CONNECTION_STATE GetState() const;
	void Stop(RD_ERROR_CODE errorCode);
	int Send(const void *data, size_t size);
	std::string GetIp() const;
	unsigned short GetPort() const;
	bool IsValid() const;
	bool operator == (const CHttpClientConnection& p) const;

	void OnConnect();
	void OnError(RD_ERROR_CODE errorCode);
	void OnWrite();
	void OnRead(bufferevent* bufferev);

private:
	void InitHttpRequestCommandMap();
	void Send404();

	static void OnAsynSendCommand(evutil_socket_t fd, short event,void *arg);

	void DecodeBuffer();
	void DecodeFirstLine();
	void DecodeHeader();
	void DecodeOneHeader(std::string& strOneHeader);
	void DecodeBody();
	void Clear();
	void OnHttpRequest();

private:
	typedef std::map<std::string, CHttpRequestCommandBase*> HttpRequestCommandMap;
	typedef enum HTTP_BUFFER_STATE
	{
		HBS_READY		= 0,
		HBS_HEADER		= 1,
		HBS_BODY		= 2,
	}HTTP_BUFFER_STATE;
	
	struct event_base*			m_pEventBase;
	SOCKET						m_socket;
	std::string					m_strIp;
	unsigned short				m_uPort;
	bufferevent*				m_bufferev;

	HttpRequestCommandMap		m_mapHttpRequestCommand;
	struct event*				m_pAsynSendHttpEvent;
	std::queue<std::string>		m_queueAsynSendBody;
	CLock						m_lockSendBodyQueue;

	RD_ERROR_CODE				m_errorCode;

	RD_CONNECTION_STATE			m_state;

	HTTP_BUFFER_STATE			m_nHttpBufferState;
	std::string					m_strRequestBuffer;
	std::string					m_strRequestMethod;
	std::string					m_strRequestPath;
	struct evkeyvalq			m_RequestHeaders;
	std::string					m_strResponseBody;
};

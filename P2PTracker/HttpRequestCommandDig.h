#pragma once
#include "..\common\def.h"
#include <event2/http.h>
#include <event2/keyvalq_struct.h>
#include "HttpRequestCommand.h"
#include "TrackerHttpServer.h"
#include "HttpClientConnection.h"

class CHttpRequestCommandDig :
	public CHttpRequestCommandBase
{
public:
	CHttpRequestCommandDig(void);
	virtual ~CHttpRequestCommandDig(void);

	static CHttpRequestCommandDig* GetInstance()
	{
		if (s_pHttpRequestCommandDig == NULL)
		{
			s_pHttpRequestCommandDig = new CHttpRequestCommandDig();
			s_pHttpRequestCommandDig->Init();
		}
		return s_pHttpRequestCommandDig;
	}
	// 重载是为了能正常调用Uninit后，再释放对象
	virtual ULONG Release()
	{
		long l = InterlockedDecrement((LONG*)&m_dwRef);
		if (l == 0)
		{
			Uninit();
			delete this;
			s_pHttpRequestCommandDig = NULL;
		}
		assert(l >= 0);
		return l;
	}

private:
	void Send200(CHttpClientConnection* pHttpClientConnection, int result, struct evkeyvalq* headers = NULL, std::string* strExtra = NULL);

public:
	virtual void Init();
	virtual void Uninit();
	virtual bool Execute(CHttpClientConnection* pHttpClientConnection, struct evkeyvalq* pheaders, std::string& strBody);

	bool OnRequestConnectPeer(CHttpClientConnection* pHttpClientConnection, struct evkeyvalq* headers, std::string& strBody);
	bool OnRequestPeerConnectPeerResult(CHttpClientConnection* pHttpClientConnection, struct evkeyvalq* headers, std::string& strBody);

private:
	static CHttpRequestCommandDig* s_pHttpRequestCommandDig;
};

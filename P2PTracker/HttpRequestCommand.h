#pragma once
#include "..\common\AutoAddReleasePtr.h"
#include <event2/bufferevent.h>
#include <string>
#include "HttpClientConnection.h"

class CHttpClientConnection;
class CHttpRequestCommandBase : public CAddReleaseRef
{
public:
	virtual ~CHttpRequestCommandBase() {};
	virtual void Init() = 0;
	virtual void Uninit() = 0;
	virtual bool Execute(CHttpClientConnection* pHttpClientConnection, struct evkeyvalq* headers, std::string& strBody) = 0;
};

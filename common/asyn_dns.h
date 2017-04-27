#pragma once

#include <event2/util.h>
#include <event2/event.h>
#include <event2/dns.h>

#include <WS2tcpip.h>
#include <assert.h>

class asyn_dns_event
{
public:
	virtual void on_get_host_by_name(const char* ip) = 0;
};

class asyn_dns
{
public:
	asyn_dns() : _thread(NULL), _base(NULL), _event(NULL)
	{
	};
	~asyn_dns()
	{
		if (_base)
		{
			event_base_loopexit(_base, NULL);
		}
	};

public:
	bool is_getting()
	{
		return _thread != NULL;
	}
	bool get_host_by_name(asyn_dns_event* event, const char* name)
	{
		tool.LogA("get_host_by_name event:0x%08X, name:%s", event, name);
		if (_thread != NULL)
		{
			tool.Log(_T("libevent is already initialized!"));
			return true;
		}

		_event = event;
		_name = name;

		unsigned thread_id = 0;
		_thread = (HANDLE)_beginthreadex(NULL, 0, &do_get_host_by_name, (LPVOID)this, 0, &thread_id);
		CloseHandle(_thread);

		if (_thread == NULL)
			return false;

		return true;
	}
	bool syn_get_host_by_name(const char* name, std::string& ip)
	{
		/* Build the hints to tell getaddrinfo howto act. */
		struct evutil_addrinfo hints;
		memset(&hints,  0,  sizeof(hints));
		hints.ai_family = AF_UNSPEC; /* v4 or v6 isfine. */
		hints.ai_socktype = SOCK_STREAM;
		hints.ai_protocol = IPPROTO_TCP; /* We wanta TCP socket */
		/* Only return addresses we can use. */
		hints.ai_flags = EVUTIL_AI_ADDRCONFIG;

		/* Look up the hostname. */
		struct evutil_addrinfo* answer = NULL;
		int err = evutil_getaddrinfo(name, NULL, &hints, &answer);
		if (err != 0)
		{
			tool.LogA("Error while resolving '%s': %s", name, evutil_gai_strerror(err));
			return false;
		}
		char buf[128] = {0};
		const char* s = NULL;
		if (answer->ai_family == AF_INET)
		{
			struct sockaddr_in* sin = (struct sockaddr_in*)answer->ai_addr;
			s = evutil_inet_ntop(AF_INET, &sin->sin_addr, buf, 128);
			ip = s;
			evutil_freeaddrinfo(answer);
			return true;
		}
		evutil_freeaddrinfo(answer);
		return false;
	}

private:
	static void getaddrinfo_call_back(int errcode, struct evutil_addrinfo* addr, void* p)
	{
		tool.Log(_T("getaddrinfo_call_back p:0x%08X"), p);
		asyn_dns* _this = (asyn_dns*)p;

		if (errcode || addr == NULL)
		{
			tool.LogA("getaddrinfo failed! %s->%s", _this->_name.c_str(), evutil_gai_strerror(errcode));
			_this->_event->on_get_host_by_name(NULL);
		}
		else
		{
			char buf[128] = {0};
			const char* s = NULL;
			if (addr->ai_family == AF_INET)
			{
				struct sockaddr_in* sin = (struct sockaddr_in*)addr->ai_addr;
				s = evutil_inet_ntop(AF_INET, &sin->sin_addr, buf, 128);
			}
			_this->_event->on_get_host_by_name(s);
			evutil_freeaddrinfo(addr);
		}
		event_base_loopexit(_this->_base, NULL);
	}
	static unsigned __stdcall do_get_host_by_name(void* p)
	{
		asyn_dns* _this = (asyn_dns*)p;
		tool.LogA("do_get_host_by_name _this:0x%08X, name:%s", _this, _this->_name.c_str());

		_this->_base = event_base_new();
		struct evdns_base* dnsbase = evdns_base_new(_this->_base, 1);

		struct evutil_addrinfo hints;
		memset(&hints, 0, sizeof(hints));
		hints.ai_family = AF_UNSPEC;
		hints.ai_flags = EVUTIL_AI_CANONNAME;
		hints.ai_socktype = SOCK_STREAM;
		hints.ai_protocol = IPPROTO_TCP;

		struct evdns_getaddrinfo_request* req = evdns_getaddrinfo(dnsbase, _this->_name.c_str(), NULL, &hints, getaddrinfo_call_back, p);
		if (req == NULL)
		{
			tool.LogA("[request for %s returned immediately]", _this->_name.c_str());
		}
		event_base_dispatch(_this->_base);
		evdns_base_free(dnsbase, 0);
		event_base_free(_this->_base);
		_this->_thread = NULL;
		_this->_base = NULL;
		return 0;
	}

private:
	HANDLE				_thread;
	struct event_base*	_base;
	asyn_dns_event*		_event;
	std::string			_name;
};

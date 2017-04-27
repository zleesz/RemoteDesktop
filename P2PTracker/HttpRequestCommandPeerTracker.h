#pragma once
#include <time.h>
#include <map>
#include <list>
#include "..\common\def.h"
#include "..\common\lock.h"
#include "HttpRequestCommand.h"

typedef struct PeerState
{
	RD_PEER_STATE	state;
	time_t			online_tm;		// 上线时间
	unsigned int	offline_times;	// 掉线次数(三次以上就当真掉线了)
	struct PeerState() : state(RDPS_INVALID), online_tm(0), offline_times(0)
	{};
}PeerState;
typedef std::list<PeerInfo> PeerList;
typedef std::map<PeerInfo, PeerState> PeerMap;

class CHttpRequestCommandPeerTracker :
	public CHttpRequestCommandBase,
	public CWindowImpl<CHttpRequestCommandPeerTracker>
{
public:
	CHttpRequestCommandPeerTracker(void);
	~CHttpRequestCommandPeerTracker(void);

	static CHttpRequestCommandPeerTracker* GetInstance()
	{
		if (s_pHttpRequestCommandPeerTracker == NULL)
		{
			s_pHttpRequestCommandPeerTracker = new CHttpRequestCommandPeerTracker();
			s_pHttpRequestCommandPeerTracker->Init();
		}
		return s_pHttpRequestCommandPeerTracker;
	}
	// 重载是为了能正常调用Uninit后，再释放对象
	virtual ULONG Release()
	{
		long l = InterlockedDecrement((LONG*)&m_dwRef);
		if (l == 0)
		{
			Uninit();
			delete this;
			s_pHttpRequestCommandPeerTracker = NULL;
		}
		assert(l >= 0);
		return l;
	}

	BEGIN_MSG_MAP(CHttpRequestCommandPeerTracker)
		MESSAGE_HANDLER(WM_TIMER, OnTimer)
	END_MSG_MAP()

protected:
	LRESULT OnTimer(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);

public:
	void GetLivePeer(PeerList& lstPeer);

public:
	virtual void Init();
	virtual void Uninit();
	virtual bool Execute(CHttpClientConnection* pHttpClientConnection, struct evkeyvalq* headers, std::string& strBody);

private:
	bool OnPeerOnline(CHttpClientConnection* pHttpClientConnection, const char* ip, unsigned short port, struct evkeyvalq* params);
	bool OnPeerPing(CHttpClientConnection* pHttpClientConnection, const char* ip, unsigned short port, struct evkeyvalq* params);
	bool OnPeerOffline(CHttpClientConnection* pHttpClientConnection, const char* ip, unsigned short port, struct evkeyvalq* params);
	bool OnPeerGetLive(CHttpClientConnection* pHttpClientConnection, const char* ip, unsigned short port, struct evkeyvalq* /*params*/);
	void Send200(bufferevent* bufferev, int result, std::string* strExtra = NULL);

private:
	static CHttpRequestCommandPeerTracker* s_pHttpRequestCommandPeerTracker;
	PeerMap	m_mapPeer;
};

#include "stdafx.h"
#include "RemoteClient.h"
#include <iostream>
#include <string>
#include "..\common\stream_buffer.h"
#include "..\common\utils.h"
#include "..\common\AutoAddReleasePtr.h"
#include "ClientReciveCmdFactory.h"

CRemoteClient::CRemoteClient(void) : 
	m_hThread(NULL),
	m_pEventBase(NULL),
	m_pBufferEvent(NULL),
	m_pRemoteClientEvent(NULL),
	m_state(RDCS_INVALID),
	m_pAsynSendCommandEvent(NULL),
	m_strSuffix(BITMAP_SUFFIX_JPEG),
	m_ip(INADDR_NONE)
{
	InitCommandData();
	Create(HWND_MESSAGE);
}

CRemoteClient::~CRemoteClient(void)
{
}

void CRemoteClient::conn_writecb(struct bufferevent* bev, void* /*user_data*/)
{
	struct evbuffer *output = bufferevent_get_output(bev);
	if (evbuffer_get_length (output) == 0)
	{
		printf("flushed answer\n" );
	}
}

void CRemoteClient::conn_readcb(struct bufferevent* bev, void* user_data)
{
	CRemoteClient* pThis = (CRemoteClient*)user_data;
	struct evbuffer* input = bufferevent_get_input(bev);
	size_t total = evbuffer_get_length(input);
	if (total <= 0)
		return;

	char* buf = new char[total+1];
	memset(buf, 0, total+1);
	evbuffer_remove(input, buf, total);
	pThis->DecodeBuffer(buf, total);
	delete[] buf;
}

void CRemoteClient::conn_eventcb(struct bufferevent* bev, short events, void* user_data)
{
	tool.Log(_T("CRemoteClient::conn_eventcb bev:0x%08X, events:%d"), bev, events);
	CRemoteClient* pThis = (CRemoteClient*)user_data;
	if (events & BEV_EVENT_EOF)
	{
		tool.Log(_T("CRemoteClient::conn_eventcb Connection closed."));
		event_base_loopexit(pThis->m_pEventBase, NULL);
	}
	else if (events & BEV_EVENT_ERROR)
	{
		tool.Log(_T("CRemoteClient::conn_eventcb lasterror:%d, Got an error on the connection: %s"), ::WSAGetLastError(), _tcserror(errno));
		event_base_loopexit(pThis->m_pEventBase, NULL);
	}
}

unsigned __stdcall CRemoteClient::DoConnect(void* pParam)
{
	tool.Log(_T("CRemoteClient::DoConnect"));
	CRemoteClient* pThis = (CRemoteClient*)pParam;

	pThis->m_pEventBase = event_base_new();
	
	bool bInit = pThis->m_P2PTrackerClient.Init(pThis, pThis->m_pEventBase);
	if (!bInit)
	{
		tool.Log(_T("m_P2PTrackerClient.Init failed!"));
		event_base_free(pThis->m_pEventBase);
		pThis->m_pEventBase = NULL;
		return 1;
	}
	bool bGet = pThis->m_P2PTrackerClient.GetLivePeerList();
	if (!bGet)
	{
		tool.Log(_T("m_P2PTrackerClient.GetLivePeerList failed!"));
		pThis->m_P2PTrackerClient.Uninit();
		event_base_free(pThis->m_pEventBase);
		pThis->m_pEventBase = NULL;
		return 2;
	}
	//m_pAsynSendCommandEvent = event_new(m_pEventBase, -1, EV_SIGNAL | EV_PERSIST, OnAsynSendCommand, this);
	event_base_dispatch(pThis->m_pEventBase);

	if (pThis->m_pAsynSendCommandEvent)
	{
		event_del(pThis->m_pAsynSendCommandEvent);
		event_free(pThis->m_pAsynSendCommandEvent);
		pThis->m_pAsynSendCommandEvent = NULL;
	}

	if (pThis->m_pBufferEvent)
	{
		bufferevent_free(pThis->m_pBufferEvent);
		pThis->m_pBufferEvent = NULL;
	}
	event_base_free(pThis->m_pEventBase);
	pThis->m_pEventBase = NULL;
	return 0;
}

bool CRemoteClient::Connect(IRemoteClientEvent* pRemoteClientEvent)
{
	tool.Log(_T("CRemoteClient::Connect"));
	if (m_hThread != NULL)
	{
		tool.Log(_T("libevent is already initialized!"));
		return true;
	}
	m_pRemoteClientEvent = pRemoteClientEvent;
	unsigned threadID = 0;
	m_hThread = (HANDLE)_beginthreadex(NULL, 0, &DoConnect, (LPVOID)this, 0, &threadID);
	CloseHandle(m_hThread);

	if (m_hThread == NULL)
		return false;

	return true;
}

void CRemoteClient::Stop()
{
	tool.Log(_T("CRemoteClient::Stop"));
	if (m_pEventBase)
	{
		event_base_loopexit(m_pEventBase, NULL);
	}
	if (m_hWnd)
	{
		DestroyWindow();
	}
}

void CRemoteClient::DecodeBuffer(char* buf, uint32_t total)
{
	if (m_Command.code == RDCC_INVALID)
	{
		stream_buffer buffer(buf, total);
		uint32_t ncode = RDCC_INVALID;
		if (FAILED(buffer.ReadUInt32(ncode)) || ncode == RDCC_INVALID)
		{
			return;
		}
		uint32_t nLength = 0;
		if (FAILED(buffer.ReadUInt32(nLength)) || nLength > CMD_ITEM_SIZE_MAX)
		{
			return;
		}
		uint32_t nReadLength = nLength;
		if (FAILED(buffer.ReadStringUTF8(m_Command.data, nReadLength)))
		{
			return;
		}
		m_Command.code = (RD_CMD_CODE)ncode;
		m_Command.length = nLength;
		if (nReadLength != nLength)
		{
			// 未读完全
			return;
		}
		else if (total > nLength + sizeof(uint32_t) * 2)
		{
			// 还有下一条命令
			// 先处理这条命令，再解析下一条
			SendMessage(WM_REMOTECLIENT_ONRECIVECOMMAND, (WPARAM)&m_Command, 0);
			InitCommandData();
			DecodeBuffer(buf + nLength + sizeof(uint32_t) * 2, total - (nLength + sizeof(uint32_t) * 2));
			return;
		}
	}
	else
	{
		// 上次的命令还没接收完
		stream_buffer buffer(buf, total);
		std::string strNext;
		uint32_t nLeaveLength = m_Command.length - m_Command.data.length();
		uint32_t nReadLength = nLeaveLength;
		if (FAILED(buffer.ReadStringUTF8(strNext, nReadLength)))
		{
			InitCommandData();
			return;
		}
		m_Command.data.append(strNext);
		if (nReadLength != nLeaveLength)
		{
			// 未读完全
			return;
		}
		else if (total > nReadLength)
		{
			// 还有下一条命令
			// 先处理这条命令，再解析下一条
			SendMessage(WM_REMOTECLIENT_ONRECIVECOMMAND, (WPARAM)&m_Command, 0);
			InitCommandData();
			DecodeBuffer(buf + nReadLength, total - nReadLength);
			return;
		}
	}
	// 到这里，一个命令接收完了
	SendMessage(WM_REMOTECLIENT_ONRECIVECOMMAND, (WPARAM)&m_Command, 0);
	InitCommandData();
}

RD_CONNECTION_STATE	CRemoteClient::GetState()
{
	return m_state;
}

void CRemoteClient::OnAsynSendCommand(evutil_socket_t /*fd*/, short /*event*/, void* arg)
{
	tool.Log(_T("CRemoteClient::OnAsynSendCommand"));
	CRemoteClient* pThis = (CRemoteClient*)arg;
	pThis->m_lockCommandQueue.Lock();
	tool.Log(_T("CRemoteClient::OnAsynSendCommand, m_queueAsynCommand.size:%d"), pThis->m_queueAsynCommand.size());
	while (pThis->m_queueAsynCommand.size() > 0)
	{
		CommandBase* pCommand = pThis->m_queueAsynCommand.front();
		pThis->m_queueAsynCommand.pop();
		pThis->Send(pCommand);
		pCommand->Release();
	}
	pThis->m_lockCommandQueue.Unlock();
}

int CRemoteClient::Send(const void *data, size_t size)
{
	if (m_pBufferEvent == NULL)
		return -1;

	return bufferevent_write(m_pBufferEvent, data, size);
}

int CRemoteClient::Send(CommandBase* pCommand)
{
	CAutoAddReleasePtr<CommandBase> spCommand(pCommand);
	tool.Log(_T("CRemoteClient::Send, pCommand:0x%08X, code:0x%08X"), spCommand, spCommand->GetCode());
	void* stream = NULL;
	int len = 0;
	spCommand->MakeStream(&stream, &len);
	if (stream == NULL)
		return -1;

	Send(stream, len);
	delete[] stream;
	return 0;
}

int CRemoteClient::AsynSend(CommandBase* pCommand)
{
	CAutoAddReleasePtr<CommandBase> spCommand(pCommand);
	tool.Log(_T("CRemoteClient::AsynSend, pCommand:0x%08X, code:0x%08X"), spCommand, spCommand->GetCode());
	if (m_pAsynSendCommandEvent == NULL)
		return -2;

	m_lockCommandQueue.Lock();
	m_queueAsynCommand.push(spCommand.Detach());
	m_lockCommandQueue.Unlock();
	event_active(m_pAsynSendCommandEvent, EV_SIGNAL, 1);
	return 1;
}

void CRemoteClient::InitCommandData()
{
	m_Command.code = RDCC_INVALID;
	m_Command.data.clear();
	m_Command.length = 0;
}

LRESULT CRemoteClient::OnConnected(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	tool.Log(_T("CRemoteClient::OnConnected"));
	m_state = RDCS_CONNECTED;
	m_pRemoteClientEvent->OnConnected();
	m_pRemoteClientEvent->OnStateChanged(m_state);

	// 连接上了，协商协议版本号
	char szExePath[MAX_PATH] = {0};
	::GetModuleFileNameA(NULL, szExePath, MAX_PATH);
	char szClientVersion[64] = {0};
	GetFileVersionStringA(szExePath, szClientVersion);
	std::string strClientVersion(szClientVersion);
	std::string strProtocalVersion(RD_PROTOCAL_VERSION);
	CCommandProtocalVersion* pCommandProtocalVersion = new CCommandProtocalVersion(strProtocalVersion, strClientVersion);
	AsynSend(pCommandProtocalVersion);

	m_state = RDCS_PROTOCOL_VERSION;
	m_pRemoteClientEvent->OnStateChanged(m_state);

	// 再发登录命令
	std::string strUserName;
	std::string strPassword;
	CCommandLogon* pCommandLogon = new CCommandLogon(strUserName, strPassword);
	AsynSend(pCommandLogon);

	m_state = RDCS_LOGON;
	m_pRemoteClientEvent->OnStateChanged(m_state);
	return 0;
}

bool CRemoteClient::CheckCmdCodeValid(RD_CMD_CODE code)
{
	switch (m_state)
	{
	case RDCS_LOGON:
		if (code == RDCC_LOGON_RESULT)
			return true;
		break;
	case RDCS_LOGON_RESULT:
		if (code == RDCC_COMPRESS_MODE)
			return true;
		break;
	case RDCS_COMPRESS_MODE:
		if (code == RDCC_TRANSFER_BITMAP)
			return true;
		break;
	case RDCS_TRANSFER:
		if (code >= RDCC_TRANSFER_BITMAP && code <= RDCC_DISCONNECT)
			return true;
		break;
	default:
		break;
	}
	if (code == RDCC_DISCONNECT && m_state >= RDCS_CONNECTED && m_state != RDCS_DISCONNECTED)
		return true;
	return false;
}

LRESULT CRemoteClient::OnReciveCommand(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	CommandData* pCommand = (CommandData*)wParam;
	tool.Log(_T("CRemoteClient::OnReciveCommand, state:0x%08X, pCommand:0x%08X, code:0x%08X"), m_state, pCommand, pCommand->code);
	if (m_state < RDCS_CONNECTED)
		return 0;

	if (pCommand == NULL)
		return 0;

	RD_CMD_CODE code = pCommand->code;
	if (!CheckCmdCodeValid(code))
		return 0;

	IClientReciveCmdBase* pClientReciveCmd = NULL;
	if (CClientReciveCmdFactory::CreateCmd(code, &pClientReciveCmd))
	{
		pClientReciveCmd->Parse(*pCommand, this);
		delete pClientReciveCmd;
	}
	return 0;
}

LRESULT CRemoteClient::OnTimer(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{

	struct sockaddr_in sin;
	memset(&sin, 0, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = m_P2PTrackerClient.GetIp(); //inet_addr(peer.ip.c_str());
	sin.sin_port = htons(m_P2PTrackerClient.GetPort()/*peer.tcp_port*/);

	SOCKET s = socket(AF_INET, SOCK_STREAM, 0);
	sockaddr_in clientAddr;
	clientAddr.sin_family = AF_INET;
	clientAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	clientAddr.sin_port = port;
	if (bind(s, (SOCKADDR*)&clientAddr, sizeof(clientAddr)) == SOCKET_ERROR)
	{
		tool.Log(_T("bind failed. lasterror:%d"), ::WSAGetLastError());
		//event_base_loopexit(m_pEventBase, NULL);
		closesocket(s);
		return 0;
	}
	evutil_make_listen_socket_reuseable(s);
	if (connect(s, (struct sockaddr*)&sin, sizeof(sin)) == SOCKET_ERROR)
	{
		tool.Log(_T("connect failed. lasterror:%d"), ::WSAGetLastError());
		//event_base_loopexit(m_pEventBase, NULL);
		closesocket(s);
		return 0;
	}
	std::string strData;
	strData += "POST /c HTTP/1.1\r\n";
	strData += "Content-Length: 3\r\n";
	strData += "Host: 150759bc.nat123.cc\r\n";
	strData += "User-Agent: RemoteClient\r\n";
	strData += "Content-Type: text/plain\r\n";
	strData += "Connection: Keep-Alive\r\n";
	strData += "\r\n";
	strData += "c=g";
	if (send(s, strData.c_str(), strData.length(), 0) == SOCKET_ERROR)
	{
		tool.Log(_T("send failed. lasterror:%d"), ::WSAGetLastError());
		closesocket(s);
		//event_base_loopexit(m_pEventBase, NULL);
		return 0;
	}
	KillTimer(wParam);
	return 0;
}

void CRemoteClient::GetSuffix(std::string& strSuffix)
{
	strSuffix = m_strSuffix;
}

void CRemoteClient::OnGetLogonResult(int nResult)
{
	tool.Log(_T("CRemoteClient::OnGetLogonResult, nResult:%d"), nResult);
	if (nResult == 0)
	{
		m_state = RDCS_LOGON_RESULT;
		m_pRemoteClientEvent->OnStateChanged(m_state);

		CCommandCompressMode* pCommandCompressMode = new CCommandCompressMode(RDCM_RAW);
		AsynSend(pCommandCompressMode);

		m_state = RDCS_COMPRESS_MODE;
		m_pRemoteClientEvent->OnStateChanged(m_state);

		CCommandTransferBitmapClient* pCommandTransferBitmapClient = new CCommandTransferBitmapClient(m_strSuffix);
		AsynSend(pCommandTransferBitmapClient);
	}
	else
	{
		// 登录失败
	}
}

void CRemoteClient::OnGetTransferBitmap(BitmapInfo& bitmapInfo, WORD wPixelBytes, unsigned char* pBuffer)
{
	tool.Log(_T("CRemoteClient::OnGetTransferBitmap, state:0x%08X, size:%d"), m_state, bitmapInfo.bmiHeader.biWidth * bitmapInfo.bmiHeader.biHeight * wPixelBytes);
	if (m_state != RDCS_TRANSFER)
	{
		m_state = RDCS_TRANSFER;
		m_pRemoteClientEvent->OnStateChanged(m_state);
	}
	m_pRemoteClientEvent->OnGetTransferBitmap(bitmapInfo, wPixelBytes, pBuffer);
	CCommandTransferBitmapClientResponse* pCommandTransferBitmapClientResponse = new CCommandTransferBitmapClientResponse(0);
	AsynSend(pCommandTransferBitmapClientResponse);
}

void CRemoteClient::OnGetTransferModifyBitmap(unsigned int nModifiedBlockCount, unsigned int* pnModifiedBlocks, unsigned int nBufferSize, unsigned char* pBuffer)
{
	tool.Log(_T("CRemoteClient::OnGetTransferModifyBitmap, nModifiedBlockCount:(%d), nBufferSize:%d"), nModifiedBlockCount, nBufferSize);
	m_pRemoteClientEvent->OnGetTransferModifyBitmap(nModifiedBlockCount, pnModifiedBlocks, nBufferSize, pBuffer);
	CCommandTransferBitmapClientResponse* pCommandTransferBitmapClientResponse = new CCommandTransferBitmapClientResponse(0);
	AsynSend(pCommandTransferBitmapClientResponse);
}

void CRemoteClient::OnGetDisconnect(int nDisconnectCode)
{
	tool.Log(_T("CRemoteClient::OnGetDisconnect, nDisconnectCode:%d"), nDisconnectCode);
	m_pRemoteClientEvent->OnGetDisconnect(nDisconnectCode);
}

void CRemoteClient::OnGetLivePeerList(std::list<PeerInfo>& listPeer)
{
	tool.Log(_T("CRemoteClient::OnGetLivePeerList, size:%d"), listPeer.size());
	if (listPeer.size() > 0)
	{
		std::list<PeerInfo>::iterator it = listPeer.begin();
		ConnectRemoteDesktop(*it);
	}
}

void CRemoteClient::OnGetLivePeerFailed()
{
	tool.Log(_T("CRemoteClient::OnGetLivePeerFailed."));
}

bool CRemoteClient::ConnectRemoteDesktop(PeerInfo& peer)
{
	tool.LogA("CRemoteClient::ConnectRemoteDesktop ip:%s, port:%u", peer.ip.c_str(), peer.tcp_port);
	return m_P2PTrackerClient.ConnectPeer(peer);
}

void CRemoteClient::OnConnectPeerResult(PeerInfo& peer, unsigned short port)
{
	tool.LogA("CRemoteClient::OnConnectPeerResult peer=%s:%u, port:%u(%u)", peer.ip.c_str(), peer.tcp_port, port, ntohs(port));
	m_P2PTrackerClient.CloseClient();

	struct sockaddr_in sin;
	memset(&sin, 0, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = m_P2PTrackerClient.GetIp(); //inet_addr(peer.ip.c_str());
	sin.sin_port = htons(m_P2PTrackerClient.GetPort()/*peer.tcp_port*/);
	SetTimer(0, 100);
	/*
	m_pBufferEvent = bufferevent_socket_new(m_pEventBase, -1, BEV_OPT_CLOSE_ON_FREE | LEV_OPT_THREADSAFE);
	bufferevent_setcb(m_pBufferEvent, conn_readcb, conn_writecb, conn_eventcb, this);
	bufferevent_enable(m_pBufferEvent, EV_WRITE | EV_READ);

	evutil_socket_t fd = bufferevent_getfd(m_pBufferEvent);
	if (fd < 0)
	{
		//该bufferevent还没有设置fd
		fd = socket(sin.sin_family, SOCK_STREAM, 0);
		//evutil_make_socket_nonblocking(fd); //设置为非阻塞
		int n = evutil_make_listen_socket_reuseable(fd);
		tool.Log(_T("CRemoteClient::OnConnectPeerResult evutil_make_listen_socket_reuseable n:%d"), n);
	}
	sockaddr_in clientAddr;
	clientAddr.sin_family = AF_INET;
	clientAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	clientAddr.sin_port = port;
	if (bind(fd, (SOCKADDR*)&clientAddr, sizeof(clientAddr)) == SOCKET_ERROR)
	{
		tool.Log(_T("bind failed. lasterror:%d"), ::WSAGetLastError());
		event_base_loopexit(m_pEventBase, NULL);
		return;
	}
	bufferevent_setfd(m_pBufferEvent, fd);
	if (bufferevent_socket_connect(m_pBufferEvent, (struct sockaddr*)&sin, sizeof(sin)) == 0)
	{
		m_pAsynSendCommandEvent = event_new(m_pEventBase, -1, EV_SIGNAL | EV_PERSIST, OnAsynSendCommand, this);
		tool.Log(_T("connect success. m_pAsynSendCommandEvent:0x%08X"), m_pAsynSendCommandEvent);
		std::string strData;
		strData += "POST /c HTTP/1.1\r\n";
		strData += "Content-Length: 3\r\n";
		strData += "Host: 150759bc.nat123.cc\r\n";
		strData += "User-Agent: RemoteClient\r\n";
		strData += "Content-Type: text/plain\r\n";
		strData += "Connection: Keep-Alive\r\n";
		strData += "\r\n";
		strData += "c=g";
		int n = bufferevent_write(m_pBufferEvent, strData.c_str(), strData.length());
		tool.Log(_T("bufferevent_write n=%d"), n);
	}
	*/
}

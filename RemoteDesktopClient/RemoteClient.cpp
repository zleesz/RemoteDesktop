#include "stdafx.h"
#include "RemoteClient.h"
#include <iostream>
#include "..\common\stream_buffer.h"
#include "..\common\utils.h"
#include "..\common\AutoAddReleasePtr.h"
#include "ClientReciveCmdFactory.h"

CRemoteClient::CRemoteClient(void) : 
	m_hThread(NULL),
	m_pEventBase(NULL),
	m_bufferev(NULL),
	m_pRemoteClientEvent(NULL),
	m_state(RDCS_INVALID),
	m_pAsynSendCommandEvent(NULL),
	m_strSuffix(BITMAP_SUFFIX_JPEG)
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
	{
		return;
	}
	char* buf = new char[total+1];
	memset(buf, 0, total+1);
	evbuffer_remove(input, buf, total);
	pThis->DecodeBuffer(buf, total);
	delete[] buf;
}

unsigned __stdcall CRemoteClient::DoConnect(void* pParam)
{
	tool.Log(_T("CRemoteClient::DoConnect"));
	CRemoteClient* pThis = (CRemoteClient*)pParam;

	// build socket
	struct sockaddr_in sin;
	memset(&sin, 0, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = inet_addr(RD_REMOTE_SERVER_HOST);
	sin.sin_port = htons(RD_REMOTE_SERVER_PORT);

	// build event base
	pThis->m_pEventBase = event_base_new();

	// set TCP_NODELAY to let data arrive at the server side quickly
	evutil_socket_t fd;
	fd = socket(AF_INET, SOCK_STREAM, 0);
	pThis->m_bufferev = bufferevent_socket_new(pThis->m_pEventBase, fd, BEV_OPT_CLOSE_ON_FREE | LEV_OPT_THREADSAFE);
	int enable = 1;
	if (setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, (const char*)&enable, sizeof(enable)) < 0)
		tool.Log(_T("ERROR: TCP_NODELAY SETTING ERROR!"));

	evutil_make_socket_nonblocking(fd);
	bufferevent_setcb(pThis->m_bufferev, conn_readcb, conn_writecb, NULL, pParam);
	bufferevent_enable(pThis->m_bufferev, EV_WRITE | EV_READ);

	if (bufferevent_socket_connect(pThis->m_bufferev, (struct sockaddr*)&sin, sizeof(sin)) == 0)
	{
		pThis->m_pAsynSendCommandEvent = event_new(pThis->m_pEventBase, -1, EV_SIGNAL|EV_PERSIST, OnAsynSendCommand, pParam);
		tool.Log(_T("connect success. m_pAsynSendCommandEvent:0x%08X"), pThis->m_pAsynSendCommandEvent);

		pThis->SendMessage(WM_REMOTECLIENT_ONCONNECTED, 0, 0);
		event_base_dispatch(pThis->m_pEventBase);

		event_del(pThis->m_pAsynSendCommandEvent);
		event_free(pThis->m_pAsynSendCommandEvent);
		pThis->m_pAsynSendCommandEvent = NULL;
	}

	bufferevent_free(pThis->m_bufferev);
	pThis->m_bufferev = NULL;
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

void CRemoteClient::OnAsynSendCommand(evutil_socket_t fd, short event, void* arg)
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
	if (m_bufferev == NULL)
		return -1;

	return bufferevent_write(m_bufferev, data, size);
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
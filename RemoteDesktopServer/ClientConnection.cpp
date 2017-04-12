#include "StdAfx.h"
#include "ClientConnection.h"
#include "..\common\socket_util.h"
#include "..\common\compress_factory.h"
#include "..\common\AutoAddReleasePtr.h"
#include "..\common\jpeg.h"
#include "ServerReciveCmdFactory.h"
#include "ScreenBitmap.h"
#include "..\common\utils.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

CClientConnection::CClientConnection(struct event_base* base, SOCKET socket, bufferevent* bufferev, IClientConnectionEvent*	pClientConnectionEvent) :
	m_pEventBase(base),
	m_bufferev(bufferev),
	m_state(RDCS_INVALID),
	m_uPort(0),
	m_socket(socket),
	m_pClientConnectionEvent(pClientConnectionEvent),
	m_pCompress(NULL),
	m_pFileTransferDlg(NULL),
	m_nQuality(50),
	m_bSendBitmapReady(false),
	m_nScanPointRowCount(0),
	m_nScanPointColumnCount(0),
	m_nModifiedBlockCount(0),
	m_pnModifiedBlocks(NULL)
{
	tool.Log(_T("CClientConnection::CClientConnection base:0x%08X, socket:%d, bufferev:0x%08X, pClientConnectionEvent:0x%08X"), base, socket, bufferev, pClientConnectionEvent);
	if (m_socket != INVALID_SOCKET)
	{
		socket_util::get_peer_ip_port(socket, &m_strIp, &m_uPort);
		tool.LogA("CClientConnection::CClientConnection get_peer_ip_port m_strIp:%s, m_uPort:%d", m_strIp.c_str(), m_uPort);
	}
	m_pAsynSendCommandEvent = event_new(m_pEventBase, -1, EV_SIGNAL|EV_PERSIST, OnAsynSendCommand, this);
	if (m_pAsynSendCommandEvent)
	{
		// event 引用了一个计数，删除event时再减
		AddRef();
	}
}

CClientConnection::~CClientConnection(void)
{
	m_lockCommandQueue.Lock();
	tool.Log(_T("CClientConnection::~CClientConnection, m_queueAsynCommand.size:%d"), m_queueAsynCommand.size());
	while (m_queueAsynCommand.size() > 0)
	{
		CommandBase* pCommand = m_queueAsynCommand.front();
		pCommand->Release();
		m_queueAsynCommand.pop();
	}
	m_lockCommandQueue.Unlock();
	if (m_pCompress)
	{
		delete m_pCompress;
		m_pCompress = NULL;
	}
	if (m_pFileTransferDlg)
	{
		delete m_pFileTransferDlg;
		m_pFileTransferDlg = NULL;
	}
	if (m_pnModifiedBlocks)
	{
		delete[] m_pnModifiedBlocks;
		m_pnModifiedBlocks = NULL;
	}
}

bufferevent* CClientConnection::GetBufferEvent()
{
	return m_bufferev;
}

RD_CONNECTION_STATE CClientConnection::GetState() const
{
	return m_state;
}

void CClientConnection::OnAsynSendCommand(evutil_socket_t /*fd*/, short /*event*/, void* arg)
{
	CAutoAddReleasePtr<CClientConnection> spThis((CClientConnection*)arg);
	spThis->m_lockCommandQueue.Lock();
	tool.Log(_T("CClientConnection::OnAsynSendCommand, m_queueAsynCommand.size:%d"), spThis->m_queueAsynCommand.size());
	while (spThis->m_queueAsynCommand.size() > 0)
	{
		CommandBase* pCommand = spThis->m_queueAsynCommand.front();
		CAutoAddReleasePtr<CommandBase> spCommand;
		spCommand.Attach(pCommand);
		spThis->m_queueAsynCommand.pop();
		spThis->Send(spCommand);
	}
	spThis->m_lockCommandQueue.Unlock();
}

int CClientConnection::Send(const void *data, size_t size)
{
	tool.Log(_T("bufferevent_write m_bufferev:0x%08X"), m_bufferev);
	if (m_bufferev == NULL)
		return -1;

	return bufferevent_write(m_bufferev, data, size);
}

int CClientConnection::Send(CommandBase* pCommand)
{
	CAutoAddReleasePtr<CommandBase> spCommand(pCommand);
	tool.Log(_T("CClientConnection::Send, pCommand:0x%08X, code:0x%08X"), spCommand, spCommand->GetCode());
	void* stream = NULL;
	int len = 0;
	spCommand->MakeStream(&stream, &len);
	if (stream == NULL)
		return -1;

	int ret = Send(stream, len);
	delete[] stream;
	return ret;
}

int CClientConnection::AsynSend(CommandBase* pCommand)
{
	CAutoAddReleasePtr<CommandBase> spCommand(pCommand);
	tool.Log(_T("CClientConnection::AsynSend, pCommand:0x%08X, code:0x%08X"), spCommand, spCommand->GetCode());
	if (m_pAsynSendCommandEvent == NULL)
		return -2;

	m_lockCommandQueue.Lock();
	m_queueAsynCommand.push(spCommand.Detach());
	m_lockCommandQueue.Unlock();
	event_active(m_pAsynSendCommandEvent, EV_SIGNAL, 1);
	return 1;
}

int CClientConnection::AsynSendLogonResult(CCommandLogonResult* pCommand)
{
	CAutoAddReleasePtr<CCommandLogonResult> spCommand(pCommand);
	tool.Log(_T("CClientConnection::AsynSendLogonResult, pCommand:0x%08X, code:0x%08X, state:0x%08X"), spCommand, spCommand->GetCode(), m_state);
	if (m_state != RDCS_LOGON)
		return -1;

	return AsynSend(spCommand);
}

int CClientConnection::AsynSendTransferBitmap()
{
	tool.Log(_T("CClientConnection::AsynSendTransferBitmap, state:0x%08X"), m_state);
	if (m_state != RDCS_TRANSFER)
		return -1;

	// 客户端来请求完整的截图，strSuffix表示客户端支持的图片格式
	CAutoAddReleasePtr<CScreenBitmap> spScreenBitmap = CScreenBitmap::GetInstance();
	unsigned char *bitmapBits = spScreenBitmap->GetBuffer(0, 0);
	if (bitmapBits == NULL)
		return -2;

	WORD wPixelBytes = spScreenBitmap->GetPixelBytes();
	BitmapInfo bitmapInfo;
	spScreenBitmap->GetBitmapInfo(&bitmapInfo);
	if (bitmapInfo.bmiHeader.biHeight <= 0 || bitmapInfo.bmiHeader.biWidth <= 0)
		return -3;

	m_nScanPointRowCount = (bitmapInfo.bmiHeader.biHeight - 1) / SCAN_MODIFY_RECT_UNIT + 1;
	m_nScanPointColumnCount = (bitmapInfo.bmiHeader.biWidth - 1) / SCAN_MODIFY_RECT_UNIT + 1;
	m_nModifiedBlockCount = 0;
	if (m_pnModifiedBlocks == NULL)
	{
		m_pnModifiedBlocks = new unsigned int[m_nScanPointRowCount * m_nScanPointColumnCount];
	}
	ZeroMemory(m_pnModifiedBlocks, m_nScanPointRowCount * m_nScanPointColumnCount * sizeof(unsigned int));

	CommandBase* pCommand = NULL;
	if (m_strSuffix == BITMAP_SUFFIX_BMP)
	{
		pCommand = new CCommandTransferBitmapServer(bitmapInfo, wPixelBytes, bitmapBits);
	}
	else if (m_strSuffix == BITMAP_SUFFIX_JPEG)
	{
		unsigned char* pDest = NULL;
		unsigned int nDestLen = 0;
		JPEG::Compress(bitmapInfo.bmiHeader.biWidth, bitmapInfo.bmiHeader.biHeight, wPixelBytes, bitmapBits, m_nQuality, &pDest, &nDestLen);
		tool.Log(_T("JPEG::Compress nSrcLen:%d, nDestLen:%d"), bitmapInfo.bmiHeader.biSizeImage, nDestLen);
		pCommand = new CCommandTransferBitmapServerJPEG(wPixelBytes, (unsigned char*)pDest, nDestLen);
	}

	return AsynSend(pCommand);
}

int CClientConnection::AsynSendTransterModifyBitmap()
{
	tool.Log(_T("CClientConnection::AsynSendTransterModifyBitmap, state:0x%08X, m_nModifiedBlockCount:%d"), m_state, m_nModifiedBlockCount);
	if (m_state != RDCS_TRANSFER)
		return -1;

	if (m_nModifiedBlockCount <= 0)
		return -2;

	tool.Log(_T("CClientConnection::AsynSendTransterModifyBitmap 11"));
	CAutoAddReleasePtr<CScreenBitmap> spScreenBitmap = CScreenBitmap::GetInstance();
	WORD wPixelBytes = spScreenBitmap->GetPixelBytes();
	CRect rcScreen = spScreenBitmap->GetRect();
	unsigned int nBufferSize = SCAN_MODIFY_RECT_UNIT * SCAN_MODIFY_RECT_UNIT * wPixelBytes * m_nModifiedBlockCount;
	unsigned int nBufferRowBytes = m_nModifiedBlockCount * SCAN_MODIFY_RECT_UNIT * wPixelBytes;
	unsigned char* pBuffer = new unsigned char[nBufferSize];
	ZeroMemory(pBuffer, nBufferSize * sizeof(unsigned char));

	tool.Log(_T("CClientConnection::AsynSendTransterModifyBitmap 22"));
	for (unsigned int i = 0; i < m_nModifiedBlockCount; i++)
	{
		unsigned int nBlockIndex = m_pnModifiedBlocks[i];
		tool.Log(_T("CClientConnection::AsynSendTransterModifyBitmap nModifiedBlock:%d, nBlockIndex:%d"), i, nBlockIndex);
		unsigned int nBlockRow = nBlockIndex / m_nScanPointColumnCount;
		unsigned int nBlockColumn = nBlockIndex % m_nScanPointColumnCount;

		CRect rcOneModified;
		rcOneModified.left = nBlockColumn * SCAN_MODIFY_RECT_UNIT;
		rcOneModified.top = nBlockRow * SCAN_MODIFY_RECT_UNIT;
		rcOneModified.right = (nBlockColumn == m_nScanPointColumnCount - 1) ? rcScreen.Width() : (rcOneModified.left + SCAN_MODIFY_RECT_UNIT);
		rcOneModified.bottom = (nBlockRow == m_nScanPointRowCount - 1) ? rcScreen.Height() : (rcOneModified.top + SCAN_MODIFY_RECT_UNIT);
		
		unsigned char* pOneBlockBuffer = NULL;
		unsigned int nLength = 0;
		if (!spScreenBitmap->ClipBitmapRect(&rcOneModified, &pOneBlockBuffer, &nLength))
		{
			tool.Log(_T("CClientConnection::AsynSendTransterModifyBitmap ClipBitmapRect failed! return -3"));
			delete[] pBuffer;
			return -3;
		}
		unsigned int nColumnStartPos = i * SCAN_MODIFY_RECT_UNIT * wPixelBytes;
		unsigned int nRowPixelByteCount = rcOneModified.Width() * wPixelBytes;
		for (int j = 0; j < rcOneModified.Height(); j++)
		{
			memcpy(pBuffer + j * nBufferRowBytes + nColumnStartPos, pOneBlockBuffer + j * nRowPixelByteCount, nRowPixelByteCount);
		}
		delete[] pOneBlockBuffer;
		tool.Log(_T("CClientConnection::AsynSendTransterModifyBitmap 33"));
	}
	//Util::SaveBitmap(SCAN_MODIFY_RECT_UNIT, m_nModifiedBlockCount * SCAN_MODIFY_RECT_UNIT, wPixelBytes, pBuffer, _T("E:\\workspaces\\GitHub\\RemoteDesktop\\Debug\\modified.bmp"));
	if (m_strSuffix == BITMAP_SUFFIX_JPEG)
	{
		unsigned char* pDest = NULL;
		unsigned int nDestLen = 0;
		JPEG::Compress(SCAN_MODIFY_RECT_UNIT * m_nModifiedBlockCount, SCAN_MODIFY_RECT_UNIT, wPixelBytes, pBuffer, m_nQuality, &pDest, &nDestLen);
		delete[] pBuffer;
		pBuffer = pDest;
		tool.Log(_T("JPEG::Compress nSrcLen:%d, nDestLen:%d"), nBufferSize, nDestLen);
		nBufferSize = nDestLen;
	}
	CommandBase* pCommand = new CCommandTransferModifyBitmap(m_nModifiedBlockCount, m_pnModifiedBlocks, nBufferSize, pBuffer);
	delete[] pBuffer;
	tool.Log(_T("CClientConnection::AsynSendTransterModifyBitmap AsynSend"));
	return AsynSend(pCommand);
}

void CClientConnection::OnScreenFirstBitmap(BitmapInfo* /*pBitmapInfo*/, WORD /*wPixelBytes*/, unsigned char* /*bitmapBits*/)
{
	AsynSendTransferBitmap();
}

void CClientConnection::OnScreenModified(unsigned int nModifiedBlockCount, unsigned int* pnModifiedBlocks)
{
	if (nModifiedBlockCount == 0 || pnModifiedBlocks == NULL)
		return;

	if (m_nModifiedBlockCount == 0)
	{
		m_nModifiedBlockCount = nModifiedBlockCount;
		memcpy(m_pnModifiedBlocks, pnModifiedBlocks, nModifiedBlockCount * sizeof(unsigned int));
	}
	else if (m_nModifiedBlockCount > 0)
	{
		// 合并变化的区域
		unsigned int nOldModifiedBlockCount = m_nModifiedBlockCount;
		unsigned int* pnOldModifiedBlocks = new unsigned int[nOldModifiedBlockCount];
		memcpy(pnOldModifiedBlocks, m_pnModifiedBlocks, nOldModifiedBlockCount * sizeof(unsigned int));

		ZeroMemory(m_pnModifiedBlocks, m_nScanPointRowCount * m_nScanPointColumnCount * sizeof(unsigned int));
		unsigned int nOldIndex = 0;
		unsigned int nNewIndex = 0;
		m_nModifiedBlockCount = 0;
		for (unsigned int i = 0; i < m_nScanPointRowCount * m_nScanPointColumnCount; i++)
		{
			if (nOldIndex >= nOldModifiedBlockCount)
			{
				m_pnModifiedBlocks[i] = pnModifiedBlocks[nNewIndex++];
				m_nModifiedBlockCount++;
			}
			else if (nNewIndex >= nModifiedBlockCount)
			{
				m_pnModifiedBlocks[i] = pnOldModifiedBlocks[nOldIndex++];
				m_nModifiedBlockCount++;
			}
			else if (pnModifiedBlocks[nNewIndex] >= pnOldModifiedBlocks[nOldIndex])
			{
				m_pnModifiedBlocks[i] = pnOldModifiedBlocks[nOldIndex++];
				m_nModifiedBlockCount++;
			}
			else
			{
				m_pnModifiedBlocks[i] = pnModifiedBlocks[nNewIndex++];
				m_nModifiedBlockCount++;
			}
			if (nOldIndex >= nOldModifiedBlockCount && nNewIndex >= nModifiedBlockCount)
			{
				break;
			}
		}
		delete[] pnOldModifiedBlocks;
	}

	if (m_bSendBitmapReady)
	{
		m_bSendBitmapReady = false;
		if (AsynSendTransterModifyBitmap() < 0)
		{
			m_bSendBitmapReady = true;
		}
		else
		{
			// 更新成功，清空更新区域
			m_nModifiedBlockCount = 0;
			ZeroMemory(m_pnModifiedBlocks, m_nScanPointRowCount * m_nScanPointColumnCount * sizeof(unsigned int));
		}
	}
}

std::string CClientConnection::GetIp() const
{
	return m_strIp;
}

unsigned short CClientConnection::GetPort() const
{
	return m_uPort;
}

void CClientConnection::Stop(RD_ERROR_CODE errorCode)
{
	tool.Log(_T("CClientConnection::Stop, errorCode:%d"), errorCode);
	if (m_state > RDCS_CONNECTED && m_state != RDCS_DISCONNECTED)
	{
		// todo 这里退出时会崩溃，应该同步发包
 		CAutoAddReleasePtr<CCommandDisconnect> spCommand = new CCommandDisconnect(errorCode);
 		Send(spCommand);
	}
	m_bufferev = NULL;
	if (m_pAsynSendCommandEvent)
	{
		event_del(m_pAsynSendCommandEvent);
		event_free(m_pAsynSendCommandEvent);
		m_pAsynSendCommandEvent = NULL;
		Release();
	}
}

bool CClientConnection::IsValid() const
{
	if (m_socket == INVALID_SOCKET || m_bufferev == NULL)
	{
		return false;
	}
	if (m_strIp.length() <= 0 || m_uPort <= 0)
	{
		return false;
	}
	return true;
}

bool CClientConnection::CheckCmdCodeValid(RD_CMD_CODE code) const
{
	switch (m_state)
	{
	case RDCS_CONNECTED:
		if (code != RDCC_PROTOCOL_VERSION)
			return false;
		break;
	case RDCS_PROTOCOL_VERSION:
		if (code != RDCC_LOGON)
			return false;
		break;
	case RDCS_LOGON:
		if (code != RDCC_COMPRESS_MODE)
			return false;
		break;
	case RDCS_COMPRESS_MODE:
		if (code != RDCC_TRANSFER_BITMAP)
			return false;
		break;
	case RDCS_TRANSFER:
		if (code < RDCC_TRANSFER_BITMAP || code > RDCC_DISCONNECT)
			return false;
		break;
	default:
		return false;
	}
	if (code == RDCC_DISCONNECT && (m_state < RDCS_CONNECTED || m_state == RDCS_DISCONNECTED))
		return false;
	return true;
}

bool CClientConnection::operator == (const CClientConnection& p) const
{
	return p.GetIp() == m_strIp && p.GetPort() == m_uPort;
}

void CClientConnection::OnConnect()
{
	tool.Log(_T("CClientConnection::OnConnect"));
	m_state = RDCS_CONNECTED;
}

void CClientConnection::OnReciveCommand(CommandData* pCommand)
{
	tool.Log(_T("CClientConnection::OnReciveCommand, pCommand:0x%08X, code:0x%08X"), pCommand, pCommand->code);
	if (m_state < RDCS_CONNECTED)
		return;

	if (pCommand == NULL)
		return;

	RD_CMD_CODE code = pCommand->code;
	if (!CheckCmdCodeValid(code))
		return;

	IServerReciveCmdBase* pServerReciveCmd = NULL;
	if (CServerReciveCmdFactory::CreateCmd(code, &pServerReciveCmd))
	{
		pServerReciveCmd->Parse(*pCommand, this);
		delete pServerReciveCmd;
	}
}

void CClientConnection::OnError(RD_ERROR_CODE errorCode)
{
	tool.Log(_T("CClientConnection::OnError, events:%d"), errorCode);
	m_state = RDCS_DISCONNECTED;

	m_bufferev = NULL;
	if (m_pAsynSendCommandEvent)
	{
		event_del(m_pAsynSendCommandEvent);
		event_free(m_pAsynSendCommandEvent);
		m_pAsynSendCommandEvent = NULL;
		Release();
	}
}

void CClientConnection::OnWrite()
{
}

void CClientConnection::OnGetProtocalVersion(std::string& strProtocalVersion, std::string& strClientVersion)
{
	tool.LogA("CClientConnection::OnGetProtocalVersion, strProtocalVersion:%s, strClientVersion:%s", strProtocalVersion.c_str(), strClientVersion.c_str());
	// 协议版本是否匹配
	if (strProtocalVersion.length() <= 0)
	{
		m_pClientConnectionEvent->OnDisconnect(this, RDEC_PROTOCOL_VERSION_FAILED);
		return;
	}

	int nClientMainVersion = atoi(strProtocalVersion.c_str());
	int nServerMainVersion = atoi(RD_PROTOCAL_VERSION);
	if (nServerMainVersion != nClientMainVersion)
	{
		// 不兼容主版本不一致的协议
		m_pClientConnectionEvent->OnDisconnect(this, RDEC_PROTOCOL_VERSION_FAILED);
		return;
	}

	m_strClientVersion = strClientVersion;
	m_state = RDCS_PROTOCOL_VERSION;
}

void CClientConnection::OnGetLogon(std::string& strUserName, std::string& strPassword)
{
	tool.LogA("CClientConnection::OnGetLogon, strUserName:%s, strPassword:%s", strUserName.c_str(), strPassword.c_str());
	// todo 验证用户名密码
	m_state = RDCS_LOGON;
	CCommandLogonResult* pCommand = new CCommandLogonResult(0);
	AsynSend(pCommand);
}

void CClientConnection::OnGetCompressMode(RD_COMPRESS_MODE mode)
{
	tool.Log(_T("CClientConnection::OnGetCompressMode, mode:%d"), mode);
	// 客户端来协定压缩方式
	if (!CCompressFactory::CreateCompress(mode, &m_pCompress))
	{
		// 服务器不支持的压缩方式，直接断开连接
		m_pClientConnectionEvent->OnDisconnect(this, RDEC_COMPRESS_MODE_NOT_SUPPORT);
		return;
	}
	m_state = RDCS_COMPRESS_MODE;
}

void CClientConnection::OnGetTransferBitmap(std::string& strSuffix)
{
	tool.LogA("CClientConnection::OnGetTransferBitmap, strSuffix:%s", strSuffix.c_str());
	m_state = RDCS_TRANSFER;
	m_strSuffix = strSuffix;
	AsynSendTransferBitmap();
}

void CClientConnection::OnGetTransferBitmapResult(int nResult)
{
	tool.Log(_T("CClientConnection::OnGetTransferBitmapResult, nResult:%d"), nResult);
	if (nResult == 0)
	{
		// 成功
		m_bSendBitmapReady = true;
	}
	else
	{
		m_pClientConnectionEvent->OnDisconnect(this, RDEC_TRANSFER_BITMAP_RESPONSE_FAILED, nResult);
	}
}

void CClientConnection::OnGetTransferMouseEvent(DWORD dwFlags, CPoint& pt, DWORD dwData)
{
	tool.Log(_T("CClientConnection::OnGetTransferMouseEvent, dwFlags:%d, pt.x:%d, pt.y:%d, dwData:%d"), dwFlags, pt.x, pt.y, dwData);
	// 收到客户端的鼠标事件
	mouse_event(dwFlags, pt.x, pt.y, dwData, 0);
}

void CClientConnection::OnGetTransferKeyboardEvent(BYTE bVk, DWORD dwFlags)
{
	tool.Log(_T("CClientConnection::OnGetTransferKeyboardEvent, bVk:%d, dwFlags:%d"), bVk, dwFlags);
	// 收到客户端的键盘事件
	keybd_event(bVk, (BYTE)MapVirtualKey(bVk, 0), dwFlags, 0);
}

void CClientConnection::OnGetTransferClipboardEvent(unsigned int uFormat, void* pData, unsigned int nLength)
{
	tool.Log(_T("CClientConnection::OnGetTransferClipboardEvent, uFormat:%d, pData:0x%08X, nLength:"), uFormat, pData, nLength);
	// 收到客户端的剪切板事件
	// todo 暂时只支持 text 和 bmp
	if (pData == NULL || nLength == 0)
		return;

	switch (uFormat)
	{
	case CF_TEXT:
		{
			HGLOBAL hGlobal = GlobalAlloc(GHND | GMEM_SHARE, (nLength + 1) * sizeof(char));
			if (hGlobal == NULL)
				return;

			char* pGlobal = static_cast<char*>(GlobalLock(hGlobal));
			if (pGlobal == NULL)
			{
				GlobalFree(hGlobal);
				return;
			}

			//copy the text
			strcpy(pGlobal, (char*)pData);
			GlobalUnlock(hGlobal);

			if (::OpenClipboard(::GetActiveWindow()))
			{
				EmptyClipboard();
				SetClipboardData(CF_TEXT, hGlobal); 
				CloseClipboard();
			}
			GlobalFree(hGlobal);
		}
		break;
	case CF_UNICODETEXT:
		{
			HGLOBAL hGlobal = GlobalAlloc(GHND | GMEM_SHARE, (nLength + 1) * sizeof(wchar_t));
			if (hGlobal == NULL)
				return;

			wchar_t* pGlobal = static_cast<wchar_t*>(GlobalLock(hGlobal));
			if (pGlobal == NULL)
			{
				GlobalFree(hGlobal);
				return;
			}

			//copy the text
			wcscpy(pGlobal, (wchar_t*)pData);
			GlobalUnlock(hGlobal);

			if (::OpenClipboard(::GetActiveWindow()))
			{
				EmptyClipboard();
				SetClipboardData(CF_UNICODETEXT, hGlobal); 
				CloseClipboard();
			}
			GlobalFree(hGlobal);
		}
		break;
	case CF_BITMAP:
		{
			if (nLength <= sizeof(BitmapInfo))
				return;

			BitmapInfo bitmapInfo = {0};
			memcpy(&bitmapInfo, pData, sizeof(BitmapInfo));

			int width = bitmapInfo.bmiHeader.biWidth;
			int height = bitmapInfo.bmiHeader.biHeight;
			if (width <= 0 || height <= 0)
				return;

			HDC hDC = ::GetDC(NULL);
			HDC hDCMem = ::CreateCompatibleDC(hDC);
			HBITMAP hMemBmp = ::CreateCompatibleBitmap(hDC, width, height);
			HGDIOBJ hOldBmp = ::SelectObject(hDCMem, hMemBmp);

			::StretchDIBits(hDCMem, 0, 0, bitmapInfo.bmiHeader.biWidth, bitmapInfo.bmiHeader.biHeight, 0, 0, bitmapInfo.bmiHeader.biWidth, bitmapInfo.bmiHeader.biHeight, (char*)pData + sizeof(BitmapInfo), (LPBITMAPINFO)&bitmapInfo, DIB_RGB_COLORS, SRCCOPY);

			if (::OpenClipboard(::GetActiveWindow()))
			{
				EmptyClipboard();
				SetClipboardData(CF_BITMAP, hMemBmp); 
				CloseClipboard();
			}
			::SelectObject(hDCMem, hOldBmp);
			::DeleteObject(hMemBmp);
			::DeleteDC(hDCMem);
			::ReleaseDC(NULL, hDC);
		}
		break;
	default:
		break;
	}
}

void CClientConnection::OnGetTransferFile(std::string& strFilePath, unsigned long long ullFileSize)
{
	tool.Log(_T("CClientConnection::OnGetTransferFile, strFilePath:%s, ullFileSize:%llu"), strFilePath.c_str(), ullFileSize);
	// 收到客户端的发送文件事件
	// todo 服务端来确认接收
	if (m_pFileTransferDlg == NULL)
	{
		m_pFileTransferDlg = new CFileTransferDlg;
	}
}

void CClientConnection::OnGetTransferDirectory(std::string& strDirPath, unsigned int nFileCount, unsigned long long ullDirSize)
{
	tool.Log(_T("CClientConnection::OnGetTransferDirectory, strDirPath:%s, nFileCount:%d, ullDirSize:%llu"), strDirPath.c_str(), nFileCount, ullDirSize);
	// 收到客户端的发送文件夹
	// todo 服务端来确认接收
	if (m_pFileTransferDlg == NULL)
	{
		m_pFileTransferDlg = new CFileTransferDlg;
	}
}

void CClientConnection::OnGetDisconnect(int nDisconnectCode)
{
	tool.Log(_T("CClientConnection::OnGetDisconnect, nDisconnectCode:%d"), nDisconnectCode);
	// 收到客户端的主动断开事件
	m_pClientConnectionEvent->OnDisconnect(this, (RD_ERROR_CODE)nDisconnectCode);
}

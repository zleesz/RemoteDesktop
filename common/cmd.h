#pragma once
#include "def.h"
#include "stream_buffer.h"
#include "AutoAddReleasePtr.h"
#include <atltypes.h>
#include <assert.h>

class CommandBase : public CAddReleaseRef
{
public:
	CommandBase(RD_CMD_CODE c = RDCC_INVALID)
	{
		m_data.code = c;
		m_data.length = 0;
	}
	virtual ~CommandBase()
	{
		assert(m_dwRef == 0);
		tool.Log(_T("~CommandBase this:0x%08X, code:0x%08X"), this, m_data.code);
	}
	void Clear()
	{
		m_data.code = RDCC_INVALID;
		m_data.data.clear();
		m_data.length = 0;
	}
	void MakeStream(void** stream, int* len)
	{
		if (m_data.code == RDCC_INVALID)
			return;

		// code(uint32), length(uint32), content(string)
		*len = sizeof(uint32_t) + sizeof(uint32_t) + m_data.data.length();
		*stream = new char[*len];
		stream_buffer buffer(*stream, *len);
		buffer.WriteUInt32(m_data.code);
		buffer.WriteUInt32(m_data.data.length());
		buffer.WriteStringUTF8(m_data.data);
	}
	void SetCode(RD_CMD_CODE c)
	{
		m_data.code = c;
	}
	void SetData(void* p, int len)
	{
		m_data.data.clear();
		m_data.data.assign((char*)p, len);
		m_data.length = len;
	}
	RD_CMD_CODE GetCode()
	{
		return m_data.code;
	}
	void GetData(void** p, int* len)
	{
		*p = (void*)m_data.data.c_str();
		*len = m_data.length;
	}
	int GetDataLength()
	{
		return m_data.data.length();
	}

public:
	CommandData	m_data;
};

class CCommandProtocalVersion : public CommandBase
{
public:
	CCommandProtocalVersion(std::string& strProtocalVersion, std::string& strClientVersion) : CommandBase(RDCC_PROTOCOL_VERSION), m_strProtocalVersion(strProtocalVersion), m_strClientVersion(strClientVersion)
	{
		uint32_t len = m_strProtocalVersion.length();
		m_data.data.append((char*)&len, sizeof(len));
		m_data.data.append(m_strProtocalVersion);
		len = m_strClientVersion.length();
		m_data.data.append((char*)&len, sizeof(len));
		m_data.data.append(m_strClientVersion);
		m_data.length = m_data.data.length();
	}

public:
	std::string GetProtocalVersion()
	{
		return m_strProtocalVersion;
	}
	std::string GetClientVersion()
	{
		return m_strClientVersion;
	}

private:
	std::string	m_strProtocalVersion;
	std::string m_strClientVersion;
};

class CCommandLogon : public CommandBase
{
public:
	CCommandLogon(std::string& strUserName, std::string& strPassword) : CommandBase(RDCC_LOGON), m_strUserName(strUserName), m_strPassword(strPassword)
	{
		uint32_t len = strUserName.length();
		m_data.data.append((char*)&len, sizeof(len));
		m_data.data.append(strUserName);
		len = strPassword.length();
		m_data.data.append((char*)&len, sizeof(len));
		m_data.data.append(strPassword);
		m_data.length = m_data.data.length();
	}

public:
	std::string GetUserName()
	{
		return m_strUserName;
	}
	std::string GetPassword()
	{
		return m_strPassword;
	}

private:
	std::string	m_strUserName;
	std::string	m_strPassword;
};

class CCommandLogonResult : public CommandBase
{
public:
	CCommandLogonResult(int nResult) : CommandBase(RDCC_LOGON_RESULT), m_nResult(nResult)
	{
		m_data.data.append((char*)&m_nResult, sizeof(m_nResult));
		m_data.length = m_data.data.length();
	}

public:
	int GetResult()
	{
		return m_nResult;
	}

private:
	int m_nResult;
};

class CCommandCompressMode : public CommandBase
{
public:
	CCommandCompressMode(RD_COMPRESS_MODE mode) : CommandBase(RDCC_COMPRESS_MODE), m_mode(mode)
	{
		m_data.data.append((char*)&m_mode, sizeof(m_mode));
		m_data.length = m_data.data.length();
	}

public:
	RD_COMPRESS_MODE GetMode()
	{
		return m_mode;
	}

private:
	RD_COMPRESS_MODE m_mode;
};

class CCommandTransferBitmapClient : public CommandBase
{
public:
	CCommandTransferBitmapClient(const std::string& strSuffix) : CommandBase(RDCC_TRANSFER_BITMAP), m_strSuffix(strSuffix)
	{
		uint32_t len = m_strSuffix.length();
		m_data.data.append((char*)&len, sizeof(len));
		m_data.data.append(m_strSuffix);
		m_data.length = m_data.data.length();
	}
	std::string GetSuffix()
	{
		return m_strSuffix;
	}

private:
	std::string	m_strSuffix;
};

class CCommandTransferBitmapServer : public CommandBase
{
public:
	CCommandTransferBitmapServer(BitmapInfo& bitmapInfo, WORD wPixelBytes, unsigned char* pBuffer) : CommandBase(RDCC_TRANSFER_BITMAP), m_bitmapInfo(bitmapInfo), m_wPixelBytes(wPixelBytes)
	{
		m_data.data.append((char*)&m_bitmapInfo, sizeof(BitmapInfo));
		m_data.data.append((char*)&m_wPixelBytes, sizeof(uint16_t));
		m_data.data.append((char*)&m_bitmapInfo.bmiHeader.biSizeImage, sizeof(uint32_t));
		m_data.data.append((char*)pBuffer, m_bitmapInfo.bmiHeader.biSizeImage);
		m_pBuffer = (const unsigned char*)m_data.data.c_str() + sizeof(BitmapInfo) + sizeof(uint16_t) + sizeof(uint32_t);
		m_data.length = m_data.data.length();
	}
	~CCommandTransferBitmapServer()
	{

	}
	BitmapInfo GetBitmapInfo()
	{
		return m_bitmapInfo;
	}
	WORD GetPixelBytes()
	{
		return m_wPixelBytes;
	}
	const unsigned char* GetBuffer()
	{
		return m_pBuffer;
	}

private:
	BitmapInfo				m_bitmapInfo;
	WORD					m_wPixelBytes;
	const unsigned char*	m_pBuffer;
};

class CCommandTransferBitmapServerJPEG : public CommandBase
{
public:
	CCommandTransferBitmapServerJPEG(WORD wPixelBytes, unsigned char* pBuffer, unsigned long nSize) : CommandBase(RDCC_TRANSFER_BITMAP), m_wPixelBytes(wPixelBytes), m_nSize(nSize)
	{
	   m_data.data.append((char*)&m_wPixelBytes, sizeof(uint16_t));
	   m_data.data.append((char*)&m_nSize, sizeof(uint32_t));
	   m_data.data.append((char*)pBuffer, nSize);
	   m_pBuffer = (const unsigned char*)m_data.data.c_str() + sizeof(uint16_t) + sizeof(uint32_t);
	   m_data.length = m_data.data.length();
	}
	~CCommandTransferBitmapServerJPEG()
	{

	}
	WORD GetPixelBytes()
	{
	   return m_wPixelBytes;
	}
	unsigned long GetSize()
	{
	   return m_nSize;
	}
	const unsigned char* GetBuffer()
	{
	   return m_pBuffer;
	}

private:
	WORD					m_wPixelBytes;
	unsigned long			m_nSize;
	const unsigned char*	m_pBuffer;
};

class CCommandTransferBitmapClientResponse : public CommandBase
{
public:
	CCommandTransferBitmapClientResponse(int nResult) : CommandBase(RDCC_TRANSFER_BITMAP_RESPONSE), m_nResult(nResult)
	{
		m_data.data.append((char*)&m_nResult, sizeof(int32_t));
		m_data.length = m_data.data.length();
	}
	~CCommandTransferBitmapClientResponse()
	{

	}
	unsigned long GetResult()
	{
		return m_nResult;
	}

private:
	int m_nResult;
};

class CCommandTransferModifyBitmap : public CommandBase
{
public:
	CCommandTransferModifyBitmap(unsigned int nModifiedBlockCount, unsigned int* pnModifiedBlocks, unsigned int nBufferSize, unsigned char* pBuffer) : CommandBase(RDCC_TRANSFER_MODIFY_BITMAP), m_nModifiedBlockCount(nModifiedBlockCount), m_nBufferSize(nBufferSize)
	{
		m_data.data.append((char*)&m_nModifiedBlockCount, sizeof(uint32_t));
		for (unsigned int i = 0; i < m_nModifiedBlockCount; i++)
		{
			m_data.data.append((char*)&pnModifiedBlocks[i], sizeof(uint32_t));
		}
		m_data.data.append((char*)&nBufferSize, sizeof(uint32_t));
		m_data.data.append((char*)pBuffer, nBufferSize);
		m_pBuffer = (const unsigned char*)m_data.data.c_str() + sizeof(uint32_t) + m_nModifiedBlockCount * sizeof(uint32_t) + sizeof(unsigned int);
		m_pnModifiedBlocks = (unsigned int*)((const unsigned char*)m_data.data.c_str() + sizeof(uint32_t));
		m_data.length = m_data.data.length();
	}
	unsigned int GetModifiedBlockCount()
	{
		return m_nModifiedBlockCount;
	}
	unsigned int* GetModifiedBlocks()
	{
		return m_pnModifiedBlocks;
	}
	unsigned int GetBufferSize()
	{
		return m_nBufferSize;
	}
	const unsigned char* GetBuffer()
	{
		return m_pBuffer;
	}

private:
	unsigned int			m_nModifiedBlockCount;
	unsigned int*			m_pnModifiedBlocks;
	unsigned int			m_nBufferSize;
	const unsigned char*	m_pBuffer;
};

class CCommandTransferMouseEvent : public CommandBase
{
public:
	CCommandTransferMouseEvent(DWORD dwFlags, CPoint& pt, DWORD dwData) : CommandBase(RDCC_TRANSFER_MOUSE_EVENT), m_dwFlags(dwFlags), m_pt(pt), m_dwData(dwData)
	{
		m_data.data.append((char*)&m_dwFlags, sizeof(unsigned long));
		m_data.data.append((char*)&m_pt.x, sizeof(long));
		m_data.data.append((char*)&m_pt.y, sizeof(long));
		m_data.data.append((char*)&m_dwData, sizeof(unsigned long));
		m_data.length = m_data.data.length();
	}
	DWORD GetFlags()
	{
		return m_dwFlags;
	}
	CPoint GetPoint()
	{
		return m_pt;
	}
	DWORD GetData()
	{
		return m_dwData;
	}

private:
	DWORD	m_dwFlags;
	CPoint	m_pt;
	DWORD	m_dwData;
};

class CCommandTransferKeyboardEvent : public CommandBase
{
public:
	CCommandTransferKeyboardEvent(BYTE bVk, DWORD dwFlags) : CommandBase(RDCC_TRANSFER_KEYBOARD_EVENT), m_bVk(bVk), m_dwFlags(dwFlags)
	{
		m_data.data.append((char*)&m_bVk, sizeof(unsigned char));
		m_data.data.append((char*)&m_dwFlags, sizeof(unsigned long));
		m_data.length = m_data.data.length();
	}
	BYTE GetVirtualKeyCode()
	{
		return m_bVk;
	}
	DWORD GetFlags()
	{
		return m_dwFlags;
	}

private:
	BYTE	m_bVk;
	DWORD	m_dwFlags;
};

class CCommandTransferClipboard : public CommandBase
{
public:
	CCommandTransferClipboard(unsigned int uFormat, void* pData, unsigned int nLength) : CommandBase(RDCC_TRANSFER_CLIPBOARD), m_uFormat(uFormat), m_nLength(nLength)
	{
		m_data.data.append((char*)&uFormat, sizeof(unsigned int));
		m_data.data.append((char*)&nLength, sizeof(unsigned int));
		m_data.data.append((char*)pData, nLength);
		m_data.length = m_data.data.length();
	}
	unsigned int GetFormat()
	{
		return m_uFormat;
	}
	void* GetData()
	{
		return m_pData;
	}
	unsigned int GetLength()
	{
		return m_nLength;
	}

private:
	unsigned int	m_uFormat;
	void*			m_pData;
	unsigned int	m_nLength;
};

class CCommandTransferFile : public CommandBase
{
public:
	CCommandTransferFile(std::string& strFilePath, unsigned long long ullFileSize) : CommandBase(RDCC_TRANSFER_FILE), m_strFilePath(strFilePath), m_ullFileSize(ullFileSize)
	{
		int len = m_strFilePath.length();
		m_data.data.append((char*)&len, sizeof(int));
		m_data.data.append(m_strFilePath);
		m_data.data.append((char*)&m_ullFileSize, sizeof(unsigned long long));
		m_data.length = m_data.data.length();
	}
	std::string GetFilePath()
	{
		return m_strFilePath;
	}
	unsigned long long GetFileSize()
	{
		return m_ullFileSize;
	}

private:
	std::string			m_strFilePath;
	unsigned long long	m_ullFileSize;
};

class CCommandTransferDirectory : public CommandBase
{
public:
	CCommandTransferDirectory(std::string& strDirPath, unsigned int nFileCount, unsigned long long ullDirSize) : CommandBase(RDCC_TRANSFER_DIRECTORY), m_strDirPath(strDirPath), m_nFileCount(nFileCount), m_ullDirSize(ullDirSize)
	{
		int len = m_strDirPath.length();
		m_data.data.append((char*)&len, sizeof(int));
		m_data.data.append(m_strDirPath);
		m_data.data.append((char*)&m_ullDirSize, sizeof(unsigned long long));
		m_data.length = m_data.data.length();
	}
	std::string GetDirPath()
	{
		return m_strDirPath;
	}
	unsigned int GetFileCount()
	{
		return m_nFileCount;
	}
	unsigned long long GetDirSize()
	{
		return m_ullDirSize;
	}

private:
	std::string			m_strDirPath;
	unsigned int		m_nFileCount;
	unsigned long long	m_ullDirSize;
};

class CCommandDisconnect : public CommandBase
{
public:
	CCommandDisconnect(int nDisconnectCode) : CommandBase(RDCC_DISCONNECT), m_nDisconnectCode(nDisconnectCode)
	{
		m_data.data.append((char*)&m_nDisconnectCode, sizeof(int));
		m_data.length = m_data.data.length();
	}
	int GetDisconnectCode()
	{
		return m_nDisconnectCode;
	}

private:
	int	m_nDisconnectCode;
};

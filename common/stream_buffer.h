#pragma once
#include "stdint.h"

#define SBS_BASE_BUFFER_SIZE	1024
#define SBS_ENLARGE_BUFFER_SIZE	1024

#define CHECK_BUFFER_VALID() if (m_pBuffer == NULL) return E_INVALIDARG;
#define CHECK_BUFFER_SIZE(length) if (m_nPos + length > m_nMaxSize && !Resize(length)) { printf("Resize failed!"); return E_OUTOFMEMORY; }

class stream_buffer
{
public:
	stream_buffer(void* buffer = NULL, size_t len = 0, bool bOwner = false)
		: m_nPos(0), m_nBufferSize(0), m_bOwner(bOwner)
	{
		if (buffer && len > 0)
		{
			m_pBuffer = buffer;
			m_nMaxSize = len;
			m_nBufferSize = len;
		}
		else
		{
			m_bOwner = true;
			m_pBuffer = malloc(SBS_BASE_BUFFER_SIZE);
			if (m_pBuffer != NULL)
			{
				memset(m_pBuffer, 0, SBS_BASE_BUFFER_SIZE);
				m_nMaxSize = SBS_BASE_BUFFER_SIZE;
			}
		}
	}
	virtual ~stream_buffer()
	{
		if (m_bOwner && m_pBuffer)
		{
			free(m_pBuffer);
			m_pBuffer = NULL;
		}
	}
	HRESULT WriteStringUTF8(const char* p, size_t length)
	{
		CHECK_BUFFER_VALID();
		CHECK_BUFFER_SIZE(length);
		if (length <= 0)
		{
			return S_OK;
		}
		memcpy((char*)m_pBuffer + m_nPos, p, length);
		m_nPos += length;
		if (m_nPos > m_nBufferSize)
		{
			m_nBufferSize = m_nPos;
		}
		return S_OK;
	}
	HRESULT ReadStringUTF8(char* p, uint32_t& length)
	{
		CHECK_BUFFER_VALID();
		if (length <= 0)
		{
			return S_OK;
		}
		size_t nEndPos = min(m_nMaxSize, m_nPos + length);
		length = nEndPos - m_nPos;
		memcpy(p, (char*)m_pBuffer + m_nPos, length);
		m_nPos += length;
		return S_OK;
	}
	HRESULT WriteStringUTF8(const std::string& str)
	{
		CHECK_BUFFER_VALID();
		CHECK_BUFFER_SIZE(str.length());
		if (str.length() <= 0)
		{
			return S_OK;
		}
		memcpy((char*)m_pBuffer + m_nPos, str.c_str(), str.length());
		m_nPos += str.length();
		if (m_nPos > m_nBufferSize)
		{
			m_nBufferSize = m_nPos;
		}
		return S_OK;
	}
	HRESULT ReadStringUTF8(std::string& str, uint32_t& length)
	{
		CHECK_BUFFER_VALID();
		if (length <= 0)
		{
			return S_OK;
		}
		size_t nEndPos = min(m_nMaxSize, m_nPos + length);
		length = nEndPos - m_nPos;
		str.assign((char*)m_pBuffer + m_nPos, length);
		m_nPos += length;
		return S_OK;
	}
	HRESULT WriteUInt8(uint8_t n)
	{
		CHECK_BUFFER_VALID();
		CHECK_BUFFER_SIZE(sizeof(uint8_t));
		memcpy((char*)m_pBuffer + m_nPos, &n, sizeof(uint8_t));
		m_nPos += sizeof(uint8_t);
		if (m_nPos > m_nBufferSize)
		{
			m_nBufferSize = m_nPos;
		}
		return S_OK;
	}
	HRESULT ReadUInt8(uint8_t& n)
	{
		CHECK_BUFFER_VALID();
		if (m_nPos + sizeof(uint8_t) > m_nMaxSize)
		{
			return E_OUTOFMEMORY;
		}
		memcpy(&n, (char*)m_pBuffer + m_nPos, sizeof(uint8_t));
		m_nPos += sizeof(uint8_t);
		return S_OK;
	}
	HRESULT WriteUInt16(uint16_t n)
	{
		CHECK_BUFFER_VALID();
		CHECK_BUFFER_SIZE(sizeof(uint16_t));
		memcpy((char*)m_pBuffer + m_nPos, &n, sizeof(uint16_t));
		m_nPos += sizeof(uint16_t);
		if (m_nPos > m_nBufferSize)
		{
			m_nBufferSize = m_nPos;
		}
		return S_OK;
	}
	HRESULT ReadUInt16(uint16_t& n)
	{
		CHECK_BUFFER_VALID();
		if (m_nPos + sizeof(uint16_t) > m_nMaxSize)
		{
			return E_OUTOFMEMORY;
		}
		memcpy(&n, (char*)m_pBuffer + m_nPos, sizeof(uint16_t));
		m_nPos += sizeof(uint16_t);
		return S_OK;
	}
	HRESULT WriteUInt32(uint32_t n)
	{
		CHECK_BUFFER_VALID();
		CHECK_BUFFER_SIZE(sizeof(uint32_t));
		memcpy((char*)m_pBuffer + m_nPos, &n, sizeof(uint32_t));
		m_nPos += sizeof(uint32_t);
		if (m_nPos > m_nBufferSize)
		{
			m_nBufferSize = m_nPos;
		}
		return S_OK;
	}
	HRESULT ReadUInt32(uint32_t& n)
	{
		CHECK_BUFFER_VALID();
		if (m_nPos + sizeof(uint32_t) > m_nMaxSize)
		{
			return E_OUTOFMEMORY;
		}
		memcpy(&n, (char*)m_pBuffer + m_nPos, sizeof(uint32_t));
		m_nPos += sizeof(uint32_t);
		return S_OK;
	}
	HRESULT WriteInt32(int32_t n)
	{
		CHECK_BUFFER_VALID();
		CHECK_BUFFER_SIZE(sizeof(int32_t));
		memcpy((char*)m_pBuffer + m_nPos, &n, sizeof(int32_t));
		m_nPos += sizeof(int32_t);
		if (m_nPos > m_nBufferSize)
		{
			m_nBufferSize = m_nPos;
		}
		return S_OK;
	}
	HRESULT ReadInt32(int32_t& n)
	{
		CHECK_BUFFER_VALID();
		if (m_nPos + sizeof(int32_t) > m_nMaxSize)
		{
			return E_OUTOFMEMORY;
		}
		memcpy(&n, (char*)m_pBuffer + m_nPos, sizeof(int32_t));
		m_nPos += sizeof(int32_t);
		return S_OK;
	}
	HRESULT WriteUInt64(uint64_t n)
	{
		CHECK_BUFFER_VALID();
		CHECK_BUFFER_SIZE(sizeof(uint64_t));
		memcpy((char*)m_pBuffer + m_nPos, &n, sizeof(uint64_t));
		m_nPos += sizeof(uint64_t);
		if (m_nPos > m_nBufferSize)
		{
			m_nBufferSize = m_nPos;
		}
		return S_OK;
	}
	HRESULT ReadUInt64(uint64_t& n)
	{
		CHECK_BUFFER_VALID();
		if (m_nPos + sizeof(uint64_t) > m_nMaxSize)
		{
			return E_OUTOFMEMORY;
		}
		memcpy(&n, (char*)m_pBuffer + m_nPos, sizeof(uint64_t));
		m_nPos += sizeof(uint64_t);
		return S_OK;
	}
	HRESULT WriteInt64(int64_t n)
	{
		CHECK_BUFFER_VALID();
		CHECK_BUFFER_SIZE(sizeof(int64_t));
		memcpy((char*)m_pBuffer + m_nPos, &n, sizeof(int64_t));
		m_nPos += sizeof(int64_t);
		if (m_nPos > m_nBufferSize)
		{
			m_nBufferSize = m_nPos;
		}
		return S_OK;
	}
	HRESULT ReadInt64(int64_t& n)
	{
		CHECK_BUFFER_VALID();
		if (m_nPos + sizeof(int64_t) > m_nMaxSize)
		{
			return E_OUTOFMEMORY;
		}
		memcpy(&n, (char*)m_pBuffer + m_nPos, sizeof(int64_t));
		m_nPos += sizeof(int64_t);
		return S_OK;
	}
	HRESULT WriteWString(const wchar_t* p, size_t length)
	{
		CHECK_BUFFER_VALID();
		CHECK_BUFFER_SIZE(length);
		if (length <= 0)
		{
			return S_OK;
		}
		memcpy((char*)m_pBuffer + m_nPos, p, length + sizeof(wchar_t));
		m_nPos += length + sizeof(wchar_t);
		if (m_nPos > m_nBufferSize)
		{
			m_nBufferSize = m_nPos;
		}
		return S_OK;
	}
	HRESULT ReadWString(wchar_t* p, uint32_t& length)
	{
		CHECK_BUFFER_VALID();
		if (length <= 0)
		{
			return S_OK;
		}
		size_t nEndPos = min(m_nMaxSize, m_nPos + length * sizeof(wchar_t));
		length = nEndPos - m_nPos;
		memcpy(p, (char*)m_pBuffer + m_nPos, length * sizeof(wchar_t));
		m_nPos += length;
		return S_OK;
	}
	HRESULT WriteWString(const std::wstring& str)
	{
		CHECK_BUFFER_VALID();
		CHECK_BUFFER_SIZE(str.length());
		if (str.length() <= 0)
		{
			return S_OK;
		}
		memcpy((char*)m_pBuffer + m_nPos, str.c_str(), str.length() * sizeof(wchar_t));
		m_nPos += str.length() * sizeof(wchar_t);
		if (m_nPos > m_nBufferSize)
		{
			m_nBufferSize = m_nPos;
		}
		return S_OK;
	}
	HRESULT ReadWString(std::wstring& str, uint32_t& length)
	{
		CHECK_BUFFER_VALID();
		if (length <= 0)
		{
			return S_OK;
		}
		size_t nEndPos = min(m_nMaxSize, m_nPos + length);
		length = nEndPos - m_nPos;
		str.assign((wchar_t*)((char*)m_pBuffer + m_nPos), length);
		m_nPos += length * sizeof(wchar_t);
		return S_OK;
	}
	HRESULT Seek(size_t pos)
	{
		CHECK_BUFFER_VALID();
		if (pos > m_nBufferSize)
		{
			return E_OUTOFMEMORY;
		}
		m_nPos = pos;
		return S_OK;
	}
	HRESULT Pass(size_t pos)
	{
		CHECK_BUFFER_VALID();
		if (m_nPos + pos > m_nBufferSize)
		{
			return E_OUTOFMEMORY;
		}
		m_nPos += pos;
		return S_OK;
	}
	size_t GetCurrentPos()
	{
		return m_nPos;
	}
	BOOL IsEnd()
	{
		return m_nPos == m_nBufferSize;
	}
	size_t GetBufferSize()
	{
		return m_nBufferSize;
	}
	void* GetBuffer()
	{
		return m_pBuffer;
	}

private:
	inline void* Resize(size_t length)
	{
		size_t nSize = max(m_nMaxSize + SBS_BASE_BUFFER_SIZE, m_nPos + length);
		m_pBuffer = realloc(m_pBuffer, nSize);
		if (m_pBuffer != NULL)
		{
			memset((char*)m_pBuffer + m_nMaxSize, 0, nSize - m_nMaxSize);
			m_nMaxSize = nSize;
		}
		return m_pBuffer;
	}

private:
	void*		m_pBuffer;
	size_t		m_nPos;
	size_t		m_nBufferSize;
	size_t		m_nMaxSize;
	bool		m_bOwner;
};
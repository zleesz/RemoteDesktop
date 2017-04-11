#pragma once

#define SCOPE_LOCK	static CLock lock ;CScopeLock autoLock(&lock);

class CLock
{
public:
	CLock()
	{
		InitializeCriticalSection(&m_cs);
	}

	~CLock()
	{
		DeleteCriticalSection(&m_cs);
	}

	void Lock()
	{
		EnterCriticalSection(&m_cs);
	}

	void Unlock()
	{
		LeaveCriticalSection(&m_cs);
	}

private:
	CRITICAL_SECTION  m_cs ;
};

class CScopeLock
{
public:
	CScopeLock(CLock * pLock):m_pLock(pLock)
	{
		m_pLock->Lock();
	}

	~CScopeLock()
	{
		m_pLock->Unlock();
	}

private:
	CLock* m_pLock ;
};

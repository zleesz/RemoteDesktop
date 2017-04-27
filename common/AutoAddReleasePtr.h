#pragma once
#include <assert.h>

class CAddReleaseRef
{
public:
	CAddReleaseRef() : m_dwRef(0) {}
	virtual ~CAddReleaseRef() {}
	virtual ULONG AddRef()
	{
		return InterlockedIncrement((LONG*)&m_dwRef);
	}
	virtual ULONG Release()
	{
		long l = InterlockedDecrement((LONG*)&m_dwRef);
		if (l == 0)
		{
			delete this;
		}
		assert(l >= 0);
		return l;
	}
protected:
	DWORD		m_dwRef;
};

template <class T>
class CAutoAddReleasePtr
{
public:
	CAutoAddReleasePtr() throw()
	{
		p = NULL;
	}
	CAutoAddReleasePtr(int nNull) throw()
	{
		assert(nNull == 0);
		(void)nNull;
		p = NULL;
	}
	CAutoAddReleasePtr(T* lp) throw()
	{
		p = lp;
		if (p != NULL)
			p->AddRef();
	}
	~CAutoAddReleasePtr() throw()
	{
		if (p)
			p->Release();
	}
	operator T*() const throw()
	{
		return p;
	}
	T& operator*() const
	{
		assert(p!=NULL);
		return *p;
	}
	//The assert on operator& usually indicates a bug.  If this is really
	//what is needed, however, take the address of the p member explicitly.
	T** operator&() throw()
	{
		assert(p==NULL);
		return &p;
	}
	T* operator->() const throw()
	{
		assert(p!=NULL);
		return (T*)p;
	}
	bool operator!() const throw()
	{
		return (p == NULL);
	}
	bool operator<(T* pT) const throw()
	{
		return p < pT;
	}
	bool operator!=(T* pT) const
	{
		return !operator==(pT);
	}
	bool operator==(T* pT) const throw()
	{
		return p == pT;
	}

	// Release the interface and set to NULL
	void Release() throw()
	{
		T* pTemp = p;
		if (pTemp)
		{
			p = NULL;
			pTemp->Release();
		}
	}
	// Attach to an existing interface (does not AddRef)
	void Attach(T* p2) throw()
	{
		if (p)
			p->Release();
		p = p2;
	}
	// Detach the interface (does not Release)
	T* Detach() throw()
	{
		T* pt = p;
		p = NULL;
		return pt;
	}
	HRESULT CopyTo(T** ppT) throw()
	{
		assert(ppT != NULL);
		if (ppT == NULL)
			return E_POINTER;
		*ppT = p;
		if (p)
			p->AddRef();
		return S_OK;
	}
	T* p;
};

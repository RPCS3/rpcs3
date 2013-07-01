#pragma once

#ifndef ASSERT

#ifdef _DEBUG
	#include <assert.h>
	#define ASSERT assert
#else
	#define ASSERT(exp) ((void)0)
#endif

#endif

inline IUnknown* AtlComPtrAssign(IUnknown** pp, IUnknown* lp)
{
    if(pp == NULL) return NULL;
        
    if(lp != NULL) lp->AddRef();
    
	if(*pp) (*pp)->Release();

    *pp = lp;

    return lp;
}

inline IUnknown* AtlComQIPtrAssign(IUnknown** pp, IUnknown* lp, REFIID riid)
{
	if(pp == NULL) return NULL;

	IUnknown* pTemp = *pp;

	if(lp == NULL || FAILED(lp->QueryInterface(riid, (void**)pp))) *pp = NULL;

	if(pTemp)  pTemp->Release();

	return *pp;
}

template<class T> class _NoAddRefReleaseOnCComPtr : public T
{
private:
	STDMETHOD_(ULONG, AddRef)() = 0;
	STDMETHOD_(ULONG, Release)() = 0;
};

template<class T> class CComPtrBase
{
protected:
	CComPtrBase() throw()
	{
		p = NULL;
	}

	CComPtrBase(T* lp) throw()
	{
		p = lp;

		if(p != NULL) p->AddRef();
	}

	void Swap(CComPtrBase& other)
	{
		T* pTemp = p;
		p = other.p;
		other.p = pTemp;
	}

public:
	typedef T _PtrClass;

	~CComPtrBase() throw()
	{
		if(p) p->Release();
	}

	operator T*() const throw()
	{
		return p;
	}

	T& operator*() const
	{
		ASSERT(p != NULL);

		return *p;
	}

	T** operator&() throw()
	{
		ASSERT(p == NULL);

		return &p;
	}

	_NoAddRefReleaseOnCComPtr<T>* operator->() const throw()
	{
		ASSERT(p != NULL);

		return (_NoAddRefReleaseOnCComPtr<T>*)p;
	}

	bool operator!() const throw()
	{	
		return p == NULL;
	}

	bool operator < (T* pT) const throw()
	{
		return p < pT;
	}

	bool operator != (T* pT) const
	{
		return !operator==(pT);
	}

	bool operator == (T* pT) const throw()
	{
		return p == pT;
	}

	void Release() throw()
	{
		T* pTemp = p;

		if(pTemp)
		{
			p = NULL;
			pTemp->Release();
		}
	}

	bool IsEqualObject(IUnknown* pOther) throw()
	{
		if(p == NULL && pOther == NULL) return true;

		if(p == NULL || pOther == NULL) return false;

		CComPtr<IUnknown> punk1;
		CComPtr<IUnknown> punk2;

		p->QueryInterface(__uuidof(IUnknown), (void**)&punk1);
		pOther->QueryInterface(__uuidof(IUnknown), (void**)&punk2);

		return punk1 == punk2;
	}

	void Attach(T* p2) throw()
	{
		if(p)
		{
			ULONG ref = p->Release();

			(ref);

			ASSERT(ref != 0 || p2 != p);
		}

		p = p2;
	}

	T* Detach() throw()
	{
		T* pt = p;
		p = NULL;
		return pt;
	}

	HRESULT CopyTo(T** ppT) throw()
	{
		ASSERT(ppT != NULL);

		if(ppT == NULL) return E_POINTER;

		*ppT = p;

		if(p) p->AddRef();

		return S_OK;
	}

	HRESULT CoCreateInstance(REFCLSID rclsid, LPUNKNOWN pUnkOuter = NULL, DWORD dwClsContext = CLSCTX_ALL) throw()
	{
		ASSERT(p == NULL);

		return ::CoCreateInstance(rclsid, pUnkOuter, dwClsContext, __uuidof(T), (void**)&p);
	}

	template<class Q> HRESULT QueryInterface(Q** pp) const throw()
	{
		ASSERT(pp != NULL);

		return p->QueryInterface(__uuidof(Q), (void**)pp);
	}

	T* p;
};

template <class T> class CComPtr : public CComPtrBase<T>
{
public:
	CComPtr() throw()
	{
	}

	CComPtr(T* lp) throw() : CComPtrBase<T>(lp)
	{
	}

	CComPtr(const CComPtr<T>& lp) throw() : CComPtrBase<T>(lp.p)
	{	
	}

	T* operator = (T* lp) throw()
	{
		if(*this != lp)
		{
			CComPtr(lp).Swap(*this);
		}

		return *this;
	}

	template<typename Q> T* operator = (const CComPtr<Q>& lp) throw()
	{
		if(!IsEqualObject(lp))
		{
			return static_cast<T*>(AtlComQIPtrAssign((IUnknown**)&p, lp, __uuidof(T)));
		}

		return *this;
	}

	T* operator = (const CComPtr<T>& lp) throw()
	{
		if(*this != lp)
		{
			CComPtr(lp).Swap(*this);
		}

		return *this;
	}

	CComPtr(CComPtr<T>&& lp) throw() : CComPtrBase<T>()
	{
		lp.Swap(*this);
	}

	T* operator = (CComPtr<T>&& lp) throw()
	{
		if(*this != lp)
		{
			CComPtr(static_cast<CComPtr&&>(lp)).Swap(*this);
		}

		return *this;
	}
};

template<> class CComPtr<IDispatch> : public CComPtrBase<IDispatch>
{
public:
	CComPtr() throw()
	{
	}

	CComPtr(IDispatch* lp) throw() : CComPtrBase<IDispatch>(lp)
	{
	}

	CComPtr(const CComPtr<IDispatch>& lp) throw() : CComPtrBase<IDispatch>(lp.p)
	{
	}

	IDispatch* operator = (IDispatch* lp) throw()
	{
		if(*this != lp)
		{
			CComPtr(lp).Swap(*this);
		}

		return *this;
	}

	IDispatch* operator = (const CComPtr<IDispatch>& lp) throw()
	{
		if(*this != lp)
		{
			CComPtr(lp).Swap(*this);
		}

		return *this;
	}

	CComPtr(CComPtr<IDispatch>&& lp) throw() : CComPtrBase<IDispatch>()
	{		
		p = lp.p;		
		lp.p = NULL;
	}

	IDispatch* operator = (CComPtr<IDispatch>&& lp) throw()
	{		
		CComPtr(static_cast<CComPtr&&>(lp)).Swap(*this);

		return *this;
	}	
};

template<class T, const IID* piid = &__uuidof(T)> class CComQIPtr : public CComPtr<T>
{
public:
	CComQIPtr() throw()
	{
	}
	/*
	CComQIPtr(decltype(__nullptr)) throw()
	{
	}
	*/
	CComQIPtr(_Inout_opt_ T* lp) throw() : CComPtr<T>(lp)
	{
	}

	CComQIPtr(_Inout_ const CComQIPtr<T,piid>& lp) throw() : CComPtr<T>(lp.p)
	{
	}

	CComQIPtr(_Inout_opt_ IUnknown* lp) throw()
	{
		if(lp != NULL)
		{
			if(FAILED(lp->QueryInterface(*piid, (void **)&p)))
			{
				p = NULL;
			}
		}
	}
	/*
	T* operator=(decltype(__nullptr)) throw()
	{
		CComQIPtr(nullptr).Swap(*this);

		return nullptr;
	}
	*/
	T* operator = (T* lp) throw()
	{
		if(*this != lp)
		{
			CComQIPtr(lp).Swap(*this);
		}

		return *this;
	}

	T* operator = (const CComQIPtr<T, piid>& lp) throw()
	{
		if(*this != lp)
		{
			CComQIPtr(lp).Swap(*this);
		}

		return *this;
	}

	T* operator = (IUnknown* lp) throw()
	{
		if(*this != lp)
		{
			return static_cast<T*>(AtlComQIPtrAssign((IUnknown**)&p, lp, *piid));
		}

		return *this;
	}
};

template<> class CComQIPtr<IUnknown, &IID_IUnknown> : public CComPtr<IUnknown>
{
public:
	CComQIPtr() throw()
	{
	}

	CComQIPtr(IUnknown* lp) throw()
	{
		if(lp != NULL)
		{
			if(FAILED(lp->QueryInterface(__uuidof(IUnknown), (void **)&p)))
			{
				p = NULL;
			}
		}
	}

	CComQIPtr(const CComQIPtr<IUnknown,&IID_IUnknown>& lp) throw() : CComPtr<IUnknown>(lp.p)
	{
	}

	IUnknown* operator = (IUnknown* lp) throw()
	{
		if(*this != lp)
		{
			return AtlComQIPtrAssign((IUnknown**)&p, lp, __uuidof(IUnknown));
		}

		return *this;
	}

	IUnknown* operator = (const CComQIPtr<IUnknown,&IID_IUnknown>& lp) throw()
	{
		if(*this != lp)
		{
			CComQIPtr(lp).Swap(*this);
		}

		return *this;
	}
};

#pragma once
#include <unknwn.h>     // For IClassFactory
#include <windows.h>
#include "HEIFThumbnailProvider.h"
#include <new>
#include <Shlwapi.h>
#pragma comment(lib, "shlwapi.lib")

extern long g_cDllRef;

class ClassFactory : public IClassFactory
{
public:
	ClassFactory() : m_cRef(1)
	{
		InterlockedIncrement(&g_cDllRef);
	}

protected:
	~ClassFactory() {
		InterlockedDecrement(&g_cDllRef);
	}

public:
	IFACEMETHODIMP QueryInterface(REFIID riid, void **ppv) {
		static const QITAB qit[] =
		{
			QITABENT(ClassFactory, IClassFactory),
			{ 0 },
		};
		return QISearch(this, qit, riid, ppv);
	}

	IFACEMETHODIMP_(ULONG) AddRef() {
		return InterlockedIncrement(&m_cRef);
	}

	IFACEMETHODIMP_(ULONG) Release() {
		ULONG cRef = InterlockedDecrement(&m_cRef);
		if (0 == cRef)
		{
			delete this;
		}
		return cRef;
	}
		
	IFACEMETHODIMP CreateInstance(IUnknown *pUnkOuter, REFIID riid, void **ppv) {
		HRESULT hr = CLASS_E_NOAGGREGATION;

		// pUnkOuter is used for aggregation. We do not support it in the sample.
		if (pUnkOuter == NULL)
		{
			hr = E_OUTOFMEMORY;

			// Create the COM component.
			HEIFThumbnailProvider *pExt = new (std::nothrow) HEIFThumbnailProvider();
			if (pExt)
			{
				// Query the specified interface.
				hr = pExt->QueryInterface(riid, ppv);
				pExt->Release();
			}
		}

		return hr;
	}

	IFACEMETHODIMP LockServer(BOOL fLock) {
		if (fLock)
		{
			InterlockedIncrement(&g_cDllRef);
		}
		else
		{
			InterlockedDecrement(&g_cDllRef);
		}
		return S_OK;
	}

private:
	long m_cRef;
};
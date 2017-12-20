#pragma once
#include <windows.h>
#include <thumbcache.h>     // For IThumbnailProvider
#include <wincodec.h>       // Windows Imaging Codecs
#include <Shlwapi.h>
//#include <Wincrypt.h>   // For CryptStringToBinary.
#include <msxml6.h>

#pragma comment(lib, "Shlwapi.lib")
//#pragma comment(lib, "Crypt32.lib")
#pragma comment(lib, "msxml6.lib")
#pragma comment(lib, "windowscodecs.lib")

extern HINSTANCE g_hInst;
extern long g_cDllRef;

class HEIFThumbnailProvider : public IInitializeWithStream, public IThumbnailProvider {
public:
	HEIFThumbnailProvider() : m_cRef(1), m_pStream(NULL) {
		InterlockedIncrement(&g_cDllRef);
	}

protected:
	~HEIFThumbnailProvider() {
		InterlockedDecrement(&g_cDllRef);
	}

public:
	IFACEMETHODIMP QueryInterface(REFIID riid, void **ppv) {
		static const QITAB qit[] =
		{
			QITABENT(HEIFThumbnailProvider, IThumbnailProvider),
			QITABENT(HEIFThumbnailProvider, IInitializeWithStream),
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

	IFACEMETHODIMP Initialize(IStream *pStream, DWORD grfMode) {
		// A handler instance should be initialized only once in its lifetime. 
		HRESULT hr = HRESULT_FROM_WIN32(ERROR_ALREADY_INITIALIZED);
		if (m_pStream == NULL)
		{
			// Take a reference to the stream if it has not been initialized yet.
			hr = pStream->QueryInterface(&m_pStream);
		}
		return hr;
	}

	IFACEMETHODIMP GetThumbnail(UINT cx, HBITMAP *phbmp, WTS_ALPHATYPE *pdwAlpha) {
		return S_OK;
	}

private:
	long m_cRef;
	IStream *m_pStream;
};
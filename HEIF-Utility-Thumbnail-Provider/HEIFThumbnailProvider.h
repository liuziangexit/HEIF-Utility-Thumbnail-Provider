#pragma once
#include <windows.h>
#include <thumbcache.h>     // For IThumbnailProvider
#include <wincodec.h>       // Windows Imaging Codecs
#include <Shlwapi.h>
#include <Wincrypt.h>   // For CryptStringToBinary.
#include <msxml6.h>
#include <memory>
#include <stdlib.h>
#include <ocidl.h>    
#include <olectl.h>
#include "liuzianglib.h"
#include "DC_File.h"
#include "DC_STR.h"
#pragma comment(lib,"HUD")
#pragma comment(lib, "Shlwapi.lib")
#pragma comment(lib, "Crypt32.lib")
#pragma comment(lib, "msxml6.lib")
#pragma comment(lib, "windowscodecs.lib")

extern HINSTANCE g_hInst;
extern long g_cDllRef;
EXTERN_C IMAGE_DOS_HEADER __ImageBase;

extern "C" __declspec(dllexport) void heif2jpg(const char heif_bin[], int input_buffer_size, const int jpg_quality, char output_buffer[], int output_buffer_size, const char* input_temp_filename, int* copysize, bool include_exif, bool color_profile, const char icc_bin[], int icc_size);

HBITMAP BitmapFromJpgBits(char* pData, int dataLen) {
	HBITMAP hBitmap = NULL;
	HBITMAP hBmpCopy;
	if (pData) {
		IPicture* pIPicture;
		IStream* pIStream;
		HGLOBAL hGlobal = GlobalAlloc(GMEM_MOVEABLE, dataLen);
		if (!hGlobal)
			throw std::bad_alloc();
		void* pBuf = GlobalLock(hGlobal);
		memcpy(pBuf, (void*)pData, dataLen);
		GlobalUnlock(hGlobal);
		HRESULT hr = CreateStreamOnHGlobal(hGlobal, TRUE, &pIStream);
		if (hr == S_OK) {
			hr = OleLoadPicture(pIStream, 0, FALSE, IID_IPicture, (void**)&pIPicture);
			if (hr == S_OK) {
				pIPicture->get_Handle((OLE_HANDLE*)&hBitmap);
			}
		}
		hBmpCopy = (HBITMAP)CopyImage(hBitmap, IMAGE_BITMAP, 0, 0, LR_DEFAULTSIZE);
		if (pIStream) {
			pIStream->Release();
		}
		if (pIPicture) {
			pIPicture->Release();
		}
	}
	return  hBmpCopy;
}

std::string make_temp_filename(const std::string& Path) {
	int32_t value;
	std::string filename;

	int count = 0;
	while (1) {
		value = DC::randomer(0, 65535);
		filename = Path + "\\HUTP_temp" + DC::STR::toString(value);
		if (!DC::File::exists(filename))
			return filename;
		if (count >= 1000)//防止死循环，虽然不应该发生
			return "";
	}
	return "";
}

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
		std::unique_ptr<char[]> DLLPathChars(new char[257]);
		memset(DLLPathChars.get(), 0, sizeof(char) * 257);

		std::string DLLPath(DLLPathChars.get(), GetModuleFileNameA((HINSTANCE)&__ImageBase, reinterpret_cast<LPSTR>(DLLPathChars.get()), 257));

		DLLPathChars.get_deleter()(DLLPathChars.release());

		if (DLLPath.size() == 0)
			return S_FALSE;

		DLLPath = DLLPath.substr(0, DLLPath.find_last_of("\\/"));

		auto TempDirectory = DC::File::read(DLLPath + "\\conf\\HUTPWriteDirectory");

		auto temp_filename = make_temp_filename(TempDirectory);
		if (temp_filename.empty())
			return S_FALSE;

		STATSTG StreamStats = { 0 };
		if (FAILED(m_pStream->Stat(&StreamStats, 0)))
			return S_FALSE;

		std::unique_ptr<char> streamData(reinterpret_cast<char*>(malloc(StreamStats.cbSize.QuadPart)));
		ULONG bytesSaved = 0;
		if (FAILED(m_pStream->Read(streamData.get(), StreamStats.cbSize.QuadPart, &bytesSaved)))
			return S_FALSE;

		std::unique_ptr<char> output(reinterpret_cast<char*>(malloc(StreamStats.cbSize.QuadPart)));
		int copysize = 0;
		heif2jpg(streamData.get(), StreamStats.cbSize.QuadPart, 10, output.get(), StreamStats.cbSize.QuadPart, temp_filename.c_str(), &copysize, false, false, 0, 0);
		
		DC::File::del(temp_filename);

		auto rv = BitmapFromJpgBits(output.get(), copysize);
		*phbmp = rv;
		*pdwAlpha = WTSAT_UNKNOWN;

		return S_OK;
	}

private:
	long m_cRef;
	IStream *m_pStream;
};
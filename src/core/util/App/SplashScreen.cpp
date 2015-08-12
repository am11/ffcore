#include "pch.h"
#include <afxres.h>

#include "App/AppGlobals.h"
#include "App/SplashScreen.h"
#include "Module/Module.h"
#include "Resource/util-resource.h"
#include "Thread/ThreadUtil.h"

#if METRO_APP

ff::SplashScreen::SplashScreen()
{
}

ff::SplashScreen::~SplashScreen()
{
}

bool ff::SplashScreen::Create(AppGlobals &app)
{
	return true;
}

void ff::SplashScreen::Close()
{
}

bool ff::SplashScreen::IsShowing() const
{
	return false;
}

void ff::SplashScreen::SetStatusText(StringRef text)
{
}

#else

ff::SplashScreen::SplashScreen()
	: _bitmapInstance(nullptr)
	, _bitmapResource(0)
	, _status(GetThisModule().GetString(IDS_STATUS_LOADING))
	, _created(false)
{
}

ff::SplashScreen::~SplashScreen()
{
	Close();
}

bool ff::SplashScreen::Create(AppGlobals &app)
{
	assertRetVal(!_created, false);
	noAssertRetVal(!app.IsAutomation(), false);

	HINSTANCE bitmapInstance = app.GetModule().GetInstance();
	UINT bitmapResource = app.GetSplashScreenBitmapResource();
	noAssertRetVal(bitmapInstance && bitmapResource != 0, false);

	_bitmapInstance = bitmapInstance;
	_bitmapResource = bitmapResource;

	UINT threadId = 0;
	WinHandle thread = reinterpret_cast<HANDLE>(::_beginthreadex(nullptr, 0, StaticWorkerThread, this, 0, &threadId));
	assertRetVal(thread != nullptr, false);

	_created = true;
	return true;
}

void ff::SplashScreen::Close()
{
	if (IsShowing())
	{
		_created = false;
		::EndDialog(Handle(), IDOK);
	}
}

bool ff::SplashScreen::IsShowing() const
{
	return _created && Handle();
}

void ff::SplashScreen::SetStatusText(StringRef text)
{
	// cache the text
	{
		LockMutex lock(_statusCs);
		_status = text.size() ? text : GetThisModule().GetString(IDS_STATUS_LOADING);
	}

	if (IsShowing())
	{
		SetDlgItemText(Handle(), IDC_STATIC, _status.c_str());
	}
}

// background thread
INT_PTR ff::SplashScreen::DialogProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_INITDIALOG:
		{
			LockMutex lock(_statusCs);
			SetDlgItemText(hwnd, IDC_STATIC, _status.c_str());
		}
		::SetTimer(hwnd, 1, 100, nullptr);
		break;

	case WM_ERASEBKGND:
		OnEraseBackground(hwnd, reinterpret_cast<HDC>(wParam));
		return 1;

	case WM_CTLCOLORSTATIC:
		return reinterpret_cast<INT_PTR>(::GetStockObject(NULL_BRUSH));

	case WM_DESTROY:
		::KillTimer(hwnd, 1);
		break;

	case WM_TIMER:
		if (wParam == 1)
		{
			String newStatus;
			{
				LockMutex lock(_statusCs);
				newStatus = _status;
			}

			if (GetDialogItemText(hwnd, IDC_STATIC) != newStatus)
			{
				SetDlgItemText(hwnd, IDC_STATIC, newStatus.c_str());
			}
		}
		break;
	}

	return __super::DialogProc(hwnd, msg, wParam, lParam);
}

// background thread
void ff::SplashScreen::PostNcDestroy()
{
	_bitmap = nullptr;
}

// static WINAPI
unsigned int ff::SplashScreen::StaticWorkerThread(void *context)
{
	SplashScreen *splashScreen = static_cast<SplashScreen *>(context);
	return splashScreen->WorkerThread();
}

// background thread
unsigned int ff::SplashScreen::WorkerThread()
{
	_bitmap = ::LoadBitmap(_bitmapInstance, MAKEINTRESOURCE(_bitmapResource));
	assertRetVal(_bitmap != nullptr, 0);

	__super::Create(IDD_SPLASH_SCREEN, nullptr, GetThisModule().GetInstance());

	return 0;
}

// background thread
void ff::SplashScreen::OnEraseBackground(HWND hwnd, HDC hdc)
{
	CreateDcHandle bitmapDc = CreateCompatibleDC(hdc);
	SelectGdiObject<HBITMAP> selectBitmap(bitmapDc, _bitmap);

	BITMAP bitmap;
	::GetObject(_bitmap, sizeof(bitmap), &bitmap);

	RectInt clientRect = GetClientRect(Handle());
	int oldMode = ::SetStretchBltMode(hdc, HALFTONE);
	::StretchBlt(hdc, 0, 0, clientRect.Width(), clientRect.Height(), bitmapDc, 0, 0, bitmap.bmWidth, bitmap.bmHeight, SRCCOPY);
	::SetStretchBltMode(hdc, oldMode);
}

#endif

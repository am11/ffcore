#include "pch.h"
#include "Windows/Handles.h"

void ff::MyDestroyHandle(HANDLE handle)
{
	::CloseHandle(handle);
}

#if !METRO_APP

void ff::MyDestroyGdiHandle(HGDIOBJ handle)
{
	::DeleteObject(handle);
}

void ff::MyDestroyDcHandle(HDC handle)
{
	::DeleteDC(handle);
}

void ff::MyDestroyCursorHandle(HCURSOR handle)
{
	::DestroyCursor(handle);
}

void ff::MyDestroyFontHandle(HANDLE handle)
{
	::RemoveFontMemResourceEx(handle);
}

void ff::MyDestroyAccelHandle(HACCEL handle)
{
	::DestroyAcceleratorTable(handle);
}

void ff::MyDestroyMenuHandle(HMENU handle)
{
	::DestroyMenu(handle);
}

void ff::MyDestroyFileChangeHandle(HANDLE handle)
{
	::FindCloseChangeNotification(handle);
}

#endif // !METRO_APP

#include "pch.h"
#include "Globals/ProcessGlobals.h"
#include "Input/GlobalDirectInput.h"
#include "String/StringUtil.h"
#include "String/SysString.h"

#include <wbemidl.h>

#if !METRO_APP

// STATIC_DATA (pod)
static ff::ComPtr<IDirectInput8> s_pDirectInput;
static long s_nDirectInputRefs = 0;

ff::GlobalDirectInput::GlobalDirectInput()
	: _initInputIds(false)
{
	LockMutex crit(GCS_DIRECT_INPUT);

	if (++s_nDirectInputRefs && !s_pDirectInput)
	{
		verify(SUCCEEDED(DirectInput8Create(
			GetThisModule().GetInstance(),
			DIRECTINPUT_VERSION,
			IID_IDirectInput8,
			(void**)&s_pDirectInput, nullptr)));
	}
}

ff::GlobalDirectInput::~GlobalDirectInput()
{
	LockMutex crit(GCS_DIRECT_INPUT);

	if (!--s_nDirectInputRefs)
	{
		s_pDirectInput = nullptr;
	}
}

IDirectInput8 *ff::GlobalDirectInput::GetInput()
{
	return s_pDirectInput;
}

bool ff::GlobalDirectInput::CreateDevice(
	HWND                  hwnd,
	REFGUID               deviceID,
	LPCDIDATAFORMAT       pDataFormat,
	DWORD                 nBufferSize,
	IDirectInputDevice8** device)
{
	assertRetVal(hwnd && device && pDataFormat && s_pDirectInput, false);
	*device = nullptr;

	ComPtr<IDirectInputDevice8> pDevice;
	assertRetVal(SUCCEEDED(s_pDirectInput->CreateDevice(deviceID, &pDevice, nullptr)), false);
	assertRetVal(SUCCEEDED(pDevice->SetDataFormat(pDataFormat)), false);

	if (nBufferSize)
	{
		DIPROPDWORD prop;
		prop.diph.dwSize       = sizeof(prop);
		prop.diph.dwHeaderSize = sizeof(prop.diph);
		prop.diph.dwObj        = 0;
		prop.diph.dwHow        = DIPH_DEVICE;
		prop.dwData            = nBufferSize;

		assertRetVal(SUCCEEDED(pDevice->SetProperty(DIPROP_BUFFERSIZE, &prop.diph)), false);
	}

	assertRetVal(SUCCEEDED(pDevice->SetCooperativeLevel(hwnd, DISCL_FOREGROUND | DISCL_NONEXCLUSIVE)), false);

	// This can fail if focus leaves the app
	pDevice->Acquire();

	*device = pDevice.Detach();
	return true;
}

bool ff::GlobalDirectInput::Poll(IDirectInputDevice8 *pDevice)
{
	assertRetVal(pDevice, false);

	HRESULT hr = pDevice->Poll();

	if (hr == DIERR_NOTACQUIRED || hr == DIERR_INPUTLOST)
	{
		if (SUCCEEDED(pDevice->Acquire()))
		{
			hr = pDevice->Poll();

			// Throw away all buffered data

			for (DWORD nCount = 1; nCount == 1; )
			{
				if (FAILED(pDevice->GetDeviceData(sizeof(DIDEVICEOBJECTDATA), nullptr, &nCount, 0)))
				{
					break;
				}
			}
		}
	}

	return SUCCEEDED(hr);
}

// Enum each PNP device using WMI and check each device ID to see if it contains 
// "IG_" (ex. "VID_045E&PID_028E&IG_00").  If it does, then it's an XInput device
// Unfortunately this information can not be found by just using DirectInput 
// (Copied from the help file for DirectInput)
static void FillXInputDeviceIds(ff::Vector<DWORD> &ids)
{
	// Create WMI
	ff::ComPtr<IWbemLocator> pLocator;
	assertRet(SUCCEEDED(CoCreateInstance(__uuidof(WbemLocator), nullptr, CLSCTX_INPROC_SERVER, __uuidof(IWbemLocator), (void**)&pLocator)));

	// Connect to WMI 
	ff::ComPtr<IWbemServices> pServices;
	static ff::StaticString staticPath(L"\\\\.\\root\\cimv2");
	assertRet(SUCCEEDED(pLocator->ConnectServer(ff::SysString(staticPath), nullptr, nullptr, 0L, 0L, nullptr, nullptr, &pServices)));

	// Switch security level to IMPERSONATE. 
	CoSetProxyBlanket(pServices, RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE, nullptr, RPC_C_AUTHN_LEVEL_CALL, RPC_C_IMP_LEVEL_IMPERSONATE, nullptr, EOAC_NONE );

	ff::ComPtr<IEnumWbemClassObject> pEnumDevices;
	static ff::StaticString staticEntity(L"Win32_PNPEntity");
	assertRet(SUCCEEDED(pServices->CreateInstanceEnum(ff::SysString(staticEntity), 0, nullptr, &pEnumDevices)));

	bool bIsXinputDevice = false;
	static ff::StaticString staticDevice(L"DeviceID");
	ff::SysString bstrDeviceID(staticDevice);

	// Loop over all devices
	while (!bIsXinputDevice)
	{
		// Get 20 at a time
		DWORD nReturned = 0;
		IWbemClassObject *pDevices[20] = { 0 };
		assertRet(SUCCEEDED(pEnumDevices->Next(10000, 20, pDevices, &nReturned)));

		if (!nReturned)
		{
			break;
		}

		for (DWORD nDevice = 0; nDevice < nReturned && !bIsXinputDevice; nDevice++)
		{
			// For each device, get its device ID
			VARIANT var;
			VariantInit(&var);

			HRESULT hr = pDevices[nDevice]->Get(bstrDeviceID, 0L, &var, nullptr, nullptr);

			if (SUCCEEDED(hr) && var.vt == VT_BSTR && var.bstrVal != nullptr)
			{
				// Check if the device ID contains "IG_".  If it does, then it's an XInput device
				// This information can not be found from DirectInput 
				if (wcsstr(var.bstrVal, L"IG_"))
				{
					// If it does, then get the VID/PID from var.bstrVal
					DWORD dwPid = 0;
					DWORD dwVid = 0;
					const wchar_t *strVid = wcsstr(var.bstrVal, L"VID_");

					if (strVid && swscanf_s(strVid, L"VID_%4X", &dwVid) != 1)
					{
						dwVid = 0;
					}

					const wchar_t *strPid = wcsstr(var.bstrVal, L"PID_");
					if (strPid && swscanf_s(strPid, L"PID_%4X", &dwPid) != 1)
					{
						dwPid = 0;
					}

					DWORD dwVidPid = MAKELONG(dwVid, dwPid);
					ids.Push(dwVidPid);
				}
			}

			VariantClear(&var);
		}

		for (DWORD nDevice = 0; nDevice < nReturned; nDevice++)
		{
			ff::ReleaseRef(pDevices[nDevice]);
		}
	}
}

bool ff::GlobalDirectInput::IsXInputDevice(REFGUID guidProduct)
{
	if (!_initInputIds)
	{
		LockMutex crit(GCS_DIRECT_INPUT);

		if (!_initInputIds)
		{
			FillXInputDeviceIds(_inputIds);
			_initInputIds = true;
		}
	}

	if (_inputIds.Find(guidProduct.Data1) != INVALID_SIZE)
	{
		return true;
	}

	return false;
}

#endif // !METRO_APP

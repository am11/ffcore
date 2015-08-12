#pragma once

#if !METRO_APP

namespace ff
{
	class GlobalDirectInput
	{
	public:
		GlobalDirectInput();
		~GlobalDirectInput();

		IDirectInput8 *GetInput();
		bool IsXInputDevice(REFGUID guidProduct);

		// Helpers
		bool CreateDevice(HWND hwnd, REFGUID deviceID, LPCDIDATAFORMAT pDataFormat, DWORD nBufferSize, IDirectInputDevice8 **device);
		bool Poll(IDirectInputDevice8 *pDevice);

	private:
		bool _initInputIds;
		Vector<DWORD> _inputIds;
	};
}

#endif // !METRO_APP

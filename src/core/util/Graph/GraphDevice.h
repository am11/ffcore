#pragma once

namespace ff
{
	class BufferCache;
	class GraphStateCache;

	class __declspec(uuid("1b26d121-cda5-4705-ae3d-4815b4a4115b")) __declspec(novtable)
		IGraphDevice : public IUnknown
	{
	public:
		virtual void Reset() = 0;
		virtual bool IsSoftware() const = 0;

		virtual ID3D11DeviceX *GetDX() = 0;
		virtual IDXGIDeviceX *GetDXGI() = 0;
		virtual ID3D11DeviceContextX *GetContext() = 0;
		virtual IDXGIAdapterX *GetAdapter() = 0;
		virtual D3D_FEATURE_LEVEL GetFeatureLevel() const = 0;

		virtual BufferCache &GetVertexBuffers() = 0;
		virtual BufferCache &GetIndexBuffers() = 0;
		virtual GraphStateCache &GetStateCache() = 0;
	};

	bool CreateHardwareGraphDevice(IGraphDevice **device);
	bool CreateSoftwareGraphDevice(IGraphDevice **device);
	bool CreateGraphDevice(IDXGIAdapterX *pCard, IGraphDevice **device);
}

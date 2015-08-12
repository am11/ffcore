#pragma once

// DirectX includes
// Precompiled header only

#ifdef DIRECTINPUT_VERSION
#undef DIRECTINPUT_VERSION
#endif
#define DIRECTINPUT_VERSION 0x0800

// The Win8 SDK and DX SDK both define these constants.
// Since the DX SDK is only used for non-Metro apps, undefine these only for non-Metro apps.
#if !METRO_APP

#ifdef D3D10_ERROR_FILE_NOT_FOUND
#undef D3D10_ERROR_FILE_NOT_FOUND
#endif

#ifdef D3D10_ERROR_TOO_MANY_UNIQUE_STATE_OBJECTS
#undef D3D10_ERROR_TOO_MANY_UNIQUE_STATE_OBJECTS
#endif

#ifdef D3D11_ERROR_DEFERRED_CONTEXT_MAP_WITHOUT_INITIAL_DISCARD
#undef D3D11_ERROR_DEFERRED_CONTEXT_MAP_WITHOUT_INITIAL_DISCARD
#endif

#ifdef D3D11_ERROR_FILE_NOT_FOUND
#undef D3D11_ERROR_FILE_NOT_FOUND
#endif

#ifdef D3D11_ERROR_TOO_MANY_UNIQUE_STATE_OBJECTS
#undef D3D11_ERROR_TOO_MANY_UNIQUE_STATE_OBJECTS
#endif

#ifdef D3D11_ERROR_TOO_MANY_UNIQUE_VIEW_OBJECTS
#undef D3D11_ERROR_TOO_MANY_UNIQUE_VIEW_OBJECTS
#endif

#ifdef DXGI_ERROR_DEVICE_HUNG
#undef DXGI_ERROR_DEVICE_HUNG
#endif

#ifdef DXGI_ERROR_DEVICE_REMOVED
#undef DXGI_ERROR_DEVICE_REMOVED
#endif

#ifdef DXGI_ERROR_DEVICE_RESET
#undef DXGI_ERROR_DEVICE_RESET
#endif

#ifdef DXGI_ERROR_DRIVER_INTERNAL_ERROR
#undef DXGI_ERROR_DRIVER_INTERNAL_ERROR
#endif

#ifdef DXGI_ERROR_FRAME_STATISTICS_DISJOINT
#undef DXGI_ERROR_FRAME_STATISTICS_DISJOINT
#endif

#ifdef DXGI_ERROR_GRAPHICS_VIDPN_SOURCE_IN_USE
#undef DXGI_ERROR_GRAPHICS_VIDPN_SOURCE_IN_USE
#endif

#ifdef DXGI_ERROR_INVALID_CALL
#undef DXGI_ERROR_INVALID_CALL
#endif

#ifdef DXGI_ERROR_MORE_DATA
#undef DXGI_ERROR_MORE_DATA
#endif

#ifdef DXGI_ERROR_NONEXCLUSIVE
#undef DXGI_ERROR_NONEXCLUSIVE
#endif

#ifdef DXGI_ERROR_NOT_CURRENTLY_AVAILABLE
#undef DXGI_ERROR_NOT_CURRENTLY_AVAILABLE
#endif

#ifdef DXGI_ERROR_NOT_FOUND
#undef DXGI_ERROR_NOT_FOUND
#endif

#ifdef DXGI_ERROR_REMOTE_CLIENT_DISCONNECTED
#undef DXGI_ERROR_REMOTE_CLIENT_DISCONNECTED
#endif

#ifdef DXGI_ERROR_REMOTE_OUTOFMEMORY
#undef DXGI_ERROR_REMOTE_OUTOFMEMORY
#endif

#ifdef DXGI_ERROR_UNSUPPORTED
#undef DXGI_ERROR_UNSUPPORTED
#endif

#ifdef DXGI_ERROR_WAS_STILL_DRAWING
#undef DXGI_ERROR_WAS_STILL_DRAWING
#endif

#ifdef DXGI_STATUS_CLIPPED
#undef DXGI_STATUS_CLIPPED
#endif

#ifdef DXGI_STATUS_GRAPHICS_VIDPN_SOURCE_IN_USE
#undef DXGI_STATUS_GRAPHICS_VIDPN_SOURCE_IN_USE
#endif

#ifdef DXGI_STATUS_MODE_CHANGE_IN_PROGRESS
#undef DXGI_STATUS_MODE_CHANGE_IN_PROGRESS
#endif

#ifdef DXGI_STATUS_MODE_CHANGED
#undef DXGI_STATUS_MODE_CHANGED
#endif

#ifdef DXGI_STATUS_NO_DESKTOP_ACCESS
#undef DXGI_STATUS_NO_DESKTOP_ACCESS
#endif

#ifdef DXGI_STATUS_NO_REDIRECTION
#undef DXGI_STATUS_NO_REDIRECTION
#endif

#ifdef DXGI_STATUS_OCCLUDED
#undef DXGI_STATUS_OCCLUDED
#endif

#endif

#include <DInput.h>
#include <DirectXMath.h>
#include <DirectXPackedVector.h>
#include <XAudio2.h>
#include <XInput.h>

#if METRO_APP
#include <DXGI1_2.h>
#include <D3D11_1.h>
#define IDXGIAdapterX IDXGIAdapter2
#define IDXGIDeviceX IDXGIDevice2
#define IDXGIFactoryX IDXGIFactory2
#define IDXGIOutputX IDXGIOutput1
#define IDXGIResourceX IDXGIResource1
#define IDXGISurfaceX IDXGISurface2
#define IDXGISwapChainX IDXGISwapChain1
#define ID3D11DeviceX ID3D11Device1
#define ID3D11DeviceContextX ID3D11DeviceContext1
#define ID3D11BlendStateX ID3D11BlendState1
#define ID3D11RasterizerStateX ID3D11RasterizerState1
#else
#include <D3D11.h>
#define IDXGIAdapterX IDXGIAdapter1
#define IDXGIDeviceX IDXGIDevice1
#define IDXGIFactoryX IDXGIFactory1
#define IDXGIOutputX IDXGIOutput
#define IDXGIResourceX IDXGIResource
#define IDXGISurfaceX IDXGISurface1
#define IDXGISwapChainX IDXGISwapChain
#define ID3D11DeviceX ID3D11Device
#define ID3D11DeviceContextX ID3D11DeviceContext
#define ID3D11BlendStateX ID3D11BlendState
#define ID3D11RasterizerStateX ID3D11RasterizerState
#endif

inline bool operator<(const D3D11_BLEND_DESC &lhs, const D3D11_BLEND_DESC &rhs)
{
	return memcmp(&lhs, &rhs, sizeof(lhs)) < 0;
}

inline bool operator<(const D3D11_DEPTH_STENCIL_DESC &lhs, const D3D11_DEPTH_STENCIL_DESC &rhs)
{
	return memcmp(&lhs, &rhs, sizeof(lhs)) < 0;
}

inline bool operator<(const D3D11_RASTERIZER_DESC &lhs, const D3D11_RASTERIZER_DESC &rhs)
{
	return memcmp(&lhs, &rhs, sizeof(lhs)) < 0;
}

inline bool operator<(const D3D11_SAMPLER_DESC &lhs, const D3D11_SAMPLER_DESC &rhs)
{
	return memcmp(&lhs, &rhs, sizeof(lhs)) < 0;
}

inline bool operator==(DirectX::FXMVECTOR lhs, DirectX::FXMVECTOR rhs)
{
	UINT record;
	DirectX::XMVectorEqualR(&record, lhs, rhs);
	return DirectX::XMComparisonAllTrue(record);
}

inline bool operator!=(DirectX::FXMVECTOR lhs, DirectX::FXMVECTOR rhs)
{
	UINT record;
	DirectX::XMVectorEqualR(&record, lhs, rhs);
	return DirectX::XMComparisonAnyFalse(record);
}

inline bool operator==(const DirectX::XMFLOAT4 &lhs, const DirectX::XMFLOAT4 &rhs)
{
	return DirectX::XMLoadFloat4(&lhs) == DirectX::XMLoadFloat4(&rhs);
}

inline bool operator!=(const DirectX::XMFLOAT4 &lhs, const DirectX::XMFLOAT4 &rhs)
{
	return DirectX::XMLoadFloat4(&lhs) != DirectX::XMLoadFloat4(&rhs);
}

MAKE_POD(DirectX::XMFLOAT2);
MAKE_POD(DirectX::XMFLOAT2A);
MAKE_POD(DirectX::XMFLOAT3);
MAKE_POD(DirectX::XMFLOAT3A);
MAKE_POD(DirectX::XMFLOAT4);
MAKE_POD(DirectX::XMFLOAT4A);
MAKE_POD(DirectX::XMFLOAT3X3);
MAKE_POD(DirectX::XMFLOAT4X3);
MAKE_POD(DirectX::XMFLOAT4X3A);
MAKE_POD(DirectX::XMFLOAT4X4);
MAKE_POD(DirectX::XMFLOAT4X4A);
MAKE_POD(DirectX::XMINT2);
MAKE_POD(DirectX::XMINT3);
MAKE_POD(DirectX::XMINT4);
MAKE_POD(DirectX::XMUINT2);
MAKE_POD(DirectX::XMUINT3);
MAKE_POD(DirectX::XMUINT4);
MAKE_POD(DirectX::XMMATRIX);
MAKE_POD(DirectX::XMVECTORF32);
MAKE_POD(DirectX::XMVECTORI32);
MAKE_POD(DirectX::XMVECTORU32);
MAKE_POD(DirectX::XMVECTORU8);

// Probably should be moved to the files that actually need it:
using namespace DirectX;

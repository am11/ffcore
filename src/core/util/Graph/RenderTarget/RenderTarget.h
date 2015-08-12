#pragma once

#include "Graph/GraphDeviceChild.h"
#include "Windows/WinUtil.h"

namespace ff
{
	class IGraphTexture;
	class IRenderDepth;

	class __declspec(uuid("fcfc35d1-be2d-4630-b09d-58100ab1bf7c")) __declspec(novtable)
		IRenderTarget : public IGraphDeviceChild
	{
	public:
		virtual PointInt GetSize() const = 0;
		virtual RectInt GetViewport() const = 0;
		virtual void Clear(const XMFLOAT4 *pColor = nullptr) = 0;

		virtual void SetDepth(IRenderDepth *pDepth) = 0;
		virtual IRenderDepth *GetDepth() = 0;

		virtual ID3D11Texture2D *GetTexture() = 0;
		virtual ID3D11RenderTargetView *GetTarget() = 0;
	};

	class __declspec(uuid("429e3eb7-2116-4a37-aa65-af17b30f1761")) __declspec(novtable)
		IRenderTargetWindow : public IRenderTarget
	{
	public:
		virtual PWND GetWindow() const = 0;
		virtual PWND GetTopWindow() const = 0;
		virtual bool IsHidden() = 0;

		virtual bool SetSize(PointInt size) = 0;
		virtual void SetAutoSize() = 0;

		virtual void Present(bool bVsync) = 0;
		virtual void WaitForVsync() const = 0;

		virtual bool CanSetFullScreen() const = 0;
		virtual bool IsFullScreen() const = 0;
		virtual bool SetFullScreen(bool bFullScreen) = 0;
	};

	class __declspec(uuid("907bf3da-2a06-4bac-a929-69868dabd9f2")) __declspec(novtable)
		IRenderDepth : public IGraphDeviceChild
	{
	public:
		virtual PointInt GetSize() const = 0;
		virtual bool SetSize(PointInt size) = 0;
		virtual void Clear(float *pDepth = nullptr, BYTE *pStencil = nullptr) = 0;
		virtual void ClearDepth(float *pDepth = nullptr) = 0;
		virtual void ClearStencil(BYTE *pStencil = nullptr) = 0;

		virtual ID3D11Texture2D *GetTexture() = 0;
		virtual ID3D11DepthStencilView *GetView() = 0;
	};

	UTIL_API bool CreateRenderTargetWindow(
		IGraphDevice *pDevice,
		PWND hwnd,
		bool bFullScreen,
		DXGI_FORMAT format,
		size_t nBackBuffers,
		size_t nMultiSamples,
		IRenderTargetWindow **ppRender);

	UTIL_API bool CreateRenderTargetTexture(
		IGraphDevice *pDevice,
		IGraphTexture *pTexture,
		size_t nArrayStart,
		size_t nArrayCount,
		size_t nMipLevel,
		IRenderTarget **ppRender);

	UTIL_API bool CreateRenderDepth(
		IGraphDevice *pDevice,
		PointInt size,
		DXGI_FORMAT format,
		size_t nMultiSamples,
		IRenderDepth **ppDepth);
}

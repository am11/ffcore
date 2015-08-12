#include "pch.h"
#include "Audio/AudioBuffer.h"
#include "Audio/AudioDevice.h"
#include "Audio/AudioEffect.h"
#include "Audio/AudioFactory.h"
#include "Globals/ProcessGlobals.h"
#include "Graph/2D/SpriteAnimation.h"
#include "Graph/2D/SpriteList.h"
#include "Graph/Data/GraphCategory.h"
#include "Graph/Font/SpriteFont.h"
#include "Graph/GraphDevice.h"
#include "Graph/GraphFactory.h"
#include "Graph/GraphTexture.h"
#include "Module/ModuleFactory.h"

// {1D2F1D6D-E72A-4EDC-AB5C-340BC3AEB594}
static const GUID CATID_GraphicObject = 
	{0x1d2f1d6d,0xe72a,0x4edc,{0xab,0x5c,0x34,0x0b,0xc3,0xae,0xb5,0x94}};

// {11EF83F2-6471-4A66-A25E-37ED609CAEDB}
static const GUID CATID_GraphicsResource = 
	{0x11ef83f2,0x6471,0x4a66,{0xa2,0x5e,0x37,0xed,0x60,0x9c,0xae,0xdb}};

// {EF0D1854-9D51-4558-B988-619D603C9B97}
static const GUID CATID_AudioObject = 
	{0xef0d1854,0x9d51,0x4558,{0xb9,0x88,0x61,0x9d,0x60,0x3c,0x9b,0x97}};

// {C74B3060-6355-4BDB-A0AF-D1C626FC85D2}
static const GUID CATID_AudioResource =
	{0xc74b3060,0x6355,0x4bdb,{0xa0,0xaf,0xd1,0xc6,0x26,0xfc,0x85,0xd2}};

REFGUID ff::GetCategoryGraphicsObject()
{
	return CATID_GraphicObject;
}

REFGUID ff::GetCategoryGraphicsResource()
{
	return CATID_GraphicsResource;
}

REFGUID ff::GetCategoryAudioObject()
{
	return CATID_AudioObject;
}

REFGUID ff::GetCategoryAudioResource()
{
	return CATID_AudioResource;
}

static HRESULT CreateDefaultGraphDevice(IUnknown *unkOuter, REFGUID clsid, REFGUID iid, void **ppObj)
{
	assertRetVal(!unkOuter && clsid == GUID_NULL && ppObj, E_INVALIDARG);

	ff::ComPtr<ff::IGraphDevice> pDevice;
	assertRetVal(ff::ProcessGlobals::Get()->GetGraphicFactory()->CreateDevice(nullptr, &pDevice), E_FAIL);

	return pDevice->QueryInterface(iid, ppObj);
}

static HRESULT CreateDefaultAudioDevice(IUnknown *unkOuter, REFGUID clsid, REFGUID iid, void **ppObj)
{
	assertRetVal(!unkOuter && clsid == GUID_NULL && ppObj, E_INVALIDARG);

	ff::ComPtr<ff::IAudioDevice> pDevice;
	assertRetVal(ff::ProcessGlobals::Get()->GetAudioFactory()->CreateDefaultDevice(&pDevice), E_FAIL);

	return pDevice->QueryInterface(iid, ppObj);
}

static ff::ModuleStartup Register([](ff::Module &module)
{
	static ff::StaticString name0(L"3D Device Class");
	static ff::StaticString name1(L"Audio Device Class");
	static ff::StaticString name2(L"Graphics Resource");
	static ff::StaticString name3(L"Audio Resource");

	module.RegisterCategory(name0, CATID_GraphicObject, CreateDefaultGraphDevice);
	module.RegisterCategory(name1, CATID_AudioObject, CreateDefaultAudioDevice);

	module.RegisterCategory(name2, CATID_GraphicsResource, CreateDefaultGraphDevice);
	module.RegisterCategory(name3, CATID_AudioResource, CreateDefaultAudioDevice);

	module.RegisterInterface(__uuidof(ff::IGraphTexture), CATID_GraphicsResource);
	module.RegisterInterface(__uuidof(ff::ISpriteList), CATID_GraphicsResource);
	module.RegisterInterface(__uuidof(ff::ISpriteAnimation), CATID_GraphicsResource);
	module.RegisterInterface(__uuidof(ff::ISpriteFont), CATID_GraphicsResource);

	module.RegisterInterface(__uuidof(ff::IAudioEffect), CATID_AudioResource);
	module.RegisterInterface(__uuidof(ff::IAudioBuffer), CATID_AudioResource);
});

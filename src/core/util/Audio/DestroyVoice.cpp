#include "pch.h"
#include "Audio/AudioDevice.h"
#include "Audio/DestroyVoice.h"
#include "Globals/ProcessGlobals.h"
#include "Graph/Data/GraphCategory.h"
#include "Module/ModuleFactory.h"

BEGIN_INTERFACES(ff::DestroyVoiceWorkItem)
	HAS_INTERFACE(ff::IWorkItem)
	HAS_INTERFACE(ff::IAudioDeviceChild)
END_INTERFACES()

// STATIC_DATA (object)
static ff::PoolAllocator<ff::ComObject<ff::DestroyVoiceWorkItem>> s_destroyVoiceAllocator;

static HRESULT CreateDestroyVoiceWorkItem(IUnknown *unkOuter, REFGUID clsid, REFGUID iid, void **ppObj)
{
	assertRetVal(clsid == GUID_NULL || clsid == __uuidof(ff::DestroyVoiceWorkItem), E_INVALIDARG);
	ff::ComPtr<ff::ComObject<ff::DestroyVoiceWorkItem>> pObj = s_destroyVoiceAllocator.New();
	assertRetVal(SUCCEEDED(pObj->_Construct(unkOuter)), E_FAIL);

	return pObj->QueryInterface(iid, ppObj);
}

bool ff::CreateDestroyVoiceWorkItem(IAudioDevice *device, IXAudio2SourceVoice *source, DestroyVoiceWorkItem **obj)
{
	assertRetVal(obj, false);

	ComPtr<DestroyVoiceWorkItem, IWorkItem> myObj;
	assertHrRetVal(::CreateDestroyVoiceWorkItem(device, GUID_NULL, __uuidof(DestroyVoiceWorkItem), (void **)&myObj), false);
	assertRetVal(myObj->Init(source), false);

	*obj = myObj.Detach();
	return true;
}

static ff::ModuleStartup RegisterWorkItem([](ff::Module &module)
{
	static ff::StaticString name(L"Destroy Source Voice Work Item");

	module.RegisterClass(
		name,
		__uuidof(ff::DestroyVoiceWorkItem),
		::CreateDestroyVoiceWorkItem,
		__uuidof(ff::DestroyVoiceWorkItem),
		ff::GetCategoryAudioObject());
});

ff::DestroyVoiceWorkItem::DestroyVoiceWorkItem()
	: _source(nullptr)
{
}

ff::DestroyVoiceWorkItem::~DestroyVoiceWorkItem()
{
	Reset();
}

HRESULT ff::DestroyVoiceWorkItem::_Construct(IUnknown *unkOuter)
{
	assertRetVal(_device.QueryFrom(unkOuter), E_INVALIDARG);

	return __super::_Construct(unkOuter);
}

void ff::DestroyVoiceWorkItem::_DeleteThis()
{
	s_destroyVoiceAllocator.Delete(static_cast<ComObject<DestroyVoiceWorkItem>*>(this));
}

bool ff::DestroyVoiceWorkItem::Init(IXAudio2SourceVoice *source)
{
	assertRetVal(_device && source, false);
	_source = source;
	return true;
}

// on helper thread
void ff::DestroyVoiceWorkItem::Run()
{
	if (_source)
	{
		_source->DestroyVoice();
		_source = nullptr;
	}
}

// on main thread
void ff::DestroyVoiceWorkItem::OnCancel()
{
	Run();
}

void ff::DestroyVoiceWorkItem::OnComplete()
{
}

ff::IAudioDevice *ff::DestroyVoiceWorkItem::GetDevice() const
{
	return _device;
}

void ff::DestroyVoiceWorkItem::Reset()
{
	if (_source)
	{
		if (!ff::ProcessGlobals::Get()->GetThreadPool()->Cancel(this))
		{
			// already started running, so wait
			ff::ProcessGlobals::Get()->GetThreadPool()->Wait(this);
		}

		assert(!_source);
	}
}

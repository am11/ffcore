#include "pch.h"
#include "App/Log.h"
#include "Dict/Dict.h"
#include "Globals/ProcessGlobals.h"
#include "String/StringUtil.h"

static const size_t MAX_SMALL_DICT = 128;

ff::StaticString ff::OPTION_APP_USE_DIRECT3D(L"App.UseDirect3d");
ff::StaticString ff::OPTION_APP_USE_JOYSTICKS(L"App.UseJoysticks");
ff::StaticString ff::OPTION_APP_USE_MAIN_WINDOW_KEYBOARD(L"App.UseMainWindowKeyboard");
ff::StaticString ff::OPTION_APP_USE_MAIN_WINDOW_MOUSE(L"App.UseMainWindowMouse");
ff::StaticString ff::OPTION_APP_USE_MAIN_WINDOW_TOUCH(L"App.UseMainWindowTouch");
ff::StaticString ff::OPTION_APP_USE_RENDER_2D(L"App.UseRender2d");
ff::StaticString ff::OPTION_APP_USE_RENDER_DEPTH(L"App.UseRenderDepth");
ff::StaticString ff::OPTION_APP_USE_RENDER_MAIN_WINDOW(L"App.UseRenderMainWindow");
ff::StaticString ff::OPTION_APP_USE_XAUDIO(L"App.UseXAudio");
ff::StaticString ff::OPTION_APP_USE_XINPUT(L"App.UseXInput");
ff::StaticString ff::OPTION_GAME_ADVANCES_PER_SECOND(L"Game.AdvancesPerSecond");
ff::StaticString ff::OPTION_GRAPH_FRAMES_PER_SECOND(L"Graph.FramesPerSecond");
ff::StaticString ff::OPTION_GRAPH_BACK_BUFFERS(L"Graph.BackBuffers");
ff::StaticString ff::OPTION_GRAPH_MULTI_SAMPLES(L"Graph.MultiSamples");
ff::StaticString ff::OPTION_GRAPH_SHOW_FPS_ON(L"Graph.ShowFpsOn");
ff::StaticString ff::OPTION_GRAPH_VSYNC_ON(L"Graph.VsyncOn");
ff::StaticString ff::OPTION_MRU_PATHS(L"Mru.Paths");
ff::StaticString ff::OPTION_MRU_NAMES(L"Mru.Names");
ff::StaticString ff::OPTION_MRU_PINNED(L"Mru.Pinned");
ff::StaticString ff::OPTION_SOUND_MASTER_ON(L"Sound.MasterOn");
ff::StaticString ff::OPTION_SOUND_MASTER_VOLUME(L"Sound.MasterVolume");
ff::StaticString ff::OPTION_SOUND_EFFECTS_VOLUME(L"Sound.EffectsVolume");
ff::StaticString ff::OPTION_SOUND_MUSIC_VOLUME(L"Sound.MusicVolume");
ff::StaticString ff::OPTION_WINDOW_DEFAULT_CLIENT_SIZE(L"Window.DefaultClientSize");
ff::StaticString ff::OPTION_WINDOW_FULL_SCREEN(L"Window.FullScreen");
ff::StaticString ff::OPTION_WINDOW_MAXIMIZED(L"Window.Maximized");
ff::StaticString ff::OPTION_WINDOW_PADDING(L"Window.Padding");
ff::StaticString ff::OPTION_WINDOW_POSITION(L"Window.Position");

ff::Dict::Dict(const Dict *parent)
	: _parent(parent)
	, _atomizer(ProcessGlobals::Get()->GetStringCache())
{
}

ff::Dict::Dict(const Dict &rhs)
	: _parent(nullptr)
	, _atomizer(rhs._atomizer)
{
	*this = rhs;
}

ff::Dict::Dict(Dict &&rhs)
	: _parent(rhs._parent)
	, _atomizer(rhs._atomizer)
	, _propsLarge(std::move(rhs._propsLarge))
	, _propsSmall(std::move(rhs._propsSmall))
{
	rhs._parent = nullptr;
}

ff::Dict::Dict(const SmallDict &rhs)
	: _parent(nullptr)
	, _atomizer(ProcessGlobals::Get()->GetStringCache())
{
	*this = rhs;
}

ff::Dict::Dict(SmallDict &&rhs)
	: _parent(nullptr)
	, _atomizer(ProcessGlobals::Get()->GetStringCache())
	, _propsSmall(std::move(rhs))
{
}

const ff::Dict &ff::Dict::operator=(const Dict &rhs)
{
	if (this != &rhs)
	{
		Clear();

		_parent = rhs._parent;
		_atomizer = rhs._atomizer;
		_propsSmall = rhs._propsSmall;
		_propsLarge.reset();
		
		if (rhs._propsLarge != nullptr)
		{
			_propsLarge.reset(new PropsMap(*rhs._propsLarge));
		}
	}

	return *this;
}

const ff::Dict &ff::Dict::operator=(const SmallDict &rhs)
{
	if (&_propsSmall != &rhs)
	{
		Clear();
		_propsSmall = rhs;
		_propsLarge.reset();
	}

	return *this;
}

ff::Dict::~Dict()
{
	Clear();
}

void ff::Dict::SetParent(const Dict *parent)
{
	for (const Dict *checkParent = parent; checkParent; checkParent = checkParent->GetParent())
	{
		if (checkParent == this)
		{
			assertSz(false, L"Recursive dict parents");
			parent = nullptr;
		}
	}

	_parent = parent;
}

const ff::Dict *ff::Dict::GetParent() const
{
	return _parent;
}

void ff::Dict::Clear()
{
	_propsSmall.Clear();
	_propsLarge.reset();
}

void ff::Dict::Add(const Dict &rhs, bool chain)
{
	Vector<String> names = rhs.GetAllNames(chain, false, false);

	for (const String &name: names)
	{
		Value *value = rhs.GetValue(name, chain);
		SetValue(name, value);
	}
}

void ff::Dict::Reserve(size_t count)
{
	if (_propsLarge == nullptr)
	{
		_propsSmall.Reserve(count, false);
	}
}

bool ff::Dict::IsEmpty(bool chain) const
{
	bool empty = true;

	if (_propsLarge != nullptr)
	{
		empty = _propsLarge->IsEmpty();
	}
	else if (_propsSmall.Size())
	{
		empty = false;
	}

	if (empty)
	{
		return !chain || !_parent || _parent->IsEmpty(true);
	}

	return false;
}

size_t ff::Dict::Size(bool chain) const
{
	size_t size = 0;

	if (_propsLarge != nullptr)
	{
		size = _propsLarge->Size();
	}
	else
	{
		size = _propsSmall.Size();
	}

	if (chain && _parent)
	{
		size += _parent->Size(true);
	}

	return size;
}

void ff::Dict::Add(const SmallDict &rhs)
{
	for (size_t i = 0; i < rhs.Size(); i++)
	{
		SetValue(rhs.KeyAt(i), rhs.ValueAt(i));
	}
}

void ff::Dict::SetValue(ff::StringRef name, ff::Value *value)
{
	if (value)
	{
		if (_propsLarge != nullptr)
		{
			hash_t hash = _atomizer->CacheString(name);
			_propsLarge->SetKey(hash, value);
		}
		else
		{
			_propsSmall.Set(name, value);
			CheckSize();
		}
	}
	else
	{
		if (_propsLarge != nullptr)
		{
			hash_t hash = _atomizer->CacheString(name);
			_propsLarge->DeleteKey(hash);
		}
		else
		{
			_propsSmall.Remove(name);
		}
	}
}

ff::Value *ff::Dict::GetValue(ff::StringRef name, bool chain) const
{
	Value *value = nullptr;

	if (_propsLarge != nullptr)
	{
		hash_t hash = _atomizer->GetHash(name);
		BucketIter iter = _propsLarge->Get(hash);

		if (iter != INVALID_ITER)
		{
			value = _propsLarge->ValueAt(iter);
		}
	}
	else
	{
		value = _propsSmall.GetValue(name);
	}

	if (!value && chain && _parent)
	{
		value = _parent->GetValue(name, chain);
	}

	return value;
}

void ff::Dict::SetInt(ff::StringRef name, int value)
{
	ValuePtr newValue;
	assertRet(Value::CreateInt(value, &newValue));

	SetValue(name, newValue);
}

void ff::Dict::SetBool(ff::StringRef name, bool value)
{
	ValuePtr newValue;
	assertRet(Value::CreateBool(value, &newValue));

	SetValue(name, newValue);
}

void ff::Dict::SetRect(ff::StringRef name, RectInt value)
{
	ValuePtr newValue;
	assertRet(Value::CreateRect(value, &newValue));

	SetValue(name, newValue);
}

void ff::Dict::SetRectF(ff::StringRef name, RectFloat value)
{
	ValuePtr newValue;
	assertRet(Value::CreateRectF(value, &newValue));

	SetValue(name, newValue);
}

void ff::Dict::SetPoint(ff::StringRef name, PointInt value)
{
	ValuePtr newValue;
	assertRet(Value::CreatePoint(value, &newValue));

	SetValue(name, newValue);
}

void ff::Dict::SetPointF(ff::StringRef name, PointFloat value)
{
	ValuePtr newValue;
	assertRet(Value::CreatePointF(value, &newValue));

	SetValue(name, newValue);
}

void ff::Dict::SetFloat(ff::StringRef name, float value)
{
	ValuePtr newValue;
	assertRet(Value::CreateFloat(value, &newValue));

	SetValue(name, newValue);
}

void ff::Dict::SetDouble(ff::StringRef name, double value)
{
	ValuePtr newValue;
	assertRet(Value::CreateDouble(value, &newValue));

	SetValue(name, newValue);
}

void ff::Dict::SetString(ff::StringRef name, String value)
{
	ValuePtr newValue;
	assertRet(Value::CreateString(value, &newValue));

	SetValue(name, newValue);
}

void ff::Dict::SetGuid(ff::StringRef name, REFGUID value)
{
	ValuePtr newValue;
	assertRet(Value::CreateGuid(value, &newValue));

	SetValue(name, newValue);
}

void ff::Dict::SetData(ff::StringRef name, IData* value)
{
	ValuePtr newValue;
	assertRet(Value::CreateData(value, &newValue));

	SetValue(name, newValue);
}

int ff::Dict::GetInt(ff::StringRef name, int defaultValue, bool chain) const
{
	Value *value = GetValue(name, chain);

	if (value)
	{
		if (value->IsType(Value::Type::Int))
		{
			return value->AsInt();
		}
		else
		{
			ValuePtr newValue;

			if (value->Convert(Value::Type::Int, &newValue))
			{
				return newValue->AsInt();
			}
		}
	}

	return defaultValue;
}

bool ff::Dict::GetBool(ff::StringRef name, bool defaultValue, bool chain) const
{
	Value *value = GetValue(name, chain);

	if (value)
	{
		if (value->IsType(Value::Type::Bool))
		{
			return value->AsBool();
		}
		else
		{
			ValuePtr newValue;

			if (value->Convert(Value::Type::Bool, &newValue))
			{
				return newValue->AsBool();
			}
		}
	}

	return defaultValue;
}

ff::RectInt ff::Dict::GetRect(ff::StringRef name, RectInt defaultValue, bool chain) const
{
	Value *value = GetValue(name, chain);

	if (value && value->IsType(Value::Type::Rect))
	{
		return value->AsRect();
	}

	return defaultValue;
}

ff::RectFloat ff::Dict::GetRectF(ff::StringRef name, RectFloat defaultValue, bool chain) const
{
	Value *value = GetValue(name, chain);

	if (value && value->IsType(Value::Type::RectF))
	{
		return value->AsRectF();
	}

	return defaultValue;
}

float ff::Dict::GetFloat(ff::StringRef name, float defaultValue, bool chain) const
{
	Value *value = GetValue(name, chain);

	if (value)
	{
		if (value->IsType(Value::Type::Float))
		{
			return value->AsFloat();
		}
		else
		{
			ValuePtr newValue;

			if (value->Convert(Value::Type::Float, &newValue))
			{
				return newValue->AsFloat();
			}
		}
	}

	return defaultValue;
}

double ff::Dict::GetDouble(ff::StringRef name, double defaultValue, bool chain) const
{
	Value *value = GetValue(name, chain);

	if (value)
	{
		if (value->IsType(Value::Type::Double))
		{
			return value->AsDouble();
		}
		else
		{
			ValuePtr newValue;

			if (value->Convert(Value::Type::Double, &newValue))
			{
				return newValue->AsDouble();
			}
		}
	}

	return defaultValue;
}

ff::PointInt ff::Dict::GetPoint(ff::StringRef name, PointInt defaultValue, bool chain) const
{
	Value *value = GetValue(name, chain);

	if (value && value->IsType(Value::Type::Point))
	{
		return value->AsPoint();
	}

	return defaultValue;
}

ff::PointFloat ff::Dict::GetPointF(ff::StringRef name, PointFloat defaultValue, bool chain) const
{
	Value *value = GetValue(name, chain);

	if (value && value->IsType(Value::Type::PointF))
	{
		return value->AsPointF();
	}

	return defaultValue;
}

ff::String ff::Dict::GetString(ff::StringRef name, String defaultValue, bool chain) const
{
	Value *value = GetValue(name, chain);

	if (value)
	{
		if (value->IsType(Value::Type::String))
		{
			return value->AsString();
		}
		else
		{
			ValuePtr newValue;

			if (value->Convert(Value::Type::String, &newValue))
			{
				return newValue->AsString();
			}
		}
	}

	return defaultValue;
}

GUID ff::Dict::GetGuid(ff::StringRef name, REFGUID defaultValue, bool chain) const
{
	Value *value = GetValue(name, chain);

	if (value)
	{
		if (value->IsType(Value::Type::Guid))
		{
			return value->AsGuid();
		}
		else
		{
			ValuePtr newValue;

			if (value->Convert(Value::Type::Guid, &newValue))
			{
				return newValue->AsGuid();
			}
		}
	}

	return defaultValue;
}

ff::IData *ff::Dict::GetData(ff::StringRef name, bool chain) const
{
	Value *value = GetValue(name, chain);

	if (value && value->IsType(Value::Type::Data))
	{
		return value->AsData();
	}

	return nullptr;
}

ff::Vector<ff::String> ff::Dict::GetAllNames(bool chain, bool sorted, bool nameHashOnly) const
{
	Set<String> nameSet;
	InternalGetAllNames(nameSet, chain, nameHashOnly);

	Vector<String> names;
	for (const String &name: nameSet)
	{
		names.Push(name);
	}

	if (sorted && names.Size())
	{
		std::sort(names.begin(), names.end());
	}

	return names;
}

void ff::Dict::InternalGetAllNames(Set<String> &names, bool chain, bool nameHashOnly) const
{
	StringCache emptyCache;

	if (chain && _parent)
	{
		_parent->InternalGetAllNames(names, chain, nameHashOnly);
	}

	if (_propsLarge != nullptr)
	{
		for (const auto &iter: *_propsLarge)
		{
			hash_t hash = iter.GetKey();
			String name = nameHashOnly ? emptyCache.GetString(hash) : _atomizer->GetString(hash);
			names.SetKey(name);
		}
	}
	else
	{
		for (size_t i = 0; i < _propsSmall.Size(); i++)
		{
			String name = nameHashOnly ? emptyCache.GetString(_propsSmall.KeyHashAt(i)) : _propsSmall.KeyAt(i);
			names.SetKey(name);
		}
	}
}

void ff::Dict::DebugDump() const
{
#ifdef _DEBUG
	if (_parent)
	{
		_parent->DebugDump();

		Log::DebugTrace(L"--------\n");
	}

	Vector<String> names = GetAllNames(false, true, false);
	for (const String &name: names)
	{
		String value = GetString(name, String(), false);

		Log::DebugTraceF(L"%s=%s\n", name.c_str(), value.c_str());
	}
#endif
}

void ff::Dict::CheckSize()
{
	if (_propsSmall.Size() > MAX_SMALL_DICT)
	{
		_propsLarge.reset(new PropsMap());
		Add(_propsSmall);
		_propsSmall.Clear();
	}
}

#include "pch.h"
#include "Data/Data.h"
#include "Dict/Dict.h"
#include "Dict/Value.h"
#include "String/StringUtil.h"

struct FakeVector { size_t fake[4]; };
static ff::PoolAllocator<FakeVector> s_fakeVectorPool;
static ff::PoolAllocator<ff::Value> s_valuePool;

class ScopeStaticValueAlloc
{
public:
	ScopeStaticValueAlloc() : _lock(ff::GCS_VALUE) { }

private:
	ff::ScopeStaticMemAlloc _staticAlloc;
	ff::LockMutex _lock;
};

template<typename T>
static T *NewFakeVector()
{
	assert(sizeof(FakeVector) >= sizeof(T));

	FakeVector *pFake = s_fakeVectorPool.New();
	T *pVector = ::new((void*)pFake) T;

	return pVector;
}

template<typename T>
static T *NewFakeVector(T &&value)
{
	assert(sizeof(FakeVector) >= sizeof(T));

	FakeVector *pFake = s_fakeVectorPool.New();
	T *pVector = ::new((void*)pFake) T(std::move(value));

	return pVector;
}

template<typename T>
static void DeleteFakeVector(T *pVector)
{
	if (pVector)
	{
		pVector->~T();

		FakeVector *pFake = (FakeVector*)pVector;
		s_fakeVectorPool.Delete(pFake);
	}
}

ff::Value::Value()
	: _refCount(-1)
{
	SetType(Type::Null);
}

ff::Value::~Value()
{
	assert(_refCount == -1 || !_refCount);

	switch (GetType())
	{
		case Type::Object:
			ff::ReleaseRef(_object);
			break;

		case Type::String:
			if (_string.str)
			{
				InternalGetString().~String();
			}
			break;

		case Type::Dict:
			_dict.AsDict()->~Dict();
			break;

		case Type::Data:
			ff::ReleaseRef(_data);
			break;

		case Type::StringVector:
			DeleteFakeVector(_stringVector);
			break;

		case Type::ValueVector:
			DeleteFakeVector(_valueVector);
			break;

		case Type::IntVector:
			DeleteFakeVector(_intVector);
			break;

		case Type::DoubleVector:
			DeleteFakeVector(_doubleVector);
			break;

		case Type::FloatVector:
			DeleteFakeVector(_floatVector);
			break;

		case Type::DataVector:
			DeleteFakeVector(_dataVector);
			break;
	}
}

ff::Value *ff::Value::NewValueOneRef()
{
	Value *pVal = s_valuePool.New();
	pVal->_refCount = 1;

	return pVal;
}

void ff::Value::DeleteValue(Value *val)
{
	s_valuePool.Delete(val);
}

void ff::Value::AddRef()
{
	if (_refCount != -1)
	{
		InterlockedIncrement(&_refCount);
	}
}

void ff::Value::Release()
{
	if (_refCount != -1 && !InterlockedDecrement(&_refCount))
	{
		DeleteValue(this);
	}
}

ff::Value::Type ff::Value::GetType() const
{
	return (Value::Type)(_type & 0xFF);
}

bool ff::Value::IsType(Type type) const
{
	return GetType() == type;
}

// STATIC_DATA(pod)
static DWORD s_curExtendedType = 0;

// static
DWORD ff::Value::CreateExtendedType()
{
	LockMutex crit(GCS_VALUE);

	return ++s_curExtendedType;
}

DWORD ff::Value::GetExtendedType() const
{
	return _type >> 8;
}

void ff::Value::SetExtendedType(DWORD type)
{
	assert(type >= 0 && type <= s_curExtendedType && !(type & 0xFF000000));
	_type = (type << 8) | _type;
}

bool ff::Value::CreateNull(Value **ppValue)
{
	assertRetVal(ppValue, false);

	static Value::StaticValue s_data;
	static Value *s_val = nullptr;

	if (!s_val)
	{
		ScopeStaticValueAlloc scopeAlloc;

		if (!s_val)
		{
			s_val = s_data.AsValue();
			s_val->Value::Value();
			s_val->SetType(Type::Null);
		}
	}

	*ppValue = s_val;

	return true;
}

bool ff::Value::CreateBool(bool val, Value **ppValue)
{
	assertRetVal(ppValue, false);

	static Value::StaticValue s_data[2];
	static bool s_init = false;

	if (!s_init)
	{
		ScopeStaticValueAlloc scopeAlloc;

		if (!s_init)
		{
			Value *pVal = s_data[0].AsValue();
			pVal->Value::Value();
			pVal->SetType(Type::Bool);
			pVal->_bool = false;

			pVal = s_data[1].AsValue();
			pVal->Value::Value();
			pVal->SetType(Type::Bool);
			pVal->_bool = true;

			s_init = true;
		}
	}

	*ppValue = s_data[val ? 1 : 0].AsValue();

	return true;
}

bool ff::Value::CreateDouble(double val, Value **ppValue)
{
	assertRetVal(ppValue, false);

	if (val)
	{
		*ppValue = NewValueOneRef();
		(*ppValue)->SetType(Type::Double);
		(*ppValue)->_double = val;
	}
	else
	{
		static Value::StaticValue s_data;
		static Value *s_val = nullptr;

		if (!s_val)
		{
			ScopeStaticValueAlloc scopeAlloc;

			if (!s_val)
			{
				s_val = s_data.AsValue();
				s_val->Value::Value();
				s_val->SetType(Type::Double);
				s_val->_double = 0;
			}
		}

		*ppValue = s_val;
	}

	return true;
}

bool ff::Value::CreateFloat(float val, Value **ppValue)
{
	assertRetVal(ppValue, false);

	if (val)
	{
		*ppValue = NewValueOneRef();
		(*ppValue)->SetType(Type::Float);
		(*ppValue)->_float = val;
	}
	else
	{
		static Value::StaticValue s_data;
		static Value *s_val = nullptr;

		if (!s_val)
		{
			ScopeStaticValueAlloc scopeAlloc;

			if (!s_val)
			{
				s_val = s_data.AsValue();
				s_val->Value::Value();
				s_val->SetType(Type::Float);
				s_val->_float = 0;
			}
		}

		*ppValue = s_val;
	}

	return true;
}

bool ff::Value::CreateInt(int val, Value **ppValue)
{
	assertRetVal(ppValue, false);

	if (val < -100 || val > 200)
	{
		*ppValue = NewValueOneRef();
		(*ppValue)->SetType(Type::Int);
		(*ppValue)->_int  = val;
	}
	else
	{
		static Value::StaticValue s_data[301];
		static bool s_init = false;

		if (!s_init)
		{
			ScopeStaticValueAlloc scopeAlloc;

			if (!s_init)
			{
				for (int i = 0; i < 301; i++)
				{
					Value *pVal = s_data[i].AsValue();
					pVal->Value::Value();
					pVal->SetType(Type::Int);
					pVal->_int  = i - 100;
				}

				s_init = true;
			}
		}

		*ppValue = s_data[val + 100].AsValue();
	}

	return true;
}

bool ff::Value::CreatePointer(void *val, Value **ppValue)
{
	assertRetVal(ppValue, false);

	if (val)
	{
		*ppValue = NewValueOneRef();
		(*ppValue)->SetType(Type::Pointer);
		(*ppValue)->_pointer = val;
	}
	else
	{
		static Value::StaticValue s_data;
		static Value *s_val = nullptr;

		if (!s_val)
		{
			ScopeStaticValueAlloc scopeAlloc;

			if (!s_val)
			{
				s_val = s_data.AsValue();
				s_val->Value::Value();
				s_val->SetType(Type::Pointer);
				s_val->_pointer = nullptr;
			}
		}

		*ppValue = s_val;
	}

	return true;
}

bool ff::Value::CreateObject(IUnknown *pObj, Value **ppValue)
{
	assertRetVal(ppValue, false);

	if (pObj)
	{
		*ppValue = NewValueOneRef();
		(*ppValue)->SetType(Type::Object);
		(*ppValue)->_object = GetAddRef(pObj);
	}
	else
	{
		static Value::StaticValue s_data;
		static Value *s_val = nullptr;

		if (!s_val)
		{
			ScopeStaticValueAlloc scopeAlloc;

			if (!s_val)
			{
				s_val = s_data.AsValue();
				s_val->Value::Value();
				s_val->SetType(Type::Object);
				s_val->_object = nullptr;
			}
		}

		*ppValue = s_val;
	}

	return true;
}

bool ff::Value::CreatePoint(const PointInt &point, Value **ppValue)
{
	assertRetVal(ppValue, false);

	if (point.x || point.y)
	{
		*ppValue = NewValueOneRef();
		(*ppValue)->SetType(Type::Point);
		(*ppValue)->InternalGetPoint() = point;
	}
	else
	{
		static Value::StaticValue s_data;
		static Value *s_val = nullptr;

		if (!s_val)
		{
			ScopeStaticValueAlloc scopeAlloc;

			if (!s_val)
			{
				s_val = s_data.AsValue();
				s_val->Value::Value();
				s_val->SetType(Type::Point);
				s_val->InternalGetPoint().SetPoint(0, 0);
			}
		}

		*ppValue = s_val;
	}

	return true;
}

bool ff::Value::CreatePointF(const PointFloat &point, Value **ppValue)
{
	assertRetVal(ppValue, false);

	if (point.x || point.y)
	{
		*ppValue = NewValueOneRef();
		(*ppValue)->SetType(Type::PointF);
		(*ppValue)->InternalGetPointF() = point;
	}
	else
	{
		static Value::StaticValue s_data;
		static Value *s_val = nullptr;

		if (!s_val)
		{
			ScopeStaticValueAlloc scopeAlloc;

			if (!s_val)
			{
				s_val = s_data.AsValue();
				s_val->Value::Value();
				s_val->SetType(Type::PointF);
				s_val->InternalGetPointF().SetPoint(0, 0);
			}
		}

		*ppValue = s_val;
	}

	return true;
}


bool ff::Value::CreateRect(const RectInt &rect, Value **ppValue)
{
	assertRetVal(ppValue, false);

	if (!rect.IsNull())
	{
		*ppValue = NewValueOneRef();
		(*ppValue)->SetType(Type::Rect);
		(*ppValue)->InternalGetRect() = rect;
	}
	else
	{
		static Value::StaticValue s_data;
		static Value *s_val = nullptr;

		if (!s_val)
		{
			ScopeStaticValueAlloc scopeAlloc;

			if (!s_val)
			{
				s_val = s_data.AsValue();
				s_val->Value::Value();
				s_val->SetType(Type::Rect);
				s_val->InternalGetRect().SetRect(0, 0, 0, 0);
			}
		}

		*ppValue = s_val;
	}

	return true;
}

bool ff::Value::CreateRectF(const RectFloat &rect, Value **ppValue)
{
	assertRetVal(ppValue, false);

	if (!rect.IsNull())
	{
		*ppValue = NewValueOneRef();
		(*ppValue)->SetType(Type::RectF);
		(*ppValue)->InternalGetRectF() = rect;
	}
	else
	{
		static Value::StaticValue s_data;
		static Value *s_val = nullptr;

		if (!s_val)
		{
			ScopeStaticValueAlloc scopeAlloc;

			if (!s_val)
			{
				s_val = s_data.AsValue();
				s_val->Value::Value();
				s_val->SetType(Type::RectF);
				s_val->InternalGetRectF().SetRect(0, 0, 0, 0);
			}
		}

		*ppValue = s_val;
	}

	return true;
}

bool ff::Value::CreateString(StringRef str, Value **ppValue)
{
	assertRetVal(ppValue, false);

	if (!str.length())
	{
		static Value::StaticValue s_data;
		static Value *s_val = nullptr;

		if (!s_val)
		{
			ScopeStaticValueAlloc scopeAlloc;

			if (!s_val)
			{
				s_val = s_data.AsValue();
				s_val->Value::Value();
				s_val->SetType(Type::String);
				s_val->InternalGetString().String::String();
			}
		}

		*ppValue = s_val;
	}
	else
	{
		*ppValue = NewValueOneRef();
		(*ppValue)->SetType(Type::String);
		(*ppValue)->InternalGetString().String::String(str);
	}

	return true;
}

bool ff::Value::CreateStringVector(Value **ppValue)
{
	assertRetVal(ppValue, false);

	*ppValue = NewValueOneRef();
	(*ppValue)->SetType(Type::StringVector);
	(*ppValue)->_stringVector = NewFakeVector<Vector<String>>();

	return true;
}

bool ff::Value::CreateValueVector(Value **ppValue)
{
	assertRetVal(ppValue, false);

	*ppValue = NewValueOneRef();
	(*ppValue)->SetType(Type::ValueVector);
	(*ppValue)->_valueVector = NewFakeVector<Vector<ValuePtr>>();

	return true;
}

bool ff::Value::CreateValueVector(Vector<ValuePtr> &&vec, Value **ppValue)
{
	assertRetVal(ppValue, false);

	*ppValue = NewValueOneRef();
	(*ppValue)->SetType(Type::ValueVector);
	(*ppValue)->_valueVector = NewFakeVector<Vector<ValuePtr>>(std::move(vec));

	return true;
}

bool ff::Value::CreateGuid(REFGUID guid, Value **ppValue)
{
	assertRetVal(ppValue, false);

	if (guid != GUID_NULL)
	{
		*ppValue = NewValueOneRef();
		(*ppValue)->SetType(Type::Guid);
		(*ppValue)->_guid = guid;
	}
	else
	{
		static Value::StaticValue s_data;
		static Value *s_val = nullptr;

		if (!s_val)
		{
			ScopeStaticValueAlloc scopeAlloc;

			if (!s_val)
			{
				s_val = s_data.AsValue();
				s_val->Value::Value();
				s_val->SetType(Type::Guid);
				s_val->_guid = GUID_NULL;
			}
		}

		*ppValue = s_val;
	}

	return true;
}

bool ff::Value::CreateIntVector(Value **ppValue)
{
	assertRetVal(ppValue, false);

	*ppValue = NewValueOneRef();
	(*ppValue)->SetType(Type::IntVector);
	(*ppValue)->_intVector = NewFakeVector<Vector<int>>();

	return true;
}

bool ff::Value::CreateDoubleVector(Value **ppValue)
{
	assertRetVal(ppValue, false);

	*ppValue = NewValueOneRef();
	(*ppValue)->SetType(Type::DoubleVector);
	(*ppValue)->_doubleVector = NewFakeVector<Vector<double>>();

	return true;
}

bool ff::Value::CreateFloatVector(Value **ppValue)
{
	assertRetVal(ppValue, false);

	*ppValue = NewValueOneRef();
	(*ppValue)->SetType(Type::FloatVector);
	(*ppValue)->_floatVector = NewFakeVector<Vector<float>>();

	return true;
}

bool ff::Value::CreateData(IData *pData, Value **ppValue)
{
	assertRetVal(ppValue, false);

	*ppValue = NewValueOneRef();
	(*ppValue)->SetType(Type::Data);
	(*ppValue)->_data = GetAddRef(pData);

	return true;
}

bool ff::Value::CreateDataVector(Value **ppValue)
{
	assertRetVal(ppValue, false);

	*ppValue = NewValueOneRef();
	(*ppValue)->SetType(Type::DataVector);
	(*ppValue)->_dataVector = NewFakeVector<Vector<ComPtr<IData>>>();

	return true;
}

bool ff::Value::CreateDict(Value **ppValue)
{
	assertRetVal(ppValue, false);

	*ppValue = NewValueOneRef();
	(*ppValue)->SetType(Type::Dict);
	::new((*ppValue)->_dict.AsDict()) ff::Dict();

	return true;
}

bool ff::Value::CreateDict(ff::Dict &&dict, Value **ppValue)
{
	assertRetVal(ppValue, false);

	*ppValue = NewValueOneRef();
	(*ppValue)->SetType(Type::Dict);
	::new((*ppValue)->_dict.AsDict()) ff::Dict(std::move(dict));

	return true;
}

bool ff::Value::AsBool() const
{
	assert(IsType(Type::Bool));
	return _bool;
}

double ff::Value::AsDouble() const
{
	assert(IsType(Type::Double));
	return _double;
}

float ff::Value::AsFloat() const
{
	assert(IsType(Type::Float));
	return _float;
}

int ff::Value::AsInt() const
{
	assert(IsType(Type::Int));
	return _int;
}

void *ff::Value::AsPointer() const
{
	assert(IsType(Type::Pointer));
	return _pointer;
}

IUnknown *ff::Value::AsObject() const
{
	assert(IsType(Type::Object));
	return _object;
}

const ff::PointInt &ff::Value::AsPoint() const
{
	assert(IsType(Type::Point));
	return InternalGetPoint();
}

const ff::PointFloat &ff::Value::AsPointF() const
{
	assert(IsType(Type::PointF));
	return InternalGetPointF();
}

const ff::RectInt &ff::Value::AsRect() const
{
	assert(IsType(Type::Rect));
	return InternalGetRect();
}

const ff::RectFloat &ff::Value::AsRectF() const
{
	assert(IsType(Type::RectF));
	return InternalGetRectF();
}

ff::Vector<ff::String> &ff::Value::AsStringVector() const
{
	assert(IsType(Type::StringVector));
	return *_stringVector;
}

ff::Vector<ff::ValuePtr> &ff::Value::AsValueVector() const
{
	assert(IsType(Type::ValueVector));
	return *_valueVector;
}

REFGUID ff::Value::AsGuid() const
{
	assert(IsType(Type::Guid));
	return _guid;
}

ff::Vector<int> &ff::Value::AsIntVector() const
{
	assert(IsType(Type::IntVector));
	return *_intVector;
}

ff::Vector<double> &ff::Value::AsDoubleVector() const
{
	assert(IsType(Type::DoubleVector));
	return *_doubleVector;
}

ff::Vector<float> &ff::Value::AsFloatVector() const
{
	assert(IsType(Type::FloatVector));
	return *_floatVector;
}

ff::IData *ff::Value::AsData() const
{
	assert(IsType(Type::Data));
	return _data;
}

ff::Vector<ff::ComPtr<ff::IData>> &ff::Value::AsDataVector() const
{
	assert(IsType(Type::DataVector));
	return *_dataVector;
}

void ff::Value::SetType(Type type)
{
	_type = (DWORD)type;
}

ff::PointInt &ff::Value::InternalGetPoint() const
{
	assert(sizeof(_point) >= sizeof(PointInt));
	return *(PointInt*)&_point;
}

ff::PointFloat &ff::Value::InternalGetPointF() const
{
	assert(sizeof(_pointF) >= sizeof(PointFloat));
	return *(PointFloat*)&_pointF;
}

ff::RectInt &ff::Value::InternalGetRect() const
{
	assert(sizeof(_rect) >= sizeof(RectInt));
	return *(RectInt*)&_rect;
}

ff::RectFloat &ff::Value::InternalGetRectF() const
{
	assert(sizeof(_rectF) >= sizeof(RectFloat));
	return *(RectFloat*)&_rectF;
}

ff::String &ff::Value::InternalGetString() const
{
	assert(sizeof(_string.str) == sizeof(String));
	return *(String*)&_string.str;
}

ff::Dict *ff::Value::SDict::AsDict() const
{
	assert(sizeof(data) >= sizeof(ff::Dict));
	return (ff::Dict*)&data;
}

ff::Value *ff::Value::StaticValue::AsValue()
{
	assert(sizeof(data) >= sizeof(Value));
	return (Value*)&data;
}

ff::StringRef ff::Value::AsString() const
{
	assert(IsType(Type::String));
	return InternalGetString();
}

ff::Dict &ff::Value::AsDict() const
{
	assert(IsType(Type::Dict));
	return *_dict.AsDict();
}

bool ff::Value::Convert(Type type, Value **ppValue)
{
	assertRetVal(ppValue, false);
	*ppValue = nullptr;

	if (type == GetType())
	{
		*ppValue = GetAddRef(this);
		return true;
	}

	wchar_t buf[256];

	switch (GetType())
	{
	case Type::Null:
		switch (type)
		{
		case Type::String:
			CreateString(String(L"null"), ppValue);
			break;
		}
		break;

	case Type::Bool:
		switch (type)
		{
		case Type::Double:
			CreateDouble(AsBool() ? 1.0 : 0.0, ppValue);
			break;

		case Type::Float:
			CreateFloat(AsBool() ? 1.0f : 0.0f, ppValue);
			break;

		case Type::Int:
			CreateInt(AsBool() ? 1 : 0, ppValue);
			break;

		case Type::String:
			CreateString(AsBool() ? String(L"true") : String(L"false"), ppValue);
			break;
		}
		break;

	case Type::Double:
		switch (type)
		{
		case Type::Bool:
			CreateBool(AsDouble() != 0, ppValue);
			break;

		case Type::Float:
			CreateFloat((float)AsDouble(), ppValue);
			break;

		case Type::Int:
			CreateInt((int)AsDouble(), ppValue);
			break;

		case Type::String:
			_snwprintf_s(buf, _countof(buf), _TRUNCATE, L"%g", AsDouble());
			CreateString(String(buf), ppValue);
			break;
		}
		break;

	case Type::Float:
		switch (type)
		{
		case Type::Bool:
			CreateBool(AsFloat() != 0, ppValue);
			break;

		case Type::Double:
			CreateDouble((double)AsFloat(), ppValue);
			break;

		case Type::Int:
			CreateInt((int)AsFloat(), ppValue);
			break;

		case Type::String:
			_snwprintf_s(buf, _countof(buf), _TRUNCATE, L"%g", AsFloat());
			CreateString(String(buf), ppValue);
			break;
		}
		break;

	case Type::Int:
		switch (type)
		{
		case Type::Bool:
			CreateBool(AsInt() != 0, ppValue);
			break;

		case Type::Double:
			CreateDouble((double)AsInt(), ppValue);
			break;

		case Type::Float:
			CreateFloat((float)AsInt(), ppValue);
			break;

		case Type::String:
			_snwprintf_s(buf, _countof(buf), _TRUNCATE, L"%d", AsInt());
			CreateString(String(buf), ppValue);
			break;
		}
		break;

	case Type::Point:
		switch (type)
		{
		case Type::String:
			_snwprintf_s(buf, _countof(buf), _TRUNCATE, L"(%d,%d)", AsPoint().x, AsPoint().y);
			CreateString(String(buf), ppValue);
			break;
		}
		break;

	case Type::PointF:
		switch (type)
		{
		case Type::String:
			_snwprintf_s(buf, _countof(buf), _TRUNCATE, L"(%g,%g)", AsPointF().x, AsPointF().y);
			CreateString(String(buf), ppValue);
			break;
		}
		break;

	case Type::Rect:
		switch (type)
		{
		case Type::String:
			_snwprintf_s(buf, _countof(buf), _TRUNCATE, L"(%d,%d,%d,%d)",
				AsRect().left, AsRect().top, AsRect().right, AsRect().bottom);
			CreateString(String(buf), ppValue);
			break;
		}
		break;

	case Type::RectF:
		switch (type)
		{
		case Type::String:
			_snwprintf_s(buf, _countof(buf), _TRUNCATE, L"(%g,%g,%g,%g)",
				AsRectF().left, AsRectF().top, AsRectF().right, AsRectF().bottom);
			CreateString(String(buf), ppValue);
			break;
		}
		break;

	case Type::String:
		switch (type)
		{
		case Type::Bool:
			if (AsString().empty() ||
				!_wcsicmp(AsString().c_str(), L"false") ||
				!_wcsicmp(AsString().c_str(), L"no") ||
				!_wcsicmp(AsString().c_str(), L"0"))
			{
				CreateBool(false, ppValue);
			}
			else if (!_wcsicmp(AsString().c_str(), L"true") ||
				!_wcsicmp(AsString().c_str(), L"yes") ||
				!_wcsicmp(AsString().c_str(), L"1"))
			{
				CreateBool(true, ppValue);
			}
			break;

		case Type::Double:
			{
				const wchar_t *start = AsString().c_str();
				wchar_t *end = nullptr;
				double val = wcstod(start, &end);

				if (end > start && !*end)
				{
					CreateDouble(val, ppValue);
				}
			} break;

		case Type::Float:
			{
				const wchar_t *start = AsString().c_str();
				wchar_t *end = nullptr;
				double val = wcstod(start, &end);

				if (end > start && !*end)
				{
					CreateFloat((float)val, ppValue);
				}
			} break;

		case Type::Int:
			{
				const wchar_t *start = AsString().c_str();
				wchar_t *end = nullptr;
				long val = wcstol(start, &end, 10);

				if (end > start && !*end)
				{
					CreateInt((int)val, ppValue);
				}
			} break;

		case Type::Guid:
			{
				GUID guid;
				if (StringToGuid(AsString(), guid))
				{
					CreateGuid(guid, ppValue);
				}
			} break;
		}
		break;

	case Type::Object:
		switch (type)
		{
		case Type::Bool:
			CreateBool(AsObject() != nullptr, ppValue);
			break;
		}
		break;

	case Type::Guid:
		switch (type)
		{
		case Type::String:
			CreateString(StringFromGuid(AsGuid()), ppValue);
			break;
		}
		break;
	}

	if (*ppValue)
	{
		return true;
	}

	return false;
}

bool ff::Value::operator==(const Value &r) const
{
	if (this == &r)
	{
		return true;
	}

	if (GetType() != r.GetType())
	{
		return false;
	}

	switch (GetType())
	{
		case Type::Null:
			return true;

		case Type::Bool:
			return AsBool() == r.AsBool();

		case Type::Double:
			return AsDouble() == r.AsDouble();

		case Type::Float:
			return AsFloat() == r.AsFloat();

		case Type::Int:
			return AsInt() == r.AsInt();

		case Type::Object:
			return AsObject() == r.AsObject();

		case Type::Point:
			return AsPoint() == r.AsPoint();

		case Type::PointF:
			return AsPointF() == r.AsPointF();

		case Type::Rect:
			return AsRect() == r.AsRect();

		case Type::RectF:
			return AsRectF() == r.AsRectF();

		case Type::String:
			return AsString() == r.AsString();

		case Type::StringVector:
			return AsStringVector() == r.AsStringVector();

		case Type::ValueVector:
			return AsValueVector() == r.AsValueVector();

		case Type::Guid:
			return AsGuid() == r.AsGuid() ? true : false;

		case Type::IntVector:
			return AsIntVector() == r.AsIntVector();

		case Type::DoubleVector:
			return AsDoubleVector() == r.AsDoubleVector();

		case Type::FloatVector:
			return AsFloatVector() == r.AsFloatVector();

		case Type::DataVector:
			return AsDataVector() == r.AsDataVector();

		case Type::Data:
			return AsData() == r.AsData();

		default:
			assert(false);
			return false;
	}
}

bool ff::Value::Compare(const Value *p) const
{
	return p ? (*this == *p) : false;
}

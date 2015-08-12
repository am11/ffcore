#include "pch.h"
#include "App/Log.h"
#include "Data/Data.h"
#include "Data/DataPersist.h"
#include "Data/DataWriterReader.h"
#include "Data/SavedData.h"
#include "Dict/Dict.h"
#include "Dict/DictPersist.h"
#include "Dict/Value.h"
#include "Module/Module.h"
#include "Module/ModuleFactory.h"
#include "String/StringCache.h"

static bool CanSaveValue(ff::Value *value)
{
	if (value)
	{
		switch (value->GetType())
		{
		case ff::Value::Type::Null:
		case ff::Value::Type::Bool:
		case ff::Value::Type::Double:
		case ff::Value::Type::Float:
		case ff::Value::Type::Int:
		case ff::Value::Type::Data:
		case ff::Value::Type::Dict:
		case ff::Value::Type::String:
		case ff::Value::Type::Guid:
		case ff::Value::Type::Point:
		case ff::Value::Type::Rect:
		case ff::Value::Type::PointF:
		case ff::Value::Type::RectF:
		case ff::Value::Type::DoubleVector:
		case ff::Value::Type::FloatVector:
		case ff::Value::Type::IntVector:
		case ff::Value::Type::DataVector:
		case ff::Value::Type::StringVector:
		case ff::Value::Type::ValueVector:
			return true;

		case ff::Value::Type::Object:
			// TODO
			return false;
		}
	}

	return false;
}

static bool InternalSaveDict(const ff::Dict &dict, bool chain, bool nameHashOnly, ff::IData **data);
static bool InternalLoadDict(ff::IDataReader *reader, ff::Dict &dict);

static bool InternalSaveValue(ff::Value *value, ff::IDataWriter *writer, bool nameHashOnly)
{
	assertRetVal(value && writer, false);

	DWORD type = (DWORD)value->GetType();
	if (!CanSaveValue(value))
	{
		type = (DWORD)ff::Value::Type::Null;
	}

	assertRetVal(ff::SaveData(writer, type), false);

	switch (value->GetType())
	{
	default:
		assertRetVal(false, false);

	case ff::Value::Type::Null:
		break;

	case ff::Value::Type::Bool:
		assertRetVal(ff::SaveData(writer, value->AsBool()), false);
		break;

	case ff::Value::Type::Double:
		assertRetVal(ff::SaveData(writer, value->AsDouble()), false);
		break;

	case ff::Value::Type::Float:
		assertRetVal(ff::SaveData(writer, value->AsFloat()), false);
		break;

	case ff::Value::Type::Int:
		assertRetVal(ff::SaveData(writer, value->AsInt()), false);
		break;

	case ff::Value::Type::Data:
		{
			ff::IData *valueData = value->AsData();
			DWORD valueDataSize = valueData ? (DWORD)valueData->GetSize() : 0;
			assertRetVal(ff::SaveData(writer, valueDataSize), false);

			if (valueDataSize)
			{
				assertRetVal(ff::SaveBytes(writer, valueData), false);
			}
		}
		break;

	case ff::Value::Type::Dict:
		{
			ff::ComPtr<ff::IData> nestedData;
			assertRetVal(InternalSaveDict(value->AsDict(), true, nameHashOnly, &nestedData), false);

			DWORD dataSize = (DWORD)nestedData->GetSize();
			assertRetVal(ff::SaveData(writer, dataSize), false);

			if (dataSize)
			{
				assertRetVal(SaveBytes(writer, nestedData), false);
			}
		}
		break;

	case ff::Value::Type::String:
		assertRetVal(ff::SaveData(writer, value->AsString()), false);
		break;

	case ff::Value::Type::Guid:
		assertRetVal(ff::SaveData(writer, value->AsGuid()), false);
		break;

	case ff::Value::Type::Point:
		assertRetVal(ff::SaveData(writer, value->AsPoint()), false);
		break;

	case ff::Value::Type::Rect:
		assertRetVal(ff::SaveData(writer, value->AsRect()), false);
		break;

	case ff::Value::Type::PointF:
		assertRetVal(ff::SaveData(writer, value->AsPointF()), false);
		break;

	case ff::Value::Type::RectF:
		assertRetVal(ff::SaveData(writer, value->AsRectF()), false);
		break;

	case ff::Value::Type::DoubleVector:
		{
			DWORD doubleCount = (DWORD)value->AsDoubleVector().Size();
			assertRetVal(ff::SaveData(writer, doubleCount), false);
				
			for (size_t h = 0; h < value->AsDoubleVector().Size(); h++)
			{
				assertRetVal(ff::SaveData(writer, value->AsDoubleVector().GetAt(h)), false);
			}
		}
		break;

	case ff::Value::Type::FloatVector:
		{
			DWORD floatCount = (DWORD)value->AsFloatVector().Size();
			assertRetVal(ff::SaveData(writer, floatCount), false);
				
			for (size_t h = 0; h < value->AsFloatVector().Size(); h++)
			{
				assertRetVal(ff::SaveData(writer, value->AsFloatVector().GetAt(h)), false);
			}
		}
		break;

	case ff::Value::Type::IntVector:
		{
			DWORD intCount = (DWORD)value->AsIntVector().Size();
			assertRetVal(ff::SaveData(writer, intCount), false);
				
			for (size_t h = 0; h < value->AsIntVector().Size(); h++)
			{
				assertRetVal(ff::SaveData(writer, value->AsIntVector().GetAt(h)), false);
			}
		}
		break;

	case ff::Value::Type::DataVector:
		{
			DWORD valueSize = (DWORD)value->AsDataVector().Size();
			assertRetVal(ff::SaveData(writer, valueSize), false);

			for (size_t h = 0; h < value->AsDataVector().Size(); h++)
			{
				ff::IData *valueData = value->AsDataVector().GetAt(h);
				valueSize = (DWORD)valueData->GetSize();
				assertRetVal(ff::SaveData(writer, valueSize), false);

				if (valueSize)
				{
					assertRetVal(SaveBytes(writer, valueData), false);
				}
			}
		}
		break;

	case ff::Value::Type::StringVector:
		{
			DWORD stringCount = (DWORD)value->AsStringVector().Size();
			assertRetVal(ff::SaveData(writer, stringCount), false);
				
			for (size_t h = 0; h < value->AsStringVector().Size(); h++)
			{
				assertRetVal(ff::SaveData(writer, value->AsStringVector().GetAt(h)), false);
			}
		}
		break;

	case ff::Value::Type::ValueVector:
		{
			DWORD valueCount = (DWORD)value->AsValueVector().Size();
			assertRetVal(ff::SaveData(writer, valueCount), false);

			for (const ff::ValuePtr &nestedValue: value->AsValueVector())
			{
				assertRetVal(InternalSaveValue(nestedValue, writer, nameHashOnly), false);
			}
		}
		break;

	case ff::Value::Type::Object:
		// TODO
		break;
	}

	return true;
}

static bool InternalLoadValue(ff::IDataReader *reader, ff::Value **value)
{
	assertRetVal(reader && value, false);

	DWORD type = 0;
	assertRetVal(ff::LoadData(reader, type), false);

	ff::Value::Type valueType = (ff::Value::Type)type;
	switch (valueType)
	{
	default:
		assertRetVal(false, false);

	case ff::Value::Type::Null:
		assertRetVal(ff::Value::CreateNull(value), false);
		break;

	case ff::Value::Type::Bool:
		{
			bool val = false;
			assertRetVal(ff::LoadData(reader, val), false);
			assertRetVal(ff::Value::CreateBool(val, value), false);
		}
		break;

	case ff::Value::Type::Double:
		{
			double val = 0;
			assertRetVal(ff::LoadData(reader, val), false);
			assertRetVal(ff::Value::CreateDouble(val, value), false);
		}
		break;

	case ff::Value::Type::Float:
		{
			float val = 0;
			assertRetVal(ff::LoadData(reader, val), false);
			assertRetVal(ff::Value::CreateFloat(val, value), false);
		}
		break;

	case ff::Value::Type::Int:
		{
			int val = 0;
			assertRetVal(ff::LoadData(reader, val), false);
			assertRetVal(ff::Value::CreateInt(val, value), false);
		}
		break;

	case ff::Value::Type::Data:
		{
			DWORD valueSize = 0;
			assertRetVal(ff::LoadData(reader, valueSize), false);

			ff::ComPtr<ff::IData> valueData;
			assertRetVal(ff::LoadBytes(reader, valueSize, &valueData), false);
			assertRetVal(ff::Value::CreateData(valueData, value), false);
		}
		break;

	case ff::Value::Type::Dict:
		{
			DWORD valueSize = 0;
			assertRetVal(ff::LoadData(reader, valueSize), false);

			ff::ComPtr<ff::IData> valueData;
			assertRetVal(LoadBytes(reader, valueSize, &valueData), false);

			ff::ComPtr<ff::IDataReader> valueReader;
			assertRetVal(CreateDataReader(valueData, 0, &valueReader), false);

			assertRetVal(ff::Value::CreateDict(value), false);
			assertRetVal(InternalLoadDict(valueReader, (*value)->AsDict()), false);
		}
		break;

	case ff::Value::Type::String:
		{
			ff::String val;
			assertRetVal(ff::LoadData(reader, val), false);
			assertRetVal(ff::Value::CreateString(val, value), false);
		}
		break;

	case ff::Value::Type::Guid:
		{
			GUID val = GUID_NULL;
			assertRetVal(ff::LoadData(reader, val), false);
			assertRetVal(ff::Value::CreateGuid(val, value), false);
		}
		break;

	case ff::Value::Type::Point:
		{
			ff::PointInt val(0, 0);
			assertRetVal(ff::LoadData(reader, val), false);
			assertRetVal(ff::Value::CreatePoint(val, value), false);
		}
		break;

	case ff::Value::Type::Rect:
		{
			ff::RectInt val(0, 0, 0, 0);
			assertRetVal(ff::LoadData(reader, val), false);
			assertRetVal(ff::Value::CreateRect(val, value), false);
		}
		break;

	case ff::Value::Type::PointF:
		{
			ff::PointFloat val(0, 0);
			assertRetVal(ff::LoadData(reader, val), false);
			assertRetVal(ff::Value::CreatePointF(val, value), false);
		}
		break;

	case ff::Value::Type::RectF:
		{
			ff::RectFloat val(0, 0, 0, 0);
			assertRetVal(ff::LoadData(reader, val), false);
			assertRetVal(ff::Value::CreateRectF(val, value), false);
		}
		break;

	case ff::Value::Type::DoubleVector:
		{
			DWORD doubleCount = 0;
			assertRetVal(ff::LoadData(reader, doubleCount), false);
			assertRetVal(ff::Value::CreateDoubleVector(value), false);
			(*value)->AsDoubleVector().Reserve(doubleCount);

			for (size_t h = 0; h < doubleCount; h++)
			{
				double val;
				assertRetVal(ff::LoadData(reader, val), false);

				(*value)->AsDoubleVector().Push(val);
			}
		}
		break;

	case ff::Value::Type::FloatVector:
		{
			DWORD floatCount = 0;
			assertRetVal(ff::LoadData(reader, floatCount), false);
			assertRetVal(ff::Value::CreateFloatVector(value), false);
			(*value)->AsFloatVector().Reserve(floatCount);

			for (size_t h = 0; h < floatCount; h++)
			{
				float val;
				assertRetVal(ff::LoadData(reader, val), false);

				(*value)->AsFloatVector().Push(val);
			}
		}
		break;

	case ff::Value::Type::IntVector:
		{
			DWORD intCount = 0;
			assertRetVal(ff::LoadData(reader, intCount), false);
			assertRetVal(ff::Value::CreateIntVector(value), false);
			(*value)->AsIntVector().Reserve(intCount);

			for (size_t h = 0; h < intCount; h++)
			{
				int val;
				assertRetVal(ff::LoadData(reader, val), false);

				(*value)->AsIntVector().Push(val);
			}
		}
		break;

	case ff::Value::Type::DataVector:
		{
			DWORD dataCount = 0;
			assertRetVal(ff::LoadData(reader, dataCount), false);
			assertRetVal(ff::Value::CreateDataVector(value), false);
			(*value)->AsDataVector().Reserve(dataCount);

			for (size_t h = 0; h < dataCount; h++)
			{
				DWORD dataSize = 0;
				assertRetVal(ff::LoadData(reader, dataSize), false);

				ff::ComPtr<ff::IData> valueData;
				assertRetVal(ff::LoadBytes(reader, dataSize, &valueData), false);

				(*value)->AsDataVector().Push(valueData);
			}
		}
		break;

	case ff::Value::Type::StringVector:
		{
			DWORD stringCount = 0;
			assertRetVal(ff::LoadData(reader, stringCount), false);
			assertRetVal(ff::Value::CreateStringVector(value), false);
			(*value)->AsStringVector().Reserve(stringCount);

			for (size_t h = 0; h < stringCount; h++)
			{
				ff::String val;
				assertRetVal(ff::LoadData(reader, val), false);

				(*value)->AsStringVector().Push(val);
			}
		}
		break;

	case ff::Value::Type::ValueVector:
		{
			DWORD valueCount = 0;
			assertRetVal(ff::LoadData(reader, valueCount), false);
			assertRetVal(ff::Value::CreateValueVector(value), false);
			(*value)->AsValueVector().Reserve(valueCount);

			for (size_t h = 0; h < valueCount; h++)
			{
				ff::ValuePtr nestedValue;
				assertRetVal(InternalLoadValue(reader, &nestedValue), false);
				(*value)->AsValueVector().Push(nestedValue);
			}
		}
		break;

	case ff::Value::Type::Object:
		// TODO
		break;
	}

	return true;
}

static bool InternalSaveDict(const ff::Dict &dict, bool chain, bool nameHashOnly, ff::IData **data)
{
	assertRetVal(data, false);
	*data = nullptr;

	ff::ComPtr<ff::IDataVector> dataVector;
	ff::ComPtr<ff::IDataWriter> writer;
	assertRetVal(CreateDataWriter(&dataVector, &writer), false);

	ff::Vector<ff::String> names = dict.GetAllNames(chain, false, nameHashOnly);
	DWORD count = (DWORD)names.Size();
	DWORD version = nameHashOnly ? 1 : 0;
	ff::StringCache emptyCache;

	assertRetVal(ff::SaveData(writer, version), false);
	assertRetVal(ff::SaveData(writer, count), false);

	for (const ff::String &name: names)
	{
		if (nameHashOnly)
		{
			ff::hash_t hash = emptyCache.GetHash(name);
			assertRetVal(ff::SaveData(writer, hash), false);
		}
		else
		{
			assertRetVal(ff::SaveData(writer, name), false);
		}

		ff::Value *value = dict.GetValue(name, chain);
		assertRetVal(InternalSaveValue(value, writer, nameHashOnly), false);
	}

	*data = dataVector.Detach();
	return true;
}

bool ff::SaveDict(const ff::Dict &dict, bool chain, bool nameHashOnly, ff::IData **data)
{
	return InternalSaveDict(dict, chain, nameHashOnly, data);
}

static bool InternalLoadDict(ff::IDataReader *reader, ff::Dict &dict)
{
	assertRetVal(reader, false);

	DWORD version = 0;
	DWORD count = 0;

	assertRetVal(ff::LoadData(reader, version) && version <= 1, false);
	assertRetVal(ff::LoadData(reader, count), false);
	dict.Reserve(count);

	bool nameHashOnly = (version == 1);
	ff::StringCache emptyCache;

	for (size_t i = 0; i < count; i++)
	{
		ff::String name;

		if (nameHashOnly)
		{
			ff::hash_t hash;
			assertRetVal(ff::LoadData(reader, hash), false);
			name = emptyCache.GetString(hash);
		}
		else
		{
			assertRetVal(ff::LoadData(reader, name), false);
		}

		ff::ValuePtr value;
		assertRetVal(InternalLoadValue(reader, &value), false);

		dict.SetValue(name, value);
	}

	return true;
}

bool ff::LoadDict(IDataReader *reader, Dict &dict)
{
	return InternalLoadDict(reader, dict);
}

void ff::DumpDict(ff::StringRef name, const Dict &dict, Log *log, bool chain, bool debugOnly)
{
	if (debugOnly && !GetThisModule().IsDebugBuild())
	{
		return;
	}

	Log extraLog;
	Log &realLog = log ? *log : extraLog;

	realLog.TraceF(L"+- Options for: %s --\r\n", name.c_str());

	Vector<String> names = dict.GetAllNames(chain, true, false);
	for (const String &key: names)
	{
		Value *value = dict.GetValue(key, chain);
		assert(value);

		String valueString;
		ValuePtr convertedValue;

		if (value->Convert(ff::Value::Type::String, &convertedValue))
		{
			valueString = convertedValue->AsString();
		}
		else if (value->Convert(ff::Value::Type::StringVector, &convertedValue))
		{
			Vector<String> &strs = convertedValue->AsStringVector();

			for (size_t i = 0; i < strs.Size(); i++)
			{
				valueString += String::format_new(L"\r\n|    [%lu]: %s", i, strs[i].c_str());
			}
		}
		else if (value->Convert(ff::Value::Type::Data, &convertedValue))
		{
			valueString.format(L"<data[%lu]>", convertedValue->AsData()->GetSize());
		}
		else
		{
			valueString = L"<data>";
		}

		realLog.TraceF(L"| %s: %s\r\n", key.c_str(), valueString.c_str());
	}

	realLog.Trace(L"+- Done --\r\n");
}

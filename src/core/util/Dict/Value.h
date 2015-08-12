#pragma once

namespace ff
{
	class Dict;
	class Value;
	class IData;

	typedef SmartPtr<Value> ValuePtr;

	// a ref-counted variant type

	class UTIL_API Value
	{
	public:
		enum class Type : BYTE
		{
			// Existing values must not be removed or changed since they are persisted.

			Null,
			Bool,
			Double,
			Float,
			Int,
			Pointer,
			Object,
			Data,
			Dict,
			String,
			Guid,
			Point,
			Rect,
			PointF,
			RectF,
			DoubleVector,
			FloatVector,
			IntVector,
			DataVector,
			StringVector,
			ValueVector,
		};

		Type GetType() const;
		bool IsType(Type type) const;

		// The extended type can mean anything that the user wants, but it must
		// be returned from CreateExtendedType()
		static DWORD CreateExtendedType();
		DWORD GetExtendedType() const;
		void SetExtendedType(DWORD type);

		// constructors
		static bool CreateBool(bool val, Value **ppValue);
		static bool CreateColorARGB(BYTE a, BYTE r, BYTE g, BYTE b, Value **ppValue);
		static bool CreateColorRGB(BYTE r, BYTE g, BYTE b, Value **ppValue);
		static bool CreateColorRef(COLORREF color, Value **ppValue);
		static bool CreateDouble(double val, Value **ppValue);
		static bool CreateFloat(float val, Value **ppValue);
		static bool CreateInt(int val, Value **ppValue);
		static bool CreatePointer(void *val, Value **ppValue);
		static bool CreateNull(Value **ppValue);
		static bool CreateObject(IUnknown *pObj, Value **ppValue);
		static bool CreatePoint(const PointInt &point, Value **ppValue);
		static bool CreateRect(const RectInt &rect, Value **ppValue);
		static bool CreatePointF(const PointFloat &point, Value **ppValue);
		static bool CreateRectF(const RectFloat &rect, Value **ppValue);
		static bool CreateString(StringRef str, Value **ppValue);
		static bool CreateStringVector(Value **ppValue);
		static bool CreateValueVector(Value **ppValue);
		static bool CreateValueVector(Vector<ValuePtr> &&vec, Value **ppValue);
		static bool CreateGuid(REFGUID guid, Value **ppValue);
		static bool CreateIntVector(Value **ppValue);
		static bool CreateDoubleVector(Value **ppValue);
		static bool CreateFloatVector(Value **ppValue);
		static bool CreateData(IData *pData, Value **ppValue);
		static bool CreateDataVector(Value **ppValue);
		static bool CreateDict(Value **ppValue);
		static bool CreateDict(ff::Dict &&dict, Value **ppValue);

		// accessors
		bool AsBool() const;
		double AsDouble() const;
		float AsFloat() const;
		int AsInt() const;
		void *AsPointer() const;
		IUnknown *AsObject() const;
		const PointInt &AsPoint() const;
		const RectInt &AsRect() const;
		const PointFloat &AsPointF() const;
		const RectFloat &AsRectF() const;
		StringRef AsString() const;
		Vector<String> &AsStringVector() const;
		Vector<ValuePtr> &AsValueVector() const;
		REFGUID AsGuid() const;
		Vector<int> &AsIntVector() const;
		Vector<double> &AsDoubleVector() const;
		Vector<float> &AsFloatVector() const;
		IData *AsData() const;
		Vector<ComPtr<IData>> &AsDataVector() const;
		ff::Dict &AsDict() const;

		bool Convert(Type type, Value **ppValue);
		bool operator==(const Value &r) const;
		bool Compare(const Value *p) const;

		void AddRef();
		void Release();

	private:
		friend class PoolAllocator<Value>;
		friend struct ff::details::PoolObj<Value>;

		Value();
		~Value();

		// not implemented, cannot copy
		Value(const Value &r);
		const Value &operator=(const Value &r);

		static Value *NewValueOneRef();
		static void DeleteValue(Value *val);

		struct SPoint { int pt[2]; };
		struct SRect { int rect[4]; };
		struct SPointF { float pt[2]; };
		struct SRectF { float rect[4]; };
		struct SString { void *str; };
		struct SDict { size_t data[4]; ff::Dict *AsDict() const; };
		struct StaticValue { size_t data[6]; Value *AsValue(); };

		void SetType(Type type);
		PointInt &InternalGetPoint() const;
		RectInt &InternalGetRect() const;
		PointFloat &InternalGetPointF() const;
		RectFloat &InternalGetRectF() const;
		String &InternalGetString() const;

		union
		{
			bool _bool;
			double _double;
			float _float;
			int _int;
			void *_pointer;
			IUnknown *_object;
			IData *_data;
			SDict _dict;
			SString _string;
			GUID _guid;
			SPoint _point;
			SRect _rect;
			SPointF _pointF;
			SRectF _rectF;
			Vector<double> *_doubleVector;
			Vector<float> *_floatVector;
			Vector<int> *_intVector;
			Vector<ComPtr<IData>> *_dataVector;
			Vector<String> *_stringVector;
			Vector<ValuePtr> *_valueVector;
		};

		long _refCount;
		DWORD _type;
	};
}

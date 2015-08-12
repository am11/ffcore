#pragma once

namespace ff
{
	class IDataWriter;
	class IGraphDevice;
	class ISprite;

	enum AnimTweenType
	{
		POSE_TWEEN_LINEAR_CLAMP,
		POSE_TWEEN_LINEAR_LOOP,
		POSE_TWEEN_SPLINE_CLAMP,
		POSE_TWEEN_SPLINE_LOOP,
	};

	struct UTIL_API FloatKey
	{
		float _value;
		float _tangent;
		float _frame;
		float _padding;

		typedef float ValueType;
		bool operator<(const FloatKey &rhs) const { return _frame < rhs._frame; }

		static const FloatKey &Identity();

		static void InitTangents(FloatKey *pKeys, size_t nKeys, float tension);
		static void Interpolate(const FloatKey &lhs, const FloatKey &rhs, float time, bool bSpline, float &output);
	};

	// Acts like XMFLOAT4, but is exportable
	struct UTIL_API EXMFLOAT4
	{
		float x, y, z, w;

		const EXMFLOAT4 operator=(const XMFLOAT4 &rhs)
			{ CopyMemory(&x, &rhs.x, sizeof(*this)); return *this; }

		operator XMFLOAT4&() const
			{ return *(XMFLOAT4*)&x; }

		XMFLOAT4* operator&()
			{ return (XMFLOAT4*)&x; }

		const XMFLOAT4* operator&() const
			{ return (const XMFLOAT4*)&x; }
	};

	struct UTIL_API VectorKey
	{
		EXMFLOAT4 _value;
		EXMFLOAT4 _tangent;
		float     _frame;
		float     _padding[3];

		typedef XMFLOAT4 ValueType;
		bool operator<(const VectorKey &rhs) const { return _frame < rhs._frame; }

		static const VectorKey& Identity();
		static const VectorKey& IdentityScale();
		static const VectorKey& IdentityTranslate();
		static const VectorKey& IdentityWhite();
		static const VectorKey& IdentityClear();

		static void InitTangents(VectorKey *pKeys, size_t nKeys, float tension);
		static void Interpolate(const VectorKey &lhs, const VectorKey &rhs, float time, bool bSpline, XMFLOAT4 &output);
	};

	struct UTIL_API QuaternionKey
	{
		EXMFLOAT4 _value;
		EXMFLOAT4 _tangent;
		float     _frame;
		float     _padding[3];

		typedef XMFLOAT4 ValueType;
		bool operator<(const QuaternionKey &rhs) const { return _frame < rhs._frame; }

		static const QuaternionKey &Identity();
		static void InitTangents(QuaternionKey *pKeys, size_t nKeys, float tension);
		static void Interpolate(const QuaternionKey &lhs, const QuaternionKey &rhs, float time, bool bSpline, XMFLOAT4 &output);
	};

	struct SpriteKey
	{
		ComPtr<ISprite> _value;
		float           _frame;

		typedef ComPtr<ISprite> ValueType;
		bool operator<(const SpriteKey &rhs) const { return _frame < rhs._frame; }

		static const SpriteKey &Identity();
		static void InitTangents(SpriteKey *pKeys, size_t nKeys, float tension);
		static void Interpolate(const SpriteKey &lhs, const SpriteKey &rhs, float time, bool bSpline, ComPtr<ISprite> &output);
	};
}

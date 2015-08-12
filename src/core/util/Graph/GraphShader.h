#pragma once

namespace ff
{
	class IData;
	class IGraphDevice;
	enum VertexType;

	enum ShaderType
	{
		SHADER_UNKNOWN,
		SHADER_EFFECT,
		SHADER_VERTEX,
		SHADER_PIXEL,
	};

	class __declspec(uuid("4869aaa3-ef89-48b0-870e-ce1662aa48c8")) __declspec(novtable)
		IGraphShader : public IUnknown
	{
	public:
		virtual IData *GetData() const = 0;
		virtual ShaderType GetType() const = 0;
		virtual StringRef GetTarget() const = 0;
		virtual void SetData(ShaderType type, StringRef target, IData *pData) = 0;
	};

	UTIL_API bool CreateGraphShader(IGraphShader **ppShader);
}

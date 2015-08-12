#include "pch.h"
#include "COM/ComAlloc.h"
#include "Data/Data.h"
#include "Data/DataPersist.h"
#include "Data/DataWriterReader.h"
#include "Data/SavedData.h"
#include "Graph/Data/DataBlob.h"
#include "Graph/Data/GraphCategory.h"
#include "Graph/GraphDevice.h"
#include "Graph/GraphShader.h"
#include "Graph/VertexFormat.h"
#include "Module/ModuleFactory.h"
#include "String/StringUtil.h"
#include "Windows/FileUtil.h"

namespace ff
{
	class __declspec(uuid("d91a2232-1247-4c34-bbc3-e61e3240e051"))
		GraphShader : public ComBase, public IGraphShader
	{
	public:
		DECLARE_HEADER(GraphShader);

		// IGraphShader
		virtual IData *GetData() const override;
		virtual ShaderType GetType() const override;
		virtual StringRef GetTarget() const override;
		virtual void SetData(ShaderType type, StringRef target, IData *pData) override;

	private:
		ShaderType _type;
		String _target;
		ComPtr<IData> _data;
	};
}

BEGIN_INTERFACES(ff::GraphShader)
	HAS_INTERFACE(ff::IGraphShader)
END_INTERFACES()

static ff::ModuleStartup Register([](ff::Module &module)
{
	static ff::StaticString name(L"Shader");
	module.RegisterClassT<ff::GraphShader>(name);
});

bool ff::CreateGraphShader(IGraphShader **ppShader)
{
	return SUCCEEDED(ComAllocator<GraphShader>::CreateInstance(
		nullptr, GUID_NULL, __uuidof(IGraphShader), (void**)ppShader));
}

ff::GraphShader::GraphShader()
{
}

ff::GraphShader::~GraphShader()
{
}

ff::IData *ff::GraphShader::GetData() const
{
	return _data;
}

ff::ShaderType ff::GraphShader::GetType() const
{
	return _type;
}

ff::StringRef ff::GraphShader::GetTarget() const
{
	return _target;
}

void ff::GraphShader::SetData(ShaderType type, StringRef target, IData *pData)
{
	_type   = type;
	_target = target;
	_data  = pData;
}

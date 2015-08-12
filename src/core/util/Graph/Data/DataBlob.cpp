#include "pch.h"
#include "COM/ComObject.h"
#include "Data/Data.h"
#include "Graph/Data/DataBlob.h"

namespace ff
{
	class __declspec(uuid("ea478997-3767-4438-8234-a3b2ab31ee89"))
		DataBlob : public ComBase, public IData
	{
	public:
		DECLARE_HEADER(DataBlob);

		bool Init(ID3DBlob *pBlob);

		// IData functions

		virtual const BYTE* GetMem() override;
		virtual size_t      GetSize() override;
		virtual IDataFile*  GetFile() override;
		virtual bool        IsStatic() override;

	private:
		ComPtr<ID3DBlob> _blob;
	};
}

BEGIN_INTERFACES(ff::DataBlob)
	HAS_INTERFACE(ff::IData)
END_INTERFACES()

bool ff::CreateDataFromBlob(ID3DBlob *pBlob, IData **ppData)
{
	assertRetVal(pBlob && ppData, false);
	*ppData = nullptr;

	ComPtr<DataBlob> pData = new ComObject<DataBlob>;
	assertRetVal(pData->Init(pBlob), false);

	*ppData = pData.Detach();

	return *ppData != nullptr;
}

ff::DataBlob::DataBlob()
{
}

ff::DataBlob::~DataBlob()
{
}

bool ff::DataBlob::Init(ID3DBlob *pBlob)
{
	assertRetVal(pBlob, false);

	_blob = pBlob;

	return true;
}

const BYTE *ff::DataBlob::GetMem()
{
	return _blob ? (const BYTE*)_blob->GetBufferPointer() : 0;
}

size_t ff::DataBlob::GetSize()
{
	return _blob ? _blob->GetBufferSize() : 0;
}

ff::IDataFile *ff::DataBlob::GetFile()
{
	return nullptr;
}

bool ff::DataBlob::IsStatic()
{
	return false;
}

#pragma once

namespace ff
{
	class IData;

	UTIL_API bool CreateDataFromBlob(ID3DBlob *pBlob, IData **ppData);
}

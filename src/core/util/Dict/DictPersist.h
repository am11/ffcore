#pragma once

namespace ff
{
	class Dict;
	class Log;
	class IData;
	class IDataReader;

	UTIL_API bool SaveDict(const Dict &dict, bool chain, bool nameHashOnly, IData **data);
	UTIL_API bool LoadDict(IDataReader *reader, Dict &dict);
	UTIL_API void DumpDict(ff::StringRef name, const Dict &dict, Log *log, bool chain, bool debugOnly);
}

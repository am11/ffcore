#pragma once

#if !METRO_APP

namespace ff
{
	class INativeHostControl;

	UTIL_API INativeHostControl *ClrStartup(StringRef versionOrAssembly);
	UTIL_API INativeHostControl *ClrStartup(StringRef configFile, StringRef versionOrAssembly);
	UTIL_API void ClrShutdown();
	UTIL_API bool IsClrRunning();
}

#endif

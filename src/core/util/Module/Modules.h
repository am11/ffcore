#pragma once

#include "Module/Module.h"

namespace ff
{
	/// Automatically creates and caches modules.
	class Modules
	{
	public:
		Modules();

		UTIL_API const Module *Get(StringRef name);
		UTIL_API const Module *Get(REFGUID id);
		UTIL_API const Module *Get(HINSTANCE instance);
		UTIL_API const Module *GetMain();

		UTIL_API Vector<HINSTANCE> GetAllInstances() const;

		UTIL_API const ModuleClassInfo *FindClassInfo(REFGUID classId);
		UTIL_API const ModuleClassInfo *FindClassInfoForInterface(REFGUID interfaceId);
		UTIL_API const ModuleInterfaceInfo *FindInterfaceInfo(REFGUID interfaceId);
		UTIL_API const ModuleCategoryInfo *FindCategoryInfo(REFGUID categoryId);
		UTIL_API const ModuleCreatorInfo *FindCreatorInfo(StringRef name);
		UTIL_API const ModuleCreatorInfo *FindCreatorInfo(REFGUID creatorId);
		UTIL_API ComPtr<IUnknown> CreateParentForCategory(REFGUID categoryId);

		UTIL_API void Clear();

	private:
		void CreateAllModules();

		Mutex _mutex;
		Vector<std::unique_ptr<Module>> _modules;
	};
}

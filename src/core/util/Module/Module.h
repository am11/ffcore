#pragma once

#include "COM/ComAlloc.h"

namespace ff
{
	class Module;
	class IDataReader;

	struct ModuleClassInfo
	{
		String _name;
		GUID _classId;
		GUID _mainInterfaceId;
		GUID _categoryId;
		ClassFactoryFunc _factory;
		const Module *_module;
	};

	struct ModuleCategoryInfo
	{
		String _name;
		GUID _categoryId;
		ClassFactoryFunc _parentObjectFactory;
		const Module *_module;
	};

	struct ModuleInterfaceInfo
	{
		GUID _interfaceId;
		GUID _categoryId;
		const Module *_module;
	};

	struct ModuleCreatorInfo
	{
		String _name;
		GUID _creatorClassId;
		GUID _classId;
		const Module *_module;
	};

	/// A module represents a reusable library of assets and objects
	class Module
	{
	public:
		Module(StringRef name, REFGUID id, HINSTANCE instance);
		virtual ~Module();

		UTIL_API void AddRef() const;
		UTIL_API void Release() const;
		UTIL_API bool IsLocked() const;

		UTIL_API String GetName() const;
		UTIL_API REFGUID GetId() const;
		UTIL_API HINSTANCE GetInstance() const;
		UTIL_API String GetPath() const;

		UTIL_API bool IsDebugBuild() const;
		UTIL_API bool IsMetroBuild() const;
		UTIL_API size_t GetBuildBits() const;
		UTIL_API StringRef GetBuildArch() const;

		UTIL_API ITypeLib *GetTypeLib(size_t index) const;

		virtual bool GetAsset(unsigned int id, IDataReader **data) const = 0;
		virtual String GetString(unsigned int id) const = 0;
		UTIL_API String GetFormattedString(unsigned int id, ...) const;

		UTIL_API const ModuleClassInfo *GetClassInfo(StringRef name) const;
		UTIL_API const ModuleClassInfo *GetClassInfo(REFGUID classId) const;
		UTIL_API const ModuleClassInfo *GetClassInfoForInterface(REFGUID mainInterfaceId) const;
		UTIL_API const ModuleInterfaceInfo *GetInterfaceInfo(REFGUID interfaceId) const;
		UTIL_API const ModuleCategoryInfo *GetCategoryInfo(StringRef name) const;
		UTIL_API const ModuleCategoryInfo *GetCategoryInfo(REFGUID categoryId) const;
		UTIL_API const ModuleCreatorInfo *GetCreatorInfo(StringRef name) const;
		UTIL_API const ModuleCreatorInfo *GetCreatorInfo(REFGUID creatorId) const;

		UTIL_API Vector<GUID> GetClasses() const;
		UTIL_API Vector<GUID> GetCategories() const;
		UTIL_API Vector<GUID> GetCreators() const;

		UTIL_API bool GetClassFactory(REFGUID classId, IClassFactory **factory) const;
		UTIL_API bool CreateClass(REFGUID classId, REFGUID interfaceId, void **obj) const;
		UTIL_API bool CreateClass(IUnknown *parent, REFGUID classId, REFGUID interfaceId, void **obj) const;

		template<typename I>
		bool CreateClass(REFGUID classId, I **obj)
		{
			return CreateClass(classId, __uuidof(I), static_cast<void **>(obj));
		}

		UTIL_API void RegisterClass(
			StringRef name,
			REFGUID classId,
			ClassFactoryFunc factory = nullptr,
			REFGUID mainInterfaceId = GUID_NULL,
			REFGUID categoryId = GUID_NULL);

		template<typename T>
		void RegisterClassT(
			StringRef name,
			REFGUID mainInterfaceId = GUID_NULL,
			REFGUID categoryId = GUID_NULL)
		{
			RegisterClass(name, __uuidof(T), ComAllocator<T>::ComClassFactory, mainInterfaceId, categoryId);
		}

		UTIL_API void RegisterInterface(
			REFGUID interfaceId,
			REFGUID categoryId = GUID_NULL);

		UTIL_API void RegisterCategory(
			StringRef name,
			REFGUID categoryId,
			ClassFactoryFunc parentObjectFactory = nullptr);

		UTIL_API void RegisterCreator(
			StringRef name,
			REFGUID creatorId,
			REFGUID classId);

	private:
		void LoadTypeLibs();

		mutable long _refs;
		String _name;
		GUID _id;
		HINSTANCE _instance;
		Vector<ComPtr<ITypeLib>> _typeLibs;

		Map<String, ModuleClassInfo> _classesByName;
		Map<GUID, ModuleClassInfo> _classesById;
		Map<GUID, ModuleClassInfo> _classesByIid;
		Map<GUID, ModuleClassInfo> _classesByCatId;
		Map<GUID, ModuleInterfaceInfo> _interfacesById;
		Map<GUID, ModuleInterfaceInfo> _interfacesByCatId;
		Map<String, ModuleCategoryInfo> _categoriesByName;
		Map<GUID, ModuleCategoryInfo> _categoriesById;
		Map<String, ModuleCreatorInfo> _creatorsByName;
		Map<GUID, ModuleCreatorInfo> _creatorsById;
	};

	const Module &GetThisModule();

	UTIL_API HINSTANCE GetMainModuleInstance();
	UTIL_API void SetMainModuleInstance(HINSTANCE instance);
}

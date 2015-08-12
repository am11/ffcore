#include "pch.h"
#include "COM/ComFactory.h"
#include "Module/Module.h"
#include "String/StringUtil.h"
#include "Windows/FileUtil.h"

static HINSTANCE s_mainModuleInstance = nullptr;

HINSTANCE ff::GetMainModuleInstance()
{
	assert(s_mainModuleInstance);
	return s_mainModuleInstance;
}

void ff::SetMainModuleInstance(HINSTANCE instance)
{
	assert(!s_mainModuleInstance && instance);
	s_mainModuleInstance = instance;
}

ff::Module::Module(StringRef name, REFGUID id, HINSTANCE instance)
	: _refs(0)
	, _name(name)
	, _id(id)
	, _instance(instance)
{
	LoadTypeLibs();
}

ff::Module::~Module()
{
}

void ff::Module::AddRef() const
{
	InterlockedIncrement(&_refs);
}

void ff::Module::Release() const
{
	InterlockedDecrement(&_refs);
}

// This isn't 100% safe because another thread could lock this module right after checking _refs
bool ff::Module::IsLocked() const
{
	return InterlockedAccess(_refs) != 0;
}

ff::String ff::Module::GetName() const
{
	return _name;
}

REFGUID ff::Module::GetId() const
{
	return _id;
}

HINSTANCE ff::Module::GetInstance() const
{
	return _instance;
}

ff::String ff::Module::GetPath() const
{
#if METRO_APP
	// Assume that the name is a DLL in the application directory
	Windows::ApplicationModel::Package ^package = Windows::ApplicationModel::Package::Current;
	String path = String::from_pstring(package->InstalledLocation->Path);
	AppendPathTail(path, _name);
	ChangePathExtension(path, ff::String(L"dll"));
	assert(FileExists(path));
	return path;
#else
	assert(_instance != nullptr);
	return GetModulePath(_instance);
#endif
}

bool ff::Module::IsDebugBuild() const
{
#ifdef _DEBUG
	return true;
#else
	return false;
#endif
}

bool ff::Module::IsMetroBuild() const
{
#if METRO_APP
	return true;
#else
	return false;
#endif
}

size_t ff::Module::GetBuildBits() const
{
#ifdef _WIN64
	return 64;
#else
	return 32;
#endif
}

ff::StringRef ff::Module::GetBuildArch() const
{
	static StaticString arch
#if defined(_M_ARM)
		(L"arm");
#elif defined(_WIN64)
		(L"x64");
#else
		(L"x86");
#endif

	return arch;
}

void ff::Module::LoadTypeLibs()
{
	assertRet(_typeLibs.IsEmpty());

	// Index zero is for no type lib
	_typeLibs.Push(ComPtr<ITypeLib>());

#if !METRO_APP
	String path = GetPath();

	for (size_t i = 1; ; i++)
	{
		ComPtr<ITypeLib> typeLib;
		if (::FindResource(_instance, MAKEINTRESOURCE(i), L"TYPELIB"))
		{
			String pathIndex = String::format_new(L"%s\\%lu", path.c_str(), i);
			verify(SUCCEEDED(::LoadTypeLibEx(pathIndex.c_str(), REGKIND_NONE, &typeLib)));
		}
		else
		{
			break;
		}

		_typeLibs.Push(typeLib);
	}
#endif
}

ITypeLib *ff::Module::GetTypeLib(size_t index) const
{
	assertRetVal(index < _typeLibs.Size(), nullptr);
	return _typeLibs[index];
}

ff::String ff::Module::GetFormattedString(unsigned int id, ...) const
{
	va_list args;
	va_start(args, id);
	String str = String::format_new_v(GetString(id).c_str(), args);
	va_end(args);

	return str;
}

const ff::ModuleClassInfo *ff::Module::GetClassInfo(StringRef name) const
{
	assertRetVal(name.size(), nullptr);

	BucketIter iter = _classesByName.Get(name);
	if (iter != INVALID_ITER)
	{
		return &_classesByName.ValueAt(iter);
	}

	return nullptr;
}

const ff::ModuleClassInfo *ff::Module::GetClassInfo(REFGUID classId) const
{
	assertRetVal(classId != GUID_NULL, nullptr);

	BucketIter iter = _classesById.Get(classId);
	if (iter != INVALID_ITER)
	{
		return &_classesById.ValueAt(iter);
	}

	return nullptr;
}

const ff::ModuleClassInfo *ff::Module::GetClassInfoForInterface(REFGUID mainInterfaceId) const
{
	assertRetVal(mainInterfaceId != GUID_NULL, nullptr);

	BucketIter iter = _classesByIid.Get(mainInterfaceId);
	if (iter != INVALID_ITER)
	{
		return &_classesByIid.ValueAt(iter);
	}

	return nullptr;
}

const ff::ModuleInterfaceInfo *ff::Module::GetInterfaceInfo(REFGUID interfaceId) const
{
	BucketIter iter = _interfacesById.Get(interfaceId);
	if (iter != INVALID_ITER)
	{
		return &_interfacesById.ValueAt(iter);
	}

	return nullptr;
}

const ff::ModuleCategoryInfo *ff::Module::GetCategoryInfo(StringRef name) const
{
	assertRetVal(name.size(), nullptr);

	BucketIter iter = _categoriesByName.Get(name);
	if (iter != INVALID_ITER)
	{
		return &_categoriesByName.ValueAt(iter);
	}

	return nullptr;
}

const ff::ModuleCategoryInfo *ff::Module::GetCategoryInfo(REFGUID categoryId) const
{
	assertRetVal(categoryId != GUID_NULL, nullptr);

	BucketIter iter = _categoriesById.Get(categoryId);
	if (iter != INVALID_ITER)
	{
		return &_categoriesById.ValueAt(iter);
	}

	return nullptr;
}

const ff::ModuleCreatorInfo *ff::Module::GetCreatorInfo(StringRef name) const
{
	assertRetVal(name.size(), nullptr);

	BucketIter iter = _creatorsByName.Get(name);
	if (iter != INVALID_ITER)
	{
		return &_creatorsByName.ValueAt(iter);
	}

	return nullptr;
}

const ff::ModuleCreatorInfo *ff::Module::GetCreatorInfo(REFGUID creatorId) const
{
	assertRetVal(creatorId != GUID_NULL, nullptr);

	BucketIter iter = _creatorsById.Get(creatorId);
	if (iter != INVALID_ITER)
	{
		return &_creatorsById.ValueAt(iter);
	}

	return nullptr;
}

ff::Vector<GUID> ff::Module::GetClasses() const
{
	Vector<GUID> ids;
	ids.Reserve(_classesById.Size());

	GUID id;
	for (const auto &i: _classesById)
	{
		if (i.GetKey() != id)
		{
			id = i.GetKey();
			ids.Push(id);
		}
	}

	return ids;
}

ff::Vector<GUID> ff::Module::GetCategories() const
{
	Vector<GUID> ids;
	ids.Reserve(_categoriesById.Size());

	GUID id;
	for (const auto &i: _categoriesById)
	{
		if (i.GetKey() != id)
		{
			id = i.GetKey();
			ids.Push(id);
		}
	}

	return ids;
}

ff::Vector<GUID> ff::Module::GetCreators() const
{
	Vector<GUID> ids;
	ids.Reserve(_creatorsById.Size());

	GUID id;
	for (const auto &i: _creatorsById)
	{
		if (i.GetKey() != id)
		{
			id = i.GetKey();
			ids.Push(id);
		}
	}

	return ids;
}

bool ff::Module::GetClassFactory(REFGUID classId, IClassFactory **factory) const
{
	const ModuleClassInfo *info = GetClassInfo(classId);
	if (info != nullptr && info->_factory != nullptr)
	{
		assertRetVal(CreateClassFactory(classId, this, info->_factory, factory), false);
		return true;
	}

	return false;
}

bool ff::Module::CreateClass(REFGUID classId, REFGUID interfaceId, void **obj) const
{
	return CreateClass(nullptr, classId, interfaceId, obj);
}

bool ff::Module::CreateClass(IUnknown *parent, REFGUID classId, REFGUID interfaceId, void **obj) const
{
	const ModuleClassInfo *info = GetClassInfo(classId);
	if (info != nullptr && info->_factory != nullptr)
	{
		assertRetVal(SUCCEEDED(info->_factory(parent, classId, interfaceId, obj)), false);
		return true;
	}

	return false;
}

void ff::Module::RegisterClass(
	StringRef name,
	REFGUID classId,
	ClassFactoryFunc factory,
	REFGUID mainInterfaceId,
	REFGUID categoryId)
{
	assertRet(name.size() && classId != GUID_NULL);

	ModuleClassInfo info;
	info._name = name;
	info._classId = classId;
	info._mainInterfaceId = mainInterfaceId;
	info._categoryId = categoryId;
	info._factory = factory;
	info._module = this;

	_classesByName.Insert(name, info);
	_classesById.Insert(classId, info);

	if (mainInterfaceId != GUID_NULL)
	{
		_classesByIid.Insert(mainInterfaceId, info);
	}

	if (categoryId != GUID_NULL)
	{
		_classesByCatId.Insert(categoryId, info);
	}
}

void ff::Module::RegisterInterface(
	REFGUID interfaceId,
	REFGUID categoryId)
{
	assertRet(interfaceId != GUID_NULL);

	ModuleInterfaceInfo info;
	info._interfaceId = interfaceId;
	info._categoryId = categoryId;

	_interfacesById.Insert(interfaceId, info);

	if (categoryId != GUID_NULL)
	{
		_interfacesByCatId.Insert(categoryId, info);
	}
}

void ff::Module::RegisterCategory(
	StringRef name,
	REFGUID categoryId,
	ClassFactoryFunc parentObjectFactory)
{
	assertRet(name.size() && categoryId != GUID_NULL);

	ModuleCategoryInfo info;
	info._name = name;
	info._categoryId = categoryId;
	info._parentObjectFactory = parentObjectFactory;
	info._module = this;

	_categoriesByName.Insert(name, info);
	_categoriesById.Insert(categoryId, info);
}

void ff::Module::RegisterCreator(
	StringRef name,
	REFGUID creatorId,
	REFGUID classId)
{
	assertRet(name.size() && creatorId != GUID_NULL && classId != GUID_NULL);

	ModuleCreatorInfo info;
	info._name = name;
	info._creatorClassId = creatorId;
	info._classId = classId;
	info._module = this;

	_creatorsByName.Insert(name, info);
	_creatorsById.Insert(creatorId, info);
}

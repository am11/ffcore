#include "pch.h"
#include "Module/ModuleFactory.h"
#include "Module/Modules.h"

ff::Modules::Modules()
{
}

static ff::Module *FindModule(const ff::Vector<std::unique_ptr<ff::Module>> &modules, ff::StringRef name)
{
	for (auto &module: modules)
	{
		if (module->GetName() == name)
		{
			return module.get();
		}
	}

	return nullptr;
}

static ff::Module *FindModule(const ff::Vector<std::unique_ptr<ff::Module>> &modules, REFGUID id)
{
	for (auto &module: modules)
	{
		if (module->GetId() == id)
		{
			return module.get();
		}
	}

	return nullptr;
}

static ff::Module *FindModule(const ff::Vector<std::unique_ptr<ff::Module>> &modules, HINSTANCE instance)
{
	for (auto &module: modules)
	{
		if (module->GetInstance() == instance)
		{
			return module.get();
		}
	}

	return nullptr;
}

const ff::Module *ff::Modules::Get(StringRef name)
{
	// Try and create it
	LockMutex lock(_mutex);

	Module *module = FindModule(_modules, name);
	if (module == nullptr)
	{
		std::unique_ptr<Module> newModule = ModuleFactory::Create(name);
		assertRetVal(newModule != nullptr, nullptr);

		_modules.Push(std::move(newModule));
		module = newModule.get();
	}

	assert(module);
	return module;
}

const ff::Module *ff::Modules::Get(REFGUID id)
{
	// Try and create it
	LockMutex lock(_mutex);

	Module *module = FindModule(_modules, id);
	if (module == nullptr)
	{
		std::unique_ptr<Module> newModule = ModuleFactory::Create(id);
		assertRetVal(newModule != nullptr, nullptr);

		module = newModule.get();
		_modules.Push(std::move(newModule));
	}

	assert(module);
	return module;
}

const ff::Module *ff::Modules::Get(HINSTANCE instance)
{
	// Try and create it
	LockMutex lock(_mutex);

	Module *module = FindModule(_modules, instance);
	if (module == nullptr)
	{
		std::unique_ptr<Module> newModule = ModuleFactory::Create(instance);
		assertRetVal(newModule != nullptr, nullptr);

		module = newModule.get();
		_modules.Push(std::move(newModule));
	}

	assert(module);
	return module;
}

const ff::Module *ff::Modules::GetMain()
{
	HINSTANCE instance = ff::GetMainModuleInstance();
#if !METRO_APP
	if (!instance)
	{
		instance = GetModuleHandle(nullptr);
	}
#endif
	return Get(instance);
}

ff::Vector<HINSTANCE> ff::Modules::GetAllInstances() const
{
	return ModuleFactory::GetAllInstances();
}

const ff::ModuleClassInfo *ff::Modules::FindClassInfo(REFGUID classId)
{
	LockMutex lock(_mutex);

	// First try modules that were already created
	for (size_t i = 0; i < 2; i++)
	{
		if (i == 1)
		{
			CreateAllModules();
		}

		for (auto &module: _modules)
		{
			const ModuleClassInfo *info = module->GetClassInfo(classId);
			if (info != nullptr)
			{
				return info;
			}
		}
	}

	return nullptr;
}

const ff::ModuleClassInfo *ff::Modules::FindClassInfoForInterface(REFGUID interfaceId)
{
	LockMutex lock(_mutex);

	// First try modules that were already created
	for (size_t i = 0; i < 2; i++)
	{
		if (i == 1)
		{
			CreateAllModules();
		}

		for (auto &module: _modules)
		{
			const ModuleClassInfo *info = module->GetClassInfoForInterface(interfaceId);
			if (info != nullptr)
			{
				return info;
			}
		}
	}

	return nullptr;
}

const ff::ModuleInterfaceInfo *ff::Modules::FindInterfaceInfo(REFGUID interfaceId)
{
	LockMutex lock(_mutex);

	// First try modules that were already created
	for (size_t i = 0; i < 2; i++)
	{
		if (i == 1)
		{
			CreateAllModules();
		}

		for (auto &module: _modules)
		{
			const ModuleInterfaceInfo *info = module->GetInterfaceInfo(interfaceId);
			if (info != nullptr)
			{
				return info;
			}
		}
	}

	return nullptr;
}

const ff::ModuleCategoryInfo *ff::Modules::FindCategoryInfo(REFGUID categoryId)
{
	LockMutex lock(_mutex);

	// First try modules that were already created
	for (size_t i = 0; i < 2; i++)
	{
		if (i == 1)
		{
			CreateAllModules();
		}

		for (auto &module: _modules)
		{
			const ModuleCategoryInfo *info = module->GetCategoryInfo(categoryId);
			if (info != nullptr)
			{
				return info;
			}
		}
	}

	return nullptr;
}

const ff::ModuleCreatorInfo *ff::Modules::FindCreatorInfo(StringRef name)
{
	LockMutex lock(_mutex);

	// First try modules that were already created
	for (size_t i = 0; i < 2; i++)
	{
		if (i == 1)
		{
			CreateAllModules();
		}

		for (auto &module: _modules)
		{
			const ModuleCreatorInfo *info = module->GetCreatorInfo(name);
			if (info != nullptr)
			{
				return info;
			}
		}
	}

	return nullptr;
}

const ff::ModuleCreatorInfo *ff::Modules::FindCreatorInfo(REFGUID creatorId)
{
	LockMutex lock(_mutex);

	// First try modules that were already created
	for (size_t i = 0; i < 2; i++)
	{
		if (i == 1)
		{
			CreateAllModules();
		}

		for (auto &module: _modules)
		{
			const ModuleCreatorInfo *info = module->GetCreatorInfo(creatorId);
			if (info != nullptr)
			{
				return info;
			}
		}
	}

	return nullptr;
}

ff::ComPtr<IUnknown> ff::Modules::CreateParentForCategory(REFGUID categoryId)
{
	const ModuleCategoryInfo *info = FindCategoryInfo(categoryId);

	if (info != nullptr && info->_parentObjectFactory != nullptr)
	{
		ComPtr<IUnknown> parent;
		if (SUCCEEDED(info->_parentObjectFactory(nullptr, GUID_NULL, __uuidof(IUnknown), (void **)&parent)))
		{
			return parent;
		}
	}

	return ComPtr<IUnknown>();
}

void ff::Modules::Clear()
{
	LockMutex lock(_mutex);

	_modules.ClearAndReduce();
}

void ff::Modules::CreateAllModules()
{
	Vector<HINSTANCE> instances = ModuleFactory::GetAllInstances();
	for (HINSTANCE inst: instances)
	{
		Get(inst);
	}
}

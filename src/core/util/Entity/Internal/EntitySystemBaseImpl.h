#pragma once

template<class EntryT>
ff::EntitySystemBase<EntryT>::EntitySystemBase(EntityDomain *domain)
	: ff::details::EntitySystem(domain, EntryT::GetComponentNames())
{
#ifdef _DEBUG
	// Make sure there's enough memory in EntryT to store all the components
	size_t entrySize = sizeof(EntityEntryBase);
	for (const String **cur = EntryT::GetComponentNames(); cur && *cur && (*cur)->size(); cur++)
	{
		entrySize += sizeof(void *);
	}

	assert(entrySize <= sizeof(EntryT));
#endif
}

template<class EntryT>
ff::EntitySystemBase<EntryT>::~EntitySystemBase()
{
	assert(_entries.Size() == 0);
}

template<class EntryT>
ff::StringRef ff::EntitySystemBase<EntryT>::GetDefaultName() const
{
	if (_defaultName.empty())
	{
		LockMutex lock(GCS_ENTITY_SYSTEM_BASE);
		if (_defaultName.empty())
		{
			String &defaultName = const_cast<String &>(_defaultName);
			defaultName = String::from_acp(typeid(*this).name() + 6); // skip "class "
		}
	}

	return _defaultName;
}

template<class EntryT>
ff::EntityEntryBase *ff::EntitySystemBase<EntryT>::NewEntry()
{
	_changeStamp++;
	return &_entries.Insert();
}

template<class EntryT>
void ff::EntitySystemBase<EntryT>::DeleteEntry(EntityEntryBase *entry)
{
	_changeStamp++;
	_entries.Delete(*static_cast<EntryT *>(entry));
}

template<class EntryT>
const EntryT *ff::EntitySystemBase<EntryT>::GetFirstEntry() const
{
	return _entries.GetFirst();
}

template<class EntryT>
const EntryT *ff::EntitySystemBase<EntryT>::GetNextEntry(const EntryT &entry) const
{
	return _entries.GetNext(entry);
}

template<class EntryT>
size_t ff::EntitySystemBase<EntryT>::GetEntryCount() const
{
	return _entries.Size();
}

template<class EntryT>
size_t ff::EntitySystemBase<EntryT>::GetEntryChangeStamp() const
{
	return _changeStamp;
}

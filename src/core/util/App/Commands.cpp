#include "pch.h"
#include "App/Commands.h"
#include "App/Idle.h"
#include "COM/ComObject.h"
#include "Globals/ProcessGlobals.h"
#include "Thread/ThreadPool.h"

class __declspec(uuid("8395f435-fb15-4e4c-80ec-638b05d630d5"))
	NullCommandRouter
		: public ff::ComBase
		, public ff::ICommandRouter
{
public:
	DECLARE_HEADER(NullCommandRouter);

	virtual bool FindCommandHandler(DWORD id, ff::ICommandHandler **handler) override;
};

BEGIN_INTERFACES(NullCommandRouter)
	HAS_INTERFACE(ff::ICommandRouter)
END_INTERFACES()

NullCommandRouter::NullCommandRouter()
{
}

NullCommandRouter::~NullCommandRouter()
{
}

bool NullCommandRouter::FindCommandHandler(DWORD id, ff::ICommandHandler **handler)
{
	return false;
}

bool ff::CreateNullCommandRouter(ICommandRouter **obj)
{
	return SUCCEEDED(ComAllocator<NullCommandRouter>::CreateInstance(
		nullptr, GUID_NULL, __uuidof(ICommandRouter), (void **)obj));
}

class __declspec(uuid("7f0ff1fd-c6e5-4ab0-a6b7-00fbb15ffb18"))
	CommandGroups
		: public ff::ICommandGroups
		, public ff::IWorkItem
{
public:
	DECLARE_HEADER(CommandGroups);

	// ICommandGroups
	virtual void SetListener(ff::ICommandGroupListener *pListener) override;
	virtual void AddCommandToGroup(DWORD id, DWORD groupId) override;
	virtual DWORD GetCommandGroup(DWORD id, size_t index) override;
	virtual void InvalidateGroup(DWORD groupId) override;
	virtual void InvalidateCommand(DWORD id) override;
	virtual void InvalidateAll() override;
	virtual void Update() override;

	// IWorkItem
	virtual void Run() override;

private:
	void KickIdle();

	ff::Mutex _cs;
	ff::ICommandGroupListener *_listener;
	ff::Map<DWORD, DWORD> _groupToCommand;
	ff::Map<DWORD, DWORD> _commandToGroup;
	ff::Set<DWORD> _invalidCommands;
	ff::Set<DWORD> _invalidGroups;
	ff::Vector<DWORD> _allGroups;
	ff::Vector<DWORD> _allCommands;
	bool _kickedIdle;
	bool _allInvalid;
};

BEGIN_INTERFACES(CommandGroups)
	HAS_INTERFACE(ff::ICommandGroups)
	HAS_INTERFACE(ff::IWorkItem)
END_INTERFACES()

bool ff::CreateCommandGroups(ICommandGroupListener *pListener, ICommandGroups **ppGroups)
{
	assertRetVal(ppGroups, false);
	*ppGroups = nullptr;

	ff::ComPtr<CommandGroups, ff::ICommandGroups> pGroups = new ff::ComObject<CommandGroups>;
	pGroups->SetListener(pListener);

	*ppGroups = pGroups.Detach();
	return *ppGroups != nullptr;
}

CommandGroups::CommandGroups()
	: _listener(nullptr)
	, _kickedIdle(false)
	, _allInvalid(false)
{
}

CommandGroups::~CommandGroups()
{
	assert(!_listener);
}

void CommandGroups::SetListener(ff::ICommandGroupListener *pListener)
{
	_listener = pListener;
}

void CommandGroups::AddCommandToGroup(DWORD id, DWORD groupId)
{
	for (ff::BucketIter i = _groupToCommand.Get(groupId); i != ff::INVALID_ITER; i = _groupToCommand.GetNext(i))
	{
		if (_groupToCommand.ValueAt(i) == id)
		{
			// already added to the group
			return;
		}
	}

	if (!_groupToCommand.Exists(groupId))
	{
		_allGroups.Push(groupId);
	}

	if (!_commandToGroup.Exists(id))
	{
		_allCommands.Push(id);
	}

	_groupToCommand.Insert(groupId, id);
	_commandToGroup.Insert(id, groupId);
}

DWORD CommandGroups::GetCommandGroup(DWORD id, size_t index)
{
	for (ff::BucketIter i = _commandToGroup.Get(id); i != ff::INVALID_ITER; i = _commandToGroup.GetNext(i))
	{
		if (index == 0)
		{
			return _commandToGroup.ValueAt(i);
		}

		index--;
	}

	return ff::INVALID_DWORD;
}

void CommandGroups::InvalidateGroup(DWORD groupId)
{
	if (!_allInvalid)
	{
		ff::LockMutex crit(_cs);

		if (!_allInvalid)
		{
			_invalidGroups.SetKey(groupId);

			for (ff::BucketIter i = _groupToCommand.Get(groupId); i != ff::INVALID_ITER; i = _groupToCommand.GetNext(i))
			{
				DWORD id = _groupToCommand.ValueAt(i);
				_invalidCommands.SetKey(id);
			}
		}
	}

	if (_listener)
	{
		_listener->OnGroupInvalidated(groupId);
	}

	KickIdle();
}

void CommandGroups::InvalidateCommand(DWORD id)
{
	if (!_allInvalid)
	{
		ff::LockMutex crit(_cs);

		if (!_allInvalid)
		{
			_invalidCommands.SetKey(id);
		}
	}

	KickIdle();
}

void CommandGroups::InvalidateAll()
{
	if (!_allInvalid)
	{
		ff::LockMutex crit(_cs);

		if (!_allInvalid)
		{
			_allInvalid = true;
			_invalidGroups.Clear();
			_invalidCommands.Clear();
		}
	}

	KickIdle();
}

void CommandGroups::Update()
{
	ff::Vector<DWORD> tempIds;
	ff::Vector<DWORD> tempGroups;
	ff::Vector<DWORD> *ids = &tempIds;
	ff::Vector<DWORD> *groups = &tempGroups;

	// Make a list of all invalid commands
	{
		ff::LockMutex crit(_cs);

		if (_allInvalid)
		{
			ids = &_allCommands;
			groups = &_allGroups;
		}
		else
		{
			ids->Reserve(_invalidCommands.Size());
			groups->Reserve(_invalidGroups.Size());

			for (DWORD id: _invalidCommands)
			{
				ids->Push(id);
			}

			for (DWORD id: _invalidGroups)
			{
				groups->Push(id);
			}
		}

		_invalidCommands.Clear();
		_invalidGroups.Clear();
		_allInvalid = false;
	}

	if (_listener)
	{
		if (ids->Size())
		{
			_listener->UpdateCommands(ids->Data(), ids->Size());
		}

		if (groups->Size())
		{
			_listener->UpdateGroups(groups->Data(), groups->Size());
		}
	}
}

void CommandGroups::KickIdle()
{
	if (!_kickedIdle)
	{
		ff::LockMutex crit(_cs);

		if (!_kickedIdle)
		{
			_kickedIdle = true;

			ff::ProcessGlobals::Get()->GetIdleMaster()->Add(this);
		}
	}
}

void CommandGroups::Run()
{
	_kickedIdle = false;

	Update();
}

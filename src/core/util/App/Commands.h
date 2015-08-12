#pragma once

namespace ff
{
	class ICommandGroups;
	class ICommandGroupListener;
	class ICommandRouter;
	class ICommandHandler;
	class ICommandExecuteHandler;
	class ICommandEditHandler;

	// This allows you to categorize your commands into different groups.
	// It's useful for invalidating the UI for every command in the same group.
	// You must set a listener to do the actual UI update, and that gets called on idle.

	class __declspec(uuid("5f63bd45-ca36-4e94-99b3-de8ac0efd4ca")) __declspec(novtable)
		ICommandGroups : public IUnknown
	{
	public:
		virtual void SetListener(ICommandGroupListener *pListener) = 0;
		virtual void AddCommandToGroup(DWORD id, DWORD groupId) = 0;

		virtual DWORD GetCommandGroup(DWORD id, size_t index = 0) = 0;
		virtual void InvalidateGroup(DWORD groupId) = 0;
		virtual void InvalidateCommand(DWORD id) = 0;
		virtual void InvalidateAll() = 0;

		virtual void Update() = 0;
	};

	UTIL_API bool CreateCommandGroups(ICommandGroupListener *pListener, ICommandGroups **ppGroups);

	class ICommandGroupListener
	{
	public:
		virtual void UpdateCommands(const DWORD *pIds, size_t nCount) = 0;
		virtual void UpdateGroups(const DWORD *pGroups, size_t nGroups) = 0;
		virtual void OnGroupInvalidated(DWORD id) = 0;
	};

	// This only finds a handler for a given command

	class __declspec(uuid("02ce93e2-2030-493d-a0a4-754fad154e26")) __declspec(novtable)
		ICommandRouter : public IUnknown
	{
	public:
		virtual bool FindCommandHandler(DWORD id, ICommandHandler **handler) = 0;
	};

	UTIL_API bool CreateNullCommandRouter(ICommandRouter **obj);

	enum class CommandCheck
	{
		CC_UNCHECKABLE,
		CC_UNCHECKED,
		CC_CHECKED,
		CC_INDETERMINATE,
	};

	// This handles updating the state of a given command

	class __declspec(uuid("e70ee6d9-6653-4999-838e-47103ec5c1b0")) __declspec(novtable)
		ICommandHandler : public IUnknown
	{
	public:
		virtual bool CommandIsEnabled(DWORD id) = 0;
		virtual CommandCheck CommandGetCheck(DWORD id) = 0;

		virtual bool CommandGetValue(DWORD id, ff::String &value) = 0;
		virtual bool CommandSetValue(DWORD id, const ff::String &value) = 0;
		virtual bool CommandGetValueChoices(DWORD id, ff::Vector<ff::String> &values) = 0;

		virtual bool CommandGetLabel(DWORD id, ff::String &label, ff::String &desc) = 0;
		virtual bool CommandGetTooltip(DWORD id, ff::String &title, ff::String &desc) = 0;
	};

	class __declspec(uuid("f3c49737-94df-4f0d-87d8-4ee700e1fcbf")) __declspec(novtable)
		ICommandExecuteHandler : public IUnknown
	{
	public:
		virtual void CommandOnExecuted(DWORD id) = 0;
	};
}

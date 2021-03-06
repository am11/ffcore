#pragma once

namespace ff
{
	class ProcessGlobals;

	/// Use static instances of this to hook into the startup of the program.
	struct ProcessStartup
	{
		typedef std::function<void(ProcessGlobals &)> FuncType;

		ProcessStartup(FuncType func);
		static void OnStartup(ProcessGlobals &program);

		FuncType const _func;
		ProcessStartup *const _next;
	};

	/// Use static instances of this to hook into the shutdown of the program.
	struct ProgramShutdown
	{
		typedef std::function<void()> FuncType;

		ProgramShutdown(FuncType func);
		static void OnShutdown();

		FuncType const _func;
		ProgramShutdown *const _next;
	};
}

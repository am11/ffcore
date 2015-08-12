#include "pch.h"
#include "Globals/ProcessStartup.h"

static ff::ProcessStartup *s_programStartup = nullptr;
static ff::ProgramShutdown *s_programShutdown = nullptr;

ff::ProcessStartup::ProcessStartup(FuncType func)
	: _func(func)
	, _next(s_programStartup)
{
	s_programStartup = this;
}

void ff::ProcessStartup::OnStartup(ProcessGlobals &program)
{
	for (const ff::ProcessStartup *func = s_programStartup; func != nullptr; func = func->_next)
	{
		if (func->_func != nullptr)
		{
			func->_func(program);
		}
	}
}

ff::ProgramShutdown::ProgramShutdown(FuncType func)
	: _func(func)
	, _next(s_programShutdown)
{
	s_programShutdown = this;
}

void ff::ProgramShutdown::OnShutdown()
{
	for (const ff::ProgramShutdown *func = s_programShutdown; func != nullptr; func = func->_next)
	{
		if (func->_func != nullptr)
		{
			func->_func();
		}
	}
}

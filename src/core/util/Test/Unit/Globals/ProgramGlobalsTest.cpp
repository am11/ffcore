#include "pch.h"
#include "Data/DataWriterReader.h"
#include "Globals/GlobalsScope.h"
#include "Globals/WinProcessGlobals.h"
#include "Resource/util-resource.h"

bool ProcessGlobalsTest()
{
	int done = 0;
	{
		ff::WinProcessGlobals program;
		assertRetVal(!program.IsValid(), false);

		ff::GlobalsScope programScope(program);
		assertRetVal(program.IsValid(), false);
		assertRetVal(ff::DidProgramStart(), false);

		assertRetVal(!program.IsShuttingDown(), false);
		assertRetVal(!ff::IsProgramShuttingDown(), false);

		ff::AtProgramShutdown([&done]
		{
			ff::Log::GlobalTrace(L"Shut down\n");
			done++;
		});

		program.AtShutdown([&done]
		{
			ff::Log::GlobalTraceF(L"Shut down %d\n", 2);
			done++;
		});

		const ff::Module *module = program.GetModules().Get(ff::String(L"util"));
		assertRetVal(module->GetName() == L"util", false);

		ff::ComPtr<ff::IDataReader> data;
		assertRetVal(module->GetAsset(ID_SHADER_PACKAGE, &data), false);
		const BYTE *bytes = data->Read(data->GetSize());
		assertRetVal(bytes != nullptr, false);
	}

	assertRetVal(done == 2, false);

	return true;
}

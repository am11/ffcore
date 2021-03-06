#pragma once

// NOTE: Each module (DLL or EXE) must include this file in their main file
// (DllMain.cpp or WinMain.cpp) and call SetThisModule during init (DllMain or WinMain).
// That will allow any code to call GetThisModule and not blow up.

#include "Core/DelayLoadInclude.h"
#include "COM/ComInclude.h"
#include "Module/ModuleInclude.h"

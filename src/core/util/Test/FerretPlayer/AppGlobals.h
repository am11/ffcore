#pragma once

#include <App/AppGlobals.h>
#include <Globals/MetroProcessGlobals.h>

namespace FerretPlayer
{
	class AppGlobals : public ff::AppGlobals
	{
	public:
		AppGlobals(ff::MetroProcessGlobals &processGlobals);
		virtual ~AppGlobals();
	};
}

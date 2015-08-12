#include "pch.h"
#include "App/AppTime.h"

ff::AppGlobalTime::AppGlobalTime()
{
	ZeroObject(*this);
}

ff::AppFrameTime::AppFrameTime()
{
	ZeroObject(*this);
	_freq = 1;
	_freqD = 1;
}

#pragma once

namespace ff
{
	struct AppGlobalTime
	{
		UTIL_API AppGlobalTime();

		size_t _advances;
		double _absoluteSeconds;
		double _deltaSeconds;
		double _bankSeconds; // the bank is extra time that wasn't used in a call to Advance()
		double _framesPerSecond;
		double _advancesPerSecond;
		double _bankScale; // From 0 to 1, the amount of the next frame that's in the bank
		float _bankScaleF;
	};

	struct AppFrameTime
	{ 
		UTIL_API AppFrameTime();

		size_t _advances;
		size_t _vsyncs;
		INT64 _advanceTime[4];
		INT64 _renderTime;
		INT64 _flipTime;

		INT64 _freq;
		double _freqD;
	};
}

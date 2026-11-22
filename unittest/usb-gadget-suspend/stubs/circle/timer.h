#pragma once
class CTimer
{
      public:
	static CTimer *Get ();
	static unsigned GetClockTicks ();
	void usDelay (unsigned microseconds);
};

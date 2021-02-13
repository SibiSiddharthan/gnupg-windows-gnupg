#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

int sleep(unsigned int seconds)
{
	Sleep(seconds * 1000);
	return 0;
}

int usleep(unsigned int usec)
{
	// FIXME: use QueryPerformanceCounter to do this properly.
	// This function is only used in tests/gpgscm/ffi.c
	Sleep(usec / 1000u);
	return 0;
}

/***
sys_win.c - win32 system utils
Copyright (C) 2018 a1batross

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details
***/

#include "platform/platform.h"
#include "menu_int.h"
#include "server.h"
#include <shellapi.h>

// [FWGS, 01.03.25]
HANDLE g_waitable_timer;

#if XASH_TIMER == TIMER_WIN32
double Platform_DoubleTime (void)
	{
	static LARGE_INTEGER	g_PerformanceFrequency;
	static LARGE_INTEGER	g_ClockStart;
	LARGE_INTEGER		CurrentTime;

	if (!g_PerformanceFrequency.QuadPart)
		{
		QueryPerformanceFrequency (&g_PerformanceFrequency);
		QueryPerformanceCounter (&g_ClockStart);
		}
	QueryPerformanceCounter (&CurrentTime);

	return (double)(CurrentTime.QuadPart - g_ClockStart.QuadPart) / (double)(g_PerformanceFrequency.QuadPart);
	}

// [FWGS, 01.03.25]
void Platform_Sleep (int msec)
	{
	Sleep (msec);
	}

#endif

// [FWGS, 01.04.25]
void Win32_Init (qboolean con_showalways)
	{
	/*HMODULE hModule = LoadLibrary ("kernel32.dll");*/
	HMODULE hModule = LoadLibraryW (L"kernel32.dll");
	if (hModule)
		{
		HANDLE (__stdcall * pfnCreateWaitableTimerExW)(LPSECURITY_ATTRIBUTES lpTimerAttributes, LPCWSTR lpTimerName,
			DWORD dwFlags, DWORD dwDesiredAccess);

		if ((pfnCreateWaitableTimerExW = (void *)GetProcAddress (hModule, "CreateWaitableTimerExW")))
			{
			g_waitable_timer = pfnCreateWaitableTimerExW (
				NULL,
				NULL,
				0x1	|		// CREATE_WAITABLE_TIMER_MANUAL_RESET
				0x2,		// CREATE_WAITABLE_TIMER_HIGH_RESOLUTION
				0x0002 |	// TIMER_MODIFY_STATE
				SYNCHRONIZE |
				DELETE
			);
			}

		FreeLibrary (hModule);
		}

	Wcon_CreateConsole (con_showalways);
	}

// [FWGS, 01.03.25]
void Win32_Shutdown (void)
	{
	Wcon_DestroyConsole ();

	if (g_waitable_timer)
		{
		CloseHandle (g_waitable_timer);
		g_waitable_timer = 0;
		}
	}

// [FWGS, 01.03.25]
qboolean Win32_NanoSleep (int nsec)
	{
	LARGE_INTEGER ts;

	if (!g_waitable_timer)
		return false;

	ts.QuadPart = -nsec / 100;

	if (!SetWaitableTimer (g_waitable_timer, &ts, 0, NULL, NULL, FALSE))
		{
		CloseHandle (g_waitable_timer);
		g_waitable_timer = 0;
		return false;
		}

	if (WaitForSingleObject (g_waitable_timer, Q_max (1, nsec / 1000000)) != WAIT_OBJECT_0)
		return false;

	return true;
	}

// [FWGS, 01.05.24]
qboolean Platform_DebuggerPresent (void)
	{
	return IsDebuggerPresent ();
	}

// [FWGS, 01.04.25]
void Platform_ShellExecute (const char *path, const char *parms)
	{
	if (!Q_strcmp (path, GENERIC_UPDATE_PAGE) || !Q_strcmp (path, PLATFORM_UPDATE_PAGE))
		path = DEFAULT_UPDATE_PAGE;

	/*ShellExecute (NULL, "open", path, parms, NULL, SW_SHOW);*/
	ShellExecuteA (NULL, "open", path, parms, NULL, SW_SHOW);
	}

// [FWGS, 01.04.25]
#if XASH_MESSAGEBOX == MSGBOX_WIN32
void Platform_MessageBox (const char *title, const char *message, qboolean parentMainWindow)
	{
	/*MessageBox (parentMainWindow ? host.hWnd : NULL, message, title, MB_OK | MB_SETFOREGROUND | MB_ICONSTOP);*/
	MessageBoxA (parentMainWindow ? host.hWnd : NULL, message, title, MB_OK | MB_SETFOREGROUND | MB_ICONSTOP);
	}
#endif

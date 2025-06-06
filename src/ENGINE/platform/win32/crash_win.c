/***
crashhandler.c - advanced crashhandler
Copyright (C) 2016 Mittorn

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details
***/

// [FWGS, 01.03.25]
#include "platform/platform.h"
#include "input.h"

#define DBGHELP 1 // we always enable dbghelp.dll on Windows targets

// [FWGS, 01.03.25]
#if XASH_SDL
#include <SDL.h>
#endif

// [FWGS, 01.03.25]
#if DBGHELP

#include <winnt.h>
#include <dbghelp.h>
#include <psapi.h>
#include <time.h>

#ifndef XASH_SDL
typedef ULONG_PTR DWORD_PTR, *PDWORD_PTR;
#endif

// [FWGS, 01.05.25]
static int Sys_ModuleName (HANDLE process, char *name, void *address, int len)
	{
	/*DWORD_PTR		baseAddress = 0;*/
	static HMODULE	*moduleArray;
	static unsigned int moduleCount;
	/*LPBYTE      moduleArrayBytes;*/
	DWORD       bytesRequired;
	/*int			i;*/

	if (len < 3)
		return 0;

	/*if (!moduleArray && EnumProcessModules (process, NULL, 0, &bytesRequired))*/
	if (!moduleArray && EnumProcessModules (process, NULL, 0, &bytesRequired))
		{
		/*if (bytesRequired)*/
		if (bytesRequired)
			{
			/*moduleArrayBytes = (LPBYTE)LocalAlloc (LPTR, bytesRequired);*/
			LPBYTE moduleArrayBytes = (LPBYTE)LocalAlloc (LPTR, bytesRequired);

			/*if (moduleArrayBytes && EnumProcessModules (process, (HMODULE *)moduleArrayBytes, bytesRequired, &bytesRequired))*/
			if (moduleArrayBytes && EnumProcessModules (process, (HMODULE *)moduleArrayBytes, bytesRequired, &bytesRequired))
				{
				moduleCount = bytesRequired / sizeof (HMODULE);
				moduleArray = (HMODULE *)moduleArrayBytes;
				}
			}
		}

	/*for (i = 0; i < moduleCount; i++)*/
	for (int i = 0; i < moduleCount; i++)
		{
		MODULEINFO info;
		/*GetModuleInformation (process, moduleArray[i], &info, sizeof (MODULEINFO));*/
		GetModuleInformation (process, moduleArray[i], &info, sizeof (MODULEINFO));

		/*if ((address > info.lpBaseOfDll) &&
			((DWORD64)address < (DWORD64)info.lpBaseOfDll + (DWORD64)info.SizeOfImage))*/
		if (address > info.lpBaseOfDll && (DWORD64)address < (DWORD64)info.lpBaseOfDll + (DWORD64)info.SizeOfImage)
			return GetModuleBaseName (process, moduleArray[i], name, len);
		}

	return Q_snprintf (name, len, "???");
	}

// [FWGS, 01.05.25]
static void Sys_StackTrace (PEXCEPTION_POINTERS pInfo)
	{
	/*char	message[8192]; // match *nix Sys_Crash
	int		len = 0;
	size_t	i;
	HANDLE	process = GetCurrentProcess ();
	HANDLE	thread = GetCurrentThread ();
	IMAGEHLP_LINE64	line;
	DWORD	dline = 0;
	DWORD	options;
	CONTEXT	context;
	STACKFRAME64	stackframe;
	DWORD	image;

	context = *pInfo->ContextRecord;

	options = SymGetOptions ();
	options |= SYMOPT_DEBUG;
	options |= SYMOPT_LOAD_LINES;*/
	char	message[8192]; // match *nix Sys_Crash
	HANDLE	process = GetCurrentProcess ();
	HANDLE	thread = GetCurrentThread ();
	DWORD	dline = 0;
	DWORD	options = SymGetOptions ();
	CONTEXT	context = *pInfo->ContextRecord;
	IMAGEHLP_LINE64	line = { .SizeOfStruct = sizeof (IMAGEHLP_LINE64), };

	SetBits (options, SYMOPT_DEBUG | SYMOPT_LOAD_LINES);
	
	SymSetOptions (options);
	SymInitialize (process, NULL, TRUE);

	/*ZeroMemory (&stackframe, sizeof (STACKFRAME64));*/

#ifdef _M_IX86
	/*image = IMAGE_FILE_MACHINE_I386;
	stackframe.AddrPC.Offset = context.Eip;
	stackframe.AddrPC.Mode = AddrModeFlat;
	stackframe.AddrFrame.Offset = context.Ebp;
	stackframe.AddrFrame.Mode = AddrModeFlat;
	stackframe.AddrStack.Offset = context.Esp;
	stackframe.AddrStack.Mode = AddrModeFlat;*/
	DWORD image = IMAGE_FILE_MACHINE_I386;
	STACKFRAME64 stackframe =
		{
		.AddrPC.Offset = context.Eip,
		.AddrPC.Mode = AddrModeFlat,
		.AddrFrame.Offset = context.Ebp,
		.AddrFrame.Mode = AddrModeFlat,
		.AddrStack.Offset = context.Esp,
		.AddrStack.Mode = AddrModeFlat,
		};
#elif _M_X64
	/*image = IMAGE_FILE_MACHINE_AMD64;
	stackframe.AddrPC.Offset = context.Rip;
	stackframe.AddrPC.Mode = AddrModeFlat;
	stackframe.AddrFrame.Offset = context.Rsp;
	stackframe.AddrFrame.Mode = AddrModeFlat;
	stackframe.AddrStack.Offset = context.Rsp;
	stackframe.AddrStack.Mode = AddrModeFlat;*/
	DWORD image = IMAGE_FILE_MACHINE_AMD64;
	STACKFRAME64 stackframe =
		{
		.AddrPC.Offset = context.Rip,
		.AddrPC.Mode = AddrModeFlat,
		.AddrFrame.Offset = context.Rsp,
		.AddrFrame.Mode = AddrModeFlat,
		.AddrStack.Offset = context.Rsp,
		.AddrStack.Mode = AddrModeFlat,
		};
#elif _M_IA64
	/*image = IMAGE_FILE_MACHINE_IA64;
	stackframe.AddrPC.Offset = context.StIIP;
	stackframe.AddrPC.Mode = AddrModeFlat;
	stackframe.AddrFrame.Offset = context.IntSp;
	stackframe.AddrFrame.Mode = AddrModeFlat;
	stackframe.AddrBStore.Offset = context.RsBSP;
	stackframe.AddrBStore.Mode = AddrModeFlat;
	stackframe.AddrStack.Offset = context.IntSp;
	stackframe.AddrStack.Mode = AddrModeFlat;*/
	DWORD image = IMAGE_FILE_MACHINE_IA64;
	STACKFRAME64 stackframe =
		{
		.AddrPC.Offset = context.StIIP,
		.AddrPC.Mode = AddrModeFlat,
		.AddrFrame.Offset = context.IntSp,
		.AddrFrame.Mode = AddrModeFlat,
		.AddrBStore.Offset = context.RsBSP,
		.AddrBStore.Mode = AddrModeFlat,
		.AddrStack.Offset = context.IntSp,
		.AddrStack.Mode = AddrModeFlat,
		};
#elif _M_ARM
	/*image = IMAGE_FILE_MACHINE_ARMNT;
	stackframe.AddrPC.Offset = context.Pc;
	stackframe.AddrPC.Mode = AddrModeFlat;
	stackframe.AddrFrame.Offset = context.R11;
	stackframe.AddrFrame.Mode = AddrModeFlat;
	stackframe.AddrStack.Offset = context.Sp;
	stackframe.AddrStack.Mode = AddrModeFlat;*/
	DWORD image = IMAGE_FILE_MACHINE_ARMNT;
	STACKFRAME64 stackframe =
		{
		.AddrPC.Offset = context.Pc,
		.AddrPC.Mode = AddrModeFlat,
		.AddrFrame.Offset = context.R11,
		.AddrFrame.Mode = AddrModeFlat,
		.AddrStack.Offset = context.Sp,
		.AddrStack.Mode = AddrModeFlat,
		};
#elif _M_ARM64
	/*image = IMAGE_FILE_MACHINE_ARM64;
	stackframe.AddrPC.Offset = context.Pc;
	stackframe.AddrPC.Mode = AddrModeFlat;
	stackframe.AddrFrame.Offset = context.Fp;
	stackframe.AddrFrame.Mode = AddrModeFlat;
	stackframe.AddrStack.Offset = context.Sp;
	stackframe.AddrStack.Mode = AddrModeFlat;*/
	DWORD image = IMAGE_FILE_MACHINE_ARM64;
	STACKFRAME64 stackframe =
		{
		.AddrPC.Offset = context.Pc,
		.AddrPC.Mode = AddrModeFlat,
		.AddrFrame.Offset = context.Fp,
		.AddrFrame.Mode = AddrModeFlat,
		.AddrStack.Offset = context.Sp,
		.AddrStack.Mode = AddrModeFlat,
		};
#elif
	#error
#endif

	/*len = Q_snprintf (message, sizeof (message), "Ver: " XASH_ENGINE_NAME " " XASH_VERSION " (build %i-%s, %s-%s)\n",
		Q_buildnum (), g_buildcommit, Q_buildos (), Q_buildarch ());*/
	int len = Q_snprintf (message, sizeof (message), "Ver: " XASH_ENGINE_NAME " " XASH_VERSION " (build %i-%s-%s, %s-%s)\n",
		Q_buildnum (), g_buildcommit, g_buildbranch, Q_buildos (), Q_buildarch ());

	len += Q_snprintf (message + len, sizeof (message) - len, "Crash: address %p, code %p\n",
		pInfo->ExceptionRecord->ExceptionAddress, (void *)pInfo->ExceptionRecord->ExceptionCode);

	/*len += Q_snprintf (message + len, 1024 - len, "Sys_Crash: address %p, code %p\n",
		pInfo->ExceptionRecord->ExceptionAddress, (void *)pInfo->ExceptionRecord->ExceptionCode);
	if (SymGetLineFromAddr64 (process, (DWORD64)pInfo->ExceptionRecord->ExceptionAddress, &dline, &line))*/
	if (SymGetLineFromAddr64 (process, (DWORD64)pInfo->ExceptionRecord->ExceptionAddress, &dline, &line))
		{
		/*len += Q_snprintf (message + len, 1024 - len, "Exception: %s:%d:%d\n",
			(char *)line.FileName, (int)line.LineNumber, (int)dline);*/
		len += Q_snprintf (message + len, sizeof (message) - len, "Exception: %s:%d:%d\n",
			COM_FileWithoutPath ((char *)line.FileName), (int)line.LineNumber, (int)dline);
		}

	/*if (SymGetLineFromAddr64 (process, stackframe.AddrPC.Offset, &dline, &line))*/
	if (SymGetLineFromAddr64 (process, stackframe.AddrPC.Offset, &dline, &line))
		{
		/*len += Q_snprintf (message + len, 1024 - len, "PC: %s:%d:%d\n",
			(char *)line.FileName, (int)line.LineNumber, (int)dline);*/
		len += Q_snprintf (message + len, sizeof (message) - len, "PC: %s:%d:%d\n",
			COM_FileWithoutPath ((char *)line.FileName), (int)line.LineNumber, (int)dline);
		}

	/*if (SymGetLineFromAddr64 (process, stackframe.AddrFrame.Offset, &dline, &line))*/
	if (SymGetLineFromAddr64 (process, stackframe.AddrFrame.Offset, &dline, &line))
		{
		/*len += Q_snprintf (message + len, 1024 - len, "Frame: %s:%d:%d\n",
			(char *)line.FileName, (int)line.LineNumber, (int)dline);*/
		len += Q_snprintf (message + len, sizeof (message) - len, "Frame: %s:%d:%d\n",
			COM_FileWithoutPath ((char *)line.FileName), (int)line.LineNumber, (int)dline);
		}

	/*for (i = 0; i < 25; i++)*/
	for (size_t i = 0; i < 25; i++)
		{
		/*char buffer[sizeof (SYMBOL_INFO) + MAX_SYM_NAME * sizeof (TCHAR)];*/
		char buffer[sizeof (SYMBOL_INFO) + MAX_SYM_NAME * sizeof (TCHAR)];
		PSYMBOL_INFO symbol = (PSYMBOL_INFO)buffer;

		/*BOOL result = StackWalk64 (
			image, process, thread,*/
		BOOL result = StackWalk64 (image, process, thread,
			&stackframe, &context, NULL,
			/*SymFunctionTableAccess64, SymGetModuleBase64, NULL);*/
			SymFunctionTableAccess64, SymGetModuleBase64, NULL);
		DWORD64 displacement = 0;

		if (!result)
			break;

		/*symbol->SizeOfStruct = sizeof (SYMBOL_INFO);*/
		symbol->SizeOfStruct = sizeof (SYMBOL_INFO);
		symbol->MaxNameLen = MAX_SYM_NAME;

		/*len += Q_snprintf (message + len, 1024 - len, "% 2d %p",
			i, (void *)stackframe.AddrPC.Offset);
		if (SymFromAddr (process, stackframe.AddrPC.Offset, &displacement, symbol))*/
		len += Q_snprintf (message + len, sizeof (message) - len, "%2d: %p",
			i, (void *)stackframe.AddrPC.Offset);
		if (SymFromAddr (process, stackframe.AddrPC.Offset, &displacement, symbol))
			{
			/*len += Q_snprintf (message + len, 1024 - len, " %s ", symbol->Name);*/
			len += Q_snprintf (message + len, sizeof (message) - len, " %s ", symbol->Name);
			}

		/*if (SymGetLineFromAddr64 (process, stackframe.AddrPC.Offset, &dline, &line))*/
		if (SymGetLineFromAddr64 (process, stackframe.AddrPC.Offset, &dline, &line))
			{
			/*len += Q_snprintf (message + len, 1024 - len, "(%s:%d:%d) ",
				(char *)line.FileName, (int)line.LineNumber, (int)dline);*/
			len += Q_snprintf (message + len, sizeof (message) - len, "(%s:%d:%d) ",
				COM_FileWithoutPath ((char *)line.FileName), (int)line.LineNumber, (int)dline);
			}

		/*len += Q_snprintf (message + len, 1024 - len, "(");
		len += Sys_ModuleName (process, message + len, (void *)stackframe.AddrPC.Offset, 1024 - len);
		len += Q_snprintf (message + len, 1024 - len, ")\n");*/
		len += Q_snprintf (message + len, sizeof (message) - len, "(");
		len += Sys_ModuleName (process, message + len, (void *)stackframe.AddrPC.Offset, sizeof (message) - len);
		len += Q_snprintf (message + len, sizeof (message) - len, ")\n");
		}

#if XASH_SDL == 2
	if (host.type != HOST_DEDICATED) // let system to restart server automaticly
		/*SDL_ShowSimpleMessageBox (SDL_MESSAGEBOX_ERROR, "Sys_Crash", message, host.hWnd);*/
		SDL_ShowSimpleMessageBox (SDL_MESSAGEBOX_ERROR, "Sys_Crash", message, NULL);
#endif

	Sys_PrintLog (message);

	SymCleanup (process);
	}

static void Sys_GetProcessName (char *processName, size_t bufferSize)
	{
	char fullpath[MAX_PATH];

	GetModuleBaseName (GetCurrentProcess (), NULL, fullpath, sizeof (fullpath) - 1);
	COM_FileBase (fullpath, processName, bufferSize);
	}

static void Sys_GetMinidumpFileName (const char *processName, char *mdmpFileName, size_t bufferSize)
	{
	time_t currentUtcTime = time (NULL);
	struct tm *currentLocalTime = localtime (&currentUtcTime);

	// [FWGS, 01.02.25]
	Q_snprintf (mdmpFileName, bufferSize, "%s_%s_crash_%d%.2d%.2d_%.2d%.2d%.2d.mdmp",
		processName,
		g_buildcommit,
		currentLocalTime->tm_year + 1900,
		currentLocalTime->tm_mon + 1,
		currentLocalTime->tm_mday,
		currentLocalTime->tm_hour,
		currentLocalTime->tm_min,
		currentLocalTime->tm_sec);
	}

// [FWGS, 01.05.25]
/*static qboolean Sys_WriteMinidump (PEXCEPTION_POINTERS exceptionInfo, MINIDUMP_TYPE minidumpType)*/
static qboolean Sys_WriteMinidump (PEXCEPTION_POINTERS exceptionInfo, MINIDUMP_TYPE minidumpType)
	{
	/*HRESULT	errorCode;*/
	string	processName;
	string	mdmpFileName;
	/*MINIDUMP_EXCEPTION_INFORMATION	minidumpInfo;*/

	Sys_GetProcessName (processName, sizeof (processName));
	Sys_GetMinidumpFileName (processName, mdmpFileName, sizeof (mdmpFileName));

	SetLastError (NOERROR);
	/*HANDLE fileHandle = CreateFile (
		mdmpFileName, GENERIC_WRITE, FILE_SHARE_WRITE,
		NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);*/
	HANDLE fileHandle = CreateFile (mdmpFileName, GENERIC_WRITE, FILE_SHARE_WRITE,
		NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

	/*errorCode = HRESULT_FROM_WIN32 (GetLastError ());
	if (!SUCCEEDED (errorCode))*/
	HRESULT errorCode = HRESULT_FROM_WIN32 (GetLastError ());
	if (!SUCCEEDED (errorCode))
		{
		CloseHandle (fileHandle);
		return false;
		}

	/*minidumpInfo.ThreadId = GetCurrentThreadId ();
	minidumpInfo.ExceptionPointers = exceptionInfo;
	minidumpInfo.ClientPointers = FALSE;*/
	MINIDUMP_EXCEPTION_INFORMATION minidumpInfo =
		{
		.ThreadId = GetCurrentThreadId (),
		.ExceptionPointers = exceptionInfo,
		.ClientPointers = FALSE
		};

	/*qboolean status = MiniDumpWriteDump (
		GetCurrentProcess (), GetCurrentProcessId (), fileHandle,
		minidumpType, &minidumpInfo, NULL, NULL);*/
	qboolean status = MiniDumpWriteDump (GetCurrentProcess (), GetCurrentProcessId (), fileHandle,
		minidumpType, &minidumpInfo, NULL, NULL);

	CloseHandle (fileHandle);
	return status;
	}

#endif

static LPTOP_LEVEL_EXCEPTION_FILTER  oldFilter;

// [FWGS, 01.05.25]
static long _stdcall Sys_Crash (PEXCEPTION_POINTERS pInfo)
	{
	// save config
	if (host.status != HOST_CRASHED)
		{
		// check to avoid recursive call
		/*host.crashed = true;*/
		host.status = HOST_CRASHED;

#if !XASH_DEDICATED
		IN_SetMouseGrab (false);
#endif

#if DBGHELP
		if (Sys_CheckParm ("-minidumps"))
			{
			int minidumpFlags = MiniDumpWithDataSegs |
				MiniDumpWithCodeSegs |
				MiniDumpWithHandleData |
				MiniDumpWithFullMemory |
				MiniDumpWithFullMemoryInfo |
				MiniDumpWithIndirectlyReferencedMemory |
				MiniDumpWithThreadInfo |
				MiniDumpWithModuleHeaders;

			if (!Sys_WriteMinidump (pInfo, (MINIDUMP_TYPE)minidumpFlags))
				{
				// fallback method, create minidump with minimal info in it
				Sys_WriteMinidump (pInfo, MiniDumpWithDataSegs);
				}
			}

		Sys_StackTrace (pInfo);
#else
		Sys_Warn ("Sys_Crash: call %p at address %p", pInfo->ExceptionRecord->ExceptionAddress,
			pInfo->ExceptionRecord->ExceptionCode);
#endif

		if (host.type == HOST_NORMAL)
			CL_Crashed (); // tell client about crash
		/*else
			host.status = HOST_CRASHED;

if DBGHELP
		if (Sys_CheckParm ("-minidumps"))
			{
			int minidumpFlags = (
				MiniDumpWithDataSegs |
				MiniDumpWithCodeSegs |
				MiniDumpWithHandleData |
				MiniDumpWithFullMemory |
				MiniDumpWithFullMemoryInfo |
				MiniDumpWithIndirectlyReferencedMemory |
				MiniDumpWithThreadInfo |
				MiniDumpWithModuleHeaders);

			if (!Sys_WriteMinidump (pInfo, (MINIDUMP_TYPE)minidumpFlags))
				{
				// fallback method, create minidump with minimal info in it
				Sys_WriteMinidump (pInfo, MiniDumpWithDataSegs);
				}
			}
endif*/

		/*if (host_developer.value <= 0)
			{
			// no reason to call debugger in release build - just exit
			Sys_Quit ("crashed");
			return EXCEPTION_CONTINUE_EXECUTION;
			}*/
		if (host_developer.value <= 0)
			{
			// no reason to call debugger in release build - just exit
			Sys_Quit ("crashed");
			return EXCEPTION_CONTINUE_EXECUTION;
			}

		/*// all other states keep unchanged to let debugger find bug
		Sys_DestroyConsole ();*/
		// all other states keep unchanged to let debugger find bug
		Sys_DestroyConsole ();
		}

	if (oldFilter)
		return oldFilter (pInfo);

	return EXCEPTION_CONTINUE_EXECUTION;
	}

// [FWGS, 01.03.25]
void Sys_SetupCrashHandler (const char *argv0)
	{
	SetErrorMode (SEM_FAILCRITICALERRORS);	// no abort/retry/fail errors
	oldFilter = SetUnhandledExceptionFilter (Sys_Crash);
	}

void Sys_RestoreCrashHandler (void)
	{
	// restore filter
	if (oldFilter)
		SetUnhandledExceptionFilter (oldFilter);
	}

//==========================================
// LIBCTINY - Matt Pietrek 2001
// MSDN Magazine, January 2001
//==========================================

// NOTE:  THIS CODE HAS BEEN MODIFIED FOR THIS DEMO APP

// The original source for the LIBCTINY library may be
// found here:  www.wheaty.net
#ifdef _MSC_VER
#include "stdafx.h"
#include "argcargv.h"

#define _MAX_CMD_LINE_ARGS 128

TCHAR *_ppszArgv[_MAX_CMD_LINE_ARGS + 1];

int _ConvertCommandLineToArgcArgv(LPCTSTR lpszSysCmdLine)
{
	if (lpszSysCmdLine == NULL || lpszSysCmdLine[0] == 0)
		return 0;

	static LPVOID pHeap = NULL;

	// Set to no argv elements, in case we have to bail out
	_ppszArgv[0] = 0;

	// First get a pointer to the system's version of the command line, and
	// figure out how long it is.
	int cbCmdLine = lstrlen(lpszSysCmdLine);

	// Allocate memory to store a copy of the command line.  We'll modify
	// this copy, rather than the original command line.  Yes, this memory
	// currently doesn't explicitly get freed, but it goes away when the
	// process terminates.
	if (pHeap) // in case we are called multiple times
	{
		HeapFree(GetProcessHeap(), 0, pHeap);
		pHeap = NULL;
	}

	pHeap = HeapAlloc(GetProcessHeap(), 0, cbCmdLine * sizeof(TCHAR) + 16);

	if (!pHeap)
		return 0;

	LPTSTR pszCmdLine = (LPTSTR)pHeap;

	// Copy the system version of the command line into our copy
	lstrcpy(pszCmdLine, lpszSysCmdLine);

	if (_T('"') == *pszCmdLine) // If command line starts with a quote ("),
	{							// it's a quoted filename.	Skip to next quote.
		pszCmdLine++;

		_ppszArgv[0] = pszCmdLine; // argv[0] == executable name

		while (*pszCmdLine && (*pszCmdLine != _T('"')))
			pszCmdLine++;

		if (*pszCmdLine)			  // Did we see a non-NULL ending?
			*pszCmdLine++ = _T('\0'); // Null terminate and advance to next char
		else
			return 0; // Oops!  We didn't see the end quote
	}
	else // A regular (non-quoted) filename
	{
		_ppszArgv[0] = pszCmdLine; // argv[0] == executable name

		while (*pszCmdLine && (_T(' ') != *pszCmdLine) && (_T('\t') != *pszCmdLine))
			pszCmdLine++;

		if (*pszCmdLine)
			*pszCmdLine++ = _T('\0'); // Null terminate and advance to next char
	}

	// Done processing argv[0] (i.e., the executable name).  Now do th
	// actual arguments

	int argc = 1;

	while (1)
	{
		// Skip over any whitespace
		while (*pszCmdLine && (_T(' ') == *pszCmdLine) || (_T('\t') == *pszCmdLine))
			pszCmdLine++;

		if (_T('\0') == *pszCmdLine) // End of command line???
			return argc;

		if (_T('"') == *pszCmdLine) // Argument starting with a quote???
		{
			pszCmdLine++; // Advance past quote character

			_ppszArgv[argc++] = pszCmdLine;
			_ppszArgv[argc] = 0;

			// Scan to end quote, or NULL terminator
			while (*pszCmdLine && (*pszCmdLine != _T('"')))
				pszCmdLine++;

			if (_T('\0') == *pszCmdLine)
				return argc;

			if (*pszCmdLine)
				*pszCmdLine++ = _T('\0'); // Null terminate and advance to next char
		}
		else // Non-quoted argument
		{
			_ppszArgv[argc++] = pszCmdLine;
			_ppszArgv[argc] = 0;

			// Skip till whitespace or NULL terminator
			while (*pszCmdLine && (_T(' ') != *pszCmdLine) && (_T('\t') != *pszCmdLine))
				pszCmdLine++;

			if (_T('\0') == *pszCmdLine)
				return argc;

			if (*pszCmdLine)
				*pszCmdLine++ = _T('\0'); // Null terminate and advance to next char
		}

		if (argc >= (_MAX_CMD_LINE_ARGS))
			return argc;
	}
}
#endif

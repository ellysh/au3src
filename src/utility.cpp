
///////////////////////////////////////////////////////////////////////////////
//
// AutoIt v3
//
// Copyright (C)1999-2005:
//		- Jonathan Bennett <jon at hiddensoft dot com>
//		- See "AUTHORS.txt" for contributors.
//
// This file is part of AutoIt.
//
// AutoIt source code is copyrighted software distributed under the terms of the
// AutoIt source code license.
//
// You may:
//
// - Customize the design and operation of the AutoIt source code to suit
// the internal needs of your organization except to the extent not
// permitted in this Agreement
//
// You may not:
//
// - Distribute the AutoIt source code and/or compiled versions of AutoIt
// created with the AutoIt source code.
// - Create derivative works based on the AutoIt source code for distribution
// or usage outside your organisation.
// - Modify and/or remove any copyright notices or labels included in the
// AutoIt source code.
//
// AutoIt is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
//
// See the LICENSE.txt file that accompanies the AutoIt source
// code for more details.
//
///////////////////////////////////////////////////////////////////////////////
//
// utility.cpp
//
// This file contains the utility library.  Simple functions
// that can be called at will to perform useful functions.
//
///////////////////////////////////////////////////////////////////////////////


// Includes
#include "StdAfx.h"								// Pre-compiled headers

#ifndef _MSC_VER								// Includes for non-MS compilers
	#include <stdio.h>
	#include <windows.h>
	#include <time.h>
	#include <sys/timeb.h>
	#include <tlhelp32.h>							// Win95 PS functions
	#include <process.h>							// Threading (beginthreadex, etc)
	#include <limits.h>								// UINT_MAX etc
	#include <wininet.h>
	#include <shlobj.h>
	#include <ctype.h>
#endif

#include "globaldata.h"
#include "utility.h"
#include "mt19937ar-cok.h"						// Custom rand()
#include "astring_datatype.h"


// Defines
#define UTIL_WINTEXTBUFFERSIZE	32767			// 64KB is enough wintext for anyone! :) (64kb is max for win95) (32KB max for WM_GETTEXT)


typedef struct									// Storage for MessageBoxEx thread
{
	DWORD	  CurrentThreadID;
	DWORD	  dwTimeout;
} MsgBoxExData;


// Globaldata unique to this file (used by some of the callbacks/threads)
HWND	g_hwndMsgBox;
bool	g_bMsgBoxThreadEnabled;
bool	g_bMsgBoxTimedOut;

char	g_szWinTextBuffer[UTIL_WINTEXTBUFFERSIZE+1];
bool	g_bDetectHiddenText;

HICON	g_IconFind_hIcon;
int		g_IconFind_nWidth;
int		g_IconFind_nHeight;
int		g_IconFind_nDepth;


///////////////////////////////////////////////////////////////////////////////
// Util_FatalError()
//
// Displays a message box with an error message (either from a string resource
// or passed string).
//
///////////////////////////////////////////////////////////////////////////////

void Util_FatalError(UINT iErrTitle, UINT iErrMsg, HWND hWnd)
{
	char szTitle[256+1];						// Max message is 256 characters
	char szText[256+1];							// Max message is 256 characters

	LoadString(GetModuleHandle(NULL), iErrTitle, szTitle, 256);
	LoadString(GetModuleHandle(NULL), iErrMsg, szText, 256);
	MessageBox(hWnd, szText, szTitle, MB_ICONSTOP | MB_OK | MB_SYSTEMMODAL | MB_SETFOREGROUND);

} // Util_FatalError()


void Util_FatalError(char *szTitle, char *szText, HWND hWnd)
{
	MessageBox(hWnd, szText, szTitle, MB_ICONSTOP | MB_OK | MB_SYSTEMMODAL | MB_SETFOREGROUND);

} // Util_FatalError()


///////////////////////////////////////////////////////////////////////////////
// Util_NewHandler()
//
// This function is called when the "new" or "malloc" functions fail to allocate
// memory
//
///////////////////////////////////////////////////////////////////////////////

int Util_NewHandler( size_t size)
{
	MessageBox(NULL, "Error allocating memory.", "AutoIt", MB_ICONSTOP | MB_OK);
	exit(1);									// Force termination

	return 0;									// Never reached, but compiler wants a return value
}


///////////////////////////////////////////////////////////////////////////////
// Util_RandInit()
//
// Call this function to initialise the C random number routine so that it
// at least _tries_ to produce random numbers
//
///////////////////////////////////////////////////////////////////////////////

void Util_RandInit(void)
{
	// Initialize the awful C random number routine
	struct _timeb timebuffer;
	_ftime( &timebuffer );
	//srand(timebuffer.millitm * (int)time(NULL));

	// Init our custom random number routine
	init_genrand(timebuffer.millitm * (int)time(NULL));

} // Util_RandInit()


///////////////////////////////////////////////////////////////////////////////
// Util_RegReadString()
//
// Read a string from a registry key
//
///////////////////////////////////////////////////////////////////////////////

void Util_RegReadString(HKEY hKey, LPCTSTR lpSubKey, LPCTSTR lpValueName, DWORD dwBufLen, char *szValue )
{
	HKEY	hRegKey;

	// Make sure the return value is blank just in case we error
	szValue[0] = '\0';

	if (RegOpenKeyEx(hKey, lpSubKey, 0, KEY_QUERY_VALUE, &hRegKey) != ERROR_SUCCESS)
		return;

	RegQueryValueEx(hRegKey, lpValueName, NULL, NULL, (LPBYTE)szValue, &dwBufLen);

	RegCloseKey(hRegKey);

} // Util_RegReadString()


///////////////////////////////////////////////////////////////////////////////
// Util_WinKill()
//
// Closes a window with extreme predjudice
//
///////////////////////////////////////////////////////////////////////////////

void Util_WinKill(HWND hWnd)
{
	DWORD      dwResult;

	LRESULT lResult = SendMessageTimeout(hWnd, WM_CLOSE, 0, 0, SMTO_ABORTIFHUNG, 500, &dwResult);	// wait 500ms

	if( !lResult )
	{
		// Use more force - Mwuahaha

		// Get the ProcessId for this window.
		DWORD	pid;
		GetWindowThreadProcessId( hWnd, &pid );

		// Open the process with all access.
		HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);

		// Terminate the process.
		TerminateProcess(hProcess, 0);

		CloseHandle(hProcess);
	}

} // Util_WinKill()


///////////////////////////////////////////////////////////////////////////////
// Util_DoesProcessExist()
//
// Checks for the existance of a process name - it will determine the OS version
// and run once piece of code for NT4 and another for 9x/2000+
//
///////////////////////////////////////////////////////////////////////////////

bool Util_DoesProcessExist(const char *szName, DWORD &dwPid, bool &bResult)
{
	if (g_oVersion.IsWinNT4())
	{
		// NT4
		return Util_DoesProcessExistNT(szName, dwPid, bResult);
	}
	else
	{
		// 9x/2000+
		return Util_DoesProcessExist9x2000(szName, dwPid, bResult);
	}

} // Util_DoesProcessExist()


///////////////////////////////////////////////////////////////////////////////
// Util_DoesProcessExist9x()
//
// Checks for the existance of a process name (NT version)
//
// Actually this will run under 2000 and XP too
//
// Dynamincally load functions to retain Win NT compatability - yes it is a
// PAIN IN THE ARSE!
//
///////////////////////////////////////////////////////////////////////////////

bool Util_DoesProcessExist9x2000(const char *szName, DWORD &dwPid, bool &bResult)
{
typedef BOOL (WINAPI *PROCESSWALK)(HANDLE hSnapshot, LPPROCESSENTRY32 lppe);
typedef HANDLE (WINAPI *CREATESNAPSHOT)(DWORD dwFlags, DWORD th32ProcessID);

	HANDLE			snapshot;
	PROCESSENTRY32	proc;
	HINSTANCE		hinstLib;
	CREATESNAPSHOT	lpfnCreateToolhelp32Snapshot = NULL;
	PROCESSWALK		lpfnProcess32First = NULL;
	PROCESSWALK		lpfnProcess32Next  = NULL;
	char			szDrive[_MAX_PATH+1];
	char			szDir[_MAX_PATH+1];
	char			szFile[_MAX_PATH+1];
	char			szExt[_MAX_PATH+1];
	DWORD			dwTemp;

	// We must dynamically load the function to retain compatibility with WinNT
	hinstLib = GetModuleHandle("KERNEL32.DLL");
	if (hinstLib == NULL)
		return false;

    lpfnCreateToolhelp32Snapshot = (CREATESNAPSHOT)GetProcAddress(hinstLib, "CreateToolhelp32Snapshot");
    lpfnProcess32First = (PROCESSWALK)GetProcAddress(hinstLib, "Process32First");
    lpfnProcess32Next  = (PROCESSWALK)GetProcAddress(hinstLib, "Process32Next");

	if (lpfnCreateToolhelp32Snapshot == NULL || lpfnProcess32First == NULL ||
		lpfnProcess32Next == NULL )
	{
		FreeLibrary(hinstLib);					// Free the DLL module.
		return false;
	}

    proc.dwSize = sizeof(proc);
	snapshot = lpfnCreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	lpfnProcess32First(snapshot, &proc);

	bResult = false;
	dwTemp = (DWORD)atoi(szName);				// Get the int value of the string (in case it is a PID)
	while (bResult == false && lpfnProcess32Next(snapshot, &proc) == TRUE )
	{
		_splitpath( proc.szExeFile, szDrive, szDir, szFile, szExt );	// Split the filename
		strcat(szFile, szExt);
		//MessageBox(NULL, szFile, szName, MB_OK);

		if (!stricmp(szFile, szName))
		{
			bResult	= true;						// Try matching as a name
			dwPid	= proc.th32ProcessID;
		}
		else
		{
			if (dwTemp != 0 && dwTemp == proc.th32ProcessID)
			{
				bResult	= true;
				dwPid	= proc.th32ProcessID;
			}
		}
	}

	CloseHandle(snapshot);
	//FreeLibrary(hinstLib);					// GetModuleHandle does not need a freelibrary

	return true;

} // Util_DoesProcessExist9x2000()


///////////////////////////////////////////////////////////////////////////////
// Util_DoesProcessExistNT()
//
// Checks for the existance of a process name (NT version).
//
// REQUIRES PSAPI.DLL - not standard under NT 4
//
// Dynamincally load functions to retain Win 9x compatability - yes it is a
// PAIN IN THE ARSE!
//
///////////////////////////////////////////////////////////////////////////////

bool Util_DoesProcessExistNT(const char *szName, DWORD &dwPid, bool &bResult)
{
typedef BOOL (WINAPI *MyEnumProcesses)(DWORD*, DWORD, DWORD*);
typedef BOOL (WINAPI *MyEnumProcessModules)(HANDLE, HMODULE*, DWORD, LPDWORD);
typedef DWORD (WINAPI *MyGetModuleBaseName)(HANDLE, HMODULE, LPTSTR, DWORD);

//BOOL EnumProcesses(
//  DWORD *lpidProcess,  // array of process identifiers
//  DWORD cb,            // size of array
//  DWORD *cbNeeded      // number of bytes returned
//);

//BOOL EnumProcessModules(
//  HANDLE hProcess,      // handle to process
//  HMODULE *lphModule,   // array of module handles
//  DWORD cb,             // size of array
//  LPDWORD lpcbNeeded    // number of bytes required
//);

//DWORD GetModuleBaseName(
//  HANDLE hProcess,    // handle to process
//  HMODULE hModule,    // handle to module
//  LPTSTR lpBaseName,  // base name buffer
//  DWORD nSize         // maximum characters to retrieve
//);

	HINSTANCE				hinstLib;

	MyEnumProcesses			lpfnEnumProcesses;
	MyEnumProcessModules	lpfnEnumProcessModules;
	MyGetModuleBaseName		lpfnGetModuleBaseName;

	DWORD					idProcessArray[512];		// 512 processes max
	DWORD					cbNeeded;					// Bytes returned
	DWORD					cProcesses;					// Number of processes
	unsigned int			i;
	char					szProcessName[_MAX_PATH+1];
	HMODULE					hMod;
	HANDLE					hProcess;
	char					szDrive[_MAX_PATH+1];
	char					szDir[_MAX_PATH+1];
	char					szFile[_MAX_PATH+1];
	char					szExt[_MAX_PATH+1];
	DWORD					dwTemp;

	// We must dynamically load the function to retain compatibility with Win95
    // Get a handle to the DLL module that contains EnumProcesses
	hinstLib = LoadLibrary("psapi.dll");
	if (hinstLib == NULL)
		return false;

  	lpfnEnumProcesses		= (MyEnumProcesses)GetProcAddress(hinstLib, "EnumProcesses");
	lpfnEnumProcessModules	= (MyEnumProcessModules)GetProcAddress(hinstLib, "EnumProcessModules");
	lpfnGetModuleBaseName	= (MyGetModuleBaseName)GetProcAddress(hinstLib, "GetModuleBaseNameA");

	if (lpfnEnumProcesses == NULL || lpfnEnumProcessModules == NULL ||
		lpfnGetModuleBaseName == NULL )
	{
		FreeLibrary(hinstLib);					// Free the DLL module.
		return false;
	}


	// Get the list of processes running
	if ( !lpfnEnumProcesses(idProcessArray, sizeof(idProcessArray), &cbNeeded))
	{
		FreeLibrary(hinstLib);					// Free the DLL module.
		return false;
	}

	// Get the count of PIDs in the array
	cProcesses = cbNeeded / sizeof(DWORD);

	bResult = false;
	dwTemp = (DWORD)atoi(szName);				// Get the int value of the string (in case it is a PID)
	for(i = 0; i<cProcesses && bResult==false; i++)
	{
		hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, idProcessArray[i] );
		lpfnEnumProcessModules(hProcess, &hMod, sizeof(hMod), &cbNeeded);

		if ( lpfnGetModuleBaseName(hProcess, hMod, szProcessName, _MAX_PATH) )
		{
			_splitpath( szProcessName, szDrive, szDir, szFile, szExt );	// Split the filename
			strcat(szFile, szExt);
			//MessageBox(NULL, szFile, "Process NT", MB_OK);

			if (!stricmp(szFile, szName))
			{
				bResult	= true;					// Try matching as a name
				dwPid	= idProcessArray[i];
			}
			else
			{
				if (dwTemp != 0 && dwTemp == idProcessArray[i])
				{
					bResult	= true;
					dwPid	= idProcessArray[i];
				}
			}
		}

		CloseHandle(hProcess);
	}

	FreeLibrary(hinstLib);					// Free the DLL module.

	return true;

} // Util_DoesProcessExistNT()


///////////////////////////////////////////////////////////////////////////////
// Util_MessageBoxEx()
//
// Windows message box with an optional timeout (0=wait forever)
///////////////////////////////////////////////////////////////////////////////

int Util_MessageBoxEx(HWND hWnd, LPCTSTR lpText, LPCTSTR lpCaption, UINT uType, UINT uTimeout)
{
	MsgBoxExData	MyData;
	unsigned int	uThreadID;
	HANDLE			hThreadHandle = NULL;

	// Reset the timed out flag
	g_bMsgBoxTimedOut = false;

	if (uTimeout)
	{
		g_bMsgBoxThreadEnabled	= true;
		MyData.CurrentThreadID	= GetCurrentThreadId();
		MyData.dwTimeout		= (DWORD)uTimeout;
		hThreadHandle = (HANDLE)_beginthreadex(NULL, 0, Util_TimeoutMsgBoxThread, (void*)&MyData, 0, &uThreadID);
	}

	// Run the windows message box - the spawned thread will force it to close if it times out
	int Res = MessageBox(hWnd, lpText, lpCaption, uType);

	if (hThreadHandle)
	{
		g_bMsgBoxThreadEnabled = false;			// Signal thread to terminate (if not already)
		WaitForSingleObject(hThreadHandle, INFINITE);
		CloseHandle(hThreadHandle);
	}

	if (g_bMsgBoxTimedOut == true)
		return -1;								// Timed out
	else
		return Res;
}


unsigned int _stdcall Util_TimeoutMsgBoxThread(void *pParam)
{
	MsgBoxExData *pMyData = (MsgBoxExData *)pParam;
	DWORD	dwStart, dwDiff, dwCur;

	// Get the current time, and close the message box after the timeout has been
	// exceeded - check every 10ms to avoid CPU load
	dwStart = timeGetTime();
	for (;;)
	{
		if (g_bMsgBoxThreadEnabled == false)
			return 0;							// Caller requested close

		// Get current time in ms
		dwCur = timeGetTime();
		if (dwCur < dwStart)
			dwDiff = (UINT_MAX - dwStart) + dwCur; // timer wraps at 2^32
		else
			dwDiff = dwCur - dwStart;

		// Timer elapsed?
		if (dwDiff >= pMyData->dwTimeout)
			break;
		else
			Sleep(10);
	}

	// Find and close the msgbox
	g_hwndMsgBox = NULL;
	EnumThreadWindows(pMyData->CurrentThreadID, Util_FindMsgBoxProc, 0);
	if (g_hwndMsgBox == NULL)
		return 0;							// MsgBox doesn't exist, just exit

	// Signal the timeout
	g_bMsgBoxTimedOut = true;

	// End the MessageBox with our special message
    EndDialog(g_hwndMsgBox, 1);				// Id 1 works no matter what buttons are used

  return 0;
}


BOOL CALLBACK Util_FindMsgBoxProc(HWND hwnd, LPARAM lParam)
{
  char	szClassname[256];
  BOOL	RetVal = TRUE;

  int nRes = GetClassName(hwnd, szClassname, 256);
  if (!strcmp(szClassname, "#32770") )			// Class name for a MessageBox window
  {
	g_hwndMsgBox = hwnd;
    RetVal = FALSE;
  }

  return RetVal;
}


///////////////////////////////////////////////////////////////////////////////
// Util_StripCR()
//
// Strips all \r from some text
//
///////////////////////////////////////////////////////////////////////////////

void Util_StripCR(char *szText)
{
	unsigned int	i = 0, j = 0;

	while (szText[i] != '\0')
	{
		if (szText[i] == '\r')
			++i;
		else
			szText[j++] = szText[i++];
	}

	szText[j] = '\0';							// Terminate

} // Util_StripCR


///////////////////////////////////////////////////////////////////////////////
// Util_AddCR()
//
// Adds a preceeding \r to all \n characters.  Call Util_AddCRCount to find
// out how many characters will be added so that you can allocate a correct
// sized buffer for the result
//
///////////////////////////////////////////////////////////////////////////////

void Util_AddCR(const char *szInput, char *szOutput)
{
	unsigned int	i = 0, j = 0;

	while (szInput[i] != '\0')
	{
		if (szInput[i] == '\n')
		{
			szOutput[j++] = '\r';
			szOutput[j++] = szInput[i++];
		}
		else
			szOutput[j++] = szInput[i++];
	}

	szOutput[j] = '\0';							// Terminate

} // Util_AddCR


///////////////////////////////////////////////////////////////////////////////
// Util_AddCRSize()
//
// Finds out how long the buffer will need to be to store the result of a
// Util_AddCR operation.  Includes the /0 character
//
///////////////////////////////////////////////////////////////////////////////

unsigned int Util_AddCRSize(const char *szText)
{
	unsigned int	i = 0, j = 0;

	while (szText[i] != '\0')
	{
		if (szText[i++] == '\n')
			j++;
	}

	return (i+1) + j;

} // Util_AddCRSize


///////////////////////////////////////////////////////////////////////////////
// Util_StripTrailingDir()
//
// Makes sure a filename does not have a trailing //
//
///////////////////////////////////////////////////////////////////////////////

void Util_StripTrailingDir(char *szPath)
{
	int len = (int)strlen(szPath)-1;

	if (szPath[len] == '\\')
		szPath[len] = '\0';

} // Util_StripTrailingDir


///////////////////////////////////////////////////////////////////////////////
// Util_FileSetTime()
// Sets the time of a file given a FILETIME struct
// 0 = modified, 1=created, 2=accessed
//
// Note, will only set directory times when used under NT based OSes
///////////////////////////////////////////////////////////////////////////////

bool Util_FileSetTime(const char *szFilename, FILETIME *ft, int nWhichTime)
{
	HANDLE	hFile;

	// If this is a directory and we are NOT running NT then just return
	if (Util_IsDir(szFilename) && g_oVersion.IsWin9x() )
		return true;

	if ( (hFile = CreateFile(szFilename, GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_BACKUP_SEMANTICS, NULL)) == INVALID_HANDLE_VALUE )
		return false;

	if (nWhichTime == 0)
		SetFileTime(hFile, NULL, NULL, ft);	// Modified
	else if (nWhichTime == 1)
		SetFileTime(hFile, ft, NULL, NULL);	// Created
	else
		SetFileTime(hFile, NULL, ft, NULL);	// Accessed

	CloseHandle(hFile);

	return true;

} // Util_FileSetTime()


///////////////////////////////////////////////////////////////////////////////
// Util_GetFileVersion()
//
// Gets the FileVersion info for a given filename, returns false if unsuccessful.
// szVersion must be 43+1 chars long for maxium version number size
//
///////////////////////////////////////////////////////////////////////////////

bool Util_GetFileVersion(char *szFile, char *szVersion)
{
	// Get size of the info block
	DWORD				dwUnused;
	DWORD dwSize = GetFileVersionInfoSize(szFile, &dwUnused);

	if (dwSize)
	{
		VS_FIXEDFILEINFO	*pFFI;
		UINT				uSize;

		BYTE *pInfo = new BYTE[dwSize];

		// Read the version resource
		GetFileVersionInfo((LPSTR)szFile, 0, dwSize, (LPVOID)pInfo);

		// Locate the fixed information
		if (VerQueryValue(pInfo, "\\", (LPVOID *)&pFFI, &uSize)!=0)
		{
			//extract the fields you want from pFFI
			uint iFileMS = (uint)pFFI->dwFileVersionMS;
			uint iFileLS = (uint)pFFI->dwFileVersionLS;
			sprintf(szVersion, "%u.%u.%u.%u", (iFileMS >> 16), (iFileMS & 0xffff), (iFileLS >> 16), (iFileLS & 0xffff) );

			//free(pInfo);
			delete [] pInfo;
			return true;
		}
		else
		{
			delete [] pInfo;
			return false;
		}
	}
	else
		return false;

} // Util_GetFileVersion


///////////////////////////////////////////////////////////////////////////////
// Util_DoesFileExist()
// Returns true if file or directory exists
///////////////////////////////////////////////////////////////////////////////

bool Util_DoesFileExist(const char *szFilename)
{
	if ( strchr(szFilename,'*')||strchr(szFilename,'?') )
	{
		WIN32_FIND_DATA	wfd;

	    HANDLE hFile = FindFirstFile(szFilename, &wfd);

		if ( hFile == INVALID_HANDLE_VALUE )
			return false;

		FindClose(hFile);
		return true;
	}
    else
	{
		if ( GetFileAttributes(szFilename) != 0xffffffff )
			return true;
		else
			return false;
	}

} // Util_DoesFileExist


///////////////////////////////////////////////////////////////////////////////
// Util_IsDir()
// Returns true if the path is a directory
///////////////////////////////////////////////////////////////////////////////

bool Util_IsDir(const char *szPath)
{
	DWORD dwTemp = GetFileAttributes(szPath);
	if ( dwTemp != 0xffffffff && (dwTemp & FILE_ATTRIBUTE_DIRECTORY) )
		return true;
	else
		return false;

} // Util_IsDir


///////////////////////////////////////////////////////////////////////////////
// Util_GetFullPathName()
// Returns the full pathname and strips any trailing \s.  Assumes output
// is _MAX_PATH in size.  Input and output CAN be the same buffer
///////////////////////////////////////////////////////////////////////////////

void Util_GetFullPathName(const char *szIn, char *szOut)
{
	char	*szFilePart;

	GetFullPathName(szIn, _MAX_PATH, szOut, &szFilePart);
	Util_StripTrailingDir(szOut);

} // Util_GetFullPathName()


///////////////////////////////////////////////////////////////////////////////
// Util_GetLongFileName()
// Returns the long filename.  Assumes output
// is _MAX_PATH in size
///////////////////////////////////////////////////////////////////////////////

bool Util_GetLongFileName(const char *szIn, char *szOut)
{
	IMalloc*		iMalloc;
	BOOL			ret = FALSE;
	IShellFolder*	iShellFolder;
	WCHAR			buffer[MAX_PATH];
	ULONG			eaten;
	ITEMIDLIST*		itemIDList;
	char			filePath[MAX_PATH];

	if(SHGetMalloc(&iMalloc) != NOERROR)
	{
		strcpy(szOut, szIn);
		return false;
	}

	// convert file path to display name
	if(SHGetDesktopFolder(&iShellFolder) == NOERROR)
	{
		MultiByteToWideChar(CP_ACP, 0, szIn, -1, buffer, MAX_PATH);
		if(iShellFolder->ParseDisplayName(NULL, NULL, buffer, &eaten, &itemIDList, NULL) == S_OK)
		{
			if(	(ret=SHGetPathFromIDList(itemIDList, filePath)) )
				strcpy(szOut, filePath);
			iMalloc->Free(itemIDList);
		}
		iShellFolder->Release();
	}
	iMalloc->Release();

	if (!ret)
	{
		strcpy(szOut, szIn);					// Error, dupicate input
		return false;
	}
	else
		return true;

} // Util_GetLongFileName()


///////////////////////////////////////////////////////////////////////////////
// Util_IsDifferentVolumes()
// Checks two paths to see if they are on the same volume
///////////////////////////////////////////////////////////////////////////////

bool Util_IsDifferentVolumes(const char *szPath1, const char *szPath2)
{
	char			szP1Drive[_MAX_DRIVE+1];
	char			szP2Drive[_MAX_DRIVE+1];

	char			szP1Dir[_MAX_DIR+1];
	char			szP2Dir[_MAX_DIR+1];

	char			szFile[_MAX_FNAME+1];
	char			szExt[_MAX_EXT+1];

	char			szP1[_MAX_PATH+1];
	char			szP2[_MAX_PATH+1];

	// Get full pathnames
	Util_GetFullPathName(szPath1, szP1);
	Util_GetFullPathName(szPath2, szP2);

	// Split the target into bits
	_splitpath( szP1, szP1Drive, szP1Dir, szFile, szExt );
//	Util_WinPrintf("", "%s - %s - %s - %s", szP1Drive, szP1Dir, szFile, szExt);
	_splitpath( szP2, szP2Drive, szP2Dir, szFile, szExt );
//	Util_WinPrintf("", "%s - %s - %s - %s", szP2Drive, szP2Dir, szFile, szExt);

	if (szP1Drive[0] == '\0' && szP2Drive[0] == '\0' )
	{
		// Both paths are UNC, if both directories are also the same then we assume
		// they are on the same volume (same UNC but different directories may be redirected
		// so I don't think we can assume they are the same volume in that case.
		if ( !stricmp(szP1Dir, szP2Dir) )
			return false;
		else
			return true;
	}
	else
	{
		if ( !stricmp(szP1Drive, szP2Drive) )
			return false;
		else
			return true;
	}

} // Util_IsDifferentVolumes()


///////////////////////////////////////////////////////////////////////////////
// Util_DeleteFile()
// Returns true if a file was deleted.  Note: returns false if file didn't
// exist OR if file did exist and could not be deleted!
///////////////////////////////////////////////////////////////////////////////

bool Util_DeleteFile(const char *szFilename)
{
	WIN32_FIND_DATA	findData;
	bool			bFound = false;				// Not found initially
	char			szDrive[_MAX_PATH+1];
	char			szDir[_MAX_PATH+1];
	char			szFile[_MAX_PATH+1];
	char			szExt[_MAX_PATH+1];
	char			szTempPath[_MAX_PATH+1];

	// Get full path and remove trailing \s
	Util_GetFullPathName(szFilename, szTempPath);

	// If the source is a directory then add *.* to the end
	if (Util_IsDir(szTempPath))
		strcat(szTempPath, "\\*.*");

	// Split the target into bits (used for reconstruction later)
	_splitpath( szTempPath, szDrive, szDir, szFile, szExt );

	// Delete all files matching the criteria
	HANDLE hSearch = FindFirstFile(szTempPath, &findData);
	bool bLoop = true;
	while (hSearch != INVALID_HANDLE_VALUE && bLoop == true)
	{
		// Make sure the returned handle is a file and not a directory before we
		// try and do delete type things on it!
		if ( (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != FILE_ATTRIBUTE_DIRECTORY)
		{
			bFound = true;							// Found at least one match

			// Returned data is just the filename, we need to add the path stuff back on
			// The find strcut only returns the file NAME, we need to reconstruct the path!
			strcpy(szTempPath, szDrive);
			strcat(szTempPath, szDir);
			strcat(szTempPath, findData.cFileName);

			if ( DeleteFile(szTempPath) != TRUE )
			{
				FindClose(hSearch);
				return false;						// Error deleting one of the files
			}

		} // End if

		if (FindNextFile(hSearch, &findData) == FALSE)
			bLoop = false;
	}

	FindClose(hSearch);

	return bFound;

} // Util_DeleteFile()


///////////////////////////////////////////////////////////////////////////////
// Util_CopyFile()
// Returns true if all files copied, else returns false
// Also used to move files too
///////////////////////////////////////////////////////////////////////////////

bool Util_CopyFile(const char *szInputSource, const char *szInputDest, bool bOverwrite, bool bMove)
{
	WIN32_FIND_DATA	findData;
	bool			bFound = false;				// Not found initially
	BOOL			bRes;

	char			szSource[_MAX_PATH+1];
	char			szDest[_MAX_PATH+1];
	char			szExpandedDest[MAX_PATH+1];
	char			szTempPath[_MAX_PATH+1];

	char			szDrive[_MAX_PATH+1];
	char			szDir[_MAX_PATH+1];
	char			szFile[_MAX_PATH+1];
	char			szExt[_MAX_PATH+1];

	// Get local version of our source/dest with full path names, strip trailing \s
	Util_GetFullPathName(szInputSource, szSource);
	Util_GetFullPathName(szInputDest, szDest);

	// Check if the files are on different volumes (affects how we do a Move operation)
	bool bDiffVol = Util_IsDifferentVolumes(szSource, szDest);

	// If the source or dest is a directory then add *.* to the end
	if (Util_IsDir(szSource))
		strcat(szSource, "\\*.*");
	if (Util_IsDir(szDest))
		strcat(szDest, "\\*.*");


	// Split source into file and extension (we need this info in the loop below to recontstruct the path)
	_splitpath( szSource, szDrive, szDir, szFile, szExt );

	// Note we now rely on the SOURCE being the contents of szDrive, szDir, szFile, etc.

	// Does the source file exist?
	HANDLE hSearch = FindFirstFile(szSource, &findData);
	bool bLoop = true;
	while (hSearch != INVALID_HANDLE_VALUE && bLoop == true)
	{
		bFound = true;							// Found at least one match

		// Make sure the returned handle is a file and not a directory before we
		// try and do copy type things on it!
		if ( (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != FILE_ATTRIBUTE_DIRECTORY)
		{
			// Expand the destination based on this found file
			Util_ExpandFilenameWildcard(findData.cFileName, szDest, szExpandedDest);

			// The find struct only returns the file NAME, we need to reconstruct the path!
			strcpy(szTempPath, szDrive);
			strcat(szTempPath, szDir);
			strcat(szTempPath, findData.cFileName);

			// Does the destination exist? - delete it first if it does (unless we not overwriting)
			if ( Util_DoesFileExist(szExpandedDest) )
			{
				if (bOverwrite == false)
				{
					FindClose(hSearch);
					return false;					// Destination already exists and we not overwriting
				}
				else
					DeleteFile(szExpandedDest);
			}

			// Move or copy operation?
			if (bMove == true)
			{
				if (bDiffVol == false)
				{
					bRes = MoveFile(szTempPath, szExpandedDest);
				}
				else
				{
					// Do a copy then delete (simulated copy)
					if ( (bRes = CopyFile(szTempPath, szExpandedDest, FALSE)) != FALSE )
						bRes = DeleteFile(szTempPath);
				}
			}
			else
				bRes = CopyFile(szTempPath, szExpandedDest, FALSE);

			if (bRes == FALSE)
			{
				FindClose(hSearch);
				return false;						// Error copying/moving one of the files
			}

		} // End If

		if (FindNextFile(hSearch, &findData) == FALSE)
			bLoop = false;

	} // End while

	FindClose(hSearch);

	return bFound;

} // Util_CopyFile()


///////////////////////////////////////////////////////////////////////////////
// Util_ExpandFilenameWildcard()
///////////////////////////////////////////////////////////////////////////////

void Util_ExpandFilenameWildcard(const char *szSource, const char *szDest, char *szExpandedDest)
{
	// copy one.two.three  *.txt     = one.two   .txt
	// copy one.two.three  *.*.txt   = one.two   .three  .txt
	// copy one.two.three  *.*.*.txt = one.two   .three  ..txt
	// copy one.two		   test      = test

	char	szFileTemp[_MAX_PATH+1];
	char	szExtTemp[_MAX_PATH+1];

	char	szSrcFile[_MAX_PATH+1];
	char	szSrcExt[_MAX_PATH+1];

	char	szDestDrive[_MAX_PATH+1];
	char	szDestDir[_MAX_PATH+1];
	char	szDestFile[_MAX_PATH+1];
	char	szDestExt[_MAX_PATH+1];

	// If the destination doesn't include a wildcard, send it back vertabim
	if (strchr(szDest, '*') == NULL)
	{
		strcpy(szExpandedDest, szDest);
		return;
	}

	// Split source and dest into file and extension
	_splitpath( szSource, szDestDrive, szDestDir, szSrcFile, szSrcExt );
	_splitpath( szDest, szDestDrive, szDestDir, szDestFile, szDestExt );

	// Source and Dest ext will either be ".nnnn" or "" or ".*", remove the period
	if (szSrcExt[0] == '.')
		strcpy(szSrcExt, &szSrcExt[1]);
	if (szDestExt[0] == '.')
		strcpy(szDestExt, &szDestExt[1]);

	// Start of the destination with the drive and dir
	strcpy(szExpandedDest, szDestDrive);
	strcat(szExpandedDest, szDestDir);

	// Replace first * in the destext with the srcext, remove any other *
	Util_ExpandFilenameWildcardPart(szSrcExt, szDestExt, szExtTemp);

	// Replace first * in the destfile with the srcfile, remove any other *
	Util_ExpandFilenameWildcardPart(szSrcFile, szDestFile, szFileTemp);

	// Concat the filename and extension if req
	if (szExtTemp[0] != '\0')
	{
		strcat(szFileTemp, ".");
		strcat(szFileTemp, szExtTemp);
	}
	else
	{
		// Dest extension was blank SOURCE MIGHT NOT HAVE BEEN!
		if (szSrcExt[0] != '\0')
		{
			strcat(szFileTemp, ".");
			strcat(szFileTemp, szSrcExt);
		}
	}

	// Now add the drive and directory bit back onto the dest
	strcat(szExpandedDest, szFileTemp);

} // Util_CopyFile


///////////////////////////////////////////////////////////////////////////////
// Util_ExpandFilenameWildcardPart()
///////////////////////////////////////////////////////////////////////////////

void Util_ExpandFilenameWildcardPart(const char *szSource, const char *szDest, char *szExpandedDest)
{
	int		i = 0, j = 0, k = 0;

	// Replace first * in the dest with the src, remove any other *
	char *lpTemp = strchr(szDest, '*');
	if (lpTemp != NULL)
	{
		// Contains at least one *, copy up to this point
		while(szDest[i] != '*')
			szExpandedDest[j++] = szDest[i++];
		// Skip the * and replace in the dest with the srcext
		while(szSource[k] != '\0')
			szExpandedDest[j++] = szSource[k++];
		// Skip any other *
		i++;
		while(szDest[i] != '\0')
		{
			if (szDest[i] == '*')
				i++;
			else
				szExpandedDest[j++] = szDest[i++];
		}
		szExpandedDest[j] = '\0';
	}
	else
	{
		// No wildcard, straight copy of destext
		strcpy(szExpandedDest, szDest);
	}

} // Util_ExpandFilenameWildcardPart()


///////////////////////////////////////////////////////////////////////////////
// Util_CreateDir()
// Recursive directory creation function
///////////////////////////////////////////////////////////////////////////////

bool Util_CreateDir(const char *szDirName)
{
	bool	bRes;

	DWORD dwTemp = GetFileAttributes(szDirName);
	if (dwTemp == FILE_ATTRIBUTE_DIRECTORY)
		return true;							// Directory exists, yay!

	if (dwTemp == 0xffffffff)
	{	// error getting attribute - what was the error?
		if (GetLastError() == ERROR_PATH_NOT_FOUND)
		{
			// Create path
			char *szTemp = Util_StrCpyAlloc(szDirName);

			char *psz_Loc = strrchr(szTemp, '\\');	/* find last \ */
			if (psz_Loc == NULL)				// not found
			{
				delete [] szTemp;
				return false;
			}
			else
			{
				*psz_Loc = '\0';				// remove \ and everything after
				bRes = Util_CreateDir(szTemp);
				delete [] szTemp;
				if (bRes)
				{
					if (CreateDirectory(szDirName, NULL))
						bRes = true;
					else
						bRes = false;
				}

				return bRes;
			}
		}
		else
		{
			if (GetLastError() == ERROR_FILE_NOT_FOUND)
			{
				// Create directory
				if (CreateDirectory(szDirName, NULL))
					return true;
				else
					return false;
			}
		}
	}

	return false;								// Unforeseen error

} // Util_CreateDir()


///////////////////////////////////////////////////////////////////////////////
// Util_CopyDir()
///////////////////////////////////////////////////////////////////////////////

bool Util_CopyDir (const char *szInputSource, const char *szInputDest, bool bOverwrite)
{
	SHFILEOPSTRUCT	FileOp;
	char			szSource[_MAX_PATH+2];
	char			szDest[_MAX_PATH+2];

	// Get the fullpathnames and strip trailing \s
	Util_GetFullPathName(szInputSource, szSource);
	Util_GetFullPathName(szInputDest, szDest);

	// Ensure source is a directory
	if (Util_IsDir(szSource) == false)
		return false;							// Nope

	// Does the destination dir exist?
	if (Util_IsDir(szDest))
	{
		if (bOverwrite == false)
			return false;
	}
	else
	{
		// We must create the top level directory
		if (!Util_CreateDir(szDest))
			return false;
	}

	// To work under old versions AND new version of shell32.dll the source must be specifed
	// as "dir\*.*" and the destination directory must already exist... Godamn Microsoft and their APIs...
	strcat(szSource, "\\*.*");

	// We must also make source\dest double nulled strings for the SHFileOp API
	szSource[strlen(szSource)+1] = '\0';
	szDest[strlen(szDest)+1] = '\0';

	// Setup the struct
	FileOp.pFrom					= szSource;
	FileOp.pTo						= szDest;
	FileOp.hNameMappings			= NULL;
	FileOp.lpszProgressTitle		= NULL;
	FileOp.fAnyOperationsAborted	= FALSE;
	FileOp.hwnd						= NULL;

	FileOp.wFunc	= FO_COPY;
	FileOp.fFlags	= FOF_SILENT | FOF_NOCONFIRMMKDIR | FOF_NOCONFIRMATION | FOF_NOERRORUI;

	if ( SHFileOperation(&FileOp) )
		return false;

	return true;

} // Util_CopyDir()


///////////////////////////////////////////////////////////////////////////////
// Util_MoveDir()
///////////////////////////////////////////////////////////////////////////////

bool Util_MoveDir (const char *szInputSource, const char *szInputDest, bool bOverwrite)
{
	SHFILEOPSTRUCT	FileOp;
	char			szSource[_MAX_PATH+2];
	char			szDest[_MAX_PATH+2];

	// Get the fullpathnames and strip trailing \s
	Util_GetFullPathName(szInputSource, szSource);
	Util_GetFullPathName(szInputDest, szDest);

	// Ensure source is a directory
	if (Util_IsDir(szSource) == false)
		return false;							// Nope

	// Does the destination dir exist?
	if (Util_IsDir(szDest))
	{
		if (bOverwrite == false)
			return false;
	}

	// Now, if the source and dest are on different volumes then we must copy rather than move
	// as move in this case only works on some OSes
	if (Util_IsDifferentVolumes(szSource, szDest))
	{
		// Copy and delete (poor man's move)
		if (Util_CopyDir(szSource, szDest, true) == false)
			return false;
		if (Util_RemoveDir(szSource, true) == false)
			return false;
		else
			return true;
	}

	// We must also make source\dest double nulled strings for the SHFileOp API
	szSource[strlen(szSource)+1] = '\0';
	szDest[strlen(szDest)+1] = '\0';

	// Setup the struct
	FileOp.pFrom					= szSource;
	FileOp.pTo						= szDest;
	FileOp.hNameMappings			= NULL;
	FileOp.lpszProgressTitle		= NULL;
	FileOp.fAnyOperationsAborted	= FALSE;
	FileOp.hwnd						= NULL;

	FileOp.wFunc	= FO_MOVE;
	FileOp.fFlags	= FOF_SILENT | FOF_NOCONFIRMMKDIR | FOF_NOCONFIRMATION | FOF_NOERRORUI;

	if ( SHFileOperation(&FileOp) )
		return false;
	else
		return true;

} // Util_MoveDir()


///////////////////////////////////////////////////////////////////////////////
// Util_RemoveDir()
///////////////////////////////////////////////////////////////////////////////

bool Util_RemoveDir (const char *szInputSource, bool bRecurse)
{
	SHFILEOPSTRUCT	FileOp;
	char			szSource[_MAX_PATH+2];

	// Get the fullpathnames and strip trailing \s
	Util_GetFullPathName(szInputSource, szSource);

	// Ensure source is a directory
	if (Util_IsDir(szSource) == false)
		return false;							// Nope

	// If recursion not on just try a standard delete on the directory (the SHFile function WILL
	// delete a directory even if not empty no matter what flags you give it...)
	if (bRecurse == false)
	{
		if (!RemoveDirectory(szSource))
			return false;
		else
			return true;
	}

	// We must also make double nulled strings for the SHFileOp API
	szSource[strlen(szSource)+1] = '\0';

	// Setup the struct
	FileOp.pFrom					= szSource;
	FileOp.pTo						= NULL;
	FileOp.hNameMappings			= NULL;
	FileOp.lpszProgressTitle		= NULL;
	FileOp.fAnyOperationsAborted	= FALSE;
	FileOp.hwnd						= NULL;

	FileOp.wFunc	= FO_DELETE;
	FileOp.fFlags	= FOF_SILENT | FOF_NOCONFIRMMKDIR | FOF_NOCONFIRMATION | FOF_NOERRORUI;

	if ( SHFileOperation(&FileOp) )
		return false;

	return true;

} // Util_RemoveDir()


///////////////////////////////////////////////////////////////////////////////
// Util_AddTextToBuffer()
//
// Adds the specified text to the end of a buffer
///////////////////////////////////////////////////////////////////////////////

void Util_AddTextToBuffer(const char *szText, char *szBuffer, unsigned int iBufSize)
{
	unsigned int nText	= 0;
	unsigned int nBuf	= (unsigned int)strlen(szBuffer);

	while ( (nBuf < iBufSize) && (szText[nText] != '\0') )
		szBuffer[nBuf++] = szText[nText++];

	szBuffer[nBuf] = '\0';						// terminate the string

} // Util_AddText()


///////////////////////////////////////////////////////////////////////////////
// Util_GetWinText()
//
// Gets the title, text and misc information on the specified window
//
///////////////////////////////////////////////////////////////////////////////

const char * Util_GetWinText(HWND hWnd, bool bDetectHiddenText)
{
	// Clear the win text buffer
	g_szWinTextBuffer[0] = '\0';

	// Set our global flag for hidden text
	g_bDetectHiddenText = bDetectHiddenText;

	// Get window text from all the child windows
	EnumChildWindows(hWnd, (WNDENUMPROC)Util_GetWinTextProc, (LPARAM)g_szWinTextBuffer);

	return g_szWinTextBuffer;

} // GetRevealText()


///////////////////////////////////////////////////////////////////////////////
// Util_GetWinTextProc()
//
// Handler function for EnumChildWindows
///////////////////////////////////////////////////////////////////////////////

BOOL CALLBACK Util_GetWinTextProc(HWND hWnd, LPARAM lParam)
{
	char	*szWinText = (char*)lParam;
	char	szBuffer[UTIL_WINTEXTBUFFERSIZE+1];	// Maximum under win95

	// Don't do any more testing on this child window if it is hung
	if ( Util_IsWinHung(hWnd) )
		return TRUE;							// Carry on searching

	// Ensure the buffer is blank
	szBuffer[0] = '\0';

	// Hidden text?
	if ( (IsWindowVisible(hWnd)) || (g_bDetectHiddenText == true) )
	{
		if (SendMessage(hWnd, WM_GETTEXTLENGTH, 0, 0) > 0)
		{
			// if (SendMessage(hWnd,(UINT)WM_GETTEXT,(WPARAM)AUT_WINTEXTBUFFER,(LPARAM)szBuffer))
			// GetWindowText(hWnd, szBuffer, AUT_WINTEXTBUFFER);
			SendMessage(hWnd, WM_GETTEXT,(WPARAM)AUT_WINTEXTBUFFER,(LPARAM)szBuffer);
			szBuffer[UTIL_WINTEXTBUFFERSIZE] = '\0';		// Ensure terminated if large amount of return text
			Util_AddTextToBuffer(szBuffer, szWinText, UTIL_WINTEXTBUFFERSIZE);
			Util_AddTextToBuffer("\n", szWinText, UTIL_WINTEXTBUFFERSIZE);
		}
	}

	return TRUE;				// Carry on searching

} // Util_GetWinTextProc()


///////////////////////////////////////////////////////////////////////////////
// Util_GetIPAddress()
//
// Gets an IP address for adapter nn (1 is first)
//
// szInetBuf must be at least 16 chars long
//
///////////////////////////////////////////////////////////////////////////////

void Util_GetIPAddress(int nAdapter, char *szInetBuf)
{
	HOSTENT		*lpHost;
	IN_ADDR		inaddr;
	WSADATA		wsadata;
	char		szBuf[256];
	int			nNumAdapters;

	if( WSAStartup(MAKEWORD(1, 1), &wsadata) == 0 )
	{
		gethostname(szBuf, sizeof(szBuf) );
		lpHost = gethostbyname(szBuf);

		// How many adapters have we?
		nNumAdapters = 0;
		while(lpHost->h_addr_list[nNumAdapters] != NULL)
			nNumAdapters++;

		if (nAdapter > nNumAdapters)
			strcpy(szInetBuf, "0.0.0.0");
		else
		{
			memcpy(&inaddr,lpHost->h_addr_list[nAdapter-1], 4);
			strcpy(szInetBuf, (char*)inet_ntoa(inaddr));
		}

		WSACleanup();

	}

} // Util_GetIPAddress()


///////////////////////////////////////////////////////////////////////////////
// Util_VariantArrayDim()
//
// Simple helper function for Diming a SINGLE dimension variant with specified
// number of elements
//
///////////////////////////////////////////////////////////////////////////////

void Util_VariantArrayDim(Variant *pvVariant, unsigned int iElements)
{
	pvVariant->ArraySubscriptClear();			// Reset the subscript
	pvVariant->ArraySubscriptSetNext(iElements);// Number of elements
	pvVariant->ArrayDim();						// Dimension array

} // Util_VariantArrayDim()


///////////////////////////////////////////////////////////////////////////////
// Util_VariantArrayGetRef()
//
// Simple helper function for getting a ref to an element of our simple array
//
///////////////////////////////////////////////////////////////////////////////

Variant * Util_VariantArrayGetRef(Variant *pvVariant, unsigned int iElement)
{
	pvVariant->ArraySubscriptClear();			// Reset the subscript
	pvVariant->ArraySubscriptSetNext(iElement);	// Set subscript we want to access
	return pvVariant->ArrayGetRef();			// Get reference to the element

} // Util_VariantArrayGetRef()


///////////////////////////////////////////////////////////////////////////////
// Util_GetClassList()
//
// Gets List of classnames from a window
//
///////////////////////////////////////////////////////////////////////////////

const char * Util_GetClassList(HWND hWnd)
{
	// Clear the win text buffer
	g_szWinTextBuffer[0] = '\0';

	// Get class names from all the child windows
	EnumChildWindows(hWnd, (WNDENUMPROC)Util_GetClassListProc, (LPARAM)g_szWinTextBuffer);

	return g_szWinTextBuffer;

}

BOOL CALLBACK Util_GetClassListProc(HWND hWnd, LPARAM lParam)
{
	char	*szWinText = (char*)lParam;
	char	szBuffer[UTIL_WINTEXTBUFFERSIZE+1];					// Maximum under win95

	GetClassName(hWnd, szBuffer, sizeof(szBuffer));
	if (szBuffer[0] != '\0')
	{
		Util_AddTextToBuffer(szBuffer, szWinText, UTIL_WINTEXTBUFFERSIZE);
		Util_AddTextToBuffer("\n", szWinText, UTIL_WINTEXTBUFFERSIZE);
	}

	return TRUE;				// Carry on searching

} // Util_GetClassListProc()


///////////////////////////////////////////////////////////////////////////////
// Util_SoundPlay()
// Asynchronous PlaySound
///////////////////////////////////////////////////////////////////////////////

void Util_SoundPlay(const char *szFilename, bool bWait)
{
	AString	sMCI;
	char	szBuffer[256];

	sMCI = "open ";
	sMCI += '"';
	sMCI += szFilename;
	sMCI += '"';
	sMCI += " alias PlayMe";

	mciSendString("status PlayMe mode",szBuffer,sizeof(szBuffer),NULL);

	if ( !szBuffer[0] == '\0' )
		mciSendString("close PlayMe",NULL,0,NULL);

	if (szFilename[0] == '\0')
		return;									// No sound to play

	if ( mciSendString(sMCI.c_str(),NULL,0,NULL)==0 )
	{
		if (bWait)
		{
            mciSendString("play PlayMe wait",NULL,0,NULL);
			mciSendString("close PlayMe",NULL,0,NULL);
		}
		else
            mciSendString("play PlayMe",NULL,0,NULL);
	}

} // Util_SoundPlay()


///////////////////////////////////////////////////////////////////////////////
// Util_Shutdown()
// Shutdown or logoff the system
//
// Returns false if the function could not get the rights to shutdown
///////////////////////////////////////////////////////////////////////////////

BOOL Util_Shutdown(int nFlag)
{

/*
flags can be a combination of:
#define EWX_LOGOFF           0
#define EWX_SHUTDOWN         0x00000001
#define EWX_REBOOT           0x00000002
#define EWX_FORCE            0x00000004
#define EWX_POWEROFF         0x00000008 */

	HANDLE				hToken;
	TOKEN_PRIVILEGES	tkp;

	// If we are running NT, make sure we have rights to shutdown
	if (g_oVersion.IsWinNT())
	{
		// Get a token for this process.
 		if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken))
			return FALSE;						// Don't have the rights

		// Get the LUID for the shutdown privilege.
 		LookupPrivilegeValue(NULL, SE_SHUTDOWN_NAME, &tkp.Privileges[0].Luid);

		tkp.PrivilegeCount = 1;  /* one privilege to set */
		tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

		// Get the shutdown privilege for this process.
 		AdjustTokenPrivileges(hToken, FALSE, &tkp, 0, (PTOKEN_PRIVILEGES)NULL, 0);

		// Cannot test the return value of AdjustTokenPrivileges.
 		if (GetLastError() != ERROR_SUCCESS)
			return FALSE;						// Don't have the rights
	}

	// if we are forcing the issue, AND this is 95/98 terminate all windows first
	if ( (g_oVersion.IsWin9x()) && (nFlag & EWX_FORCE) )
	{
		nFlag ^= EWX_FORCE;	// remove this flag - not valid in 95
		EnumWindows((WNDENUMPROC) Util_ShutdownHandler, 0);
	}

	// for suspend and hibernate code
	if ( (nFlag == 32) || (nFlag == 64) )
		return SetSystemPowerState(  (nFlag == 32),  FALSE);

	// ExitWindows
	return ExitWindowsEx(nFlag, 0);

} // Util_Shutdown()


BOOL Util_ShutdownHandler(HWND hwnd, DWORD lParam)
{
	// if the window is me, don't terminate!
	if (g_hWnd != hwnd)
		Util_WinKill(hwnd);

	// Continue the enumeration.
	return TRUE;

} // Util_ShutdownHandler()


///////////////////////////////////////////////////////////////////////////////
// Util_GetInt()
///////////////////////////////////////////////////////////////////////////////

bool Util_GetInt(const char *szInt, int &pos, int &nValue)
{
	int nSign = 1;
	bool retval = false;

	nValue=0;
	switch (szInt[pos])
	{
	case '-':
		nSign = -1;
	case '+':
		++pos;
	}
	for (; szInt[pos] != '\0'; ++pos)
	{
		if (isdigit(szInt[pos]))
		{
			nValue *= 10;
			nValue += szInt[pos] - '0';
			retval = true;
		}
		else
			break;
	}
	nValue *= nSign;
	return retval;
}


///////////////////////////////////////////////////////////////////////////////
// Util_ConvHex()
///////////////////////////////////////////////////////////////////////////////

bool Util_ConvHex(const UINT uVal, char *szBuffer, const int iDigits)
{

	char	szHexData[17] = "0123456789ABCDEF";
	int		k;
	UINT	n = uVal;

	for (int i=iDigits-1; i>=0; i--)
	{
		k = n % 16;
		szBuffer[i]= szHexData[k];
		n = n / 16;
	}

	szBuffer[iDigits] = '\0';

	if (n)
		return false;							// Left overs!
	else
		return true;

} // Util_ConvHex()


///////////////////////////////////////////////////////////////////////////////
// Util_ConvDec()
///////////////////////////////////////////////////////////////////////////////

bool Util_ConvDec(const char *szHex, int &nDec)
{
	// Really crappy hex conversion
	int i = (int)strlen(szHex) - 1;

	nDec = 0;
	int nMult = 1;
	for (int j = 0; j < 8; ++j)
	{
		if (i < 0)
			break;

		if (szHex[i] >= '0' && szHex[i] <= '9')
			nDec += (szHex[i] - '0') * nMult;
		else if (szHex[i] >= 'A' && szHex[i] <= 'F')
			nDec += (((szHex[i] - 'A'))+10) * nMult;
		else if (szHex[i] >= 'a' && szHex[i] <= 'f')
			nDec += (((szHex[i] - 'a'))+10) * nMult;
		else
		{
			nDec = 0;					// Set value as 0
			return false;
		}

		--i;
		nMult = nMult * 16;
	}

	if (i != -1)
	{
		nDec = 0;
		return false;
	}
	else
		return true;

} // Util_ConvDec()


///////////////////////////////////////////////////////////////////////////////
// Util_IsWinHung()
//
// Safely checks if a window a in a hung state (default timeout is 5000ms)
///////////////////////////////////////////////////////////////////////////////

bool Util_IsWinHung(HWND hWnd, UINT nTimeOut)
{
	DWORD dwResult;

	if (SendMessageTimeout(hWnd, WM_NULL, 0, 0, SMTO_ABORTIFHUNG, nTimeOut, &dwResult))
		return false;
	else
		return true;

} // Util_IsWinHung()


///////////////////////////////////////////////////////////////////////////////
// Util_AttachThreadInput()
//
// Attaches this thread to another input thread - doesn't if hung
///////////////////////////////////////////////////////////////////////////////

void Util_AttachThreadInput(HWND hWnd, bool bAttach)
{
	if (bAttach == true)
	{
		if (Util_IsWinHung(hWnd) == false)
			AttachThreadInput(GetCurrentThreadId(),GetWindowThreadProcessId(hWnd, NULL), TRUE);
	}
	else
		AttachThreadInput(GetCurrentThreadId(),GetWindowThreadProcessId(hWnd, NULL), FALSE);

} // Util_AttachThreadInput()


///////////////////////////////////////////////////////////////////////////////
// Util_MouseDown()
//
// Produces a Mouse Down event
// Buttons recognized:  left|middle|right|main|menu|primary|secondary
///////////////////////////////////////////////////////////////////////////////

int Util_MouseDown(const char *szButton)
{
	char swapped[4];
	DWORD event=0;

	if ( szButton[0] == '\0' || stricmp(szButton, "LEFT")==0 ) // empty string or "left"
		event = MOUSEEVENTF_LEFTDOWN;
	else if ( stricmp(szButton,"RIGHT")==0 )
		event = MOUSEEVENTF_RIGHTDOWN;
	else if ( stricmp(szButton,"MIDDLE")==0 )
		event = MOUSEEVENTF_MIDDLEDOWN;
	else
	{
		Util_RegReadString(HKEY_CURRENT_USER, "Control Panel\\Mouse", "SwapMouseButtons", 4, swapped );
		if ( swapped[0] == '1')	// buttons swapped
		{
			if ( stricmp(szButton,"MAIN")==0 || stricmp(szButton, "PRIMARY"))
				event = MOUSEEVENTF_RIGHTDOWN;
			else if ( stricmp(szButton,"MENU")==0 || stricmp(szButton, "SECONDARY"))
				event = MOUSEEVENTF_LEFTDOWN;
		}
		else
		{
			if ( stricmp(szButton,"MAIN")==0 || stricmp(szButton, "PRIMARY"))
				event = MOUSEEVENTF_LEFTDOWN;
			else if ( stricmp(szButton,"MENU")==0 || stricmp(szButton, "SECONDARY"))
				event = MOUSEEVENTF_RIGHTDOWN;
		}
	}
	if (event != 0) {
		mouse_event(event, 0, 0, 0, 0);
		return 1;
	}
	else
		return 0;

} // Util_MouseDown()


///////////////////////////////////////////////////////////////////////////////
// Util_MouseUp()
//
// Produces a Mouse Down event
///////////////////////////////////////////////////////////////////////////////

int Util_MouseUp(const char *szButton)
{
	char swapped[4];
	DWORD event = 0;	// default to 0 in case no strings match

	if ( szButton[0] == '\0' || stricmp(szButton, "LEFT")==0 ) // empty string or "left"
		event = MOUSEEVENTF_LEFTUP;
	else if ( stricmp(szButton,"RIGHT")==0 )
		event = MOUSEEVENTF_RIGHTUP;
	else if ( stricmp(szButton,"MIDDLE")==0 )
		event = MOUSEEVENTF_MIDDLEUP;
	else
	{
		Util_RegReadString(HKEY_CURRENT_USER, "Control Panel\\Mouse", "SwapMouseButtons", 4, swapped );
		if ( swapped[0] = '1')	// buttons swapped
		{
			if ( stricmp(szButton,"MAIN")==0 || stricmp(szButton, "PRIMARY"))
				event = MOUSEEVENTF_RIGHTUP;
			else if ( stricmp(szButton,"MENU")==0 || stricmp(szButton, "SECONDARY"))
				event = MOUSEEVENTF_LEFTUP;
		}
		else
		{
			if ( stricmp(szButton,"MAIN")==0 || stricmp(szButton, "PRIMARY"))
				event = MOUSEEVENTF_LEFTUP;
			else if ( stricmp(szButton,"MENU")==0 || stricmp(szButton, "SECONDARY"))
				event = MOUSEEVENTF_RIGHTUP;
		}
	}
	if (event != 0) {
		mouse_event(event, 0, 0, 0, 0);
		return 1;
	}
	else
		return 0;

} // Util_MouseUp()


///////////////////////////////////////////////////////////////////////////////
// Util_MouseWheel()
//
// Produces a Mouse Down event
///////////////////////////////////////////////////////////////////////////////

int Util_MouseWheel(const char *szDirection)
{
	if ( stricmp(szDirection,"UP")==0 )
		mouse_event(MOUSEEVENTF_WHEEL, 0, 0, +120, 0);
	else if ( stricmp(szDirection, "DOWN")==0 )
		mouse_event(MOUSEEVENTF_WHEEL, 0, 0, (DWORD)-120, 0);
	else
		return 0;

	return 1;

} // Util_MouseWheel()


///////////////////////////////////////////////////////////////////////////////
// Util_Strncpy()
//
// Copies a given number of characters of a string to a buffer and unlike
// strncpy ensures it is terminated.  The buffer size given should be the
// length of the string PLUS a SINGLE null char - i.e. your entire buffer size.
///////////////////////////////////////////////////////////////////////////////

void Util_Strncpy(char *szBuffer, const char *szString, int nBufSize)
{
	strncpy(szBuffer, szString, nBufSize-1);	// Copy buf-1 characters
	szBuffer[nBufSize-1] = '\0';

} // Util_Strncpy()


///////////////////////////////////////////////////////////////////////////////
// Util_fgets()
//
// Reads is line fo text for files in BINARY mode, cr, crlf and lf are the end
// of line terminators.  BufSize is the size of the buffer INCLUDING the \0
// Data returned is always terminated.
///////////////////////////////////////////////////////////////////////////////

char * Util_fgetsb(char *szBuffer, int nBufSize, FILE *fptr)
{
	int		nMaxPos;
	int		nPos = 0;
	int		ch;

	// If we are already at end of file then return NULL - note feof only returns eof AFTER
	// a read operation has already bounced off the end so make sure we start with a read
	ch = fgetc(fptr);
	if ( ch == EOF )
		return NULL;

	// Work out where the last valid position is and make sure last char in the buffer
	// is terminated
	nMaxPos = nBufSize - 2;

	while (nPos <= nMaxPos)
	{
		if (ch == EOF)
			break;								// Exit loop on end of file

		if (ch == '\r')							// is the next char cr?
		{
			// Is the next char lf?
			ch = fgetc(fptr);					// Get next char
			if (ch == EOF)
				break;							// Exit loop on end of file
			if (ch != '\n')
				fseek(fptr, -1, SEEK_CUR);		// Oops, not a lf, rewind for next time
			break;
		}
		else if (ch == '\n')					// is the next char lf?
			break;								// break, don't store lf
		else
			szBuffer[nPos++] = ch;

		ch = fgetc(fptr);						// Get next char
	}

	// Terminate
	szBuffer[nPos] = '\0';

	return szBuffer;

} // Util_fgetsb()


///////////////////////////////////////////////////////////////////////////////
// Util_Sleep()
//
// A more accurate Sleep(), uses performance counters if possible, otherwise
// just does a normal sleep (timegettime no more accurate than sleep!)
//
// A thread time slice is 10ms on NT, 15ms on NT multiprocessor and 55 ms on
// 9x
///////////////////////////////////////////////////////////////////////////////

void Util_Sleep(int nTimeOut)
{
	// Handle the special cases -1 and 0 first
	if (nTimeOut < 0)
		return;									// No sleep at all for -ve numbers
	else if (nTimeOut == 0)
	{
		::Sleep(0);								// Special case for 0
		return;
	}

	__int64		start, cur, freq;
	double		diff;
	DWORD		dwMin;
	DWORD		dwTimeOut = (DWORD)nTimeOut;

	// Set the minimum Sleep accuracy
	if (g_oVersion.IsWin9x())
		dwMin = 55;
	else
		dwMin = 10;

	// If Sleep is >= dwMin or no performance counters are available then use native Sleep()
	if (dwTimeOut >= dwMin || !QueryPerformanceCounter((LARGE_INTEGER *)&start))
	{
		::Sleep(dwTimeOut);
		return;
	}

	// Get frequency
	QueryPerformanceFrequency((LARGE_INTEGER *)&freq);

	// Note that we must at least do a Sleep(0) once otherwise the SendTo functions get out of
	// sync - especially on 9x/NT4 but also had one report for XP too (seems be on the keydowndelay)
	do
	{
		::Sleep(0);								// Reduce CPU usage - Sleep 0 is special
		QueryPerformanceCounter((LARGE_INTEGER *)&cur);
		diff = ((double)(cur - start) / (double)freq) * 1000.0;

	} while ((DWORD)diff < dwTimeOut);

} // Util_Sleep()



///////////////////////////////////////////////////////////////////////////////
// Util_WinPrintf()
//
// A formatting message box.
//
///////////////////////////////////////////////////////////////////////////////

int Util_WinPrintf(const char *szTitle, const char *szFormat, ...)
{
	char	szBuffer[1024];
	va_list	pArgList;

	va_start(pArgList, szFormat);

	_vsnprintf(szBuffer, sizeof(szBuffer), szFormat, pArgList);

	va_end(pArgList);

	return MessageBox(NULL, szBuffer, szTitle, MB_OK);

} // Util_WinPrintf()


///////////////////////////////////////////////////////////////////////////////
// Util_BGRtoRGB()
//
// Converts a colour from BGR (MS colorref) to RGB
//
///////////////////////////////////////////////////////////////////////////////

void Util_BGRtoRGB(int &nCol)
{
	int nR = (nCol & 0x0000ff) << 16;
	int nG = (nCol & 0x00ff00);
	int nB = (nCol & 0xff0000) >> 16;

	nCol = nR | nG | nB;

} // Util_BGRtoRGB()


///////////////////////////////////////////////////////////////////////////////
// Util_RGBtoBGR()
//
// Converts a colour from RGB to BGR (MS colorref)
//
///////////////////////////////////////////////////////////////////////////////

void Util_RGBtoBGR(int &nCol)
{
	int nR = (nCol & 0xff0000) >> 16;
	int nG = (nCol & 0x00ff00);
	int nB = (nCol & 0x0000ff) << 16;

	nCol = nR | nG | nB;

} // Util_RGBtoBGR()


///////////////////////////////////////////////////////////////////////////////
// Util_DebugMsg()
//
// A formatting debug message
//
///////////////////////////////////////////////////////////////////////////////

#ifdef _DEBUG

void Util_DebugMsg(const char *szFormat, ...)
{
	char	szBuffer[1024];
	va_list	pArgList;

	va_start(pArgList, szFormat);

	_vsnprintf(szBuffer, sizeof(szBuffer), szFormat, pArgList);

	va_end(pArgList);

	OutputDebugString(szBuffer);

} // Util_DebugMsg()

#endif


///////////////////////////////////////////////////////////////////////////////
// Util_StrCpyAlloc()
//
// Copies a string but allocates (new) the memory first - caller must call
// delete [] on the area when no longer required
//
///////////////////////////////////////////////////////////////////////////////

char * Util_StrCpyAlloc(const char *szSource)
{
	char *szNew = new char [strlen(szSource)+1];
	strcpy(szNew, szSource);

	return szNew;

} // Util_StrCpyAlloc()


///////////////////////////////////////////////////////////////////////////////
// Util_EnumResCallback()
///////////////////////////////////////////////////////////////////////////////

BOOL CALLBACK Util_EnumResCallback( HMODULE hExe,   // module handle
									LPCTSTR lpszType,  // resource type
									LPTSTR lpszName,   // resource name
									LPARAM lParam    // application-defined parameter
)
{
// #pragmas are used here to insure that the structure's
// packing in memory matches the packing of the EXE or DLL.
#pragma pack( push, 2 )
typedef struct
{
	BYTE	bWidth;               // Width, in pixels, of the image
	BYTE	bHeight;              // Height, in pixels, of the image
	BYTE	bColorCount;          // Number of colors in image (0 if >=8bpp)
	BYTE	bReserved;            // Reserved
	WORD	wPlanes;              // Color Planes
	WORD	wBitCount;            // Bits per pixel
	DWORD	dwBytesInRes;         // how many bytes in this resource?
	WORD	nID;                  // the ID
} GRPICONDIRENTRY, *LPGRPICONDIRENTRY;

typedef struct
{
	WORD			idReserved;   // Reserved (must be 0)
	WORD			idType;       // Resource type (1 for icons)
	WORD			idCount;      // How many images?
	GRPICONDIRENTRY	idEntries[1]; // The entries for each image
} GRPICONDIR, *LPGRPICONDIR;
#pragma pack( pop )


	HRSRC				hRsrc;
	HGLOBAL				hMem;
	DWORD				nDataLen;
	LPGRPICONDIR		pDirHeader;
	BYTE*				pData;
	unsigned int		k;
	char				szReqResName[6], szResName[6];

	sprintf(szResName, "%d", (int)lpszName);				// Convert requested ID into a string
	sprintf(szReqResName, "%d", (int)lParam);				// Convert requested ID into a string

	if (stricmp(szReqResName, szResName))
		return TRUE;							// Not the right resource ID, get the next one

	hRsrc = FindResource(hExe, lpszName, RT_GROUP_ICON);
	hMem = LoadResource(hExe, hRsrc);
	pDirHeader = (LPGRPICONDIR)LockResource(hMem);

	for (k = 0; k < pDirHeader->idCount; k++)
	{
		hRsrc = FindResource(hExe, MAKEINTRESOURCE(pDirHeader->idEntries[k].nID), RT_ICON);
		hMem = LoadResource(hExe, hRsrc );

		nDataLen = SizeofResource( hExe, hRsrc );
		pData = (unsigned char *)LockResource(hMem);

		if (pDirHeader->idEntries[k].bWidth == g_IconFind_nWidth && pDirHeader->idEntries[k].bHeight == g_IconFind_nHeight &&
				pDirHeader->idEntries[k].wBitCount == g_IconFind_nDepth)
		{
			g_IconFind_hIcon = CreateIconFromResourceEx(pData, nDataLen, TRUE, 0x00030000, g_IconFind_nWidth, g_IconFind_nHeight, LR_DEFAULTCOLOR);
			break;					// Stop the for loop
		}
	}

    return FALSE;					// stop enumeration

} // Util_EnumResCallback()


///////////////////////////////////////////////////////////////////////////////
// Util_LoadIcon()
//
// Same as LoadImage() for icons but allows you to specify a particular colour
// depth to return.  If the requested icon cannot be found the OS decides which
// icon to use - rarely what is wanted.
//
// Must call DestroyIcon on HICONs returned from this call
//
///////////////////////////////////////////////////////////////////////////////

HICON Util_LoadIcon(int nIconID, int nWidth, int nHeight, int nDepth)
{
	// If a depth of -1 was specified then just do a usual LoadImage and let the OS decide the
	// best depth
	if (nDepth == -1)
		return (HICON)LoadImage(g_hInstance, MAKEINTRESOURCE(nIconID), IMAGE_ICON, nWidth, nHeight, LR_DEFAULTCOLOR);

	g_IconFind_hIcon = NULL;
	g_IconFind_nWidth = nWidth;
	g_IconFind_nHeight = nHeight;
	g_IconFind_nDepth = nDepth;

	EnumResourceNames(NULL, RT_GROUP_ICON, Util_EnumResCallback, (LPARAM)nIconID);

	// Did we get a match?  If we didn't then just let the OS load any old icon
	if (g_IconFind_hIcon == NULL)
		return (HICON)LoadImage(g_hInstance, MAKEINTRESOURCE(nIconID), IMAGE_ICON, nWidth, nHeight, LR_DEFAULTCOLOR);
	else
		return g_IconFind_hIcon;

} // Util_LoadIcon()


///////////////////////////////////////////////////////////////////////////////
// Util_ConvSystemTime()
//
///////////////////////////////////////////////////////////////////////////////

bool Util_ConvSystemTime(const char *szTime, SYSTEMTIME *st, bool bDate, int nSep)
{
	char		szTemp[4+1];

	// Set LOCAL date/time defaults
	GetLocalTime(st);

	// Must at least have a year in the time (or completely blank)
	if ((szTime[0] == '\0') || (strlen(szTime) < 4))
		return false;

	if (bDate)
	{	// convert date
		strncpy(szTemp, szTime, 4);
		szTemp[4] = '\0';
		st->wYear = (WORD)atoi(szTemp);
		szTime += 4+nSep;

		if (strlen(szTime) >= 2)
		{
			strncpy(szTemp, szTime, 2);
			szTemp[2] = '\0';
			st->wMonth = (WORD)atoi(szTemp);
			szTime += 2+nSep;

			if (strlen(szTime) >= 2)
			{
				strncpy(szTemp, szTime, 2);
				szTemp[2] = '\0';
				st->wDay = (WORD)atoi(szTemp);
				szTime += 2;
				if (strlen(szTime)>0)
					szTime += nSep;
			}
		}
	}

	// convert time
	if (strlen(szTime) >= 2)
	{
		strncpy(szTemp, szTime, 2);
		szTemp[2] = '\0';
		st->wHour = (WORD)atoi(szTemp);
		szTime += 2+nSep;

		if (strlen(szTime) >= 2)
		{
			strncpy(szTemp, szTime, 2);
			szTemp[2] = '\0';
			st->wMinute = (WORD)atoi(szTemp);
			szTime += 2+nSep;

			if (strlen(szTime) >= 2)
			{
				strncpy(szTemp, szTime, 2);
				szTemp[2] = '\0';
				st->wSecond = (WORD)atoi(szTemp);
			}
		}
	}

	return true;

} // Util_ConvSystemTime()


///////////////////////////////////////////////////////////////////////////////
// Util_IsSpace
//
// Returns 1 if a char is 0x09 - 0x0D or 0x20.  The C runtime isspace() is unreliable
// on non UK/US characters sets and no windows version exists.
//
///////////////////////////////////////////////////////////////////////////////

int Util_IsSpace(int c)
{
	if ( (c >= 0x09 && c <= 0x0D) || (c == 0x20) )
		return 1;
	else
		return 0;

} // Util_IsSpace()


///////////////////////////////////////////////////////////////////////////////
// Util_ANSItoUNICODE()
//
// Converts an ANSI string into UNICODE - the memory for the UNICODE string is
// automatically allocated - caller must call
// delete [] on the area when no longer required.
//
// nMinLen is the minimum number of wide characters (including /0) to allocate
// for the return buffer.  Default is 0.
//
///////////////////////////////////////////////////////////////////////////////

wchar_t * Util_ANSItoUNICODE(const char *szANSI, int nMinLen)
{
	wchar_t *szUNI;

	// First find out how many wide chars we need
	int nLen = MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, szANSI, -1, NULL, 0);

	if (nLen == 0)
		return NULL;							// Unable to convert (unlikely)

	// Allocate our buffer (must be at least nMinLen chars)
	if (nMinLen > nLen)
		szUNI = new wchar_t[nMinLen];
	else
		szUNI = new wchar_t[nLen];

	// Do the conversion proper
	MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, szANSI, -1, szUNI, nLen);

	return szUNI;

} // Util_ANSItoUNICODE()


///////////////////////////////////////////////////////////////////////////////
// Util_UNICODEtoANSI()
//
// Converts a UNICODE string into ANSI - the memory for the ANSI string is
// automatically allocated - caller must call
// delete [] on the area when no longer required.
//
// nMinLen is the minimum number of characters (including /0) to allocate
// for the return buffer.  Default is 0.
//
///////////////////////////////////////////////////////////////////////////////

char * Util_UNICODEtoANSI(const wchar_t *szUNI, int nMinLen)
{
	char *szANSI;

	// First find out how many wide chars we need
	int nLen = WideCharToMultiByte(CP_ACP, 0, szUNI, -1, NULL, 0, NULL, NULL);

	if (nLen == 0)
		return NULL;							// Unable to convert (unlikely)

	// Allocate our buffer (must be at least nMinLen chars)
	if (nMinLen > nLen)
		szANSI = new char[nMinLen];
	else
		szANSI = new char[nLen];

	// Do the conversion proper
	WideCharToMultiByte(CP_ACP, 0, szUNI, -1, szANSI, nLen, NULL, NULL);

	return szANSI;

} // Util_UNICODEtoANSI()


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
// script_win.cpp
//
// Contains window related functions.  Part of script.cpp
//
///////////////////////////////////////////////////////////////////////////////


// Includes
#include "StdAfx.h"								// Pre-compiled headers

#ifndef _MSC_VER								// Includes for non-MS compilers
	#include <windows.h>
	#include <limits.h>
	#include <commctrl.h>
#endif

#include "AutoIt.h"								// Autoit values, macros and config options

#include "globaldata.h"
#include "script.h"
#include "shared_memory.h"
#include "utility.h"
#include "regexp.h"
#include "resources\resource.h"


///////////////////////////////////////////////////////////////////////////////
// Check if a winwait-style operation is in progress, process and returns
// true.  If no processing required returns false
//
// Just for kicks, this also processes the Sleep() command
///////////////////////////////////////////////////////////////////////////////

bool AutoIt_Script::Win_HandleWinWait(void)
{
	// Any winwait or Sleep commands to process?
	if (m_nCurrentOperation != AUT_SLEEP && m_nCurrentOperation != AUT_WINWAIT &&
		m_nCurrentOperation != AUT_WINWAITCLOSE && m_nCurrentOperation != AUT_WINWAITACTIVE &&
		m_nCurrentOperation != AUT_WINWAITNOTACTIVE)
		return false;

	// Idle a little to remove CPU usage
	Sleep(AUT_IDLE);


	// If required, process the timeout
	if (m_nWinWaitTimeout != 0)
	{
		// Get current time in ms
		DWORD	dwDiff;
		DWORD	dwCur = timeGetTime();
		if (dwCur < m_tWinTimerStarted)
			dwDiff = (UINT_MAX - m_tWinTimerStarted) + dwCur; // timer wraps at 2^32
		else
			dwDiff = dwCur - m_tWinTimerStarted;

		// Timer elapsed?
		if (dwDiff >= m_nWinWaitTimeout)
		{
			if (m_nCurrentOperation != AUT_SLEEP)
				m_vUserRetVal = 0;				// We timed out (default = 1, Sleep gives no error)

			m_bUserFuncReturned = true;			// Request exit from Execute()
			m_nCurrentOperation = AUT_RUN;		// Continue script
			return true;
		}
	}


	// Perform relevant command
	bool bRes = false;
	switch (m_nCurrentOperation)
	{
		case AUT_WINWAITACTIVE:
			bRes = Win_WinActive();
			break;
		case AUT_WINWAITNOTACTIVE:
			bRes = !Win_WinActive();
			break;
		case AUT_WINWAIT:
			bRes = Win_WinExists();
			break;
		case AUT_WINWAITCLOSE:
			bRes = !Win_WinExists();
			break;

		case AUT_SLEEP:
			break;								// Don't do anything, timer processed above
	}


	// Wait Command successful?
	if (bRes == true)
	{
		m_bUserFuncReturned = true;			// Request exit from Execute()
		m_nCurrentOperation = AUT_RUN;		// Continue script
		Util_Sleep(m_nWinWaitDelay);		// Briefly pause before continuing
	}

	return true;

} // Win_HandleWinWait()


///////////////////////////////////////////////////////////////////////////////
// Win_WindowSearchInit()
///////////////////////////////////////////////////////////////////////////////

void AutoIt_Script::Win_WindowSearchInit(VectorVariant &vParams)
{
	// Parameters are title, text - only title is mandatory

	m_vWindowSearchTitle	= vParams[0];	// Title

	if (vParams.size() >= 2)
		m_vWindowSearchText	= vParams[1];	// Text
	else
		m_vWindowSearchText	= "";

} // Win_WindowSearchInit()


///////////////////////////////////////////////////////////////////////////////
// Win_WindowWaitInit()
///////////////////////////////////////////////////////////////////////////////

void AutoIt_Script::Win_WindowWaitInit(VectorVariant &vParams)
{
	// Parameters are title, text, timeout - only title is mandatory

	// Setup the search for title/text
	Win_WindowSearchInit(vParams);

	if (vParams.size() == 3)
		m_nWinWaitTimeout	= vParams[2].nValue() * 1000;	// Timeout
	else
		m_nWinWaitTimeout	= 0;


	// Make a note of current system time for comparision in timer
	m_tWinTimerStarted			= timeGetTime();

} // Win_WindowWaitInit()


///////////////////////////////////////////////////////////////////////////////
// Win_WindowSearchDeleteList()
//
// When the window search is run a list of windows matched is created, this
// function clears the list.
//
///////////////////////////////////////////////////////////////////////////////

void AutoIt_Script::Win_WindowSearchDeleteList(void)
{
	WinListNode	*lpTemp = m_lpWinListFirst;
	WinListNode	*lpNext;

	while (lpTemp)
	{
		lpNext = lpTemp->lpNext;
		delete lpTemp;
		lpTemp = lpNext;
	}

	m_lpWinListFirst	= NULL;
	m_lpWinListLast		= NULL;
	m_nWinListCount		= 0;

} // Win_WindowSearchDeleteList()


///////////////////////////////////////////////////////////////////////////////
// Win_WindowSearchAddToList()
//
// Adds a hwnd to the window list
//
///////////////////////////////////////////////////////////////////////////////

void AutoIt_Script::Win_WindowSearchAddToList(HWND hWnd)
{
	WinListNode	*lpTemp = new WinListNode;

	lpTemp->hWnd = hWnd;
	lpTemp->lpNext = NULL;

	if (m_lpWinListLast)
	{
		m_lpWinListLast->lpNext = lpTemp;
		m_lpWinListLast = lpTemp;
	}
	else
		m_lpWinListFirst = m_lpWinListLast = lpTemp;

	++m_nWinListCount;

} // Win_WindowSearchAddToList()


///////////////////////////////////////////////////////////////////////////////
// Win_WindowSearch()
///////////////////////////////////////////////////////////////////////////////

bool AutoIt_Script::Win_WindowSearch(bool bFirstOnly)
{
	// Are we looking for all matching windows or just the first?
	m_bWindowSearchFirstOnly = bFirstOnly;

	// Clear previous search - also set number of found windows (m_nWinListCount) to 0
	Win_WindowSearchDeleteList();

	// If the Title parameter was a HWND type then we don't need to search at all
	if (m_vWindowSearchTitle.isHWND())
	{
		if (IsWindow(m_vWindowSearchTitle.hWnd()) )
		{
			m_WindowSearchHWND = m_vWindowSearchTitle.hWnd();
			Win_WindowSearchAddToList(m_WindowSearchHWND);	// Create a 1 entry list for those functions that use it
			return true;
		}
		else
			return false;
	}

	// If both title and text is blank, then assume active window
	// If we are in mode 4 then do some other checks
	if (m_nWindowSearchMatchMode == 4)
	{
		if (m_vWindowSearchTitle.szValue()[0] == '\0' || !stricmp(m_vWindowSearchTitle.szValue(), "last") )
		{
			if (m_WindowSearchHWND)					// Make sure that there HAS been a previous match
			{
				Win_WindowSearchAddToList(m_WindowSearchHWND);	// Create a 1 entry list for those functions that use it
				return true;
			}
			else
				return false;
		}
		else if (!stricmp(m_vWindowSearchTitle.szValue(), "active") )
		{
			m_WindowSearchHWND = GetForegroundWindow();
			Win_WindowSearchAddToList(m_WindowSearchHWND);	// Create a 1 entry list for those functions that use it
			return true;
		}
		else if ( !strnicmp(m_vWindowSearchTitle.szValue(), "handle=", 7) )	// handle=
		{
			int		nTemp;

			// Assumes int32 is big enough for HWND (it currently is... 4 bytes)
			// We can always bump up to 64 if required in the future (IA64?)
			Util_ConvDec( &m_vWindowSearchTitle.szValue()[7], nTemp );
			m_WindowSearchHWND = (HWND)nTemp;

			if (IsWindow(m_WindowSearchHWND) )
			{
				Win_WindowSearchAddToList(m_WindowSearchHWND);	// Create a 1 entry list for those functions that use it
				return true;
			}
			else
				return false;
		}
	}
	else if (m_vWindowSearchTitle.szValue()[0] == '\0'  && m_vWindowSearchText.szValue()[0] == '\0' )
	{
		m_WindowSearchHWND = GetForegroundWindow();
		Win_WindowSearchAddToList(m_WindowSearchHWND);	// Create a 1 entry list for those functions that use it
		return true;
	}

	// Do the search
	if (!m_bWinSearchChildren)
		EnumWindows((WNDENUMPROC)Win_WindowSearchProc, 0);
	else
		EnumChildWindows(GetDesktopWindow(), (WNDENUMPROC)Win_WindowSearchProc, 0);

	if (m_nWinListCount)
	{
		// Set the m_WindowSearchHWND to the FIRST window matched to retain compability with the older code
		// that uses WindowSearch and doesn't know anything about the new WinList struct
		m_WindowSearchHWND = m_lpWinListFirst->hWnd;
		return true;
	}
	else
	{
		m_WindowSearchHWND = NULL;
		return false;
	}
}

BOOL CALLBACK AutoIt_Script::Win_WindowSearchProc(HWND hWnd, LPARAM lParam)
{
	return g_oScript.Win_WindowSearchProcHandler(hWnd, lParam);
}

BOOL AutoIt_Script::Win_WindowSearchProcHandler(HWND hWnd, LPARAM lParam)
{
	char	szBuffer[1024+1] = "";				// 1024 chars is more than enough for a title

	// Get the window text
	GetWindowText(hWnd, szBuffer, 1024);

	m_WindowSearchHWND = hWnd;					// Save the handle of the window for use in the WinTextSearch()

	switch (m_nWindowSearchMatchMode)
	{
		case 1:
			if ( !strncmp(m_vWindowSearchTitle.szValue(), szBuffer, strlen(m_vWindowSearchTitle.szValue()) ) )
			{
				if ( Win_WindowSearchText() == true && m_bWindowSearchFirstOnly == true)
					return FALSE;				// No need to search any more
			}
			break;

		case 2:
			if ( strstr(szBuffer, m_vWindowSearchTitle.szValue()) != NULL )
			{
				if ( Win_WindowSearchText() == true && m_bWindowSearchFirstOnly == true)
					return FALSE;				// No need to search any more
			}
			break;

		case 3:
			if ( !strcmp(szBuffer, m_vWindowSearchTitle.szValue()) )
			{
				if ( Win_WindowSearchText() == true && m_bWindowSearchFirstOnly == true)
					return FALSE;				// No need to search any more
			}
			break;

		case 4:
			// valid options are "classname=", "handle=", "", "all", "regexp="
			if ( !strnicmp(m_vWindowSearchTitle.szValue(), "classname=", 10) )	// classname=
			{
				GetClassName(hWnd, szBuffer, 1024);
				if (!strcmp(szBuffer, &m_vWindowSearchTitle.szValue()[10]) )
				{
					if ( Win_WindowSearchText() == true && m_bWindowSearchFirstOnly == true)
						return FALSE;			// No need to search any more
				}
			}
			else if ( !stricmp(m_vWindowSearchTitle.szValue(), "all") )
			{
				//AUT_MSGBOX("here", "here")
				// We don't care about the title, so try matching the text
				if ( Win_WindowSearchText() == true && m_bWindowSearchFirstOnly == true)
					return FALSE;				// No need to search any more
			}
			else if ( !strncmp(m_vWindowSearchTitle.szValue(), szBuffer, strlen(m_vWindowSearchTitle.szValue())) ) // Try default case 1
			{
				if ( Win_WindowSearchText() == true && m_bWindowSearchFirstOnly == true)
					return FALSE;				// No need to search any more
			}
			break;

	} // End Switch

	return TRUE;								// Search more

} // Win_WindowSearchProcHandler()


///////////////////////////////////////////////////////////////////////////////
// Win_WindowSearchText()
///////////////////////////////////////////////////////////////////////////////

bool AutoIt_Script::Win_WindowSearchText(void)
{
	// If the optional text is blank, always return find=yes
	if (m_vWindowSearchText.szValue()[0] == '\0')
	{
		Win_WindowSearchAddToList(m_WindowSearchHWND);
		return true;	// Found!
	}

	// If current search window seems hung then don't attempt to read it (WM_GETTEXT would hang)
	// If we are not using the WM_GETEXT mode then we don't care if it's hung as GetWindowText()
	// will cope
	if ( m_nWindowSearchTextMode == 1 && Util_IsWinHung(m_WindowSearchHWND) )
		return false;

	int nLastCount = m_nWinListCount;

	EnumChildWindows(m_WindowSearchHWND, (WNDENUMPROC)Win_WindowSearchTextProc, 0);

	if (nLastCount != m_nWinListCount)
		return true;							// New window matched
	else
		return false;
}

BOOL CALLBACK AutoIt_Script::Win_WindowSearchTextProc(HWND hWnd, LPARAM lParam)
{
	return g_oScript.Win_WindowSearchTextProcHandler(hWnd, lParam);
}

BOOL AutoIt_Script::Win_WindowSearchTextProcHandler(HWND hWnd, LPARAM lParam)
{
	char	szBuffer[AUT_WINTEXTBUFFER+1];
	int		nLen;

	// WM_GETTEXT seems to get more info
	szBuffer[0] = '\0';							// Blank in case of error with WM_GETTEXT

	// Hidden text?
	if ( (IsWindowVisible(hWnd)) || (m_bDetectHiddenText == true) )
	{
		if (m_nWindowSearchTextMode == 2)
		{
			nLen = GetWindowText(hWnd, szBuffer, AUT_WINTEXTBUFFER);	// Quicker mode
		}
		else //if (SendMessage(hWnd, WM_GETTEXTLENGTH, 0, 0) > 0)
		{
			nLen = (int)SendMessage(hWnd, WM_GETTEXT,(WPARAM)AUT_WINTEXTBUFFER,(LPARAM)szBuffer);
		}

		szBuffer[AUT_WINTEXTBUFFER] = '\0';		// Ensure terminated if large amount of return text

		if ( nLen && strstr(szBuffer, m_vWindowSearchText.szValue()) )
		{
			Win_WindowSearchAddToList(m_WindowSearchHWND);
			return FALSE;						// No more searching needed in this window
		}

	}	// EndIf Visible

	return TRUE;								// Carry on searching

} // Win_WindowSearchTextProcHandler()


///////////////////////////////////////////////////////////////////////////////
// WinWait( "title" [,"text"] [,timeout]] )
///////////////////////////////////////////////////////////////////////////////

AUT_RESULT AutoIt_Script::F_WinWait(VectorVariant &vParams, Variant &vResult)
{
	Win_WindowWaitInit(vParams);
	m_vUserRetVal = 1;					// Default return value is 1
	m_nCurrentOperation = AUT_WINWAIT;
	Execute();
	vResult = m_vUserRetVal;			// Get return value (0 = timed out)

	return AUT_OK;

} // F_WinWait()


///////////////////////////////////////////////////////////////////////////////
// WinWaitActive( "title" [,"text"] [,timeout]] )
///////////////////////////////////////////////////////////////////////////////

AUT_RESULT AutoIt_Script::F_WinWaitActive(VectorVariant &vParams, Variant &vResult)
{
	Win_WindowWaitInit(vParams);
	m_vUserRetVal = 1;					// Default return value is 1
	m_nCurrentOperation = AUT_WINWAITACTIVE;
	Execute();
	vResult = m_vUserRetVal;			// Get return value (0 = timed out)

	return AUT_OK;

} // F_WinWaitActive()


///////////////////////////////////////////////////////////////////////////////
// WinWaitNotActive( "title" [,"text"] [,timeout]] )
///////////////////////////////////////////////////////////////////////////////

AUT_RESULT AutoIt_Script::F_WinWaitNotActive(VectorVariant &vParams, Variant &vResult)
{
	Win_WindowWaitInit(vParams);
	m_vUserRetVal = 1;					// Default return value is 1
	m_nCurrentOperation = AUT_WINWAITNOTACTIVE;
	Execute();
	vResult = m_vUserRetVal;			// Get return value (0 = timed out)

	return AUT_OK;

} // F_WinWaitNotActive()


///////////////////////////////////////////////////////////////////////////////
// WinWaitClose( "title" [,"text"] [,timeout]] )
///////////////////////////////////////////////////////////////////////////////

AUT_RESULT AutoIt_Script::F_WinWaitClose(VectorVariant &vParams, Variant &vResult)
{
	Win_WindowWaitInit(vParams);
	m_vUserRetVal = 1;					// Default return value is 1
	m_nCurrentOperation = AUT_WINWAITCLOSE;
	Execute();
	vResult = m_vUserRetVal;			// Get return value (0 = timed out)

	return AUT_OK;

} // F_WinWaitClose()


///////////////////////////////////////////////////////////////////////////////
// WinActive( "title" [,"text"] )
///////////////////////////////////////////////////////////////////////////////

AUT_RESULT AutoIt_Script::F_WinActive(VectorVariant &vParams, Variant &vResult)
{
	Win_WindowSearchInit(vParams);
	if (Win_WinActive() == false)
		vResult = 0;					// Default is 1

	return AUT_OK;

} // F_WinActive()


///////////////////////////////////////////////////////////////////////////////
// Win_WinActive()
// Must call Win_WindowSearchInit() first to set title and text to search for
///////////////////////////////////////////////////////////////////////////////

bool AutoIt_Script::Win_WinActive(void)
{
	// If the window doesn't exist it can't be active
	if ( Win_WindowSearch() == false)
		return false;

	if (m_WindowSearchHWND == GetForegroundWindow())
		return true;
	else
		return false;

} // Win_WinActive()


///////////////////////////////////////////////////////////////////////////////
// WinExists( "title" [,"text"] )
///////////////////////////////////////////////////////////////////////////////

AUT_RESULT AutoIt_Script::F_WinExists(VectorVariant &vParams, Variant &vResult)
{
	Win_WindowSearchInit(vParams);
	if (Win_WinExists() == false)
		vResult = 0;					// Default is 1

	return AUT_OK;

} // F_WinExists()


///////////////////////////////////////////////////////////////////////////////
// Win_WinExists()
// Must call Win_WindowSearchInit() first to set title and text to search for
///////////////////////////////////////////////////////////////////////////////

bool AutoIt_Script::Win_WinExists(void)
{
	return Win_WindowSearch();

} // Win_WinExists()



///////////////////////////////////////////////////////////////////////////////
// WinActivate( "title" [,"text"] )
///////////////////////////////////////////////////////////////////////////////

AUT_RESULT AutoIt_Script::F_WinActivate(VectorVariant &vParams, Variant &vResult)
{
	Win_WindowSearchInit(vParams);

	if (Win_WindowSearch() == false)
		return AUT_OK;							// No window to activate

	g_oSetForeWinEx.Activate(m_WindowSearchHWND);
	Util_Sleep(m_nWinWaitDelay);				// Briefly pause before continuing

	return AUT_OK;

} // F_WinActivate()


///////////////////////////////////////////////////////////////////////////////
// WinShow( "title" ,"text", flag )
///////////////////////////////////////////////////////////////////////////////

AUT_RESULT AutoIt_Script::F_WinShow(VectorVariant &vParams, Variant &vResult)
{
	Win_WindowSearchInit(vParams);

	if (Win_WindowSearch() == false)
		return AUT_OK;							// Required window not found

	int nFlag = vParams[2].nValue();

	if (nFlag == SW_ENABLE)
		EnableWindow(m_WindowSearchHWND, TRUE);
	else if (nFlag == SW_DISABLE)
		EnableWindow(m_WindowSearchHWND, FALSE);
	else
	{
		ShowWindow(m_WindowSearchHWND, nFlag);
		Util_Sleep(m_nWinWaitDelay);				// Briefly pause before continuing
	}

	return AUT_OK;

} // F_WinShow()


///////////////////////////////////////////////////////////////////////////////
// WinMove()
// Moves a window
///////////////////////////////////////////////////////////////////////////////

AUT_RESULT AutoIt_Script::F_WinMove(VectorVariant &vParams, Variant &vResult)
{
	uint	iNumParams = vParams.size();
	int		nWidth, nHeight;
	RECT	rect;

	Win_WindowSearchInit(vParams);

	if (Win_WindowSearch() == false)
		return AUT_OK;									// Required window not found

	GetWindowRect(m_WindowSearchHWND, &rect);

	if (iNumParams < 5)
		nWidth = rect.right - rect.left;
	else
		nWidth = vParams[4].nValue();

	if (iNumParams < 6)
		nHeight = rect.bottom - rect.top;
	else
		nHeight = vParams[5].nValue();

	MoveWindow(m_WindowSearchHWND, vParams[2].nValue(), vParams[3].nValue(), nWidth, nHeight, TRUE);
	//Util_Sleep(m_nWinWaitDelay);				// Briefly pause before continuing

	return AUT_OK;

} // WinMove()


///////////////////////////////////////////////////////////////////////////////
// WinClose()
///////////////////////////////////////////////////////////////////////////////

AUT_RESULT AutoIt_Script::F_WinClose(VectorVariant &vParams, Variant &vResult)
{
	Win_WindowSearchInit(vParams);

	if (Win_WindowSearch() == false)
		return AUT_OK;							// Required window not found

	PostMessage(m_WindowSearchHWND, WM_CLOSE, 0, 0L);
	Util_Sleep(m_nWinWaitDelay);				// Briefly pause before continuing

	return AUT_OK;

} // WinClose()


///////////////////////////////////////////////////////////////////////////////
// WinKill()
// Closes a window - uses more force than WinClose
///////////////////////////////////////////////////////////////////////////////

AUT_RESULT AutoIt_Script::F_WinKill(VectorVariant &vParams, Variant &vResult)
{
	Win_WindowSearchInit(vParams);

	if (Win_WindowSearch() == false)
		return AUT_OK;							// Required window not found

	Util_WinKill(m_WindowSearchHWND);
	Util_Sleep(m_nWinWaitDelay);				// Briefly pause before continuing

	return AUT_OK;

} // WinKill()


///////////////////////////////////////////////////////////////////////////////
// WinSetTitle()
// Changes the text in a window title
///////////////////////////////////////////////////////////////////////////////

AUT_RESULT AutoIt_Script::F_WinSetTitle(VectorVariant &vParams, Variant &vResult)
{
	Win_WindowSearchInit(vParams);

	if (Win_WindowSearch() == false)
		return AUT_OK;									// Required window not found

	SetWindowText( m_WindowSearchHWND, vParams[2].szValue() );

	return AUT_OK;

} // WinSetTitle()


///////////////////////////////////////////////////////////////////////////////
// Win_WinGetTitle()
// Must call Win_WindowSearchInit() first to set title and text to search for
// Gets the Full title of a window
///////////////////////////////////////////////////////////////////////////////

AUT_RESULT AutoIt_Script::F_WinGetTitle(VectorVariant &vParams, Variant &vResult)
{
	char	szBuffer[AUT_WINTEXTBUFFER+1];

	Win_WindowSearchInit(vParams);

	if (Win_WindowSearch() == false)
		return AUT_OK;									// Required window not found

	GetWindowText(m_WindowSearchHWND, szBuffer, AUT_WINTEXTBUFFER);

	vResult = szBuffer;

	return AUT_OK;

} // Win_WinGetTitle()


///////////////////////////////////////////////////////////////////////////////
// Win_WinGetText()
// Must call Win_WindowSearchInit() first to set title and text to search for
// Gets the static text from a window
///////////////////////////////////////////////////////////////////////////////

AUT_RESULT AutoIt_Script::F_WinGetText(VectorVariant &vParams, Variant &vResult)
{
	// $var = WinGetText(<title>, [<text>])
	Win_WindowSearchInit(vParams);

	if (Win_WindowSearch() == false)
		return AUT_OK;									// Required window not found

	vResult = Util_GetWinText(m_WindowSearchHWND, m_bDetectHiddenText);

	return AUT_OK;

} // Win_WinGetText()


///////////////////////////////////////////////////////////////////////////////
// ControlSearch() - requires a title, text and controlname
///////////////////////////////////////////////////////////////////////////////

bool AutoIt_Script::ControlSearch(VectorVariant &vParams)
{
	// Init the window search and see if it exists
	m_vWindowSearchTitle	= vParams[0];
	m_vWindowSearchText		= vParams[1];

	if (Win_WindowSearch() == false)
		return false;							// Required window not found


	// Zero control search params
	m_vControlSearchValue		= vParams[2];	// The Control ID/Class or Text to find
	m_iControlSearchInstance	= 0;			// Variable to keep track of class instance
	m_bControlSearchFoundFlag	= false;		// Found nothing yet


	// We have a valid parent window, if the control to search for is a HWND then we can return straight away
	// This used to ignore title/text and use GetParent() but it messed up sometimes.
	if (m_vControlSearchValue.isHWND())
	{
		m_ControlSearchHWND = m_vControlSearchValue.hWnd();
		return true;
	}


	// If the class name is blank, return the main window handle as the control handle...
	if (m_vControlSearchValue.isTrue() == false)	// Blank string is false, and 0 is false
	{
		m_ControlSearchHWND = m_WindowSearchHWND;
		return true;
	}

	// If the class is a number - assume Control ID, otherwise try classnameNN and then text
	if (m_vControlSearchValue.isNumber())
	{
		m_nControlSearchMethod = AUT_CONTROLSEARCH_ID;
		EnumChildWindows(m_WindowSearchHWND, (WNDENUMPROC)ControlSearchProc, 0);
	}
	else
	{
		m_nControlSearchMethod = AUT_CONTROLSEARCH_CLASS;
		EnumChildWindows(m_WindowSearchHWND, (WNDENUMPROC)ControlSearchProc, 0);

		if (m_bControlSearchFoundFlag == false)
		{
			m_nControlSearchMethod = AUT_CONTROLSEARCH_TEXT;
			EnumChildWindows(m_WindowSearchHWND, (WNDENUMPROC)ControlSearchProc, 0);
		}
	}

	return m_bControlSearchFoundFlag;
}

BOOL CALLBACK AutoIt_Script::ControlSearchProc(HWND hWnd, LPARAM lParam)
{
	return g_oScript.ControlSearchProcHandler(hWnd, lParam);
}

BOOL AutoIt_Script::ControlSearchProcHandler(HWND hWnd, LPARAM lParam)
{
	char	szBuffer[1024+1];
	BOOL	bRes = TRUE;						// Return TRUE to continue enumeration

	// Determine the search method to use, ClassNN, ID or Text
	if (m_nControlSearchMethod == AUT_CONTROLSEARCH_ID)
	{
		if (m_vControlSearchValue.nValue() == GetDlgCtrlID(hWnd))
			bRes = FALSE;
	}
	else if (m_nControlSearchMethod == AUT_CONTROLSEARCH_CLASS)
	{
		GetClassName(hWnd, szBuffer, 256);

		if ( strncmp(m_vControlSearchValue.szValue(), szBuffer, strlen(szBuffer)) == 0 )
		{
			m_iControlSearchInstance++;				//Control name found, increment instance

			sprintf(szBuffer, "%s%u", szBuffer, m_iControlSearchInstance);

			if ( strcmp(szBuffer, m_vControlSearchValue.szValue()) == 0 )	//Do we match control name AND num
				bRes = FALSE;
		}
	}
	else										// Use window text
	{
		GetWindowText(hWnd, szBuffer, 1024);

		if ( strcmp(m_vControlSearchValue.szValue(), szBuffer) == 0 )
			bRes = FALSE;
	}

	if (bRes == FALSE)
	{
		m_ControlSearchHWND			= hWnd;	// Save the hwnd of this control
		m_bControlSearchFoundFlag	= true;	// Set the found flag
	}

	return bRes;

} // ControlSearchProcHandler()


///////////////////////////////////////////////////////////////////////////////
// WinSetOnTop()
// Must call Win_WindowSearchInit() first to set title and text to search for
// Renders a window TOPMOST or NOTOPMOST
///////////////////////////////////////////////////////////////////////////////

AUT_RESULT AutoIt_Script::F_WinSetOnTop(VectorVariant &vParams, Variant &vResult)
{
	Win_WindowSearchInit(vParams);

	if (Win_WindowSearch() == false)
		return AUT_OK;									// Required window not found

	if ( vParams[2].nValue() == 1 )							// 1 = TopMost... else NoTopMost
		SetWindowPos(m_WindowSearchHWND, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE);
	else
		SetWindowPos(m_WindowSearchHWND, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE);

	return AUT_OK;

} // WinSetOnTop()


///////////////////////////////////////////////////////////////////////////////
// WinGetPos()
//
// $array[4] = WinGetPos("title" [,"text"])
//
///////////////////////////////////////////////////////////////////////////////

AUT_RESULT AutoIt_Script::F_WinGetPos(VectorVariant &vParams, Variant &vResult)
{
	RECT	rect;
	Variant	*pvTemp;

	Win_WindowSearchInit(vParams);

	if (Win_WindowSearch() == false)
	{
		SetFuncErrorCode(1);
		return AUT_OK;									// Required window not found
	}

	GetWindowRect(m_WindowSearchHWND, &rect);	// Load the window stats

	// Setup vResult as an Array to hold the 4 values we want to return
	Util_VariantArrayDim(&vResult, 4);

	pvTemp = Util_VariantArrayGetRef(&vResult, 0);	// First element
	*pvTemp = (int)rect.left;					// X

	pvTemp = Util_VariantArrayGetRef(&vResult, 1);
	*pvTemp = (int)rect.top;					// Y

	pvTemp = Util_VariantArrayGetRef(&vResult, 2);
	*pvTemp = (int)(rect.right - rect.left);	// Width

	pvTemp = Util_VariantArrayGetRef(&vResult, 3);
	*pvTemp = (int)(rect.bottom - rect.top);	// Height

	return AUT_OK;

} // WinGetPos()


///////////////////////////////////////////////////////////////////////////////
// ControlFocus()
///////////////////////////////////////////////////////////////////////////////

AUT_RESULT AutoIt_Script::F_ControlFocus(VectorVariant &vParams, Variant &vResult)
{
	if ( ControlSearch(vParams) == false )
	{
		vResult = 0;
		return AUT_OK;									// Required control not found
	}

	Util_AttachThreadInput(m_WindowSearchHWND, true);

	if ( SetFocus(m_ControlSearchHWND) == NULL)
		vResult = 0;			// Error

	Util_AttachThreadInput(m_WindowSearchHWND, false);

	return AUT_OK;

} // ControlFocus()


///////////////////////////////////////////////////////////////////////////////
// ControlClick()
// ControlClick("title,"text", "control", [button], [numclicks])
///////////////////////////////////////////////////////////////////////////////

AUT_RESULT AutoIt_Script::F_ControlClick(VectorVariant &vParams, Variant &vResult)
{
	uint	iNumParams = vParams.size();
	int		nClicks;
	UINT	msgdown, msgup;
	WPARAM	wParam;
	RECT	rect;
	LPARAM	lParam;

	if ( ControlSearch(vParams) == false )
	{
		vResult = 0;
		return AUT_OK;									// Required control not found
	}

	Util_AttachThreadInput(m_WindowSearchHWND, true);

	SetActiveWindow(m_WindowSearchHWND);		// See BM_CLICK documentation, applies to this too

	// Get the dimensions of the control so we can click the centre of it (maybe safer and more natural than 0,0)
	GetWindowRect(m_ControlSearchHWND, &rect);
	lParam = MAKELPARAM( (rect.right - rect.left) / 2, (rect.bottom - rect.top) / 2);

	// Get number of clicks
	if (iNumParams >= 5)
		nClicks = vParams[4].nValue();
	else
		nClicks = 1;

	// Default is left button
	msgdown = WM_LBUTTONDOWN;
	msgup	= WM_LBUTTONUP;
	wParam	= MK_LBUTTON;

	if (iNumParams >= 4)
	{
		if (!stricmp(vParams[3].szValue(), "RIGHT"))
		{
			msgdown = WM_RBUTTONDOWN;
			msgup	= WM_RBUTTONUP;
			wParam	= MK_RBUTTON;
		}
		else if (!stricmp(vParams[3].szValue(), "MIDDLE"))
		{
			msgdown = WM_MBUTTONDOWN;
			msgup	= WM_MBUTTONUP;
			wParam	= MK_MBUTTON;
		}
	}

	for (int i=0; i<nClicks; ++i)
	{
		PostMessage( m_ControlSearchHWND, msgdown, wParam, lParam);
		//Sleep(m_nMouseClickDownDelay); - causing failures?
		PostMessage( m_ControlSearchHWND, msgup, 0, lParam);
		Util_Sleep(m_nMouseClickDelay);
	}

	Util_AttachThreadInput(m_WindowSearchHWND, false);

	return AUT_OK;

} // ControlClick()


///////////////////////////////////////////////////////////////////////////////
// ControlSetText()
///////////////////////////////////////////////////////////////////////////////

AUT_RESULT AutoIt_Script::F_ControlSetText(VectorVariant &vParams, Variant &vResult)
{
	if ( ControlSearch(vParams) == false )
	{
		vResult = 0;
		return AUT_OK;									// Required control not found
	}

	if ( SendMessage(m_ControlSearchHWND,WM_SETTEXT, 0, (LPARAM)vParams[3].szValue()) != TRUE)
		vResult = 0;			// Error

	return AUT_OK;

} // ControlSetText()



///////////////////////////////////////////////////////////////////////////////
// Win_ControlGetText()
///////////////////////////////////////////////////////////////////////////////

AUT_RESULT AutoIt_Script::F_ControlGetText(VectorVariant &vParams, Variant &vResult)
{
	char	szBuffer[AUT_WINTEXTBUFFER+1];

	vResult = "";								// In this case set the default to be a blank string

	if ( ControlSearch(vParams) == false )
	{
		SetFuncErrorCode(1);
		return AUT_OK;									// Required control not found
	}

	if ( !(SendMessage(m_ControlSearchHWND,WM_GETTEXT,(WPARAM)AUT_WINTEXTBUFFER, (LPARAM)szBuffer) > 0) )
		SetFuncErrorCode(1);			// Error
	else
		vResult = szBuffer;

	return AUT_OK;

} // ControlGetText()


///////////////////////////////////////////////////////////////////////////////
// Win_ControlGetPos()
///////////////////////////////////////////////////////////////////////////////

AUT_RESULT AutoIt_Script::F_ControlGetPos(VectorVariant &vParams, Variant &vResult)
{
	Variant	*pvTemp;
	POINT	point;
	RECT	rect;

	if ( ControlSearch(vParams) == false )
	{
		SetFuncErrorCode(1);
		return AUT_OK;									// Required control not found
	}

	if ( GetWindowRect(m_ControlSearchHWND, &rect) )	// Load the window stats
	{
		point.x = rect.left;
		point.y = rect.top;

		ScreenToClient(m_WindowSearchHWND,&point);

		// Setup vResult as an Array to hold the 4 values we want to return
		Util_VariantArrayDim(&vResult, 4);

		pvTemp = Util_VariantArrayGetRef(&vResult, 0);		// First element
		*pvTemp = (int)point.x;					// X

		pvTemp = Util_VariantArrayGetRef(&vResult, 1);
		*pvTemp = (int)point.y;					// Y

		pvTemp = Util_VariantArrayGetRef(&vResult, 2);
		*pvTemp = (int)(rect.right - rect.left);		// Width

		pvTemp = Util_VariantArrayGetRef(&vResult, 3);
		*pvTemp = (int)(rect.bottom - rect.top);		// Height

	}
	else
		SetFuncErrorCode(1);			// Error

	return AUT_OK;

} // ControlGetPos()


///////////////////////////////////////////////////////////////////////////////
// ControlMove()
///////////////////////////////////////////////////////////////////////////////

AUT_RESULT AutoIt_Script::F_ControlMove(VectorVariant &vParams, Variant &vResult)
{
	uint	iNumParams = vParams.size();
	int		nWidth, nHeight;
	RECT	rect;

	if ( ControlSearch(vParams) == false )
	{
		vResult = 0;
		return AUT_OK;									// Required control not found
	}

	Util_AttachThreadInput(m_WindowSearchHWND, true);

	if ( GetWindowRect(m_ControlSearchHWND, &rect) )	// Load the window stats
	{
		if (iNumParams < 6)
			nWidth = rect.right - rect.left;
		else
			nWidth = vParams[5].nValue();

		if (iNumParams < 7)
			nHeight = rect.bottom - rect.top;
		else
			nHeight = vParams[6].nValue();

		MoveWindow(m_ControlSearchHWND, vParams[3].nValue(), vParams[4].nValue(), nWidth, nHeight, true);
	}
	else
	{
		vResult = 0;
		return AUT_OK;
	}

	Util_AttachThreadInput(m_WindowSearchHWND, false);

	return AUT_OK;

} // ControlMove()


///////////////////////////////////////////////////////////////////////////////
// ControlEnable()
///////////////////////////////////////////////////////////////////////////////

AUT_RESULT AutoIt_Script::F_ControlEnable(VectorVariant &vParams, Variant &vResult)
{
	if ( ControlSearch(vParams) == false )
	{
		vResult = 0;
		return AUT_OK;									// Required control not found
	}

	EnableWindow(m_ControlSearchHWND,TRUE);

	return AUT_OK;

} // ControlEnable()


///////////////////////////////////////////////////////////////////////////////
// Win_ControlDisable()
///////////////////////////////////////////////////////////////////////////////

AUT_RESULT AutoIt_Script::F_ControlDisable(VectorVariant &vParams, Variant &vResult)
{
	if ( ControlSearch(vParams) == false )
	{
		vResult = 0;
		return AUT_OK;									// Required control not found
	}

	EnableWindow(m_ControlSearchHWND,FALSE);

	return AUT_OK;

} // ControlDisable()


///////////////////////////////////////////////////////////////////////////////
// ControlShow()
///////////////////////////////////////////////////////////////////////////////

AUT_RESULT AutoIt_Script::F_ControlShow(VectorVariant &vParams, Variant &vResult)
{
	if ( ControlSearch(vParams) == false )
	{
		vResult = 0;
		return AUT_OK;									// Required control not found
	}

	ShowWindow(m_ControlSearchHWND,SW_SHOWNOACTIVATE);

	return AUT_OK;

} // ControlShow()


///////////////////////////////////////////////////////////////////////////////
// ControlHide()
///////////////////////////////////////////////////////////////////////////////

AUT_RESULT AutoIt_Script::F_ControlHide(VectorVariant &vParams, Variant &vResult)
{
	if ( ControlSearch(vParams) == false )
	{
		vResult = 0;
		return AUT_OK;									// Required control not found
	}

	ShowWindow(m_ControlSearchHWND,SW_HIDE);

	return AUT_OK;

} // ControlHide()


///////////////////////////////////////////////////////////////////////////////
// ControlCommand()
// - ControlCommand(<title>,<text>,<control ref>,<cmd>,<extra>,<extra>)
// - cmds:
// - - ShowDropDown (drops a combo list)
// - - HideDropDown (UNdrops a combo list)
// - - AddString, string (adds a string to the end of a combo list)
// - - DelString, occurrence (deletes a string according to occurence)
// - - TabLeft, (navigates to the next tab to the left of a SysTabControl32)
// - - TabRight, (navigates to the next tab to the right of a SysTabControl32)
// - - CurrentTab, (returns the current tab in focus of a SysTabControl32)
// - - FindString, string (returns occurence ref of the exact string)
// - - GetCurrentSelection, occurrence (sets selection to occurrence ref)
// - - SetCurrentSelection, occurrence (sets selection to occurrence ref)
// - - SelectString, string (sets selection according to string)
// - - IsChecked (returns 1 if Button is checked)
// - - IsVisible (returns 1 if Control is Visible)
// - - IsEnabled (returns 1 if Control is Enabled)
// - - Check (Checks Button)
// - - UnCheck (Unchecks Button)
// - - GetLineCount (Returns # of lines in and Edit)
// - - EditPaste (Pastes text into current cursor pos)
// - - GetCurrentLine (Returns line # of current (caret pos) line in and Edit)
// - - GetCurrentCol (Returns column # of current (caret pos) line in an Edit)
// - - GetLine, # (Returns text line # passed)
//
// Return value depends on the command, @error is used to indicate errors.
///////////////////////////////////////////////////////////////////////////////

AUT_RESULT AutoIt_Script::F_ControlCommand(VectorVariant &vParams,  Variant &vResult)
{
	char	szBuffer[AUT_WINTEXTBUFFER+1];
	const char	*szCmd;
	UINT		vMsg = 0;
	UINT		xMsg = 0;
	UINT		yMsg = 0;
	RECT		rect;
	LPARAM		lParam;
	Variant		vTemp;

	if ( ControlSearch(vParams) == false )
	{
		SetFuncErrorCode(1);
		return AUT_OK;									// Required control not found
	}


	// Make sure that any unused parameters are ""
	vTemp = "";
	while (vParams.size() < 5)
		vParams.push_back(vTemp);


	Util_AttachThreadInput(m_WindowSearchHWND, true);

	// Easy access to the command parameter
	szCmd = vParams[3].szValue();

	while (1)
	{
		if ( strcmpi(szCmd,"ISVISIBLE")==0 )
		{
			if ( IsWindowVisible(m_ControlSearchHWND) )
				vResult = 1;
			else
				vResult = 0;
			break;							// ISENABLED performed, exit switch
		}
		if ( strcmpi(szCmd,"ISENABLED")==0 )
		{
			if ( IsWindowEnabled(m_ControlSearchHWND) )
				vResult = 1;
			else
				vResult = 0;
			break;							// ISENABLED performed, exit switch
		}
		if ( strcmpi(szCmd,"TABLEFT")==0 )
		{// must be a Tab Control
			PostMessage(m_ControlSearchHWND, WM_KEYDOWN, VK_LEFT, (LPARAM)( (MapVirtualKey(VK_LEFT, 0)<<16) | 0x00000001 ) );
			Sleep(0);
			PostMessage(m_ControlSearchHWND, WM_KEYUP, VK_LEFT, (LPARAM)( (MapVirtualKey(VK_LEFT, 0)<<16) | 0xC0000001 ) );
			break;							// TABLEFT performed, exit switch
		}
		if ( strcmpi(szCmd,"TABRIGHT")==0 )
		{// must be a Tab Control
			PostMessage(m_ControlSearchHWND,WM_KEYDOWN,VK_RIGHT, (LPARAM)( (MapVirtualKey(VK_LEFT, 0)<<16) | 0x00000001 ) );
			Sleep(0);
			PostMessage(m_ControlSearchHWND,WM_KEYUP,VK_RIGHT, (LPARAM)( (MapVirtualKey(VK_LEFT, 0)<<16) | 0xC0000001 ));
			break;							// TABRIGHT performed, exit switch
		}
		if ( strcmpi(szCmd,"CURRENTTAB")==0 )
		{// must be a Tab Control
			int nTab = (int)SendMessage(m_ControlSearchHWND,TCM_GETCURSEL,vParams[4].nValue(),0);
			if ( nTab==-1 )
				SetFuncErrorCode(1);
			else
			{
				nTab++;
				vResult = nTab;
			}
			break;							// CURRENTTAB performed, exit switch
		}

		if ( strcmpi(szCmd,"SHOWDROPDOWN")==0 )
		{// must be a ComboBox
			if ( !(SendMessage(m_ControlSearchHWND, CB_SHOWDROPDOWN, (WPARAM)TRUE, 0)) )
				SetFuncErrorCode(1);
			break;							// SHOWDROPDOWN performed, exit switch
		}

		if ( strcmpi(szCmd,"HIDEDROPDOWN")==0 )
		{// must be a ComboBox
			if ( !(SendMessage(m_ControlSearchHWND, CB_SHOWDROPDOWN, (WPARAM)FALSE, 0)) )
				SetFuncErrorCode(1);
			break;							// HIDEDROPDOWN performed, exit switch
		}

		if ( strcmpi(szCmd,"ADDSTRING")==0 )
		{
			if ( strnicmp(vParams[2].szValue(),"Combo",5)==0 )
				vMsg = CB_ADDSTRING;
			if ( strnicmp(vParams[2].szValue(),"List",4)==0 )
				vMsg = LB_ADDSTRING;
			if ( vMsg )
			{// Must be ComboBox or ListBox
				if ( !(SendMessage(m_ControlSearchHWND, vMsg, 0, (LPARAM)vParams[4].szValue())) )
					SetFuncErrorCode(1);
			}
			break;							// ADDSTRING performed, exit switch
		}

		if ( strcmpi(szCmd,"DELSTRING")==0 )
		{
			if ( strnicmp(vParams[2].szValue(),"Combo",5)==0 )
				vMsg = CB_DELETESTRING;
			if ( strnicmp(vParams[2].szValue(),"List",4)==0 )
				vMsg = LB_DELETESTRING;
			if ( vMsg )
			{// Must be ComboBox or ListBox
				if ( !(SendMessage(m_ControlSearchHWND, vMsg, (WPARAM)vParams[4].nValue(), 0)) )
					SetFuncErrorCode(1);
			}
			break;							// DELSTRING performed, exit switch
		}

		if ( strcmpi(szCmd,"FINDSTRING")==0 )
		{
			if ( strnicmp(vParams[2].szValue(),"Combo",5)==0 )
				vMsg = CB_FINDSTRINGEXACT;
			if ( strnicmp(vParams[2].szValue(),"List",4)==0 )
				vMsg = LB_FINDSTRINGEXACT;
			if ( vMsg )
			{// Must be ComboBox or ListBox
				vResult = (int)SendMessage(m_ControlSearchHWND, vMsg, (WPARAM)1, (LPARAM)vParams[4].szValue());
				if ( vResult.nValue() == -1 )
					SetFuncErrorCode(1);
			}
			else
				SetFuncErrorCode(1);
			break;							// FINDSTRING performed, exit switch
		}

		if ( strcmpi(szCmd,"SETCURRENTSELECTION")==0 )
		{
			if ( strnicmp(vParams[2].szValue(),"Combo",5)==0 )
			{
				vMsg = CB_SETCURSEL;
				xMsg = CBN_SELCHANGE;
				yMsg = CBN_SELENDOK;
			}
			if ( strnicmp(vParams[2].szValue(),"List",4)==0 )
			{
				vMsg = LB_SETCURSEL;
				xMsg = LBN_SELCHANGE;
				yMsg = LBN_DBLCLK;
			}
			if ( vMsg )
			{// Must be ComboBox or ListBox
				if ( SendMessage(m_ControlSearchHWND, vMsg, (WPARAM)vParams[4].nValue(), 0) == -1 )
					SetFuncErrorCode(1);
				else
				{
					SendMessage(GetParent(m_ControlSearchHWND),WM_COMMAND,(WPARAM)MAKELONG(GetDlgCtrlID(m_ControlSearchHWND),xMsg),(LPARAM)m_ControlSearchHWND);
					SendMessage(GetParent(m_ControlSearchHWND),WM_COMMAND,(WPARAM)MAKELONG(GetDlgCtrlID(m_ControlSearchHWND),yMsg),(LPARAM)m_ControlSearchHWND);
				}
			}
			break;							// SETCURRENTSELECTION performed, exit switch
		}

		if ( stricmp(szCmd,"GETCURRENTSELECTION")==0 )
		{
			if ( strnicmp(vParams[2].szValue(),"ComboBox",8)==0 )
			{
				vMsg = CB_GETCURSEL;
				xMsg = CB_GETLBTEXTLEN;
				yMsg = CB_GETLBTEXT;
			}
			if ( strnicmp(vParams[2].szValue(),"ListBox",7)==0 )
			{
				vMsg = LB_GETCURSEL;
				xMsg = LB_GETTEXTLEN;
				yMsg = LB_GETTEXT;
			}
			if ( vMsg )
			{// Must be ComboBox or ListBox
				int          nIndex, nLen;
				nIndex = (int)SendMessage(m_ControlSearchHWND, vMsg, 0, 0);
				if ( nIndex == -1)
					SetFuncErrorCode(1);
				else
				{
					nLen = (int)SendMessage(m_ControlSearchHWND,xMsg,(WPARAM)nIndex,0);
					if ( nLen == -1 )
						SetFuncErrorCode(1);
					else
					{
						char     *pBuffer = NULL;
						nLen++;
						pBuffer=(char*)calloc(256+nLen,1);
						nLen = (int)SendMessage(m_ControlSearchHWND,yMsg,(WPARAM)nIndex,(LPARAM)pBuffer);
						if ( nLen == -1 )
								SetFuncErrorCode(1);
						else
								vResult = pBuffer;
						if(pBuffer)
								free(pBuffer);
					}
				}
			}
			break;     // GETCURRENTSELECTION performed, exit switch
		}

		if ( strcmpi(szCmd,"SELECTSTRING")==0 )
		{
			if ( strnicmp(vParams[2].szValue(),"ComboBox",8)==0 )
			{
				vMsg = CB_SELECTSTRING;
				xMsg = CBN_SELCHANGE;
				yMsg = CBN_SELENDOK;
			}
			if ( strnicmp(vParams[2].szValue(),"ListBox",7)==0 )
			{
				vMsg = LB_SELECTSTRING;
				xMsg = LBN_SELCHANGE;
				yMsg = LBN_DBLCLK;
			}
			if ( vMsg )
			{// Must be ComboBox or ListBox
				if ( SendMessage(m_ControlSearchHWND, vMsg, (WPARAM)1, (LPARAM)vParams[4].szValue()) == -1 )
					SetFuncErrorCode(1);
				else
				{
					SendMessage(GetParent(m_ControlSearchHWND),WM_COMMAND,(WPARAM)MAKELONG(GetDlgCtrlID(m_ControlSearchHWND),xMsg),(LPARAM)m_ControlSearchHWND);
					SendMessage(GetParent(m_ControlSearchHWND),WM_COMMAND,(WPARAM)MAKELONG(GetDlgCtrlID(m_ControlSearchHWND),yMsg),(LPARAM)m_ControlSearchHWND);
				}
			}
			break;							// SELECTSTRING performed, exit switch
		}

		if ( strcmpi(szCmd,"ISCHECKED")==0  )
		{//Must be a Button
			if ( SendMessage(m_ControlSearchHWND,BM_GETCHECK, 0, 0) == BST_CHECKED )
				vResult = 1;				// Is checked (0 is default/not checked)
			else
				vResult = 0;
			break;							// ISCHECKED performed, exit switch
		}

		if ( strcmpi(szCmd,"CHECK")==0 )
		{//Must be a Button
			// Only send the "check" if the button is not already checked
			if ( SendMessage(m_ControlSearchHWND,BM_GETCHECK, 0, 0) == BST_UNCHECKED )
			{
				//SendMessage(m_ControlSearchHWND,BM_SETCHECK,(WPARAM)BST_CHECKED, 0);
				//SendMessage(GetParent(m_ControlSearchHWND),WM_COMMAND,(WPARAM)MAKELONG(GetDlgCtrlID(m_ControlSearchHWND),BN_CLICKED),(LPARAM)m_ControlSearchHWND);
				SetActiveWindow(m_WindowSearchHWND);		// See BM_CLICK docs, applies to this too
				GetWindowRect(m_ControlSearchHWND, &rect);	// Code to primary click the centre of the control
				lParam = MAKELPARAM( (rect.right - rect.left) / 2, (rect.bottom - rect.top) / 2);
				PostMessage(m_ControlSearchHWND, WM_LBUTTONDOWN, MK_LBUTTON, lParam);
				PostMessage(m_ControlSearchHWND, WM_LBUTTONUP, 0, lParam);
			}
			break;							// CHECK performed, exit switch
		}

		if ( strcmpi(szCmd,"UNCHECK")==0 )
		{//Must be a Button
			// Only send the "uncheck" if the button is not already unchecked
			if ( SendMessage(m_ControlSearchHWND,BM_GETCHECK, 0, 0) == BST_CHECKED )
			{
				//SendMessage(m_ControlSearchHWND,BM_SETCHECK,(WPARAM)BST_UNCHECKED, 0);
				//SendMessage(GetParent(m_ControlSearchHWND),WM_COMMAND,(WPARAM)MAKELONG(GetDlgCtrlID(m_ControlSearchHWND),BN_CLICKED),(LPARAM)m_ControlSearchHWND);
				SetActiveWindow(m_WindowSearchHWND);		// See BM_CLICK docs, applies to this too
				GetWindowRect(m_ControlSearchHWND, &rect);	// Code to primary click the centre of the control
				lParam = MAKELPARAM( (rect.right - rect.left) / 2, (rect.bottom - rect.top) / 2);
				PostMessage(m_ControlSearchHWND, WM_LBUTTONDOWN, MK_LBUTTON, lParam);
				PostMessage(m_ControlSearchHWND, WM_LBUTTONUP, 0, lParam);
			}
				break;							// UNCHECK performed, exit switch
		}

		if ( strcmpi(szCmd,"GETSELECTED")==0 )
		{//Must be an Edit
			UINT	nLen,nStart,nEnd;
			char	*pBuffer = NULL;

			SendMessage(m_ControlSearchHWND,EM_GETSEL,(WPARAM)&nStart,(LPARAM)&nEnd);
			if (nStart!=nEnd)
			{
				if ( (nLen = (int)SendMessage(m_ControlSearchHWND,WM_GETTEXTLENGTH,0 , 0)) )
				{
					pBuffer=(char*)calloc(256+nLen,1);
					if ( SendMessage(m_ControlSearchHWND,WM_GETTEXT,(WPARAM)(nLen+1), (LPARAM)pBuffer) && nEnd <= AUT_WINTEXTBUFFER )
					{
						if (nEnd != nLen )
							pBuffer[nEnd]='\0';
						vResult = pBuffer+nStart;
					}
					else
						SetFuncErrorCode(1);
				}
				else
					SetFuncErrorCode(1);
			}
			else
				SetFuncErrorCode(1);
			if(pBuffer)
				free(pBuffer);
			break;							// GETSELECTED performed, exit switch
		}

		if ( strcmpi(szCmd,"GETLINECOUNT")==0 )
		{//Must be an Edit
			vResult = (int)SendMessage(m_ControlSearchHWND,EM_GETLINECOUNT, 0, 0);
			break;							// GETLINECOUNT performed, exit switch
		}

		if ( strcmpi(szCmd,"GETCURRENTLINE")==0 )
		{
			vResult = (int)SendMessage(m_ControlSearchHWND,EM_LINEFROMCHAR, (WPARAM)-1, 0)+1;
			break;
		}

		if ( strcmpi(szCmd,"GETCURRENTCOL")==0 )
		{
			uint nStart, nEnd, nOriginal;
			int  nLine;

			SendMessage(m_ControlSearchHWND,EM_GETSEL,(WPARAM)&nStart,(LPARAM)&nEnd);
			nOriginal = nStart;  //the charcter index

			//Decrement the character index until the row changes
			//Difference between this char index and original is the column.

			nLine = (int)SendMessage(m_ControlSearchHWND,EM_LINEFROMCHAR,(WPARAM)nStart,0);
			if (nLine >= 1)
			{
				while (nLine == (int)SendMessage(m_ControlSearchHWND,EM_LINEFROMCHAR,(WPARAM)nStart,0))
				{
					nStart--;
				}
				vResult = (int)nOriginal - (int)nStart;  //no off-by one error :)
			}
			else
				vResult = (int)nStart + 1;  //add 1 because first char index is 0
			break;
		}

		if ( strcmpi(szCmd,"EDITPASTE")==0 )
		{
			SendMessage(m_ControlSearchHWND,EM_REPLACESEL, TRUE, (LPARAM)vParams[4].szValue());
			break;
		}
		if ( strcmpi(szCmd,"GETLINE")==0 )
		{
			int nFoo;
			*((LPINT)szBuffer) = sizeof(szBuffer);

			nFoo = (int)SendMessage(m_ControlSearchHWND, EM_GETLINE, (WPARAM)vParams[4].nValue()-1, (LPARAM)szBuffer);
			if ( nFoo )
			{
				szBuffer[nFoo]='\0';
				vResult = szBuffer;
			}
			else
				SetFuncErrorCode(1);
			break;							// GETLINE performed, exit switch
		}

		// If we get to here, no command was matched, or there was an error during a commmand
		SetFuncErrorCode(1);
		break;
	}

	Util_AttachThreadInput(m_WindowSearchHWND, false);

	return AUT_OK;

} // ControlCommand()


///////////////////////////////////////////////////////////////////////////////
// ControlSend()
// Needs more info in the LPARAM part of the PostMessage...
///////////////////////////////////////////////////////////////////////////////

AUT_RESULT AutoIt_Script::F_ControlSend(VectorVariant &vParams, Variant &vResult)
{
	if ( ControlSearch(vParams) == false )
	{
		vResult = 0;
		return AUT_OK;									// Required control not found
	}

	// Send the keys
	if (vParams.size() >= 5 && vParams[4].nValue() != 0)
		m_oSendKeys.SendRaw(vParams[3].szValue(), m_ControlSearchHWND);
	else
		m_oSendKeys.Send(vParams[3].szValue(), m_ControlSearchHWND);

	return AUT_OK;

} // ControlSend()


///////////////////////////////////////////////////////////////////////////////
// WinMenuSelectItem()
///////////////////////////////////////////////////////////////////////////////

AUT_RESULT AutoIt_Script::F_WinMenuSelectItem(VectorVariant &vParams, Variant &vResult)
{
	uint	iNumParams = vParams.size();
	char	szBuffer[AUT_WINTEXTBUFFER+1];
	UINT	c,i,nCount;
	UINT	nId = 0;
	BOOL	bFound;
	HMENU	hMenu;

	Win_WindowSearchInit(vParams);

	if (Win_WindowSearch() == false)
	{
		vResult = 0;								// Default is 1
		return AUT_OK;								// Required window not found
	}

	if ( !(hMenu = GetMenu(m_WindowSearchHWND)) )
	{
		vResult = 0;								// Default is 1
		return AUT_OK;								// Required menu not found
	}

	for (i=3; i<=iNumParams; i++)
	{
		if ( (nCount=GetMenuItemCount(hMenu))<=0)
		{
			vResult = 0;							// Default is 1
			return AUT_OK;							// Required menu items not found
		}
		bFound=FALSE;
		for ( c=0 ;c<=nCount-1; c++ )
		{
			GetMenuString(hMenu,c,szBuffer,AUT_WINTEXTBUFFER,MF_BYPOSITION);
			if ( strncmp(vParams[i-1].szValue(),szBuffer,strlen(vParams[i-1].szValue()))==0 )
			{
				if ( i == iNumParams )
				{
					nId = GetMenuItemID(hMenu,c);
					bFound=TRUE;
					break;
				}
				else
				{
					if ( !(hMenu = GetSubMenu(hMenu,c)) )
					{
						vResult = 0;				// Default is 1
						return AUT_OK;				// Required submenu not found
					}
					else
					{
						bFound=TRUE;
						break;
					}
				}
			}
		}
		if ( !bFound )
		{
			vResult = 0;							// Default is 1
			return AUT_OK;							// ERROR
		}
	}

	if ( nId == (UINT)-1 )
		vResult = 0;								// Default is 1
	else
	{
		Util_AttachThreadInput(m_WindowSearchHWND, true);
		PostMessage(m_WindowSearchHWND,WM_COMMAND,(WPARAM)nId,0);
		Util_AttachThreadInput(m_WindowSearchHWND, false);
	}

	return AUT_OK;

} // WinMenuSelectItem()


///////////////////////////////////////////////////////////////////////////////
// WinGetClassList()
// Must call Win_WindowSearchInit() first to set title and text to search for
// Gets the static text from a window
///////////////////////////////////////////////////////////////////////////////

AUT_RESULT AutoIt_Script::F_WinGetClassList(VectorVariant &vParams, Variant &vResult)
{
	// $var = WinGetClassList(<title>, [<text>])
	Win_WindowSearchInit(vParams);

	if (Win_WindowSearch() == false)
	{
		SetFuncErrorCode(1);
		return AUT_OK;									// Required window not found
	}

	vResult = Util_GetClassList(m_WindowSearchHWND);

	return AUT_OK;

} // WinGetClassList()


///////////////////////////////////////////////////////////////////////////////
// WinGetClientSize()
///////////////////////////////////////////////////////////////////////////////

AUT_RESULT AutoIt_Script::F_WinGetClientSize(VectorVariant &vParams, Variant &vResult)
{
	RECT	rect;
	Variant	*pvTemp;

	Win_WindowSearchInit(vParams);

	if (Win_WindowSearch() == false)
	{
		SetFuncErrorCode(1);
		return AUT_OK;									// Required window not found
	}

	if ( GetClientRect(m_WindowSearchHWND, &rect) )	// Load the window stats
	{
		// Setup vResult as an Array to hold the 4 values we want to return
		Util_VariantArrayDim(&vResult, 2);

		pvTemp = Util_VariantArrayGetRef(&vResult, 0);	// First element
		*pvTemp = (int)(rect.right - rect.left);	// Width

		pvTemp = Util_VariantArrayGetRef(&vResult, 1);
		*pvTemp = (int)(rect.bottom - rect.top);	// Height
	}
	else
		SetFuncErrorCode(1);

	return AUT_OK;

} // WinGetClientSize()


///////////////////////////////////////////////////////////////////////////////
// WinGetHandle()
///////////////////////////////////////////////////////////////////////////////

AUT_RESULT AutoIt_Script::F_WinGetHandle(VectorVariant &vParams, Variant &vResult)
{
//	char	szTemp[65+1];			// Big enough to hold __int64

	Win_WindowSearchInit(vParams);

	if (Win_WindowSearch() == false)
	{
		SetFuncErrorCode(1);
		vResult = "";
		return AUT_OK;									// Required window not found
	}

//	sprintf(szTemp, "%p", m_WindowSearchHWND);
//	vResult = szTemp;

	vResult = m_WindowSearchHWND;

	return AUT_OK;

} // WinGetHandle()


///////////////////////////////////////////////////////////////////////////////
// ControlGetHandle()
///////////////////////////////////////////////////////////////////////////////

AUT_RESULT AutoIt_Script::F_ControlGetHandle(VectorVariant &vParams, Variant &vResult)
{
	if ( ControlSearch(vParams) == false )
	{
		vResult = "";
		return AUT_OK;									// Required control not found
	}

	vResult = m_ControlSearchHWND;

	return AUT_OK;

} // ControlGetHandle()


///////////////////////////////////////////////////////////////////////////////
// ControlWithFocus()
///////////////////////////////////////////////////////////////////////////////

void AutoIt_Script::ControlWithFocus(HWND hWnd, Variant &vResult)
{
	char	szClass[256];


	Util_AttachThreadInput(hWnd, true);
	m_ControlSearchHWND=GetFocus();				// Get control with focus' hWnd
	Util_AttachThreadInput(hWnd, false);

	if(!m_ControlSearchHWND)
	{
		SetFuncErrorCode(1);
		return;
	}

	GetClassName(m_ControlSearchHWND, szClass, 255);
	m_vControlSearchValue		= szClass;		// Set the class to find

	m_iControlSearchInstance	= 0;			// Variable to keep track of class instance
	m_bControlSearchFoundFlag	= false;		// Found nothing yet

	EnumChildWindows(hWnd, (WNDENUMPROC)ControlWithFocusProc, 0);

	if (m_bControlSearchFoundFlag)
	{
		sprintf(szClass,"%s%d",m_vControlSearchValue.szValue(),m_iControlSearchInstance);
		vResult=szClass;
	}
	else
		SetFuncErrorCode(1);

}

BOOL CALLBACK AutoIt_Script::ControlWithFocusProc(HWND hWnd, LPARAM lParam)
{
	return g_oScript.ControlWithFocusProcHandler(hWnd, lParam);
}

BOOL AutoIt_Script::ControlWithFocusProcHandler(HWND hWnd, LPARAM lParam)
{
	char	szBuffer[256];

	GetClassName(hWnd, szBuffer, 255);

	if ( strcmp(m_vControlSearchValue.szValue(), szBuffer) == 0 )
	{
		m_iControlSearchInstance++;				//Control name found, increment instance

		if ( hWnd==m_ControlSearchHWND )	//Do we match control hWnd
		{
			m_bControlSearchFoundFlag	= true;	// Set the found flag
			return FALSE;						// End the search/enumeration
		}
	}

	return TRUE;								// Continue the search

} // ControlWithFocusProcHandler()


///////////////////////////////////////////////////////////////////////////////
// F_ControlGetFocus()
///////////////////////////////////////////////////////////////////////////////

AUT_RESULT AutoIt_Script::F_ControlGetFocus(VectorVariant &vParams, Variant &vResult)
{
	// Set default text as "" for error
	vResult = "";

	Win_WindowSearchInit(vParams);

	if (Win_WindowSearch() == false)
		SetFuncErrorCode(1);
	else
		ControlWithFocus(m_WindowSearchHWND, vResult);

	return AUT_OK;

} // ControlGetFocus()


///////////////////////////////////////////////////////////////////////////////
// WinGetCaretPos()
//
// pos = WinGetCaretPos()
// Gets the caret position of the foreground window
// Trying to get the caret position of a chosen window seems unreliable
// Coordinates are relative to the window its in (This gets funky in MDI's but the correct
// coordinates can be determined by some scripting to fix the offfset)
///////////////////////////////////////////////////////////////////////////////

AUT_RESULT AutoIt_Script::F_WinGetCaretPos(VectorVariant &vParams, Variant &vResult)
{
	POINT point, ptOrigin;
	Variant *pvTemp;
	HWND hWnd = GetForegroundWindow();

	// Doesn't work without attaching
	Util_AttachThreadInput(hWnd, true);

	if (GetCaretPos(&point) == FALSE)
		SetFuncErrorCode(1);
	else
	{
		// point contains the caret pos in CLIENT area coordinates, convert to screen (absolute coords)
		// and then let the current mode decide how they will be returned
		ClientToScreen(hWnd, &point);
		ConvertCoords(m_nCoordCaretMode, ptOrigin);
		point.x -= ptOrigin.x;
		point.y -= ptOrigin.y;

		Util_VariantArrayDim(&vResult, 2);

		pvTemp = Util_VariantArrayGetRef(&vResult, 0);		// First element
		*pvTemp = (int)point.x;					// X

		pvTemp = Util_VariantArrayGetRef(&vResult, 1);
		*pvTemp = (int)point.y;					// Y
	}

	Util_AttachThreadInput(hWnd, false);

	return AUT_OK;

} // WinGetCaretPos()


///////////////////////////////////////////////////////////////////////////////
// WinGetState()
//
// state = WinGetState("title", "text))
// 0 = not found
// 1 = exists
// 2 = visible
// 4 = enabled
// 8 = active
// 16 = minimized
// 32 = maximized
// 64 =
///////////////////////////////////////////////////////////////////////////////

AUT_RESULT AutoIt_Script::F_WinGetState(VectorVariant &vParams, Variant &vResult)
{
	int		nState = 1;

	Win_WindowSearchInit(vParams);

	if (Win_WindowSearch() == false)
	{
		vResult = 0;							// Default is 1
		SetFuncErrorCode(1);
		return AUT_OK;
	}

	// Is it visible?
	if (IsWindowVisible(m_WindowSearchHWND))
		nState |= 2;

	// Is it enabled?
	if (IsWindowEnabled(m_WindowSearchHWND))
		nState |= 4;

	// Is it active?
	if (GetForegroundWindow() == m_WindowSearchHWND)
		nState |= 8;

	// Is it minimized?
	if (IsIconic(m_WindowSearchHWND))
		nState |= 16;

	// Is it maximized?
	if (IsZoomed(m_WindowSearchHWND))
		nState |= 32;

	vResult = nState;

	return AUT_OK;

} // WinGetState()



///////////////////////////////////////////////////////////////////////////////
// ToolTip()
//
// ToolTip("text", [x,y])
//
// Creates a tooltip with the specified text at any location on the screen.
// The window isn't created until it's first needed, so no resources are used until then.
// Also, the window is destroyed in AutoIt_Script's destructor so no resource leaks occur.
///////////////////////////////////////////////////////////////////////////////

AUT_RESULT AutoIt_Script::F_ToolTip(VectorVariant &vParams, Variant &vResult)
{
	uint iNumParam = vParams.size();

	// MingW missing some include defines
#ifndef TTF_TRACK
	#define TTF_TRACK 0x0020
	#define TTF_ABSOLUTE 0x0080
	#define TTM_SETMAXTIPWIDTH (WM_USER+24)
	#define TTM_TRACKPOSITION (WM_USER+18)
	#define TTM_TRACKACTIVATE (WM_USER+17)
#endif

	TOOLINFO ti;
	POINT  pt;

	ti.cbSize = sizeof(ti);
	ti.uFlags = TTF_TRACK | TTF_ABSOLUTE;
	ti.hwnd  = NULL;
	ti.hinst = NULL;
	ti.uId  = 0;
	ti.lpszText = (LPSTR)vParams[0].szValue();
	ti.rect.left = ti.rect.top = ti.rect.right = ti.rect.bottom = 0;

	// Set default values for the tip as the current mouse position
	GetCursorPos(&pt);
	pt.x += 16;
	pt.y += 16;

	if (iNumParam >= 2)
		pt.x = vParams[1].nValue();
	if (iNumParam >= 3)
		pt.y = vParams[2].nValue();

	if (!m_hWndTip)
	{
		m_hWndTip = CreateWindowEx(WS_EX_TOPMOST, TOOLTIPS_CLASS, NULL, TTS_NOPREFIX | TTS_ALWAYSTIP,
			CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, NULL, NULL, NULL, NULL);

		RECT dtw;
		GetWindowRect(GetDesktopWindow(), &dtw);

		SendMessage (m_hWndTip , TTM_ADDTOOL, 0, (LPARAM) (LPTOOLINFO)&ti);
		SendMessage(m_hWndTip, TTM_SETMAXTIPWIDTH, 0, (LPARAM)dtw.right);
	}
	else
		SendMessage(m_hWndTip, TTM_UPDATETIPTEXT, 0, (LPARAM)&ti);

	SendMessage (m_hWndTip , TTM_TRACKPOSITION, 0, (LPARAM)MAKELONG(pt.x, pt.y));
	SendMessage (m_hWndTip , TTM_TRACKACTIVATE, true, (LPARAM)&ti);

	return AUT_OK;

} // ToolTip()


///////////////////////////////////////////////////////////////////////////////
// WinMinimizeAll()
///////////////////////////////////////////////////////////////////////////////

AUT_RESULT AutoIt_Script::F_WinMinimizeAll(VectorVariant &vParams, Variant &vResult)
{
	PostMessage(FindWindow("Shell_TrayWnd", NULL), WM_COMMAND, 419, 0);
	Util_Sleep(m_nWinWaitDelay);				// Briefly pause before continuing

	return AUT_OK;

} // WinMinimizeAll()


///////////////////////////////////////////////////////////////////////////////
// WinMinimizeAllUndo()
///////////////////////////////////////////////////////////////////////////////

AUT_RESULT AutoIt_Script::F_WinMinimizeAllUndo(VectorVariant &vParams, Variant &vResult)
{
	PostMessage(FindWindow("Shell_TrayWnd", NULL), WM_COMMAND, 416, 0);
	Util_Sleep(m_nWinWaitDelay);				// Briefly pause before continuing

	return AUT_OK;

} // WinMinimizeAllUndo()


///////////////////////////////////////////////////////////////////////////////
// WinSetTrans
//
// Sets the transparency of a window (Windows 2000/XP or later)
// Takes 3 parameters:
//		vParam[0] - Window Title
//		vParam[1] - Window Text
//		vParam[2] - Integer that controls the transparency
///////////////////////////////////////////////////////////////////////////////
AUT_RESULT AutoIt_Script::F_WinSetTrans(VectorVariant &vParams, Variant &vResult)
{
#ifndef WS_EX_LAYERED	// Only defined on Windows 2000+
	#define WS_EX_LAYERED 0x00080000
#endif
#ifndef LWA_ALPHA
	#define LWA_ALPHA 0x00000002
#endif

	typedef BOOL (WINAPI *SLWA)(HWND, COLORREF, BYTE, DWORD);	// Prototype for SetLayeredWindowAttributes()

	uint value = vParams[2].nValue() < 0 ? 0 : vParams[2].nValue();		// Valid range is 0 - 255

	Win_WindowSearchInit(vParams);
	if (Win_WindowSearch() == false)
		return AUT_OK; // No window

	HMODULE hMod = LoadLibrary("user32.dll");
	if (!hMod)
		return AUT_OK;	// If this happens, we have major OS issues.

	SLWA lpSetLayeredWindowAttributes = (SLWA)GetProcAddress(hMod, "SetLayeredWindowAttributes");
	if (lpSetLayeredWindowAttributes)
	{
		LONG style = GetWindowLong(m_WindowSearchHWND, GWL_EXSTYLE);
		if (value >= 255 && (style & WS_EX_LAYERED))
			SetWindowLong(m_WindowSearchHWND, GWL_EXSTYLE, style ^ WS_EX_LAYERED);	// Remove
		else
		{
			SetWindowLong(m_WindowSearchHWND, GWL_EXSTYLE, style | WS_EX_LAYERED);
			lpSetLayeredWindowAttributes(m_WindowSearchHWND, 0, value, LWA_ALPHA);
		}
		vResult = 1;
	}
	else
		SetFuncErrorCode(1);	// This means the OS isn't supported since the function wasn't loaded from the DLL
	FreeLibrary(hMod);

	return AUT_OK;

}	// WinSetTrans()



///////////////////////////////////////////////////////////////////////////////
// WinList()
//
// WinList( ["title" [, "text] )
// Gets a list of all matching windows.
//
///////////////////////////////////////////////////////////////////////////////

AUT_RESULT AutoIt_Script::F_WinList(VectorVariant &vParams, Variant &vResult)
{
	Variant		*pvVariant;
	char		szTitle[AUT_WINTEXTBUFFER+1];
	int			nOldMode = m_nWindowSearchMatchMode;	// save the mode, we might change it

	// Matching defaults
	m_vWindowSearchTitle = "all";
	m_vWindowSearchText = "";

	if (vParams.size() == 0)
		m_nWindowSearchMatchMode = 4;
	else
	{
		if (vParams.size() > 0)
			m_vWindowSearchTitle = vParams[0].szValue();
		if (vParams.size() > 1)
			m_vWindowSearchText = vParams[1].szValue();
	}

	// Generate the list of matching windows (false = make list rather than stop at first match)
	Win_WindowSearch(false);
	m_nWindowSearchMatchMode = nOldMode;		// Restore matching mode


	// Create a 2d array big enough for all the windows, +1 for the count
	vResult.ArraySubscriptClear();						// Reset the subscript
	vResult.ArraySubscriptSetNext(m_nWinListCount + 1);	// Number of elements
	vResult.ArraySubscriptSetNext(2);					// Number of elements ([0]=title. [1]=hwnd)
	vResult.ArrayDim();									// Dimension array

	// Set up the count in [0][0]
	vResult.ArraySubscriptClear();						// Reset the subscript
	vResult.ArraySubscriptSetNext(0);
	vResult.ArraySubscriptSetNext(0);					// [0][0]
	pvVariant = vResult.ArrayGetRef();					// Get reference to the element
	*pvVariant = m_nWinListCount;						// Store the count


	WinListNode	*lpTemp = m_lpWinListFirst;
	for (int i = 1; i <= m_nWinListCount; ++i)
	{
		// Get the window text
		GetWindowText(lpTemp->hWnd, szTitle, AUT_WINTEXTBUFFER);
		vResult.ArraySubscriptClear();						// Reset the subscript
		vResult.ArraySubscriptSetNext(i);
		vResult.ArraySubscriptSetNext(0);					// [i][0]
		pvVariant = vResult.ArrayGetRef();					// Get reference to the element
		*pvVariant = szTitle;								// Store the title

		// Get the window handle
		vResult.ArraySubscriptClear();						// Reset the subscript
		vResult.ArraySubscriptSetNext(i);
		vResult.ArraySubscriptSetNext(1);					// [i][1]
		pvVariant = vResult.ArrayGetRef();					// Get reference to the element
		*pvVariant = lpTemp->hWnd;							// Store the handle

		lpTemp = lpTemp->lpNext;							// Next element
	}

	return AUT_OK;
}


///////////////////////////////////////////////////////////////////////////////
// WinGetProcess()
//
// WinGetProcess("title", "text")
//
///////////////////////////////////////////////////////////////////////////////

AUT_RESULT AutoIt_Script::F_WinGetProcess(VectorVariant &vParams, Variant &vResult)
{
	Win_WindowSearchInit(vParams);

	if (Win_WindowSearch() == false)
	{
		vResult = -1;							// Default is 1
		return AUT_OK;
	}

	DWORD	dwPid;

	GetWindowThreadProcessId(m_WindowSearchHWND, &dwPid);
	vResult = (int)dwPid;

	return AUT_OK;

} // WinGetProcess()



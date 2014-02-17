
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
// setforegroundwinex.cpp
//
// The SetForegroundWindow() hack object.
//
///////////////////////////////////////////////////////////////////////////////


// Includes
#include "StdAfx.h"								// Pre-compiled headers

#ifndef _MSC_VER								// Includes for non-MS compilers
	#include <windows.h>
#endif

#include "setforegroundwinex.h"
#include "os_version.h"


///////////////////////////////////////////////////////////////////////////////
// Patch()
//
// This patch will fail under 2000/XP unless the calling thread is the one
// currently associated with user input.  So, call as soon as your program
// starts for best results
//
///////////////////////////////////////////////////////////////////////////////

void SetForegroundWinEx::Patch()
{
	OS_Version		oVersion;
	DWORD			dwTimeout;

	m_bWin98orLater = oVersion.IsWin98orLater();
	m_bWin2000orLater = oVersion.IsWin2000orLater();

	// If we are running 98/2000 or later, do the crazy patches
	if ( (m_bWin98orLater == true) || (m_bWin2000orLater == true) )
	{
		// Get current systemparam timeout
		SystemParametersInfo(SPI_GETFOREGROUNDLOCKTIMEOUT, 0, &dwTimeout, 0);
		m_dwOldSystemTimeout = dwTimeout;	// Use local var to stop API freaking due to C++

		// Set new timeout of 0
		SystemParametersInfo(SPI_SETFOREGROUNDLOCKTIMEOUT, 0, (PVOID)0, SPIF_SENDCHANGE);
	}

} // Patch()


///////////////////////////////////////////////////////////////////////////////
// UnPatch()
//
// Call this whether the Patch was successful or not - it won't do any harm
// If the patch was successful, it doesn't appear to matter when you call the UnPatch
// it will always work.
//
///////////////////////////////////////////////////////////////////////////////

void SetForegroundWinEx::UnPatch()
{
	DWORD			dwTimeout;

	// If we are running 98/2000 or later, undo the crazy patches
	if ( (m_bWin98orLater == true) || (m_bWin2000orLater == true) )
	{
		// Get old systemparam timeout
		dwTimeout = m_dwOldSystemTimeout;	// Use local var to stop API freaking due to C++
#ifdef _MSC_VER
	#pragma warning( disable : 4312 )
#endif
		SystemParametersInfo(SPI_SETFOREGROUNDLOCKTIMEOUT, 0, (PVOID)dwTimeout, SPIF_SENDCHANGE);
#ifdef _MSC_VER
	#pragma warning( default : 4312 )
#endif
	}

} // UnPatch()


///////////////////////////////////////////////////////////////////////////////
// Activate()
///////////////////////////////////////////////////////////////////////////////

void SetForegroundWinEx::Activate(HWND hWnd)
{
	DWORD	myThread, curThread, newThread;
	HWND	hForegroundWnd;

	// Here we try to find foreground window
	hForegroundWnd = GetForegroundWindow();

	// if there is not any foreground window, then input focus is on the TaskBar.
	if (!hForegroundWnd)
		hForegroundWnd = FindWindow("Shell_TrayWnd", NULL);

	// If the target window is currently top - don't bother
	if (hWnd == hForegroundWnd)
		return;

	// If the target window is minimized, restore before we start
	if ( IsIconic(hWnd) )
		ShowWindow(hWnd, SW_RESTORE);

	//BringWindowToTop(hWnd);						// IE 5.5 related hack

	// Try and SetForegroundWindow and check for a failure
	// Should work fine on NT4 and 95.  Will usually fail on 98,ME,2000 and XP
	if ( !SetForegroundWindow(hWnd) )
	{
		// Get the details of all the input threads involved (myappswin, foreground win,
		// target win)
		curThread = GetWindowThreadProcessId(hForegroundWnd, NULL);
		myThread  = GetCurrentThreadId();
		newThread = GetWindowThreadProcessId(hWnd, NULL);

		// Attach all our input threads, will cause SetForeground to work under 98/ME
		AttachThreadInput(myThread, newThread, TRUE);
		AttachThreadInput(myThread, curThread, TRUE);
		AttachThreadInput(curThread, newThread, TRUE);

		// Try and SetForeground again
		if ( !SetForegroundWindow(hWnd) )
		{
			// OK, this is not funny - bring out the extreme measures (usually for 2000/XP)
			// Simulate two single ALT keystrokes, See LockSetForegroundWindow
			keybd_event((BYTE)VK_MENU, MapVirtualKey(VK_MENU, 0), 0, 0);
			keybd_event((BYTE)VK_MENU, MapVirtualKey(VK_MENU, 0), KEYEVENTF_KEYUP, 0);
			keybd_event((BYTE)VK_MENU, MapVirtualKey(VK_MENU, 0), 0, 0);
			keybd_event((BYTE)VK_MENU, MapVirtualKey(VK_MENU, 0), KEYEVENTF_KEYUP, 0);

			SetForegroundWindow(hWnd);
		}

		// Detach out input thread
		AttachThreadInput(myThread, newThread, FALSE);
		AttachThreadInput(myThread, curThread, FALSE);
		AttachThreadInput(curThread, newThread, FALSE);
	}

} // Activate()

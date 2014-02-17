
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
// application.cpp
//
// This is the main application object, where all the windows are created,
// Timer Procs, etc.
//
///////////////////////////////////////////////////////////////////////////////


// Includes
#include "StdAfx.h"								// Pre-compiled headers

#ifndef _MSC_VER								// Includes for non-MS compilers
	#include <stdio.h>
	#include <windows.h>
	#include <commctrl.h>
#endif

#include "AutoIt.h"								// Autoit values, macros and config options

#include "globaldata.h"
#include "application.h"
#include "script.h"
#include "resources\resource.h"
#include "utility.h"


///////////////////////////////////////////////////////////////////////////////
// Constructor()
///////////////////////////////////////////////////////////////////////////////

AutoIt_App::AutoIt_App()
{
	// Store initial values and zero important variables (the splash and progress functions
	// in particular rely on these being NULL to work out their status)
	g_hWnd				= NULL;					// Main window
	g_hWndEdit			= NULL;					// Edit window
	g_hWndSplash		= NULL;					// Splash window handle
	g_hSplashBitmap		= NULL;					// Splash window bitmap
	g_hWndProgress		= NULL;					// Progress window handle
	g_hWndProgBar		= NULL;					// Progress progressbar control handle
	g_hWndProgLblA		= NULL;					// Progress Top label control handle
	g_hWndProgLblB		= NULL;					// Progress Bottom label control handle
	g_bTrayIcon			= false;				// Icon not initially visible
	g_bTrayIconInitial	= true;					// Usually we want the tray icon to be displayed at start

	m_bShowingPauseIcon	= false;

	m_bSingleCmdMode	= false;				// Script mode by default

	// Script/win communication defaults
	g_bScriptPaused			= false;
	g_bBreakEnabled			= true;
	g_bTrayIconDebug		= false;
	g_bStdOut				= false;
	g_bTrayExitClicked		= false;
	g_bKillWorkerThreads	= false;

	g_HotKeyNext		= 0;					// Initial position in hot key buffer

	m_hIconSmall		= NULL;

} // AutoIt_App()


///////////////////////////////////////////////////////////////////////////////
// Destructor()
///////////////////////////////////////////////////////////////////////////////

AutoIt_App::~AutoIt_App()
{

	if (m_hIconSmall)
		DestroyIcon(m_hIconSmall);

} // AutoIt_App()


///////////////////////////////////////////////////////////////////////////////
// Run
//
// This is the main function for the application object, it just registers
// classes, creates the window and then waits until the user quits.
//
///////////////////////////////////////////////////////////////////////////////

void AutoIt_App::Run(void)
{
	char	szOldWorkingDir[_MAX_PATH+1];
	char	szTempFileName[_MAX_PATH+1];


	// Parse the command line options (get scriptname, reveal mode, single cmd, etc.)
	ParseCmdLine();

	// If we are in single command mode (/c) then manually add a line of script, otherwise
	// load a script
	if (m_bSingleCmdMode == true)
	{
		// Create a "loaded" script that contains a single line - cunning eh? :)
		//MessageBox(NULL, m_sSingleLine.c_str(), "", MB_OK);

		g_oScriptFile.AddLine(1, m_sSingleLine.c_str(), -1);
	}
	else
	{
		// Check that we can load the script file.  Returns the FULLPATH of the script loaded
		if (g_oScriptFile.LoadScript(m_szScriptFileName) == false)
		{
			g_nExitCode = 1;						// Main exit code
			return;									// Error loading script
		}
		else
			GetFullPathName(m_szScriptFileName, _MAX_PATH, szTempFileName, &m_szScriptFilePart);
	}

	// Prepare the script for use
	g_oScriptFile.PrepareScript();

	// Perform initial check of the script (check userfunctions, etc)
	if (g_oScript.InitScript(m_szScriptFileName) != AUT_OK)
	{
		g_oScriptFile.UnloadScript();
		g_nExitCode = 1;						// Main exit code
		return;									// Error initialising script
	}

	// Save the current working directory
	GetCurrentDirectory(_MAX_PATH, szOldWorkingDir);

	// Register our classes (including the GUI class) and create the main windows
	RegisterClass();
	WindowCreate();

	// Show the tray icon if required
	if (g_bTrayIconInitial == true)
		CreateTrayIcon();


	// Run the script from line 1, the Execute() function will run the windows message queue
	g_oScript.Execute(1);

	// Unload the script (even if one wasn't loaded - this is OK)
	g_oScriptFile.UnloadScript();

	// Restore the old working directory
	SetCurrentDirectory(szOldWorkingDir);

} // Run()


///////////////////////////////////////////////////////////////////////////////
// RegisterClass()
///////////////////////////////////////////////////////////////////////////////

void AutoIt_App::RegisterClass(void)
{
	WNDCLASSEX	wndClass;
	HBRUSH		hBrush	= GetSysColorBrush(COLOR_BTNFACE);
	HCURSOR		hCursor = LoadCursor(NULL, IDC_ARROW);

	// Get a handle to use for the tray icon, we need to use LoadImage so that we can
	// try and specify a 16x16 icon - LoadIcon is buggy and tends to pick a 32x32 icon.  If
	// a 16x16 cannot be found it seems to pick one - which is fine.
	m_hIcon			= LoadIcon(g_hInstance, MAKEINTRESOURCE(IDI_MAIN));
	m_hIconPause	= LoadIcon(g_hInstance, MAKEINTRESOURCE(IDI_PAUSED));

	// Load the right tray icon for the OS
	if (g_oVersion.IsWinXPorLater() || g_oVersion.IsWinMeorLater())
		m_hIconSmall	= Util_LoadIcon(IDI_MAIN, 16, 16, -1);
	else
		m_hIconSmall	= Util_LoadIcon(IDI_MAIN, 16, 16, 4);

	// Init the class structure for the main class
	wndClass.cbSize			= sizeof(wndClass);
	wndClass.style			= CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
	wndClass.cbClsExtra		= 0;
	wndClass.cbWndExtra		= 0;
	wndClass.hInstance		= g_hInstance;
	wndClass.hCursor		= hCursor;
	wndClass.hbrBackground	= hBrush;
	wndClass.lpszMenuName	= NULL;
	wndClass.lpszClassName	= AUT_APPCLASS;
	wndClass.hIcon			= m_hIcon;
	wndClass.hIconSm		= m_hIconSmall;
	wndClass.lpfnWndProc	= WndProc;

	RegisterClassEx(&wndClass);


	// Also register the AU3GUI stuff
	#ifdef AUT_CONFIG_GUI							// Is GUI enabled?
		g_oGUI.RegisterClass(g_hInstance, g_hWnd, m_hIcon, m_hIconSmall);
	#endif

} // RegisterClass()


/////////////////////////////////////////////////////////////////////////////
// CreateWindow()
/////////////////////////////////////////////////////////////////////////////

void AutoIt_App::WindowCreate(void)
{
	g_hWnd = CreateWindow(  AUT_APPCLASS, AUT_APPTITLE, WS_OVERLAPPEDWINDOW,
							CW_USEDEFAULT, CW_USEDEFAULT, 300, 100, NULL, NULL, g_hInstance, NULL);

	// Add read-only edit control to our main window
	g_hWndEdit = CreateWindow("edit", NULL, WS_CHILD | WS_VISIBLE | WS_HSCROLL |
					WS_VSCROLL | WS_BORDER | ES_LEFT | ES_MULTILINE | ES_AUTOHSCROLL |
					ES_AUTOVSCROLL | ES_READONLY, 0, 0, 0, 0,
					g_hWnd, (HMENU)1, g_hInstance, NULL);

	// Twice incase the first one is ignored
	ShowWindow(g_hWnd, SW_HIDE);
	ShowWindow(g_hWnd, SW_HIDE);

} // CreateWindow()


#ifndef AUTOITSC

///////////////////////////////////////////////////////////////////////////////
// ParseCmdLine() - NATIVE VERSION
//
// Valid native AutoIt cmdlines
//		autoit.exe								(num params = 0)
//		autoit.exe myscript.aut param1 param2	(num params = 1+)
//		autoit.exe /c "line"					(num params = 2)
//
///////////////////////////////////////////////////////////////////////////////

void AutoIt_App::ParseCmdLine(void)
{
	char	szTemp[CMDLINE_MAXLEN+1];
	Variant	*pvTemp, *pvElement;
	bool	bConst = false;
	Variant	vTemp;
	int		nNumParams;

	m_szScriptFileName[0]	= '\0';				// Scriptfilename is blank by default
	m_szScriptFilePart		= m_szScriptFileName;

	m_bSingleCmdMode		= false;			// set defaults

	// Create the variable $CmdLineRaw which holds the original command line
	vTemp = g_oCmdLine.GetCmdLine();
	g_oVarTable.Assign("CmdLineRaw", vTemp, true);
	vTemp = 0;									// Will be used later as a dummy var so make 0 to save space

	// Create a single variable called $CmdLine to hold cmdline details
	// We will dimension it to the correct size later.
	g_oVarTable.Assign("CmdLine", vTemp, true);		// vTemp is dummy variable for array
	g_oVarTable.GetRef("CmdLine", &pvTemp, bConst);

	nNumParams = g_oCmdLine.GetNumParams();

	// If num parameters is zero, create the relevant CmdLine variable and return
	if ( nNumParams == 0 )
	{
		pvTemp->ArraySubscriptClear();			// Reset the subscript
		pvTemp->ArraySubscriptSetNext(1);		// Array is 1 element (num params)
		pvTemp->ArrayDim();						// Dimension array

		pvTemp->ArraySubscriptClear();			// Reset the subscript
		pvTemp->ArraySubscriptSetNext(0);		// Set subscript we want to access
		pvElement = pvTemp->ArrayGetRef();		// Get reference to the element
		*pvElement = 0;							// $CmdLines[0] = 0

		return;
	}


	// We are here, so must be 1 or more parameters, get the first parameter
	g_oCmdLine.GetNextParam(szTemp);

	// Check if this is a /c request
	// Use the .exe file as the scriptname in this case
	if ( !stricmp("/c", szTemp) )
	{
		m_bSingleCmdMode = true;
		GetModuleFileName(NULL, m_szScriptFileName, _MAX_PATH);

		g_oCmdLine.GetNextParam(szTemp);
		m_sSingleLine = szTemp;					// Store the single line

		pvTemp->ArraySubscriptClear();			// Reset the subscript
		pvTemp->ArraySubscriptSetNext(1);		// Array is 1 element (num params)
		pvTemp->ArrayDim();						// Dimension array

		pvTemp->ArraySubscriptClear();			// Reset the subscript
		pvTemp->ArraySubscriptSetNext(0);		// Set subscript we want to access
		pvElement = pvTemp->ArrayGetRef();		// Get reference to the element
		*pvElement = 0;							// $CmdLines[0] = 0

		return;
	}
	else if (stricmp("/ErrorStdOut", szTemp) == 0)
	{
		g_bStdOut = true;
		if (nNumParams == 1)
		{
			pvTemp->ArraySubscriptClear();		// Reset the subscript
			pvTemp->ArraySubscriptSetNext(1);	// Array is 1 element (num params)
			pvTemp->ArrayDim();					// Dimension array

			pvTemp->ArraySubscriptClear();		// Reset the subscript
			pvTemp->ArraySubscriptSetNext(0);	// Set subscript we want to access
			pvElement = pvTemp->ArrayGetRef();	// Get reference to the element
			*pvElement = 0;						// $CmdLines[0] = 0
			return;
		}
		--nNumParams;							// Decrement because we strip the /ErrorStdOut from the command line
		g_oCmdLine.GetNextParam(szTemp);		// skip to the next item
	}

	// If we are here must just be the scriptfilename.au3 param1 param2 etc - store the script
	// name - note, this will be converted to a FULLPATH by the loadscript() function
	strcpy(m_szScriptFileName, szTemp);

	// How many parameters left?  Use this number to Dim our $CmdLines array
	pvTemp->ArraySubscriptClear();				// Reset the subscript
	pvTemp->ArraySubscriptSetNext( nNumParams );	// (number of params, minus script, plus size in first element)
	pvTemp->ArrayDim();							// Dimension array

	// Set first param to number of params (not counting scriptname)
	pvTemp->ArraySubscriptClear();				// Reset the subscript
	pvTemp->ArraySubscriptSetNext(0);			// Set subscript we want to access
	pvElement = pvTemp->ArrayGetRef();			// Get reference to the element
	*pvElement = nNumParams-1;					// $CmdLines[0] = numparams-1 (we don't include scriptname)


	// While there are more parameters, store them
	int		nVar = 0;

	while ( g_oCmdLine.GetNextParam(szTemp) == true )
	{
		nVar++;

		pvTemp->ArraySubscriptClear();				// Reset the subscript
		pvTemp->ArraySubscriptSetNext(nVar);		// Set subscript we want to access
		pvElement = pvTemp->ArrayGetRef();			// Get reference to the element
		*pvElement = szTemp;						// $CmdLines[nVar] = param x
	}

} // ParseCmdLine()

#else

///////////////////////////////////////////////////////////////////////////////
// ParseCmdLine() - SELFCONTAINED VERSION
//
// Valid native AutoIt cmdlines
//		script.exe								(num params = 0)
//		script.exe param1 param2				(num params = 1+)
//		script.exe /c "line"					(num params = 2)
//
///////////////////////////////////////////////////////////////////////////////

void AutoIt_App::ParseCmdLine(void)
{
	char	szTemp[CMDLINE_MAXLEN+1];
	Variant	*pvTemp, *pvElement;
	bool	bConst = false;
	Variant	vTemp;
	int		nNumParams;

	// Because this is a compiled script, name will always be the .exe
	// note, this will be converted to a FULLPATH by the loadscript() function
	GetModuleFileName(NULL, m_szScriptFileName, _MAX_PATH);
	m_szScriptFilePart		= m_szScriptFileName;

	m_bSingleCmdMode		= false;			// set defaults

	// Create the variable $CmdLineRaw which holds the original command line
	vTemp = g_oCmdLine.GetCmdLine();
	g_oVarTable.Assign("CmdLineRaw", vTemp, true);
	vTemp = 0;									// Will be used later as a dummy var so make 0 to save space

	// Create a single variable called "CmdLine" to hold cmdline details
	// We will dimension it to the correct size later.
	g_oVarTable.Assign("CmdLine", vTemp, true);	// vTemp is dummy variable for array
	g_oVarTable.GetRef("CmdLine", &pvTemp, bConst);

	nNumParams = g_oCmdLine.GetNumParams();


	// If num parameters is zero, create the relevant CmdLine variable and return
	if ( nNumParams == 0 )
	{
		pvTemp->ArraySubscriptClear();				// Reset the subscript
		pvTemp->ArraySubscriptSetNext(1);			// Array is 1 element (num params)
		pvTemp->ArrayDim();							// Dimension array

		pvTemp->ArraySubscriptClear();				// Reset the subscript
		pvTemp->ArraySubscriptSetNext(0);			// Set subscript we want to access
		pvElement = pvTemp->ArrayGetRef();			// Get reference to the element
		*pvElement = 0;								// $CmdLines[0] = 0

		return;
	}

	// We are here, so must be 1 or more parameters, get the first parameter
	g_oCmdLine.GetNextParam(szTemp);

	// Check if this is a /c request
	// Use the .exe file as the scriptname in this case
	if ( !stricmp("/c", szTemp) )
	{
		m_bSingleCmdMode = true;

		g_oCmdLine.GetNextParam(szTemp);
		m_sSingleLine = szTemp;						// Store the single line

		pvTemp->ArraySubscriptClear();				// Reset the subscript
		pvTemp->ArraySubscriptSetNext(1);			// Array is 1 element (num params)
		pvTemp->ArrayDim();							// Dimension array

		pvTemp->ArraySubscriptClear();				// Reset the subscript
		pvTemp->ArraySubscriptSetNext(0);			// Set subscript we want to access
		pvElement = pvTemp->ArrayGetRef();			// Get reference to the element
		*pvElement = 0;								// $CmdLines[0] = 0

		return;
	}
	else if (stricmp("/ErrorStdOut", szTemp) == 0)
	{
		g_bStdOut = true;
		if (nNumParams == 1)
		{
			pvTemp->ArraySubscriptClear();		// Reset the subscript
			pvTemp->ArraySubscriptSetNext(1);	// Array is 1 element (num params)
			pvTemp->ArrayDim();					// Dimension array

			pvTemp->ArraySubscriptClear();		// Reset the subscript
			pvTemp->ArraySubscriptSetNext(0);	// Set subscript we want to access
			pvElement = pvTemp->ArrayGetRef();	// Get reference to the element
			*pvElement = 0;						// $CmdLines[0] = 0
			return;
		}
		--nNumParams;							// Decrement because we strip the /ErrorStdOut from the command line
		g_oCmdLine.GetNextParam(szTemp);		// skip to the next item
	}

	// How many parameters left?  Use this number to Dim our $CmdLines array
	pvTemp->ArraySubscriptClear();				// Reset the subscript
	pvTemp->ArraySubscriptSetNext( nNumParams+1 );	// Number of params plus size in [0]
	pvTemp->ArrayDim();							// Dimension array

	// Set first param to number of params
	pvTemp->ArraySubscriptClear();				// Reset the subscript
	pvTemp->ArraySubscriptSetNext(0);			// Set subscript we want to access
	pvElement = pvTemp->ArrayGetRef();			// Get reference to the element
	*pvElement = nNumParams;					// $CmdLines[0] = numparams

	// Store the first param
	pvTemp->ArraySubscriptClear();				// Reset the subscript
	pvTemp->ArraySubscriptSetNext(1);			// Set subscript we want to access
	pvElement = pvTemp->ArrayGetRef();			// Get reference to the element
	*pvElement = szTemp;						// $CmdLines[nVar] = param x

	// While there are more parameters, store them
	int		nVar = 1;							// Will be incremented to 2 before next set

	while ( g_oCmdLine.GetNextParam(szTemp) == true )
	{
		nVar++;

		pvTemp->ArraySubscriptClear();			// Reset the subscript
		pvTemp->ArraySubscriptSetNext(nVar);	// Set subscript we want to access
		pvElement = pvTemp->ArrayGetRef();		// Get reference to the element
		*pvElement = szTemp;					// $CmdLines[nVar] = param x
	}

} // ParseCmdLine()


#endif


///////////////////////////////////////////////////////////////////////////////
// WndProc
//
// C++ Gotcha
// This member function has been declared as STATIC so that it can be used as
// a callback function.  This means that there is only ONE instance of this
// function irrespective of how many parent class objects there are.
// Very dodgy, but IMHO better that having a global callback function.  Also
// This function will not know which object instance called it - so we will have
// to go back to the g_oApp object to get our bearings.
// In our case there is only ever ONE application object, so no problem
///////////////////////////////////////////////////////////////////////////////

LRESULT CALLBACK AutoIt_App::WndProc (HWND hWnd, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
	return g_oApplication.WndProcHandler(hWnd, iMsg, wParam, lParam);

} // WndProc()


///////////////////////////////////////////////////////////////////////////////
// WndProcHandler
// Handle messages that are sent to our app window
//
// Note: the splash and progress windows will also use this handler, so be
// careful whose messages you are handling.
//
///////////////////////////////////////////////////////////////////////////////

LRESULT AutoIt_App::WndProcHandler (HWND hWnd, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
	static UINT s_uTaskbarRestart;				// Holds taskbar recreate message

	// What window is the message for? (note, g_hWnd may be NULL during startup WM_CREATE)
	if (hWnd == g_hWnd || g_hWnd == NULL)
	{
		//
		// Main window messages
		//

		switch (iMsg)
		{
			// Window initial creation
			case WM_CREATE:
				AUT_DEBUGMSG("==> AutoIt_App::WndProc, g_hWnd: WM_CREATE\n")

				// Start the main timer used for flashing the "pause" icon
				SetTimer(hWnd, AUT_MAIN_TIMER_ID, AUT_MAIN_TIMER_DELAY, NULL);

				// Register a message so we know if explorer crashes and we have to redraw taskbar
				s_uTaskbarRestart = RegisterWindowMessage("TaskbarCreated");

				return 0;

			// Some form of timer has been triggered - check it out
			case WM_TIMER:
				HandleTimer(hWnd, wParam, lParam);

				return 0;

			// Menu command received
			case WM_COMMAND:
				if (HandleCommand(hWnd, iMsg, wParam, lParam) == true)
					 return 0;                              // completely handled
				break;

			// Window close clicked, if this returns 0 then user will be prevented from closing
			case WM_CLOSE:
				AUT_DEBUGMSG("==> AutoIt_App::WndProc, g_hWnd: WM_CLOSE\n")

				// Notify the script that close was requested but DON'T allow the system to continue
				// with the defaut WM_CLOSE - we will call DestroyWindow() ourselves from the script
				g_bTrayExitClicked = true;
				g_bKillWorkerThreads = true;			// Ask worker threads to stop (otherwise the script
														// may be blocked - e.g InetGet on a big download)
				//break;
				return 0;

				// Window is being destroyed
			case WM_DESTROY:
				AUT_DEBUGMSG("==> AutoIt_App::WndProc, g_hWnd: WM_DESTROY\n")

				KillTimer(hWnd, AUT_MAIN_TIMER_ID);		// Remove any timers in use
				DestroyTrayIcon();						// Remove tray icon if present
				PostQuitMessage(0);						// Tell the message loop to stop
				return 0;

			// Window is getting focus
			case WM_SETFOCUS:
				SetFocus(g_hWndEdit);                         // Put focus on the main edit window
				return 0;

			// Window has been resized in some way
			case WM_SIZE:
				MoveWindow(g_hWndEdit, 0, 0, LOWORD(lParam), HIWORD(lParam), TRUE);
				return 0;

			// Popup menu accessed
			case AUT_WM_NOTIFYICON:
				NotifyIcon(hWnd, iMsg, wParam, lParam);
				return 0;

			case WM_HOTKEY:                                        // Ignore when paused
				if (!g_bScriptPaused)
				{
					 g_HotKeyQueue[g_HotKeyNext++] = wParam;     // Store the hotkey ID pressed
					 if (g_HotKeyNext >= AUT_HOTKEYQUEUESIZE)
						  g_HotKeyNext = 0;
				}

				//MessageBox(NULL, "", "Hotkey pressed", MB_OK);
				break;

			default:
				// If the icon was visible and explorer crashed then redraw
				if(iMsg == s_uTaskbarRestart && g_bTrayIcon == true)
				{
					AUT_DEBUGMSG("==> AutoIt_App::WndProc, g_hWnd: TaskbarRestart received - redrawing trayicon\n")
					DestroyTrayIcon();
					CreateTrayIcon();
				}
				break;

		} // end main switch

	}

	// If not handled, return the default
	return DefWindowProc(hWnd, iMsg, wParam, lParam);

} // WndProcHandler()


///////////////////////////////////////////////////////////////////////////////
// HandleTimer()
//
// This function handles the main timer event, and also any other timer
// events
//
///////////////////////////////////////////////////////////////////////////////

void AutoIt_App::HandleTimer(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
	switch (wParam)
	{
		case AUT_MAIN_TIMER_ID:
			NOTIFYICONDATA	nic;

			// Make sure tool tip is up to date
			SetTrayIconToolTip();

			// If we are paused then toggle the tray icon, if we are not paused then
			// make sure we are not showing the pause icon
			if (g_bTrayIcon)
			{
				nic.cbSize	= sizeof(NOTIFYICONDATA);
				nic.hWnd	= hWnd;
				nic.uID		= AUT_NOTIFY_ICON_ID;
				nic.uFlags	= NIF_ICON;

				if (g_bScriptPaused)
				{
					if (m_bShowingPauseIcon == true)
					{
						m_bShowingPauseIcon = false;
						nic.hIcon	= m_hIconSmall;
					}
					else
					{
						m_bShowingPauseIcon = true;
						nic.hIcon	= m_hIconPause;
					}

					Shell_NotifyIcon(NIM_MODIFY, &nic);
				}
				else
				{
					// Not paused, make sure we not showing the pause icon
					if (m_bShowingPauseIcon == true)
					{
						m_bShowingPauseIcon = false;
						nic.hIcon	= m_hIconSmall;
						Shell_NotifyIcon(NIM_MODIFY, &nic);
					}
				}
			}

			// Remove and reset the timer in use (killing removes any unused timer messages that may be queued)
			KillTimer(hWnd, AUT_MAIN_TIMER_ID);
			SetTimer(hWnd, AUT_MAIN_TIMER_ID, AUT_MAIN_TIMER_DELAY, NULL);

			break;
	}

} // HandleTimer()


///////////////////////////////////////////////////////////////////////////////
// HandleCommand()
//
// This function handles general command events (standard menus, tray menus, etc.)
//
///////////////////////////////////////////////////////////////////////////////

bool AutoIt_App::HandleCommand (HWND hWnd, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
	switch (LOWORD(wParam))
	{
		// Don't need to check if break is enabled - these menu items wouldn't
		// be clickable if break was disabled
		case ID_EXIT:
			g_bTrayExitClicked = true;
			//PostMessage(hWnd, WM_CLOSE, 0, 0);	// Will eventually call destroy on the GUI as it is a child window
			return true;

		// The pause ID message is only sent when the user clicks to UNPAUSE, the act of clicking on the tray icon is what
		// puts us in the paused state
		case ID_TRAY_PAUSE:
			g_bScriptPaused = false;

			// Change the tooltip based on the unpause
			SetTrayIconToolTip();

			Sleep(500);							// Have a small delay in the scriptwhen unpausing

			return true;
	}

	return false;								// We didn't handle, please use DefWindowProc()

} // HandleCommand()


///////////////////////////////////////////////////////////////////////////////
// CreateTrayIcon()
//
// Creates and displays the AutoIt tray icon.  Pass it the handle of the window
// to receive tray icon messages
//
///////////////////////////////////////////////////////////////////////////////

void AutoIt_App::CreateTrayIcon(void)
{
	NOTIFYICONDATA	nic;

	if (g_bTrayIcon == true)
		return;									// Icon is already present
	else
		g_bTrayIcon = true;

	nic.cbSize				= sizeof(NOTIFYICONDATA);
	nic.hWnd				= g_hWnd;
	nic.uID					= AUT_NOTIFY_ICON_ID;
	nic.uFlags				= NIF_ICON | NIF_MESSAGE;
	nic.uCallbackMessage	= AUT_WM_NOTIFYICON;

	//nic.hIcon				= LoadIcon(g_hInstance, MAKEINTRESOURCE(IDI_MAIN_32x32x16));
	nic.hIcon				= m_hIconSmall;

	Shell_NotifyIcon(NIM_ADD, &nic);

	// Also set the tool tip based on the script name and paused status
	SetTrayIconToolTip();

} // CreateTrayIcon()


///////////////////////////////////////////////////////////////////////////////
// SetTrayIconToolTip()
//
// Modifies the tool tip on the tray icon to say "scriptname" or "paused" or
// whatever
//
///////////////////////////////////////////////////////////////////////////////

void AutoIt_App::SetTrayIconToolTip(void)
{
	NOTIFYICONDATA	nic;
	char			szTip[63+1];				// Tool tip
	AString			sTip(_MAX_PATH);			// Preallocate for speed
	int				nCurLine;

	// Only update if we need to and an icon exists
	if (g_bTrayIcon == false)
		return;									// No icon to modify?

	nic.cbSize				= sizeof(NOTIFYICONDATA);
	nic.hWnd				= g_hWnd;
	nic.uID					= AUT_NOTIFY_ICON_ID;
	nic.uFlags				= NIF_TIP;

	// Set up the tooltip, add "paused" to the start if required
	if (g_bScriptPaused == true)
		LoadString(g_hInstance, IDS_AUT_TIP_PAUSED, szTip, 63);
	else
		szTip[0] = '\0';

	sTip = szTip;								// Get it into our String class (for easy concat, sizing)

#ifndef AUTOITSC
	// Normal
	if (!g_bTrayIconDebug)
	{
		sTip += "AutoIt - ";					// Only Prefix with "AutoIt" when not debugging
		sTip += m_szScriptFilePart;				// Get the scriptname from the script filename
	}
	else
	{
		nCurLine = g_oScript.GetCurLineNumber();
		if (g_oScriptFile.GetLine(nCurLine) != NULL)
		{
			sTip += g_oScriptFile.GetIncludeFileName(g_oScriptFile.GetIncludeID(nCurLine));
			sprintf(szTip, "\nLine %d: ", g_oScriptFile.GetAutLineNumber(nCurLine));
			sTip += szTip;
			sTip += g_oScriptFile.GetLine(nCurLine);
		}
	}
#else
	// Compiled
	sTip += m_szScriptFilePart;				// Get the scriptname from the script filename

	if (g_bTrayIconDebug)
	{
		nCurLine = g_oScript.GetCurLineNumber();
		if (g_oScriptFile.GetLine(nCurLine) != NULL)
		{
			sTip += "\nLine: ";
			sTip += g_oScriptFile.GetLine(nCurLine);
		}
	}
#endif

	// Tip can only be 64 characters, enforce and display
	Util_Strncpy(szTip, sTip.c_str(), 63+1);
	strcpy(nic.szTip, szTip);
	Shell_NotifyIcon(NIM_MODIFY, &nic);

} // SetTrayIconToolTip()


///////////////////////////////////////////////////////////////////////////////
// DestroyTrayIcon()
//
// Removes the tray icon and removes it from the passed windows message queue
//
///////////////////////////////////////////////////////////////////////////////

void AutoIt_App::DestroyTrayIcon(void)
{
	if (g_bTrayIcon == false)
		return;									// No icon to delete?
	else
		g_bTrayIcon = false;

	NOTIFYICONDATA nic;

	nic.cbSize	= sizeof(NOTIFYICONDATA);
	nic.hWnd	= g_hWnd;
	nic.uID		= AUT_NOTIFY_ICON_ID;

	Shell_NotifyIcon(NIM_DELETE, &nic);

} // DestroyTrayIcon()


///////////////////////////////////////////////////////////////////////////////
// NotifyIcon()
//
// This function is called when the user clicks the tray icon
//
///////////////////////////////////////////////////////////////////////////////

void AutoIt_App::NotifyIcon (HWND hWnd, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
	switch (lParam)
	{
		//case WM_LBUTTONDBLCLK:
		//	break;

		case WM_RBUTTONDOWN:
		case WM_LBUTTONDOWN:
			HMENU hMenu;
			HMENU hMenuTrackPopup;
			POINT pt;

			// If break is not enabled then don't even bring the menu up
			if (g_bBreakEnabled == false)
				break;

			// The act of clicking on the tray icon puts us in the paused state no matter what
			g_bScriptPaused = true;

			hMenu = LoadMenu(g_hInstance, MAKEINTRESOURCE(IDR_TRAY1));
			// check and gray out menu items as neccessary
			//EnableMenuItem(hMenu, ID_EXIT, !g_oScript.IsBreakAllowed());
			//EnableMenuItem(hMenu, ID_TRAY_PAUSE, !g_oScript.IsBreakAllowed());

			// We we actually FORCE the script to pause when the menu is active, so
			// the menu state will always have the pause menu checked
			CheckMenuItem(hMenu, ID_TRAY_PAUSE, MF_CHECKED);

			// TrackPopupMenu cannot display the menu bar so get
			// the handle to the first shortcut menu.
 			hMenuTrackPopup = GetSubMenu(hMenu, 0);

			GetCursorPos(&pt);
			SetForegroundWindow(hWnd);			// TrackPopupMenu bugfix

			TrackPopupMenuEx(hMenuTrackPopup, TPM_LEFTALIGN | TPM_LEFTBUTTON,
								pt.x, pt.y, hWnd, NULL);

			DestroyMenu(hMenu);
			PostMessage(hWnd, WM_NULL, 0, 0);	// TrackPopupMenu bugfix

			// Change the tooltip based on the fact that the script is paused
			SetTrayIconToolTip();

			break;

	} // End Switch

} // NotifyIcon()


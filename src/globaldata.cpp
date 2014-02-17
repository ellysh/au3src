
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
// globaldata.cpp
//
// Single place for all global data (all the naughtiness in one place!)
//
///////////////////////////////////////////////////////////////////////////////


// Includes
#include "StdAfx.h"								// Pre-compiled headers

#ifndef _MSC_VER								// Includes for non-MS compilers
	#include <windef.h>
#endif

#include "AutoIt.h"								// Autoit values, macros and config options

#include "script.h"
#include "application.h"
#include "cmdline.h"
#include "setforegroundwinex.h"
#include "variabletable.h"
#include "scriptfile.h"
#include "os_version.h"
#include "guibox.h"
#include "shared_memory.h"


// Global data
HINSTANCE				g_hInstance;			// Main application instance

HWND					g_hWnd;					// Main window handle
HWND					g_hWndEdit;				// Main window edit control handle
HWND					g_hWndProgress;			// Progress window handle
HWND					g_hWndProgBar;			// Progress progressbar control handle
HWND					g_hWndProgLblA;			// Progress Top label control handle
HWND					g_hWndProgLblB;			// Progress Bottom label control handle

HWND					g_hWndSplash;			// Splash window handle
HBITMAP					g_hSplashBitmap;		// Splash window bitmap


#ifdef AUT_CONFIG_GUI							// Is GUI enabled?
CGuiBox					g_oGUI;					// GUI object
#endif

int						g_nExitCode;			// Windows exit code
int						g_nExitMethod;			// The way AutoIt finished

OS_Version				g_oVersion;				// Version object

AutoIt_App				g_oApplication;			// Main application object
AutoIt_Script			g_oScript;				// The scripting engine object

CmdLine					g_oCmdLine;				// CmdLine object
SetForegroundWinEx		g_oSetForeWinEx;		// Foreground window hack object

VariableTable			g_oVarTable;			// Object for accessing autoit variables
AutoIt_ScriptFile		g_oScriptFile;			// The script file object


// Script/main window comms
bool					g_bScriptPaused;		// True when script is paused
bool					g_bBreakEnabled;		// True when user is allowed to quit script
bool					g_bTrayIcon;			// True when tray icon is displayed
bool					g_bTrayIconInitial;		// Initial state of tray icon
bool					g_bTrayIconDebug;		// True when TrayIcon debugng is allowed
bool					g_bStdOut;				// True when /ErrorStdOut used on the command line
bool					g_bTrayExitClicked;		// True when the user clicks "exit"
bool					g_bKillWorkerThreads;	// True when requesting all thread finish up (script is about to die)

WPARAM					g_HotKeyQueue[AUT_HOTKEYQUEUESIZE];	// Queue for hotkeys pressed
int						g_HotKeyNext;			// Next free hotkey position in queue


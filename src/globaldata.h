#ifndef __GLOBALDATA_H
#define __GLOBALDATA_H

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
// globaldata.h
//
// Global data "externs" are declared here, along with any include files
// they require.
//
///////////////////////////////////////////////////////////////////////////////


// Includes
#include "AutoIt.h"

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
extern HINSTANCE				g_hInstance;		// Main application instance

extern HWND						g_hWnd;				// Main window handle
extern HWND						g_hWndEdit;			// Main window edit control handle
extern HWND						g_hWndProgress;		// Progress window handle
extern HWND						g_hWndProgBar;		// Progress progressbar control handle
extern HWND						g_hWndProgLblA;		// Progress Top label control handle
extern HWND						g_hWndProgLblB;		// Progress Bottom label control handle

extern HWND						g_hWndSplash;		// Splash window handle
extern HBITMAP					g_hSplashBitmap;	// Splash window bitmap

#ifdef AUT_CONFIG_GUI								// Is GUI enabled?
extern CGuiBox					g_oGUI;				// GUI object
#endif

extern int						g_nExitCode;		// Windows exit code
extern int						g_nExitMethod;		// The way AutoIt finished

extern OS_Version				g_oVersion;			// Version object

extern AutoIt_App				g_oApplication;		// Main application object
extern AutoIt_Script			g_oScript;			// The scripting engine object

extern CmdLine					g_oCmdLine;			// CmdLine object
extern SetForegroundWinEx		g_oSetForeWinEx;	// Foreground window hack object

extern VariableTable			g_oVarTable;		// Object for accessing autoit variables
extern AutoIt_ScriptFile		g_oScriptFile;		// The script file object


// Script to main window comms
extern bool						g_bScriptPaused;		// True when script is paused
extern bool						g_bBreakEnabled;		// True when user is allowed to quit script
extern bool						g_bTrayIcon;			// True when tray icon is displayed
extern bool						g_bTrayIconInitial;		// Initial state of tray icon
extern bool						g_bTrayIconDebug;		// True when TrayIcon debugng is allowed
extern bool						g_bStdOut;				// True when /ErrorStdOut used on the command line
extern bool						g_bTrayExitClicked;		// True when the user clicks "exit"
extern bool						g_bKillWorkerThreads;	// True when requesting all thread finish up (script is about to die)

extern WPARAM					g_HotKeyQueue[AUT_HOTKEYQUEUESIZE];	// Queue for hotkeys pressed
extern int						g_HotKeyNext;		// Next free hotkey position in queue


///////////////////////////////////////////////////////////////////////////////

#endif

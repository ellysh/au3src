#ifndef __APPLICATION_H
#define __APPLICATION_H

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
// application.h
//
// This is the main application object, where all the windows are created,
// Timer Procs, etc.
//
///////////////////////////////////////////////////////////////////////////////


// Class and window titles
#define AUT_APPCLASS		"AutoIt v3"
#define AUT_APPTITLE		"AutoIt v3"

// Windows timers
#define AUT_MAIN_TIMER_ID		1
#define AUT_MAIN_TIMER_DELAY	750				// Tray icon hiding/flashing/drawing is checked every 750ms

// Tray / popup menu identifiers
#define AUT_WM_NOTIFYICON		WM_USER+1
#define AUT_NOTIFY_ICON_ID		1



class AutoIt_App
{
public:
	// Functions
	AutoIt_App::AutoIt_App();					// Constructor
	AutoIt_App::~AutoIt_App();					// Denstructor
	void		CreateTrayIcon(void);
	void		DestroyTrayIcon(void);
	void		Run(void);

	// Variables
	HICON		m_hIcon;
	HICON		m_hIconSmall;
	HICON		m_hIconPause;

private:
	// Variables

	bool		m_bSingleCmdMode;				// TRUE=/c cmdline mode
	AString		m_sSingleLine;					// Single line for the /c cmdline
	char		m_szScriptFileName[_MAX_PATH+1];// FileName (fullpath) of current script
	char		*m_szScriptFilePart;			// Just the filename (no path)
	bool		m_bShowingPauseIcon;			// State of the flashing paused icon


	// Functions

	// Main window handler messages and processing
	static LRESULT CALLBACK WndProc (HWND hWnd, UINT iMsg, WPARAM wParam, LPARAM lParam);
	LRESULT		WndProcHandler (HWND hWnd, UINT iMsg, WPARAM wParam, LPARAM lParam);

	void		HandleTimer(HWND hWnd, WPARAM wParam, LPARAM lParam);
	bool		HandleCommand (HWND hWnd, UINT iMsg, WPARAM wParam, LPARAM lParam);

	void		RegisterClass(void);
	void		WindowCreate(void);
	void		ParseCmdLine(void);

	// Tray icon
	void		SetTrayIconToolTip(void);
	void		NotifyIcon (HWND hWnd, UINT iMsg, WPARAM wParam, LPARAM lParam);
};


///////////////////////////////////////////////////////////////////////////////

#endif

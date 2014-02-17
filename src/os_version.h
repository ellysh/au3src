#ifndef __OS_VERSION_H
#define __OS_VERSION_H

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
// os_version.h
//
// A standalone class for easy checking of the OS version.
//
///////////////////////////////////////////////////////////////////////////////


// Includes
#include <windows.h>

class OS_Version
{
public:
	// Functions
	OS_Version();													// Constructor

	bool	IsWinNT(void) const {return m_bWinNT;}					// Returns true if NT
	bool	IsWin9x(void) const {return m_bWin9x;}					// Returns true if 9x

	bool	IsWinNT4(void) const {return m_bWinNT4;}				// Returns true if WinNT 4
	bool	IsWin2000(void) const {return m_bWin2000;}				// Returns true if Win2000
	bool	IsWin2003(void) const {return m_bWin2003;}				// Returns true if Win2003 Server
	bool	IsWinXP(void) const {return m_bWinXP;}					// Returns true if WinXP
	bool	IsWinNT4orLater(void) const {return m_bWinNT4orLater;}	// Returns true if WinNT 4+
	bool	IsWin2000orLater(void) const {return m_bWin2000orLater;}// Returns true if Win2000+
	bool	IsWin2003orLater(void) const {return m_bWin2003orLater;}// Returns true if Win2003+
	bool	IsWinXPorLater(void) const {return m_bWinXPorLater;}	// Returns true if WinXP+

	bool	IsWin95(void) const {return m_bWin95;}					// Returns true if 95
	bool	IsWin98(void) const {return m_bWin98;}					// Returns true if 98
	bool	IsWinMe(void) const {return m_bWinMe;}					// Returns true if Me
	bool	IsWin95orLater(void) const {return m_bWin95orLater;}	// Returns true if 95
	bool	IsWin98orLater(void) const {return m_bWin98orLater;}	// Returns true if 98
	bool	IsWinMeorLater(void) const {return m_bWinMeorLater;}	// Returns true if Me

	DWORD	BuildNumber(void) {return m_dwBuildNumber;}
	const char * CSD(void) {return m_szCSDVersion;}

private:
	// Variables
	DWORD			m_dwMajorVersion;			// Major OS version
	DWORD			m_dwMinorVersion;			// Minor OS version
	DWORD			m_dwBuildNumber;			// Build number
	char			m_szCSDVersion [256];

	bool			m_bWinNT;
	bool			m_bWin9x;

	bool			m_bWinNT4;
	bool			m_bWinNT4orLater;
	bool			m_bWinXP;
	bool			m_bWinXPorLater;
	bool			m_bWin2000;
	bool			m_bWin2000orLater;
	bool			m_bWin2003;
	bool			m_bWin2003orLater;
	bool			m_bWin98;
	bool			m_bWin98orLater;
	bool			m_bWin95;
	bool			m_bWin95orLater;
	bool			m_bWinMe;
	bool			m_bWinMeorLater;
};

///////////////////////////////////////////////////////////////////////////////

#endif


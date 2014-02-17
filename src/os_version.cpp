
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
// os_version.cpp
//
// A standalone class for easy checking of the OS version.
//
///////////////////////////////////////////////////////////////////////////////


// Includes
#include "StdAfx.h"								// Pre-compiled headers

#ifndef _MSC_VER								// Includes for non-MS compilers
	#include <windows.h>
#endif

#include "os_version.h"


/*
OSVERSIONINFO structure details
===============================

dwOSVersionInfoSize
Specifies the size, in bytes, of this data structure. Set this member to sizeof(OSVERSIONINFO)
before calling the GetVersionEx function.

dwMajorVersion
Major version number of the operating system. This member can be one of the following values. Operating System Meaning
Windows 95 4
Windows 98 4
Windows Me 4
Windows NT 3.51 3
Windows NT 4.0 4
Windows 2000 5
Windows XP 5
Windows Server 2003 family 5

dwMinorVersion
Minor version number of the operating system. This member can be one of the following values. Operating System Meaning
Windows 95 0
Windows 98 10
Windows Me 90
Windows NT 3.51 51
Windows NT 4.0 0
Windows 2000 0
Windows XP 1
Windows Server 2003 family 2

dwBuildNumber
Build number of the operating system.
Windows Me/98/95:  The low-order word contains the build number of the operating system. The high-order word contains the major and minor version numbers.
dwPlatformId
Operating system platform. This member can be one of the following values. Value Meaning
VER_PLATFORM_WIN32s Win32s on Windows 3.1.
VER_PLATFORM_WIN32_WINDOWS Windows 95, Windows 98, or Windows Me.
VER_PLATFORM_WIN32_NT Windows NT, Windows 2000, Windows XP, or Windows Server 2003 family.

szCSDVersion
Pointer to a null-terminated string, such as "Service Pack 3", that indicates the latest Service Pack installed on the system. If no Service Pack has been installed, the string is empty.
Windows Me/98/95:  Pointer to a null-terminated string that indicates additional version information. For example, " C" indicates Windows 95 OSR2 and " A" indicates Windows 98 Second Edition.

*/


///////////////////////////////////////////////////////////////////////////////
// Constructor
///////////////////////////////////////////////////////////////////////////////

OS_Version::OS_Version()
{
	int				i;
	OSVERSIONINFO	OSvi;						// OS Version data

	// Get details of the OS we are running on
	OSvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
	GetVersionEx(&OSvi);

	// Populate Major and Minor version numbers
	m_dwMajorVersion	= OSvi.dwMajorVersion;
	m_dwMinorVersion	= OSvi.dwMinorVersion;
	m_dwBuildNumber		= OSvi.dwBuildNumber;

	// Get CSD information
	int nTemp = (int)strlen(OSvi.szCSDVersion);

	if (nTemp > 0)
	{
		//	strip trailing
		for (i=nTemp-1; i>0; i--)
		{
			if ((char) OSvi.szCSDVersion[i] != ' ')
				break;
			OSvi.szCSDVersion[i] = '\0';
		}

		//	strip leading
		nTemp = i;
		for (i=0; i<nTemp; i++)
		{
			if ((char) OSvi.szCSDVersion[i] != ' ')
				break;
		}
		strcpy(m_szCSDVersion, &OSvi.szCSDVersion[i]);
	}
	else
		m_szCSDVersion[0] = '\0';				// No CSD info, make it blank to avoid errors


	// Set all options to false by default
	m_bWinNT	= false;
	m_bWin9x	= false;

	m_bWinNT4	= false;	m_bWinNT4orLater	= false;
	m_bWin2000	= false;	m_bWin2000orLater	= false;
	m_bWinXP	= false;	m_bWinXPorLater		= false;
	m_bWin2003	= false;	m_bWin2003orLater	= false;

	m_bWin98	= false;	m_bWin98orLater		= false;
	m_bWin95	= false;	m_bWin95orLater		= false;
	m_bWinMe	= false;	m_bWinMeorLater		= false;


	// Work out if NT or 9x
	if (OSvi.dwPlatformId == VER_PLATFORM_WIN32_NT)
	{
		// Windows NT
		m_bWinNT = true;

		switch (m_dwMajorVersion)
		{
			case 4:								// NT 4
				m_bWinNT4 = m_bWinNT4orLater = true;
				break;

			case 5:								// Win2000 / XP
				m_bWinNT4orLater = true;

				if ( m_dwMinorVersion == 0 )	// Win2000
					m_bWin2000 = m_bWin2000orLater = true;
				else							// WinXP
					m_bWinXP = m_bWinXPorLater = m_bWin2000orLater = true;
				break;

		} // End Switch
	}
	else
	{
		// Windows 9x -- all major versions = 4
		m_bWin9x = true;
		m_bWin95orLater = true;
		m_dwBuildNumber	= (WORD) OSvi.dwBuildNumber;	// Build number in lower word on 9x

		switch ( m_dwMinorVersion )
		{
			case 0:								// 95
				m_bWin95 = true;
				break;

			case 10:							// 98
				m_bWin98 = m_bWin98orLater = true;
				break;

			case 90:							// ME
				m_bWinMe = 	m_bWinMeorLater = m_bWin98orLater = true;
				break;
		} // End Switch
	} // End If

} // Constructor()


#ifndef __STDAFX_H
#define __STDAFX_H

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
// StdAfx.h
//
// The pre-compiled header file.  This should be included in any source file
// that uses the includes defined here (it is a lot faster!)
// If you don't want to include this header then change the properties of the
// source file to not use .pch
//
///////////////////////////////////////////////////////////////////////////////


// Only use the includes here if running Visual C - it is really slow under
// MingW to use this method so we will comment out the headers unless the
// compiler is VC (and manually include the required headers in the source files)

#define	WINVER	0x0400							// Make "windows.h" support 95 & NT4.0
#define	_WIN32_IE 0x0400						// Set commctrl.h Compatability to IE4+ (some GUI features require IE4, but will run on less)

#ifdef _MSC_VER
	#include <stdio.h>
	#include <stdlib.h>
	#include <stdarg.h>
	#include <math.h>
	#include <time.h>
	#include <sys/timeb.h>
	#include <string.h>
	#include <ctype.h>							// tolower()
	#include <limits.h>							// INT limits macros
	#include <windows.h>
	#include <mmsystem.h>
	#include <process.h>						// Multithreaded commands (beginthreadex, etc)
	#include <tlhelp32.h>						// Win95 PS functions
	#include <olectl.h>
	#include <commctrl.h>
	#include <shlobj.h>
	#include <wininet.h>
	#include <winnetwk.h>
	#include <new.h>
#endif

///////////////////////////////////////////////////////////////////////////////

#endif

#ifndef __AUTOIT_H
#define __AUTOIT_H

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
// AutoIt.h
//
// The main include file containing general macros and defines for AutoIt.
// This should be included in most other files (except the standalone classes).
//
///////////////////////////////////////////////////////////////////////////////


// Configuration options
//#define AUT_CONFIG_GUI							// Enable GUI functionality
#define AUT_CONFIG_LEXERCACHE					// Enable lexer caching
//#define AUT_CONFIG_DEBUG						// Enable debug functionality


// Disable 64bit warnings on Visual C .NET
#ifdef _MSC_VER
	#pragma warning(disable : 4311 4312)
#endif


// Basic types
// 	Assumptions:
//		char 	= 	8bit
//		short	=	16bit
//		int		=	32bit
//		long	=	32bit
typedef unsigned char 		uchar;
typedef unsigned short 		ushort;
typedef unsigned int 		uint;
typedef unsigned long 		ulong;


#define AUT_MAX_LINESIZE		4095			// Max size for a line of script
#define AUT_STRBUFFER			4095			// Size of a general string buffer
#define AUT_MAX_ENVSIZE			32767			// Max size for an ENV variable (See SetEnvironmentVariable)
#define AUT_WINTEXTBUFFER		32767			// GetWindowText fails under 95 if>65535, WM_GETTEXT randomly fails if > 32767
#define AUT_IDLE				10				// Number of ms to wait when idling
#define AUT_ADLIB_DELAY			250				// Default delay in ms between ADLIB triggers
#define AUT_HOTKEYQUEUESIZE		64				// Number of queued hotkeys to buffer
#define AUT_MAXEXECUTERECURSE	384				// Maximum number of times the Execute() function can recurse to itself
#define AUT_BATCHLINES			1


// AutoIt function result macros (for simple checking for success/failure)
#define AUT_RESULT	int
#define AUT_OK		0
#define AUT_ERR		1
#define AUT_FAILED(Status)		((AUT_RESULT)(Status)!=AUT_OK)
#define AUT_SUCCEEDED(Status)	((AUT_RESULT)(Status)==AUT_OK)


// Macros
#define AUT_MSGBOX(szTitle, szText) MessageBox(NULL, szText, szTitle, MB_OK);


// Debug build macros for assertion checking and debug general output
#ifdef _DEBUG
	#include <assert.h>

	#define AUT_DEBUGMSG(szText) OutputDebugString(szText);
	#define AUT_ASSERT(x) assert(x);
	#define AUT_DEBUGMESSAGEBOX(szText) MessageBox(NULL, szText, "", MB_OK);
#else
	#define AUT_DEBUGMSG(szText)
	#define AUT_ASSERT(x)
	#define AUT_DEBUGMESSAGEBOX(szText)
#endif


///////////////////////////////////////////////////////////////////////////////

#endif

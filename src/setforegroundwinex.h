#ifndef __SETFOREGROUNDWINEX_H
#define __SETFOREGROUNDWINEX_H

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
// setforegroundwinex.h
//
// The SetForegroundWindow() hack object.
//
///////////////////////////////////////////////////////////////////////////////


#ifndef SPI_GETFOREGROUNDLOCKTIMEOUT
	#define SPI_GETFOREGROUNDLOCKTIMEOUT 0x2000
#endif

#ifndef SPI_SETFOREGROUNDLOCKTIMEOUT
	#define SPI_SETFOREGROUNDLOCKTIMEOUT 0x2001
#endif


class SetForegroundWinEx
{
public:
	// Functions
	void	Patch();							// Patch system
	void	UnPatch();							// UnPatch system
	void	Activate(HWND hWnd);				// SetForegroundWindow()


private:
	// Variables
	bool	m_bWin2000orLater;
	bool	m_bWin98orLater;
	DWORD	m_dwOldSystemTimeout;				// Timeout from SystemParameters
	DWORD	m_dwOldRegTimeout;					// Timeout from user reg key
};

///////////////////////////////////////////////////////////////////////////////

#endif

